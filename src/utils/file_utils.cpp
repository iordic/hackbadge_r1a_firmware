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

bool FileUtils::saveRaw(String path, String fileName, const RawSignal* sig) {
    if (!sig || !sig->durations || sig->count == 0) return false;
    _mkdirs(path);
    String full_path = path;
    if (!full_path.endsWith("/"))
        full_path += "/";
    full_path += fileName;
    File file = LittleFS.open(full_path, "w");
    if (!file) return false;
    RawFileHeader header;
    header.magic = RAW_FILE_MAGIC;
    header.version = RAW_FILE_VERSION;
    header.frequency = (uint8_t) sig->frequency;
    header.preset = (uint8_t) sig->preset;
    header.count = sig->count;
    size_t written = file.write((uint8_t*)&header, sizeof(RawFileHeader));
    written += file.write((uint8_t*)sig->durations, sig->count * sizeof(int32_t));
    file.close();
    return written == sizeof(RawFileHeader) + sig->count * sizeof(int32_t);
}

// Devuelve una RawSignal reservada en heap (liberar con freeRawSignal) o NULL.
RawSignal* FileUtils::loadRaw(String path, String fileName) {
    String full_path = path;
    if (!full_path.endsWith("/"))
        full_path += "/";
    full_path += fileName;
    if (!LittleFS.exists(full_path)) return NULL;
    File file = LittleFS.open(full_path, "r");
    if (!file) return NULL;
    RawFileHeader header;
    if (file.read((uint8_t*)&header, sizeof(RawFileHeader)) != sizeof(RawFileHeader) ||
        header.magic != RAW_FILE_MAGIC || header.version != RAW_FILE_VERSION ||
        header.count == 0 || header.count > RAW_MAX_CHANGES) {
        file.close();
        return NULL;
    }
    size_t dataSize = header.count * sizeof(int32_t);
    RawSignal* sig = (RawSignal*) malloc(sizeof(RawSignal));
    if (!sig) { file.close(); return NULL; }
    sig->durations = (int32_t*) malloc(dataSize);
    if (!sig->durations) { free(sig); file.close(); return NULL; }
    size_t readed = file.read((uint8_t*)sig->durations, dataSize);
    file.close();
    if (readed != dataSize) {
        free(sig->durations);
        free(sig);
        return NULL;
    }
    sig->count = header.count;
    sig->frequency = header.frequency;
    sig->preset = header.preset;
    return sig;
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