#include "radio_task.h"
#include "config/io_config.h"
#include "config/cc1101_config.h"

ELECHOUSE_CC1101 *cc1101;

void radio_task(void *pv) {
    RadioTaskParams *params = (RadioTaskParams *) pv;
    TaskHandle_t caller = params->callerHandle;
    cc1101 = new ELECHOUSE_CC1101(CC1101_GDO0, CC1101_GDO2, CC1101_SCLK, CC1101_MISO, CC1101_MOSI, CC1101_CS, FSPI);
    cc1101->init();
    if (params->operation == CHECK) {
        bool result = cc1101->getCC1101();
        xTaskNotify(caller, result, eSetValueWithOverwrite);
    }
    free(params);
    vTaskDelete(NULL);
}
