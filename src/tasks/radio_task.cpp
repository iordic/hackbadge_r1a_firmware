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
    default:
        break;
    }  
    cc1101->setSidle();
    free(params);
    vTaskDelete(NULL);
}

void loadConfiguration(int frequencyOption, int preset) {
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