#include "app.h"
#include "apps.h"
#include "literals.h"
#include <RDA5807.h>
#include <cstdio>
#include <cstring>

RDA5807 rda5807;
/*********************************************************
   RDS
 *********************************************************/
bool rdsReceived = false;

char bufferStatioName[16] = {0};
char bufferRdsMsg[40] = {0};
char bufferRdsTime[20] = {0};

struct RDA5807Config {
    uint8_t volume = 3; // 0 to 15
    uint16_t frequency = 8800; // 8800 to 10800 for FM band
} rda5807_config;

// Modo de sintonía que cicla el botón OK. La frecuencia se guarda en unidades
// de 10 kHz (8800 = 88.00 MHz), así que FINE salta 0.1 MHz y FAST 1 MHz.
enum FmMode { FM_FINE, FM_FAST, FM_SCAN };
FmMode fm_mode = FM_FINE;

const char *fm_modeName(FmMode m) {
  switch (m) {
    case FM_FINE: return "Fine";
    case FM_FAST: return "Fast";
    case FM_SCAN: return "Scan";
  }
  return "";
}

// Sincroniza el chip y descarta los datos RDS de la emisora anterior.
void fm_retune(uint16_t frequency) {
  rda5807_config.frequency = frequency;
  rda5807.setFrequency(frequency);
  rdsReceived = false;
  bufferStatioName[0] = '\0';
  bufferRdsMsg[0] = '\0';
}

void fm_tunner_onStart() {
    rda5807.setup();
    rda5807.setMono(true);
    rda5807.setVolume(rda5807_config.volume);
    rda5807.setFrequency(rda5807_config.frequency);
    rda5807.setRDS(true);
    rda5807.setRdsFifo(true);
}

void fm_tunner_onStop() {}

void fm_tunner_onEvent(int evt) {
  if (evt == BTN_BACK) {
    rda5807.powerDown();
    extern App *currentApp;
    currentApp = &app_menu;
    currentApp->onStart();
  } else if (evt == BTN_OK) {
    fm_mode = (FmMode)((fm_mode + 1) % 3);
  } else if (evt == BTN_UP) {
    if (rda5807_config.volume < 15) {
      rda5807_config.volume++;
      rda5807.setVolume(rda5807_config.volume);
    }
  } else if (evt == BTN_DOWN) {
    if (rda5807_config.volume > 0) {
      rda5807_config.volume--;
      rda5807.setVolume(rda5807_config.volume);
    }
  } else if (evt == BTN_LEFT) {
    if (fm_mode == FM_SCAN) {
      // Busca la emisora anterior; seek bloquea hasta que el chip termina.
      rda5807.seek(RDA_SEEK_WRAP, RDA_SEEK_DOWN, NULL);
      fm_retune(rda5807.getFrequency());
    } else {
      uint16_t step = (fm_mode == FM_FAST) ? 100 : 10;
      if (rda5807_config.frequency >= 8800 + step) {
        fm_retune(rda5807_config.frequency - step);
      }
    }
  } else if (evt == BTN_RIGHT) {
    if (fm_mode == FM_SCAN) {
      // Busca la emisora siguiente.
      rda5807.seek(RDA_SEEK_WRAP, RDA_SEEK_UP, NULL);
      fm_retune(rda5807.getFrequency());
    } else {
      uint16_t step = (fm_mode == FM_FAST) ? 100 : 10;
      if (rda5807_config.frequency + step <= 10800) {
        fm_retune(rda5807_config.frequency + step);
      }
    }
  }
}

void fm_tunner_onDraw(U8G2 *u8g2) {
;
  // The char pointers above will be populate by the call below. So, the char pointers need to be passed by reference (pointer to pointer).
  if (rda5807.getRdsReady()) {
    char* stationName = rda5807.getRdsStationName();
    const char* programInfo = rda5807.getRdsProgramInformation();
    const char* rdsTime = rda5807.getRdsTime();
    if (stationName != nullptr && stationName[0] != '\0') {
      strncpy(bufferStatioName, stationName, sizeof(bufferStatioName) - 1);
      bufferStatioName[sizeof(bufferStatioName) - 1] = '\0';
      rdsReceived = true;
    }
    if (programInfo != nullptr && programInfo[0] != '\0') {
      strncpy(bufferRdsMsg, programInfo, sizeof(bufferRdsMsg) - 1);
      bufferRdsMsg[sizeof(bufferRdsMsg) - 1] = '\0';
    }
    if (rdsTime != nullptr && rdsTime[0] != '\0') {
      strncpy(bufferRdsTime, rdsTime, sizeof(bufferRdsTime) - 1);
      bufferRdsTime[sizeof(bufferRdsTime) - 1] = '\0';
    }
  }

  char volbuf[4];
    u8g2->clearBuffer();
    u8g2->setDrawColor(1);
    u8g2->setFontMode(1);
    u8g2->setBitmapMode(1);
    u8g2->setFont(u8g2_font_t0_22_tr);
    u8g2->drawStr(0, 33, "<");
    u8g2->drawStr(14, 33, (String(rda5807_config.frequency / 100.0, 1) + " MHz").c_str());
    u8g2->drawStr(116, 33, ">");
    if (rdsReceived) {
      u8g2->setFont(u8g2_font_6x13_tr);
      u8g2->drawStr(16, 50, bufferStatioName);
      u8g2->setFont(u8g2_font_5x8_tr);
      u8g2->drawUTF8(0, 61, bufferRdsMsg);
    } else {
      u8g2->setFont(u8g2_font_6x13_tr);
      u8g2->drawStr(16, 50, "No RDS data");
    }
    u8g2->setFont(u8g2_font_5x8_tr);
    u8g2->drawStr(2, 8, "rssi");
    u8g2->drawStr(24, 8, String(rda5807.getRssi()).c_str());
    u8g2->drawStr(50, 9, fm_modeName(fm_mode));
    snprintf(volbuf, sizeof(volbuf), "%02u", rda5807_config.volume);
    u8g2->drawStr(108, 9, volbuf);
    u8g2->setFont(u8g2_font_siji_t_6x10);
    u8g2->drawGlyph(84, 10, VOLUME_ICON);
    u8g2->drawGlyph(96, 10, VOLUME_UP_ICON);
    u8g2->drawGlyph(118, 10, VOLUME_DOWN_ICON);
    u8g2->sendBuffer();
}

App app_fm_tunner = {
  .name = "FM Tuner",
  .onStart = fm_tunner_onStart,
  .onEvent = fm_tunner_onEvent,
  .onDraw = fm_tunner_onDraw,
  .onStop = fm_tunner_onStop
};
