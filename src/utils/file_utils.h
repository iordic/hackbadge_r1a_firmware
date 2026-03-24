#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <Arduino.h>
#include <LittleFS.h>
#include <SimpleList.h>

class FileUtils {
    public:
        static bool begin();
        static bool save(String path, String fileName, uint8_t* data, size_t size);
        static bool load(String path, String fileName, uint8_t* data, size_t size);
        static SimpleList<String>* listFiles(String path);
    private:
        static void _mkdirs(String path);
};
#endif
