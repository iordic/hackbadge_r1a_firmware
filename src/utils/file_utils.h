#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <Arduino.h>
#include <LittleFS.h>
#include <SimpleList.h>
#include "tasks/radio_task.h"

// Cabecera de los ficheros RAW (tamaño variable, a diferencia de RFMessage).
#define RAW_FILE_MAGIC   0x5752   // "RW"
#define RAW_FILE_VERSION 1

typedef struct __attribute__((packed)) {
    uint16_t magic;
    uint8_t  version;
    uint8_t  frequency;   // enum AvailableFrequencies
    uint8_t  preset;      // enum PresetConfigs
    uint16_t count;       // nº de duraciones que siguen (int32_t cada una)
} RawFileHeader;

class FileUtils {
    public:
        static bool begin();
        static bool save(String path, String fileName, uint8_t* data, size_t size);
        static bool remove(String path, String fileName);
        static bool load(String path, String fileName, uint8_t* data, size_t size);
        static SimpleList<String>* listFiles(String path);
        static int getFileSize(String path, String fileName);
        // RAW: tamaño variable (cabecera + array de duraciones con signo).
        static bool saveRaw(String path, String fileName, const RawSignal* sig);
        static RawSignal* loadRaw(String path, String fileName);
    private:
        static void _mkdirs(String path);
};
#endif
