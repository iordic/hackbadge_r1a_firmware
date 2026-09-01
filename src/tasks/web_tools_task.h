#ifndef WEB_TOOLS_TASK_H_
#define WEB_TOOLS_TASK_H_
#include <Arduino.h>

// Worker del portal Web Tools: levanta WiFi (AP o STA) + mDNS + un WebServer
// síncrono que sirve la SPA (web_ui.h) y expone /api/* para ver/editar la config.
// Autenticación por token: se genera al arrancar, se muestra en el OLED y la web
// lo cambia por una cookie de sesión. Sigue el patrón de wifi_portal_task
// (params malloc/free cedidos al worker, estado compartido bajo mutex). Solo un
// portal vivo a la vez (Web Tools y Evil Portal son apps exclusivas).

enum WebToolsCommands {
    WEBTOOLS_STOP = 1
};

enum WebToolsMode {
    WEB_MODE_AP = 0,
    WEB_MODE_STA = 1
};

enum WebToolsStatus {
    WEB_STARTING = 0,  // arrancando red
    WEB_RUNNING_AP,    // AP levantado
    WEB_RUNNING_STA,   // unido a la WiFi
    WEB_FAILED         // no se pudo levantar la red
};

// Parámetros de arranque: malloc en startWebToolsTask, free dentro del worker.
typedef struct {
    uint8_t mode;
    char    staSsid[33];
    char    staPass[65];
    char    apSsid[33];
    char    apPass[65];
} WebToolsParams;

extern TaskHandle_t webToolsTaskHandle;

// Arranca el portal. Devuelve false si ya hay uno vivo o falla la creación.
bool startWebToolsTask(uint8_t mode, const String &staSsid, const String &staPass,
                       const String &apSsid, const String &apPass);
// Pide parada y espera (acotado) a que el worker desmonte WiFi/mDNS/web.
void stopWebToolsTask();

// Accesores de estado seguros para leer desde ui_task.
uint8_t webGetStatus();
void    webGetIp(char *buf, size_t len);
void    webGetToken(char *buf, size_t len);
uint8_t webGetClientCount();

#endif
