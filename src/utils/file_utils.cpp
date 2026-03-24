#include "file_utils.h"

bool FileUtils::begin() {
    if(LittleFS.begin(true)) {
        Serial.printf("LittleFS mounted! Used: %u/%u bytes\n", LittleFS.usedBytes(), LittleFS.totalBytes());
        return true;
    }
    return false;
}

void FileUtils::_mkdirs(String path) {
    String currentPath = "";
    int start = 0;
    int end = path.indexOf('/', 1);
    while (end != -1) {
        currentPath = path.substring(0, end);
        if (!LittleFS.exists(currentPath)) {
            LittleFS.mkdir(currentPath);
        }
        end = path.indexOf('/', end + 1);
    }
    if (!LittleFS.exists(path)) {
        LittleFS.mkdir(path);
    }
}

bool FileUtils::save(String path, String fileName, uint8_t* data, size_t size) {
    _mkdirs(path);
    String full_path = path;
    if (!full_path.endsWith("/"))
        full_path += "/";
    full_path += fileName;
    File file = LittleFS.open(full_path, "w");
    if (!file)
        return false;
    size_t written = file.write(data, size);
    file.close();
    return written == size;
}

bool FileUtils::remove(String path, String fileName) {
    String full_path = path;
    if (path.length() > 0 && !full_path.endsWith("/")) {
        full_path += "/";
    }
    full_path += fileName;
    if (!LittleFS.exists(full_path)) {
        return false; 
    }
    return LittleFS.remove(full_path);
}

bool FileUtils::load(String path, String fileName, uint8_t* data, size_t size) {
    String full_path = path;
    if (!full_path.endsWith("/"))
        full_path += "/";
    full_path += fileName;
    if (!LittleFS.exists(full_path)) return false;
    File file = LittleFS.open(full_path, "r");
    if (!file) return false;
    size_t readed = file.read(data, size);
    file.close();
    return readed == size;
}

int FileUtils::getFileSize(String path, String fileName) {
    String full_path = path;
    if (!full_path.endsWith("/"))
        full_path += "/";
    full_path += fileName;
    if (!LittleFS.exists(full_path)) return 0;
    File file = LittleFS.open(full_path, "r");
    if (!file) return 0;
    int size = file.size();
    file.close();
    return size;
}

SimpleList<String>* FileUtils::listFiles(String path) {
    // TODO: create pagination
    SimpleList<String>* fileList = new SimpleList<String>();
    File root = LittleFS.open(path, "r");
    if (!root || !root.isDirectory()) {
        return fileList;
    }
    File file = root.openNextFile();
    while (file) {
        if (!file.isDirectory()) {
            fileList->add(file.name());
        }
        file = root.openNextFile();
    }
    return fileList;
}