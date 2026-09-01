#include "web_tools_task.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <esp_random.h>
#include "config/constants.h"
#include "config/web_ui.h"
#include "utils/radio_utils.h" // enums FREQ_*/PRESET_* para clamp
#include "tasks/ui_task.h"     // sendNeopixelIdle (aplicar brillo en vivo)

#ifndef VERSION
#define VERSION "dev"
#endif

// --- Instancias únicas (solo hay un portal a la vez). Rutas registradas una
// sola vez; los handlers leen estado de fichero-scope refrescado en cada arranque.
static WebServer webServer(80);
static Preferences wprefs; // handle propio al namespace "configuration"

TaskHandle_t webToolsTaskHandle = NULL;

// Estado compartido con ui_task (bajo mutex salvo lo volátil simple).
static SemaphoreHandle_t webMutex = NULL;
static volatile uint8_t s_status = WEB_STARTING;
static char s_ip[24] = {0};
static char s_token[7] = {0};
static volatile uint8_t s_mode = WEB_MODE_AP;

// -------- Utilidades --------

static void genToken() {
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 0; i < 6; i++) s_token[i] = hex[esp_random() & 0x0F];
    s_token[6] = '\0';
}

// Escapa una cadena para incrustarla en JSON (comillas y barra invertida).
static String jsonEsc(const String &in) {
    String out;
    out.reserve(in.length() + 4);
    for (size_t i = 0; i < in.length(); i++) {
        char c = in[i];
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

// Valida la cookie de sesión contra el token vigente.
static bool checkAuth() {
    if (!webServer.hasHeader("Cookie")) return false;
    String cookie = webServer.header("Cookie");
    return cookie.indexOf(String("auth=") + s_token) >= 0;
}

// -------- Handlers --------

static void handleRoot() { webServer.send_P(200, "text/html", WEB_UI_HTML); }

// POST /api/login: valida token -> set-cookie de sesión.
static void handleLogin() {
    if (webServer.arg("token") == s_token) {
        webServer.sendHeader("Set-Cookie", String("auth=") + s_token + "; Path=/; SameSite=Strict");
        webServer.send(200, "application/json", "{\"ok\":true}");
    } else {
        webServer.send(401, "application/json", "{\"ok\":false}");
    }
}

// GET /api/config: estado + config (sin passwords).
static void handleGetConfig() {
    if (!checkAuth()) { webServer.send(401, "application/json", "{\"ok\":false}"); return; }

    uint8_t freq = wprefs.getUChar("frequency", FREQ_433MHZ);
    uint8_t preset = wprefs.getUChar("preset", PRESET_AM650);
    uint8_t bright = wprefs.getUChar("brightness", DEFAULT_NEOPIXEL_BRIGHTNESS);
    String name = wprefs.getString("user_name", "John Doe");
    String nick = wprefs.getString("user_nick", "johndoe");
    String apSsid = wprefs.getString("ap_ssid", WEBTOOLS_AP_SSID);
    String staSsid = wprefs.getString("wifi_ssid", "");

    String j = "{";
    j += "\"frequency\":" + String(freq) + ",";
    j += "\"preset\":" + String(preset) + ",";
    j += "\"brightness\":" + String(bright) + ",";
    j += "\"user_name\":\"" + jsonEsc(name) + "\",";
    j += "\"user_nick\":\"" + jsonEsc(nick) + "\",";
    j += "\"mode\":" + String(s_mode) + ",";
    j += "\"ap_ssid\":\"" + jsonEsc(apSsid) + "\",";
    j += "\"sta_ssid\":\"" + jsonEsc(staSsid) + "\",";
    j += "\"mdns\":\"" WEBTOOLS_MDNS_HOST ".local\",";
    j += "\"ip\":\"" + String(s_ip) + "\",";
    j += "\"clients\":" + String(webGetClientCount()) + ",";
    j += "\"version\":\"" VERSION "\"";
    j += "}";
    webServer.send(200, "application/json", j);
}

// POST /api/config: valida y persiste. Aplica en vivo lo aplicable (brillo LEDs).
// Los ajustes de red se guardan pero solo surten efecto al reiniciar el portal.
static void handleSetConfig() {
    if (!checkAuth()) { webServer.send(401, "application/json", "{\"ok\":false}"); return; }

    if (webServer.hasArg("frequency")) {
        int v = webServer.arg("frequency").toInt();
        if (v >= FREQ_315MHZ && v <= FREQ_915MHZ) wprefs.putUChar("frequency", v);
    }
    if (webServer.hasArg("preset")) {
        int v = webServer.arg("preset").toInt();
        if (v >= PRESET_AM270 && v <= PRESET_FM476) wprefs.putUChar("preset", v);
    }
    if (webServer.hasArg("brightness")) {
        int v = webServer.arg("brightness").toInt();
        if (v < 0) v = 0;
        if (v > 10) v = 10;
        wprefs.putUChar("brightness", v);
        sendNeopixelIdle(v); // efecto inmediato, como saveNeopixelConfig()
    }
    if (webServer.hasArg("user_name")) wprefs.putString("user_name", webServer.arg("user_name"));
    if (webServer.hasArg("user_nick")) wprefs.putString("user_nick", webServer.arg("user_nick"));

    // Ajustes de red (se aplican en el próximo arranque del portal).
    if (webServer.hasArg("mode")) {
        int m = webServer.arg("mode").toInt();
        if (m == WEB_MODE_AP || m == WEB_MODE_STA) wprefs.putUChar("web_mode", m);
    }
    if (webServer.hasArg("ap_ssid")) wprefs.putString("ap_ssid", webServer.arg("ap_ssid"));
    if (webServer.hasArg("ap_pass")) wprefs.putString("ap_pass", webServer.arg("ap_pass"));
    if (webServer.hasArg("sta_ssid")) wprefs.putString("wifi_ssid", webServer.arg("sta_ssid"));
    if (webServer.hasArg("sta_pass")) wprefs.putString("wifi_pass", webServer.arg("sta_pass"));

    webServer.send(200, "application/json", "{\"ok\":true}");
}

// -------- Gestión de ficheros (LittleFS) --------

// Directorios que expone el gestor (coinciden con donde guardan las apps).
static const char *WEBTOOLS_DIRS[] = {SIMPLE_TRANSCEIVER_PATH, RAW_TRANSCEIVER_PATH, EVIL_PORTAL_PATH};

// Rechaza rutas con traversal; exige ruta absoluta.
static bool safePath(const String &p) { return p.startsWith("/") && p.indexOf("..") < 0; }

static void appendDirFiles(const char *dir, String &j, bool &first) {
    File root = LittleFS.open(dir, "r");
    if (!root || !root.isDirectory()) return;
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
        if (f.isDirectory()) continue;
        String nm = f.name();
        String full = nm.startsWith("/") ? nm : (String(dir) + "/" + nm);
        String base = full.substring(full.lastIndexOf('/') + 1);
        if (!first) j += ",";
        j += "{\"path\":\"" + jsonEsc(full) + "\",\"name\":\"" + jsonEsc(base) +
             "\",\"size\":" + String(f.size()) + "}";
        first = false;
    }
}

// GET /api/files: uso de almacenamiento + lista de ficheros de los dirs conocidos.
static void handleFilesList() {
    if (!checkAuth()) { webServer.send(401, "application/json", "{\"ok\":false}"); return; }
    String j = "{\"total\":" + String((uint32_t) LittleFS.totalBytes()) +
               ",\"used\":" + String((uint32_t) LittleFS.usedBytes()) + ",\"files\":[";
    bool first = true;
    for (auto dir : WEBTOOLS_DIRS) appendDirFiles(dir, j, first);
    j += "]}";
    webServer.send(200, "application/json", j);
}

// GET /api/file?path=...: descarga (stream) de un fichero.
static void handleFileDownload() {
    if (!checkAuth()) { webServer.send(401, "text/plain", "unauthorized"); return; }
    String path = webServer.arg("path");
    if (!safePath(path) || !LittleFS.exists(path)) { webServer.send(404, "text/plain", "not found"); return; }
    File f = LittleFS.open(path, "r");
    if (!f) { webServer.send(404, "text/plain", "not found"); return; }
    String name = path.substring(path.lastIndexOf('/') + 1);
    webServer.sendHeader("Content-Disposition", "attachment; filename=\"" + name + "\"");
    webServer.streamFile(f, "application/octet-stream");
    f.close();
}

// POST /api/file/delete?path=...: borra un fichero.
static void handleFileDelete() {
    if (!checkAuth()) { webServer.send(401, "application/json", "{\"ok\":false}"); return; }
    String path = webServer.arg("path");
    if (!safePath(path)) { webServer.send(400, "application/json", "{\"ok\":false}"); return; }
    bool ok = LittleFS.remove(path);
    webServer.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

// POST /api/file?dir=...: subida (multipart). El callback escribe por chunks; el
// handler final responde. Auth comprobada al empezar la subida.
static File s_uploadFile;
static bool s_uploadOk = false;

static void handleUpload() {
    HTTPUpload &up = webServer.upload();
    if (up.status == UPLOAD_FILE_START) {
        s_uploadOk = false;
        if (!checkAuth()) return;
        String dir = webServer.hasArg("dir") ? webServer.arg("dir") : String(RAW_TRANSCEIVER_PATH);
        if (!safePath(dir)) return;
        // Solo el nombre base (evita traversal desde el cliente).
        String fname = up.filename;
        int slash = fname.lastIndexOf('/');
        if (slash >= 0) fname = fname.substring(slash + 1);
        if (fname.isEmpty() || fname.indexOf("..") >= 0) return;
        s_uploadFile = LittleFS.open(dir + "/" + fname, "w");
        s_uploadOk = (bool) s_uploadFile;
    } else if (up.status == UPLOAD_FILE_WRITE) {
        if (s_uploadOk && s_uploadFile) s_uploadFile.write(up.buf, up.currentSize);
    } else if (up.status == UPLOAD_FILE_END || up.status == UPLOAD_FILE_ABORTED) {
        if (s_uploadFile) s_uploadFile.close();
    }
}

static void handleUploadDone() {
    if (!checkAuth()) { webServer.send(401, "application/json", "{\"ok\":false}"); return; }
    webServer.send(s_uploadOk ? 200 : 500, "application/json",
                   s_uploadOk ? "{\"ok\":true}" : "{\"ok\":false}");
}

static void handleNotFound() { webServer.send(404, "text/plain", "not found"); }

static void setupRoutes() {
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/api/login", HTTP_POST, handleLogin);
    webServer.on("/api/config", HTTP_GET, handleGetConfig);
    webServer.on("/api/config", HTTP_POST, handleSetConfig);
    webServer.on("/api/files", HTTP_GET, handleFilesList);
    webServer.on("/api/file", HTTP_GET, handleFileDownload);
    webServer.on("/api/file/delete", HTTP_POST, handleFileDelete);
    webServer.on("/api/file", HTTP_POST, handleUploadDone, handleUpload);
    webServer.onNotFound(handleNotFound);
    const char *headerKeys[] = {"Cookie"};
    webServer.collectHeaders(headerKeys, 1);
}

// -------- Bringup de red --------

static bool bringUpAp(const char *ssid, const char *pass) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    const char *pw = (pass && strlen(pass) >= 8) ? pass : NULL; // WPA2 requiere >=8
    bool ok = WiFi.softAP(ssid, pw, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // Reconfigura netif + reinicia el DHCP con el AP ya arrancado (misma secuencia
    // que el Evil Portal, que reparte IP de forma fiable). IP privada 192.168.4.1
    // para que el móvil la enrute por WiFi (172.0.0.1 no es privada y fallaba).
    IPAddress gw(192, 168, 4, 1);
    WiFi.softAPConfig(gw, gw, IPAddress(255, 255, 255, 0));
    vTaskDelay(300 / portTICK_PERIOD_MS);
    IPAddress ip = WiFi.softAPIP();
    strncpy(s_ip, ip.toString().c_str(), sizeof(s_ip) - 1);
    Serial.printf("[WEBTOOLS] AP '%s' ip=%s\n", ssid, s_ip);
    return ok;
}

// Intenta unirse a la WiFi. Devuelve true si conecta antes del timeout.
static bool bringUpSta(const char *ssid, const char *pass) {
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    Serial.printf("[WEBTOOLS] STA connecting to '%s'...\n", ssid);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
        uint32_t note;
        if (xTaskNotifyWait(0, 0, &note, 0) == pdTRUE && note == WEBTOOLS_STOP) return false;
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WEBTOOLS] STA connect timeout");
        return false;
    }
    IPAddress ip = WiFi.localIP();
    strncpy(s_ip, ip.toString().c_str(), sizeof(s_ip) - 1);
    Serial.printf("[WEBTOOLS] STA connected ip=%s\n", s_ip);
    return true;
}

// -------- Tarea --------

static void web_tools_task(void *pv) {
    WebToolsParams *params = (WebToolsParams *) pv;

    genToken();
    s_status = WEB_STARTING;
    s_ip[0] = '\0';
    s_mode = params->mode;

    wprefs.begin("configuration", false);

    bool up = false;
    if (params->mode == WEB_MODE_STA && strlen(params->staSsid) > 0) {
        up = bringUpSta(params->staSsid, params->staPass);
        if (up) { s_status = WEB_RUNNING_STA; s_mode = WEB_MODE_STA; }
        else {
            // Fallback a AP para no quedarnos sin consola.
            Serial.println("[WEBTOOLS] falling back to AP");
            up = bringUpAp(params->apSsid, params->apPass);
            if (up) { s_status = WEB_RUNNING_AP; s_mode = WEB_MODE_AP; }
        }
    } else {
        up = bringUpAp(params->apSsid, params->apPass);
        if (up) { s_status = WEB_RUNNING_AP; s_mode = WEB_MODE_AP; }
    }

    free(params); // ownership cedido a la tarea (igual que RadioTaskParams)

    if (!up) {
        s_status = WEB_FAILED;
        wprefs.end();
        WiFi.mode(WIFI_OFF);
        webToolsTaskHandle = NULL;
        vTaskDelete(NULL);
        return;
    }

    // mDNS solo en STA: ahí sirve para encontrar el badge por nombre en tu LAN.
    // En AP la IP es fija y conocida (192.168.4.1) y el responder mDNS sobre softAP
    // es innecesario (y a veces inestable).
    if (s_status == WEB_RUNNING_STA && MDNS.begin(WEBTOOLS_MDNS_HOST)) {
        MDNS.addService("http", "tcp", 80);
        Serial.println("[WEBTOOLS] mDNS -> http://" WEBTOOLS_MDNS_HOST ".local");
    }

    // Rutas registradas una única vez (los handlers leen estado global).
    static bool routesInit = false;
    if (!routesInit) { setupRoutes(); routesInit = true; }
    webServer.begin();
    Serial.printf("[WEBTOOLS] up, token=%s\n", s_token);

    uint32_t note;
    while (true) {
        if (xTaskNotifyWait(0, 0, &note, 0) == pdTRUE && note == WEBTOOLS_STOP) break;
        webServer.handleClient();
        vTaskDelay(3 / portTICK_PERIOD_MS); // cede CPU a ui_task
    }

    // Desmontaje.
    webServer.stop();
    MDNS.end();
    WiFi.softAPdisconnect(true);
    WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    wprefs.end();

    webToolsTaskHandle = NULL;
    vTaskDelete(NULL);
}

// -------- API pública --------

bool startWebToolsTask(uint8_t mode, const String &staSsid, const String &staPass,
                       const String &apSsid, const String &apPass) {
    if (webToolsTaskHandle != NULL) return false; // ya hay uno vivo
    if (!webMutex) webMutex = xSemaphoreCreateMutex();

    WebToolsParams *p = (WebToolsParams *) malloc(sizeof(WebToolsParams));
    if (!p) return false;
    p->mode = mode;
    strncpy(p->staSsid, staSsid.c_str(), sizeof(p->staSsid) - 1); p->staSsid[sizeof(p->staSsid) - 1] = '\0';
    strncpy(p->staPass, staPass.c_str(), sizeof(p->staPass) - 1); p->staPass[sizeof(p->staPass) - 1] = '\0';
    strncpy(p->apSsid, apSsid.c_str(), sizeof(p->apSsid) - 1);    p->apSsid[sizeof(p->apSsid) - 1] = '\0';
    strncpy(p->apPass, apPass.c_str(), sizeof(p->apPass) - 1);    p->apPass[sizeof(p->apPass) - 1] = '\0';

    BaseType_t ok =
        xTaskCreatePinnedToCore(web_tools_task, "web_tools_task", 8192, p, 1, &webToolsTaskHandle, 0);
    if (ok != pdPASS) {
        free(p);
        webToolsTaskHandle = NULL;
        return false;
    }
    return true;
}

void stopWebToolsTask() {
    if (webToolsTaskHandle == NULL) return;
    xTaskNotify(webToolsTaskHandle, WEBTOOLS_STOP, eSetValueWithOverwrite);
    for (int i = 0; i < 60 && webToolsTaskHandle != NULL; i++) vTaskDelay(10 / portTICK_PERIOD_MS);
}

uint8_t webGetStatus() { return s_status; }

void webGetIp(char *buf, size_t len) {
    if (!len) return;
    strncpy(buf, s_ip, len - 1);
    buf[len - 1] = '\0';
}

void webGetToken(char *buf, size_t len) {
    if (!len) return;
    strncpy(buf, s_token, len - 1);
    buf[len - 1] = '\0';
}

uint8_t webGetClientCount() {
    if (s_mode == WEB_MODE_AP) return WiFi.softAPgetStationNum();
    return (WiFi.status() == WL_CONNECTED) ? 1 : 0;
}
