#include "radio_task.h"
#include "config/io_config.h"
#include "config/cc1101_config.h"
#include "radio.h"


ELECHOUSE_CC1101 *cc1101;


void radio_task(void *pv) {
    cc1101 = radio_get();
    RadioTaskParams *params = (RadioTaskParams *) pv;
    TaskHandle_t caller = params->callerHandle;   
    cc1101->init();
    switch (params->operation) {
    case CHECK:
        xTaskNotify(caller, cc1101->getCC1101(), eSetValueWithOverwrite);
        break;
    case START_JAMMER:
        Serial.print("freq preset: ");
        Serial.println(params->frequency);
        loadConfiguration(params->frequency, params->preset);
        cc1101->setTx();
        Serial.println(cc1101->getFrequency());
        lockJamming();
        break;
    default:
        break;
    }  
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

float getFrequencyFromEnum(int freqEnum) {
    switch (freqEnum) {
    case FREQ_315MHZ:
        return 315.0;
        break;
    case FREQ_433MHZ:
        return 433.92;
        break;
    case FREQ_868MHZ:
        return 868.0;
        break;
    case FREQ_915MHZ:
        return 915.0;
        break;
    default:
        return 433.92;
    }
}

void lockJamming() {
    digitalWrite(CC1101_GDO0, HIGH);
    uint32_t keepJamming;   // dummy variable to receive notification value
    Serial.println("Jamming started.");
    xTaskNotifyWait(0, 0, &keepJamming, portMAX_DELAY);
    digitalWrite(CC1101_GDO0, LOW);
    cc1101->setSidle();
    Serial.println("Jamming stopped.");
}