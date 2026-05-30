#include "spiffs.h"
#include "config.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <stdio.h>

// ============ SPIFFS INITIALIZATION ============

int initSPIFFS() {
    if (!SPIFFS.begin(true)) {  // true = format if mount failed
        return -1;
    }
    return 0;
}

int isSPIFFSMounted() {
    return SPIFFS.mounted() ? 1 : 0;
}

// ============ UID DATABASE FILE OPERATIONS ============

int loadUIDsFromFile(UIDDatabase *db, const char *filepath) {
    if (db == NULL || filepath == NULL) {
        return -1;
    }

    if (!SPIFFS.exists(filepath)) {
        // File doesn't exist, initialize empty database
        db->count = 0;
        db->last_sync = millis();
        strcpy(db->version, "1.0");
        return 0;
    }

    File file = SPIFFS.open(filepath, "r");
    if (!file) {
        return -1;
    }

    // Read JSON and parse
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        return -1;
    }

    // Extract database fields
    db->count = 0;
    if (doc.containsKey("entries")) {
        JsonArray entries = doc["entries"];
        for (JsonObject entry : entries) {
            if (db->count >= MAX_UID_ENTRIES) {
                break;
            }

            strlcpy(db->entries[db->count].uid, entry["uid"] | "", 9);
            strlcpy(db->entries[db->count].name, entry["name"] | "", 50);
            db->entries[db->count].timestamp_reg = entry["timestamp_reg"] | 0;
            db->entries[db->count].rolling_state = entry["rolling_state"] | 0;
            db->count++;
        }
    }

    if (doc.containsKey("last_sync")) {
        db->last_sync = doc["last_sync"];
    } else {
        db->last_sync = millis();
    }

    if (doc.containsKey("version")) {
        strlcpy(db->version, doc["version"] | "1.0", 16);
    } else {
        strcpy(db->version, "1.0");
    }

    return 0;
}

int saveUIDsToFile(UIDDatabase *db, const char *filepath) {
    if (db == NULL || filepath == NULL) {
        return -1;
    }

    StaticJsonDocument<4096> doc;

    // Add entries array
    JsonArray entries = doc.createNestedArray("entries");
    for (int i = 0; i < db->count; i++) {
        JsonObject entry = entries.createNestedObject();
        entry["uid"] = db->entries[i].uid;
        entry["name"] = db->entries[i].name;
        entry["timestamp_reg"] = db->entries[i].timestamp_reg;
        entry["rolling_state"] = db->entries[i].rolling_state;
    }

    // Add metadata
    doc["count"] = db->count;
    doc["last_sync"] = db->last_sync;
    doc["version"] = db->version;

    // Write to file
    File file = SPIFFS.open(filepath, "w");
    if (!file) {
        return -1;
    }

    serializeJson(doc, file);
    file.close();

    return 0;
}

int addUIDToFile(UIDEntry *entry, const char *filepath) {
    if (entry == NULL || filepath == NULL) {
        return -1;
    }

    // Load existing database
    UIDDatabase db;
    if (loadUIDsFromFile(&db, filepath) != 0) {
        return -1;
    }

    // Check if UID already exists
    for (int i = 0; i < db.count; i++) {
        if (strcmp(db.entries[i].uid, entry->uid) == 0) {
            return -1;  // Duplicate UID
        }
    }

    // Add new entry
    if (db.count >= MAX_UID_ENTRIES) {
        return -1;  // Database full
    }

    memcpy(&db.entries[db.count], entry, sizeof(UIDEntry));
    db.count++;
    db.last_sync = millis();

    // Save back to file
    return saveUIDsToFile(&db, filepath);
}

int deleteUIDFromFile(const char *uid_hex, const char *filepath) {
    if (uid_hex == NULL || filepath == NULL) {
        return -1;
    }

    // Load existing database
    UIDDatabase db;
    if (loadUIDsFromFile(&db, filepath) != 0) {
        return -1;
    }

    // Find and remove UID
    int found_index = -1;
    for (int i = 0; i < db.count; i++) {
        if (strcmp(db.entries[i].uid, uid_hex) == 0) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) {
        return -1;  // UID not found
    }

    // Remove by shifting remaining entries
    for (int i = found_index; i < db.count - 1; i++) {
        memcpy(&db.entries[i], &db.entries[i + 1], sizeof(UIDEntry));
    }
    db.count--;
    db.last_sync = millis();

    // Save back to file
    return saveUIDsToFile(&db, filepath);
}

int uidExistsInFile(const char *uid_hex, const char *filepath) {
    if (uid_hex == NULL || filepath == NULL) {
        return 0;
    }

    UIDDatabase db;
    if (loadUIDsFromFile(&db, filepath) != 0) {
        return 0;
    }

    for (int i = 0; i < db.count; i++) {
        if (strcmp(db.entries[i].uid, uid_hex) == 0) {
            return 1;
        }
    }

    return 0;
}

// ============ SPIFFS FILE UTILITIES ============

int fileExists(const char *filepath) {
    if (filepath == NULL) {
        return 0;
    }
    return SPIFFS.exists(filepath) ? 1 : 0;
}

int deleteFile(const char *filepath) {
    if (filepath == NULL) {
        return -1;
    }
    return SPIFFS.remove(filepath) ? 0 : -1;
}

int getFileSize(const char *filepath) {
    if (filepath == NULL) {
        return -1;
    }

    if (!SPIFFS.exists(filepath)) {
        return -1;
    }

    File file = SPIFFS.open(filepath, "r");
    if (!file) {
        return -1;
    }

    int size = file.size();
    file.close();
    return size;
}

void listSPIFFSFiles() {
    File root = SPIFFS.open("/");
    File file = root.openNextFile();

    Serial.println("SPIFFS Files:");
    while (file) {
        Serial.printf("  %s - %d bytes\n", file.name(), file.size());
        file = root.openNextFile();
    }
}

int getSPIFFSInfo(uint32_t *total_bytes, uint32_t *used_bytes) {
    if (total_bytes == NULL || used_bytes == NULL) {
        return -1;
    }

    *total_bytes = SPIFFS.totalBytes();
    *used_bytes = SPIFFS.usedBytes();

    return 0;
}
