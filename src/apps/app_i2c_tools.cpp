#include <Wire.h>

#include "config/io_config.h"
#include "literals.h"
#include "app.h"
#include "apps.h"

// ---------------------------------------------------------------------------
// App I2C Tools. Dos vistas:
//   - GRID  : parrilla de direcciones 0x00..0x7F. Las flechas mueven el cursor,
//             OK abre la dirección seleccionada y BACK sale al menú.
//   - DEVICE: lectura/escritura de un registro. LEFT/RIGHT elige nibble
//             (REG alto/bajo, WR alto/bajo), UP/DOWN lo cambia, OK escribe
//             [addr][reg][val] y BACK vuelve a la parrilla.
// El registro se relee cada I2C_POLL_MS: en I2C no llega nada solo, es el
// maestro quien tiene que ir a buscar el valor.
// ---------------------------------------------------------------------------

enum I2cView { I2C_GRID, I2C_DEVICE };
static I2cView i2cView = I2C_GRID;

int i2cCursor = 0;

// Resultado del último escaneo, un bit por dirección (128 bits = 16 bytes).
// La parrilla se pinta desde esta caché en vez de sondear el bus entero en
// cada frame (~50 veces/s), que tenía ocupados el bus y el ui_task para nada.
static uint8_t i2cFound[16];
static uint32_t i2cLastScan = 0;
static const uint32_t I2C_SCAN_MS = 1500;
// El barrido completo son 128 transacciones y el ui_task solo mira los botones
// una vez por tick (20 ms), así que hacerlas todas seguidas se traga
// pulsaciones. Se reparte en tandas: i2cScanNext es la siguiente dirección a
// sondear y 128 significa barrido terminado.
static const int I2C_SCAN_CHUNK = 16;
static int i2cScanNext = 128;
// Timeout de Wire para nuestras transacciones. El de por defecto son 50 ms
// (Wire.cpp), una eternidad para un tick de 20 ms: un solo dispositivo que
// estire el reloj o falle se lleva por delante varias pulsaciones. Se pone
// justo antes de tocar el bus y se restaura, para no alterar al OLED, que
// cuelga del mismo Wire y necesita el margen de siempre.
static const uint16_t I2C_TIMEOUT_MS = 10;

// Estado de la vista de dispositivo. REG y WR se conservan al cambiar de
// dirección, que es lo cómodo para mirar el mismo registro en varios chips.
static const uint32_t I2C_POLL_MS = 300;
static uint8_t i2cReg = 0;         // registro a leer/escribir
static uint8_t i2cWriteValue = 0;  // valor a escribir
static uint8_t i2cNibble = 0;      // 0-1: nibbles de REG, 2-3: nibbles de WR
static uint8_t i2cReadValue = 0;
static bool i2cReadOk = false;
static uint32_t i2cLastPoll = 0;
// El OLED cuelga del mismo bus (SCREEN_ADDRESS): escribirle bytes sueltos son
// comandos para él y deja la pantalla inservible. El primer OK avisa, el
// segundo confirma.
static bool i2cWriteArmed = false;
// Resultado de la última escritura; se borra solo pasado I2C_STATUS_MS.
static const uint32_t I2C_STATUS_MS = 1200;
static char i2cStatus[16] = "";
static uint32_t i2cStatusAt = 0;

// Posición en X de los dos nibbles de cada campo editable.
static const int I2C_NIBBLE_X[2] = {26, 36};

boolean check_i2c_address(int address);
static void i2c_startScan();
static void i2c_scanChunk();
static bool i2c_isPresent(int address);
static void i2c_setPresent(int address, bool present);
static bool i2c_readRegister(int address, uint8_t reg, uint8_t *value);
static bool i2c_writeRegister(int address, uint8_t reg, uint8_t value);
static void i2c_pollRegister();
static void i2c_enterDevice();
static void i2c_bumpNibble(int delta);
static void i2c_doWrite();
static void i2c_setStatus(const char *message);
static void i2c_gridEvent(int evt);
static void i2c_deviceEvent(int evt);
static void i2c_drawGrid(U8G2 *u8g2);
static void i2c_drawDevice(U8G2 *u8g2);

void i2c_tools_onStart() {
    i2cView = I2C_GRID;
    i2c_startScan();
}

void i2c_tools_onStop() {

}

void i2c_tools_onEvent(int evt) {
    if (i2cView == I2C_DEVICE) i2c_deviceEvent(evt);
    else i2c_gridEvent(evt);
}

void i2c_tools_onDraw(U8G2 *u8g2) {
    // El ui_task solo llama a onEvent cuando hay pulsación, así que el sondeo
    // periódico del bus tiene que colgar del ciclo de dibujo.
    if (i2cView == I2C_DEVICE) {
        i2c_pollRegister();
        i2c_drawDevice(u8g2);
    } else {
        if (i2cScanNext < 128) i2c_scanChunk();
        else if (millis() - i2cLastScan >= I2C_SCAN_MS) i2c_startScan();
        i2c_drawGrid(u8g2);
    }
}

// --- Acceso al bus ------------------------------------------------------------

boolean check_i2c_address(int address) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
        return true;
    } else {
        return false;
    }
}

static void i2c_startScan() {
    i2cScanNext = 0;
}

// Una tanda de direcciones por frame. Se van actualizando los bits sobre la
// marcha en vez de borrar la caché al empezar, para que la parrilla no
// parpadee a "todo vacío" mientras dura el barrido.
static void i2c_scanChunk() {
    uint16_t previousTimeout = Wire.getTimeOut();
    Wire.setTimeOut(I2C_TIMEOUT_MS);
    int last = i2cScanNext + I2C_SCAN_CHUNK;
    if (last > 128) last = 128;
    for (; i2cScanNext < last; i2cScanNext++) {
        i2c_setPresent(i2cScanNext, check_i2c_address(i2cScanNext));
    }
    Wire.setTimeOut(previousTimeout);
    if (i2cScanNext >= 128) i2cLastScan = millis();
}

static bool i2c_isPresent(int address) {
    return (i2cFound[address >> 3] & (1 << (address & 0x07))) != 0;
}

static void i2c_setPresent(int address, bool present) {
    if (present) i2cFound[address >> 3] |= 1 << (address & 0x07);
    else i2cFound[address >> 3] &= ~(1 << (address & 0x07));
}

// Lectura clásica de registro: se escribe el índice y, sin soltar el bus
// (repeated start), se pide el byte.
static bool i2c_readRegister(int address, uint8_t reg, uint8_t *value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    // Ojo: endTransmission(false) no llega a tocar el bus, solo marca la
    // transacción como "sin stop" y devuelve 0 siempre. Quien la ejecuta de
    // verdad, y quien informa del fallo, es el requestFrom de abajo.
    Wire.endTransmission(false);
    if (Wire.requestFrom(address, 1) != 1) return false;
    *value = Wire.read();
    return true;
}

static bool i2c_writeRegister(int address, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

static void i2c_pollRegister() {
    if (millis() - i2cLastPoll < I2C_POLL_MS) return;
    i2cLastPoll = millis();
    uint8_t value = 0;
    uint16_t previousTimeout = Wire.getTimeOut();
    Wire.setTimeOut(I2C_TIMEOUT_MS);
    i2cReadOk = i2c_readRegister(i2cCursor, i2cReg, &value);
    Wire.setTimeOut(previousTimeout);
    if (i2cReadOk) i2cReadValue = value;
}

// --- Eventos ------------------------------------------------------------------

static void i2c_gridEvent(int evt) {
    if (evt == BTN_LEFT) {
        if ((i2cCursor & 0x0f) == 0) i2cCursor = i2cCursor | 0x0f;
        else i2cCursor = i2cCursor - 1;
    } else if (evt == BTN_RIGHT) {
        if ((i2cCursor & 0x0f) == 0x0f) i2cCursor = i2cCursor & 0xf0;
        else i2cCursor = i2cCursor + 1;
    } else if (evt == BTN_UP) {
        if ((i2cCursor & 0xf0) == 0) i2cCursor = i2cCursor | 0x70;
        else i2cCursor = i2cCursor - 0x10;
    } else if (evt == BTN_DOWN) {
        if ((i2cCursor & 0x70) == 0x70) i2cCursor = i2cCursor & 0x0f;
        else i2cCursor = i2cCursor + 0x10;
    }

    if (evt == BTN_OK) {
        // Solo se entra si hay alguien en esa dirección: en un hueco vacío no
        // hay nada que leer ni escribir. Se sondea en vivo en lugar de mirar la
        // caché, que lleva hasta I2C_SCAN_MS de retraso; de paso la casilla
        // queda actualizada, que es el aviso de por qué no se ha entrado.
        uint16_t previousTimeout = Wire.getTimeOut();
        Wire.setTimeOut(I2C_TIMEOUT_MS);
        bool present = check_i2c_address(i2cCursor);
        Wire.setTimeOut(previousTimeout);
        i2c_setPresent(i2cCursor, present);
        if (present) i2c_enterDevice();
        return;
    }

    if (evt == BTN_BACK) {
        extern App *currentApp;
        currentApp = &app_menu;
        currentApp->onStart();
    }
}

static void i2c_deviceEvent(int evt) {
    if (evt == BTN_OK) {
        i2c_doWrite();
        return;
    }
    if (evt == BTN_BACK) {
        i2cView = I2C_GRID;
        i2c_startScan();
        return;
    }
    // Moverse cancela una escritura pendiente de confirmar.
    i2cWriteArmed = false;
    if (evt == BTN_LEFT) i2cNibble = (i2cNibble + 3) & 0x03;
    else if (evt == BTN_RIGHT) i2cNibble = (i2cNibble + 1) & 0x03;
    else if (evt == BTN_UP) i2c_bumpNibble(1);
    else if (evt == BTN_DOWN) i2c_bumpNibble(-1);
}

static void i2c_enterDevice() {
    i2cView = I2C_DEVICE;
    i2cNibble = 0;
    i2cReadOk = false;
    i2cWriteArmed = false;
    i2cLastPoll = 0;
    i2cStatus[0] = '\0';
}

static void i2c_bumpNibble(int delta) {
    uint8_t *field = (i2cNibble < 2) ? &i2cReg : &i2cWriteValue;
    bool high = (i2cNibble & 1) == 0;
    uint8_t nibble = (high ? (*field >> 4) : *field) & 0x0f;
    nibble = (nibble + delta) & 0x0f;
    *field = high ? ((nibble << 4) | (*field & 0x0f)) : ((*field & 0xf0) | nibble);
    // Si cambia el registro, el valor leído del anterior ya no vale.
    if (i2cNibble < 2) i2cReadOk = false;
}

static void i2c_doWrite() {
    if (i2cCursor == SCREEN_ADDRESS && !i2cWriteArmed) {
        i2cWriteArmed = true;
        return;
    }
    i2cWriteArmed = false;
    char message[16];
    uint16_t previousTimeout = Wire.getTimeOut();
    Wire.setTimeOut(I2C_TIMEOUT_MS);
    bool written = i2c_writeRegister(i2cCursor, i2cReg, i2cWriteValue);
    Wire.setTimeOut(previousTimeout);
    if (written) {
        snprintf(message, sizeof(message), "wrote %02X", i2cWriteValue);
    } else {
        snprintf(message, sizeof(message), "write NAK");
    }
    i2c_setStatus(message);
    i2cLastPoll = 0;   // releer ya, para ver el efecto de la escritura
}

static void i2c_setStatus(const char *message) {
    snprintf(i2cStatus, sizeof(i2cStatus), "%s", message);
    i2cStatusAt = millis();
}

// --- Dibujo -------------------------------------------------------------------

static void i2c_drawGrid(U8G2 *u8g2) {
boolean detected = false;
u8g2->clearBuffer();
u8g2->setDrawColor(1);
u8g2->setFontDirection(3);
u8g2->setFont(u8g2_font_Wizzard_tr);
u8g2->drawStr(10, 63, ("cursor: " + String(i2cCursor, HEX)).c_str());
u8g2->setFont(u8g2_font_5x7_tr);
u8g2->setFontDirection(0);
// draw cols header
for (int i = 0; i < 16; i++) u8g2->drawStr(30 + i*6, 10, String(i, HEX).c_str());
// draw rows header
for (int i = 0; i < 8; i++) u8g2->drawStr(20, 20 + i*6, String(i, HEX).c_str());

u8g2->drawLine(15, 1, 26, 12);
u8g2->drawLine(16, 13, 126, 13);
u8g2->drawLine(27, 1, 27, 62);
u8g2->drawFrame(15, 1, 113, 63);
u8g2->drawStr(17, 11, "x");
u8g2->drawStr(23, 7, "y");
char c;
for (int i = 0; i < 8; i++) {
  for (int j = 0; j < 16; j++) {
    int address = (i << 4) | j;
    if (address == i2cCursor) {
        u8g2->drawBox(30 + j*6, 20 + (i-1)*6, 5, 7);
        u8g2->setDrawColor(0);
    } else {
        u8g2->setDrawColor(1);
    }
    detected = i2c_isPresent(address);
    if (detected && address == SCREEN_ADDRESS) c = 'S';
    else c = detected ? 'x' : '-';
    u8g2->drawStr(30 + j*6, 20 + i*6, String(c).c_str());
  }
}
u8g2->sendBuffer();
}

static void i2c_drawDevice(U8G2 *u8g2) {
    char buffer[16];
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    u8g2->setFontDirection(0);
    u8g2->setFont(u8g2_font_5x7_tr);

    // Cabecera: dirección abierta y si el último sondeo llegó a su destino.
    // Solo se entra aquí con dispositivo presente, así que "no ack" significa
    // que ha dejado de responder (desconectado) o que rechaza ese registro.
    snprintf(buffer, sizeof(buffer), "I2C 0x%02X", i2cCursor);
    u8g2->drawStr(2, 7, buffer);
    const char *ack = i2cReadOk ? "ack" : "no ack";
    u8g2->drawStr(126 - u8g2->getStrWidth(ack), 7, ack);
    u8g2->drawHLine(0, 10, 128);
    u8g2->drawVLine(57, 11, 40);

    // Columna izquierda: los dos campos editables, un subrayado en el nibble
    // sobre el que está el cursor.
    u8g2->drawStr(2, 24, "REG");
    u8g2->drawStr(2, 38, "WR");
    u8g2->setFont(u8g2_font_7x13_tr);
    for (int i = 0; i < 4; i++) {
        uint8_t field = (i < 2) ? i2cReg : i2cWriteValue;
        uint8_t nibble = ((i & 1) == 0) ? (field >> 4) : (field & 0x0f);
        int x = I2C_NIBBLE_X[i & 1];
        int y = (i < 2) ? 25 : 39;
        snprintf(buffer, sizeof(buffer), "%X", nibble);
        u8g2->drawStr(x, y, buffer);
        if (i == i2cNibble) u8g2->drawHLine(x, y + 2, 7);
    }

    // Columna derecha: último valor leído, en hex y en binario.
    u8g2->setFont(u8g2_font_5x7_tr);
    u8g2->drawStr(62, 24, "READ");
    u8g2->setFont(u8g2_font_7x13_tr);
    if (i2cReadOk) snprintf(buffer, sizeof(buffer), "%02X", i2cReadValue);
    else snprintf(buffer, sizeof(buffer), "--");
    u8g2->drawStr(90, 25, buffer);
    if (i2cReadOk) {
        u8g2->setFont(u8g2_font_5x7_tr);
        for (int i = 0; i < 8; i++) buffer[i + i/4] = (i2cReadValue & (0x80 >> i)) ? '1' : '0';
        buffer[4] = ' ';
        buffer[9] = '\0';
        u8g2->drawStr(62, 38, buffer);
    }

    u8g2->setFont(u8g2_font_5x7_tr);
    u8g2->drawHLine(0, 52, 128);
    const char *footer;
    if (i2cWriteArmed) footer = "! OLED - OK confirms";
    else if (i2cStatus[0] != '\0' && millis() - i2cStatusAt < I2C_STATUS_MS) footer = i2cStatus;
    else footer = "OK:write  BACK:grid";
    u8g2->drawStr(2, 61, footer);
    u8g2->sendBuffer();
}

App app_i2c_tools = {
  .name = "I2C Tools",
  .onStart = i2c_tools_onStart,
  .onEvent = i2c_tools_onEvent,
  .onDraw = i2c_tools_onDraw,
  .onStop = i2c_tools_onStop
};
