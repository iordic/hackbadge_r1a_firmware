#include "radio_task.h"
#include "config/io_config.h"
#include "config/cc1101_config.h"
#include "devices/radio.h"
#include "utils/radio_utils.h"

extern TaskHandle_t radioTransmitterTaskHandle; 

ELECHOUSE_CC1101 *cc1101;
BaseType_t xRadioResult;
uint32_t radioNotificationValue;
QueueHandle_t radioQueue;
static int currentFrequencyOpt;   // enum de freq/preset activos (para etiquetar capturas)
static int currentPresetOpt;
// Cola dedicada UI->task para peticiones de replay RAW (definida en app_radio_raw.cpp).
// Separada de radioQueue (task->UI de capturas) para no mezclar productores/consumidores.
extern QueueHandle_t rawReplayQueue;

void radio_task(void *pv) {extern TaskHandle_t radioTransmitterTaskHandle; 
    cc1101 = radio_get();
    RadioTaskParams *params = (RadioTaskParams *) pv;
    TaskHandle_t caller = params->callerHandle;   
    radioQueue = params->queueHandle;
    cc1101->init();
    loadConfiguration(params->frequency, params->preset);

    switch (params->operation) {
    case CHECK:
        xTaskNotify(caller, cc1101->getCC1101(), eSetValueWithOverwrite);
        break;
    case START_JAMMER:
        lockJamming();
        break;
    case RECEIVE_SIGNAL:
        radioReceiveSignal();
        break;
    case SEND_SIGNAL:
        sendSignal();
        radioTransmitterTaskHandle = NULL;
        break;
    case RECEIVE_RAW:
        radioReceiveRaw();
        break;
    case SEND_RAW:
        radioSendRaw();
        radioTransmitterTaskHandle = NULL;
        break;
    default:
        break;
    }  
    cc1101->setSidle();
    free(params);
    vTaskDelete(NULL);
}

void loadConfiguration(int frequencyOption, int preset) {
    currentFrequencyOpt = frequencyOption;
    currentPresetOpt = preset;
    float frequency = getFrequencyFromEnum(frequencyOption);
    uint8_t *settings;
    int settingsSize;
    switch (preset) {
    case PRESET_AM270:
        settingsSize = sizeof(subghz_device_cc1101_preset_ook_270khz_async_regs);
        settings = (uint8_t *)subghz_device_cc1101_preset_ook_270khz_async_regs;
        break;
    case PRESET_FM238:
        settingsSize = sizeof(subghz_device_cc1101_preset_2fsk_dev2_38khz_async_regs);
        settings = (uint8_t *)subghz_device_cc1101_preset_2fsk_dev2_38khz_async_regs;
        break;
    case PRESET_FM476:
        settingsSize = sizeof(subghz_device_cc1101_preset_2fsk_dev47_6khz_async_regs);
        settings = (uint8_t *)subghz_device_cc1101_preset_2fsk_dev47_6khz_async_regs;
        break;
    case PRESET_AM650:
    default:
        settingsSize = sizeof(subghz_device_cc1101_preset_ook_650khz_async_regs);
        settings = (uint8_t *)subghz_device_cc1101_preset_ook_650khz_async_regs;
        break;
    }
    int i, patableSize = 8;
    uint8_t reg, value;
    for (i = 0; i<settingsSize; i+=2) {
        reg = settings[i];
        value = settings[i+1];
        if (!reg && !value) break;
        cc1101->spiWriteReg(reg, value);
    }
    cc1101->spiWriteBurstReg(CC1101_PATABLE, &settings[i+2], patableSize);
    cc1101->setFrequency(frequency);
}

void lockJamming() {
    cc1101->setTx();
    pinMode(CC1101_GDO0, OUTPUT);
    digitalWrite(CC1101_GDO0, HIGH);
    Serial.println("Jamming started.");
    xTaskNotifyWait(0, 0, &radioNotificationValue, portMAX_DELAY);
    digitalWrite(CC1101_GDO0, LOW);
    Serial.println("Jamming stopped.");
}

void radioReceiveSignal() {
    RFMessage msg;
    cc1101->setRx();
    pinMode(CC1101_GDO0, INPUT);
    RCSwitch mySwitch = RCSwitch();
    mySwitch.enableReceive(CC1101_GDO0);
    Serial.print("Radio receiver started. Listening on ");
    Serial.println(cc1101->getFrequency());
    while (true) {
        if (mySwitch.available()) {
            output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(),mySwitch.getReceivedProtocol());
            msg.value = mySwitch.getReceivedValue();
            msg.length = mySwitch.getReceivedBitlength();
            msg.protocol = mySwitch.getReceivedProtocol();
            xQueueSend(radioQueue, &msg, 0);
            mySwitch.resetAvailable();
        }
        xRadioResult = xTaskNotifyWait(0, 0, &radioNotificationValue, 0);
        if (xRadioResult == pdTRUE && radioNotificationValue == RADIO_STOP)  break;
        if (xRadioResult == pdTRUE && radioNotificationValue == REPLAY_SIGNAL) {
            RFMessage sendMsg;
            xQueueReceive(radioQueue, &sendMsg, portMAX_DELAY);
            replaySignal(&mySwitch, sendMsg);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void sendSignal() {
    pinMode(CC1101_GDO0, OUTPUT);
    RFMessage msg;
    RCSwitch mySwitch = RCSwitch();
    mySwitch.enableTransmit(CC1101_GDO0);
    while (true) {
        xRadioResult = xTaskNotifyWait(0, 0, &radioNotificationValue, 0);
        if (xRadioResult == pdTRUE && radioNotificationValue == SEND_SIGNAL) {
            xQueueReceive(radioQueue, &msg, portMAX_DELAY);
            sendSignal(&mySwitch, msg);
        } else if (xRadioResult == pdTRUE && radioNotificationValue == RADIO_STOP)  break;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    mySwitch.disableTransmit();
}

void replaySignal(RCSwitch *mySwitch, RFMessage msg) {
    Serial.println("Replaying signal...");
    mySwitch->disableReceive();
    detachInterrupt(digitalPinToInterrupt(CC1101_GDO0));
    sendSignal(mySwitch, msg);
    mySwitch->disableTransmit(); 
    cc1101->setRx();
    pinMode(CC1101_GDO0, INPUT);
    mySwitch->enableReceive(digitalPinToInterrupt(CC1101_GDO0)); 
    Serial.println("Volviendo a modo RX...");
}

void sendSignal(RCSwitch *mySwitch, RFMessage msg) {
    cc1101->setTx();
    pinMode(CC1101_GDO0, OUTPUT);
    mySwitch->enableTransmit(CC1101_GDO0);
    mySwitch->setRepeatTransmit(10);
    mySwitch->setProtocol(msg.protocol);
    mySwitch->send(msg.value, msg.length);
}

// ---------------------------------------------------------------------------
// RAW: captura y reproducción de timings crudos (OOK async), enfoque Evil Crow.
// La ISR ignora glitches (< RAW_MIN_PULSE_US) y un silencio largo reinicia el
// contador. La trama se da por completa con suficientes muestras + 100 ms de
// silencio. Sin RSSI, sin filtros estadísticos: solo interrupciones.
// ---------------------------------------------------------------------------

static int32_t rawBuf[RAW_MAX_CHANGES];   // duraciones con signo (µs)
static volatile uint16_t rawIdx = 0;      // nº de muestras almacenadas
static volatile uint32_t rawLastEdge = 0; // micros() del último flanco

void IRAM_ATTR rawEdgeISR() {
    const uint32_t now = micros();
    const uint32_t duration = now - rawLastEdge;
    // Tras el flanco el pin ya tiene el nivel NUEVO, así que la duración que
    // acaba de terminar corresponde al nivel contrario. Guardamos el signo
    // explícito (+HIGH / -LOW) en vez de deducirlo por la paridad del índice:
    // así los glitches descartados no invierten el resto de la trama.
    const int newLevel = digitalRead(CC1101_GDO0);
    if (duration > RAW_END_GAP_US) rawIdx = 0;   // silencio largo: nueva trama
    if (duration >= RAW_MIN_PULSE_US && rawIdx < RAW_MAX_CHANGES) {
        rawBuf[rawIdx++] = newLevel ? -(int32_t)duration : (int32_t)duration;
    }
    rawLastEdge = now;
}

static void rawResetCapture() {
    noInterrupts();
    rawIdx = 0;
    rawLastEdge = micros();
    interrupts();
}

// Empaqueta el buffer capturado en una RawSignal de heap.
static RawSignal *rawFinalizeCapture(uint16_t count, int frequency, int preset) {
    RawSignal *sig = (RawSignal *)malloc(sizeof(RawSignal));
    if (!sig) return NULL;
    sig->durations = (int32_t *)malloc(count * sizeof(int32_t));
    if (!sig->durations) { free(sig); return NULL; }
    memcpy(sig->durations, rawBuf, count * sizeof(int32_t));
    sig->count = count;
    sig->frequency = frequency;
    sig->preset = preset;
    return sig;
}

void radioReceiveRaw() {
    cc1101->setRx();
    pinMode(CC1101_GDO0, INPUT);
    rawResetCapture();
    attachInterrupt(digitalPinToInterrupt(CC1101_GDO0), rawEdgeISR, CHANGE);
    Serial.print("RAW RX started @ ");
    Serial.println(cc1101->getFrequency());

    while (true) {
        // Recepción completa: muestras suficientes + 100 ms de silencio.
        if (rawIdx >= RAW_MIN_CHANGES && (micros() - rawLastEdge) > RAW_END_GAP_US) {
            detachInterrupt(digitalPinToInterrupt(CC1101_GDO0));
            uint16_t count = rawIdx;
            Serial.printf("[raw] Count=%u\n      ", count);
            for (uint16_t i = 0; i < count; i++) {
                Serial.print(rawBuf[i]); Serial.print(',');
            }
            Serial.println();
            RawSignal *sig = rawFinalizeCapture(count, currentFrequencyOpt, currentPresetOpt);
            if (sig) {
                RawSignal *ptr = sig;
                xQueueSend(radioQueue, &ptr, 0); // el puntero; libera la UI
            }
            rawResetCapture();
            attachInterrupt(digitalPinToInterrupt(CC1101_GDO0), rawEdgeISR, CHANGE);
        }

        xRadioResult = xTaskNotifyWait(0, 0, &radioNotificationValue, 0);
        if (xRadioResult == pdTRUE && radioNotificationValue == RADIO_STOP) break;
        if (xRadioResult == pdTRUE && radioNotificationValue == REPLAY_RAW) {
            RawSignal *sig = NULL;
            if (xQueueReceive(rawReplayQueue, &sig, portMAX_DELAY) == pdTRUE && sig) {
                replayRaw(sig);
                // La RawSignal de replay es propiedad de la UI: no la liberamos.
            }
        }
        vTaskDelay(2 / portTICK_PERIOD_MS);
    }
    detachInterrupt(digitalPinToInterrupt(CC1101_GDO0));
}

void radioSendRaw() {
    while (true) {
        xRadioResult = xTaskNotifyWait(0, 0, &radioNotificationValue, 0);
        if (xRadioResult == pdTRUE && radioNotificationValue == SEND_RAW) {
            RawSignal *sig = NULL;
            if (xQueueReceive(radioQueue, &sig, portMAX_DELAY) == pdTRUE && sig) {
                sendRaw(sig);
                // En SEND_RAW la señal se carga de fichero solo para enviarla:
                // la libera el task (a diferencia de REPLAY_RAW, donde es de la UI).
                freeRawSignal(sig);
            }
        } else if (xRadioResult == pdTRUE && radioNotificationValue == RADIO_STOP) {
            break;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// delayMicroseconds no es fiable con valores grandes; troceamos.
static void rawDelayUs(uint32_t us) {
    while (us > 16000UL) { delayMicroseconds(16000); us -= 16000UL; }
    if (us) delayMicroseconds((uint16_t)us);
}

// Bit-bang de GDO0 siguiendo las duraciones. El nivel sale del SIGNO de cada
// muestra (igual que el sendBits de Evil Crow: buff[i] < 0 ? LOW : HIGH), no de
// la posición: así una muestra perdida no invierte el resto de la trama.
void sendRaw(const RawSignal *sig) {
    if (!sig || !sig->durations || sig->count == 0) return;
    cc1101->setTx();
    pinMode(CC1101_GDO0, OUTPUT);
    for (int r = 0; r < RAW_REPEAT; r++) {
        for (uint16_t i = 0; i < sig->count; i++) {
            int32_t d = sig->durations[i];
            digitalWrite(CC1101_GDO0, d < 0 ? LOW : HIGH);
            rawDelayUs((uint32_t)(d < 0 ? -d : d));
        }
        digitalWrite(CC1101_GDO0, LOW);
        rawDelayUs(RAW_REPEAT_GAP_US);
    }
    cc1101->setSidle();
}

// Reproduce una RawSignal en mitad de una sesión de RX y vuelve a modo escucha.
void replayRaw(const RawSignal *sig) {
    Serial.println("Replaying RAW signal...");
    detachInterrupt(digitalPinToInterrupt(CC1101_GDO0));
    sendRaw(sig);
    cc1101->setRx();
    pinMode(CC1101_GDO0, INPUT);
    rawResetCapture();
    attachInterrupt(digitalPinToInterrupt(CC1101_GDO0), rawEdgeISR, CHANGE);
    Serial.println("Volviendo a modo RX RAW...");
}

void freeRawSignal(RawSignal *sig) {
    if (!sig) return;
    if (sig->durations) free(sig->durations);
    free(sig);
}