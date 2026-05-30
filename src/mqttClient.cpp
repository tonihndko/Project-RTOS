#include "comm.h"
#include "config.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>

// Global MQTT client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ============ MQTT CONNECTION ============

void initMQTT() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(onMQTTMessage);
    
    // Try initial connection
    if (WiFi.status() == WL_CONNECTED) {
        reconnectMQTT();
    }
}

int isMQTTConnected() {
    return mqttClient.connected() ? 1 : 0;
}

int reconnectMQTT() {
    if (!WiFi.isConnected()) {
        return -1;  // WiFi not connected
    }

    // Try reconnect
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
        // Subscribe to topics
        subscribeMQTTTopics();
        return 0;
    }
    return -1;
}

void* getMQTTClient() {
    return &mqttClient;
}

// ============ MQTT PUBLISH OPERATIONS ============

int publishRollingTokenUpdate(RollingTokenUpdate *update) {
    if (update == NULL || !mqttClient.connected()) {
        return -1;
    }

    StaticJsonDocument<256> doc;
    char uid_hex[9];
    char new_uid_hex[9];
    
    // Convert UID to hex strings
    snprintf(uid_hex, 9, "%02X%02X%02X%02X",
             update->old_uid[0], update->old_uid[1],
             update->old_uid[2], update->old_uid[3]);
    snprintf(new_uid_hex, 9, "%02X%02X%02X%02X",
             update->new_uid[0], update->new_uid[1],
             update->new_uid[2], update->new_uid[3]);

    doc["old_uid"] = uid_hex;
    doc["new_uid"] = new_uid_hex;
    doc["user_id"] = update->user_id;
    doc["timestamp"] = update->timestamp;

    char payload[256];
    serializeJson(doc, payload);

    return mqttClient.publish(TOPIC_UID_UPDATE, payload) ? 0 : -1;
}

int publishEventLog(EventLog *event) {
    if (event == NULL || !mqttClient.connected()) {
        return -1;
    }

    StaticJsonDocument<256> doc;
    char uid_hex[9];

    snprintf(uid_hex, 9, "%02X%02X%02X%02X",
             event->uid[0], event->uid[1],
             event->uid[2], event->uid[3]);

    doc["type"] = event->type;
    doc["timestamp"] = event->timestamp;
    doc["uid"] = uid_hex;
    doc["user_name"] = event->user_name;
    doc["result"] = event->result;
    doc["message"] = event->message;

    char payload[512];
    serializeJson(doc, payload);

    return mqttClient.publish(TOPIC_EVENT_LOG, payload) ? 0 : -1;
}

int publishUIDRegistration(UIDEntry *entry) {
    if (entry == NULL || !mqttClient.connected()) {
        return -1;
    }

    StaticJsonDocument<256> doc;
    doc["uid"] = entry->uid;
    doc["name"] = entry->name;
    doc["timestamp"] = entry->timestamp_reg;

    char payload[256];
    serializeJson(doc, payload);

    return mqttClient.publish(TOPIC_REGISTER, payload) ? 0 : -1;
}

int publishDatabaseSyncRequest() {
    if (!mqttClient.connected()) {
        return -1;
    }

    StaticJsonDocument<128> doc;
    doc["request"] = "sync_database";
    doc["timestamp"] = millis();

    char payload[128];
    serializeJson(doc, payload);

    return mqttClient.publish(TOPIC_DATABASE_PULL, payload) ? 0 : -1;
}

// ============ MQTT SUBSCRIBE/CALLBACK ============

void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
    // Handle incoming MQTT messages
    if (strcmp(topic, TOPIC_DATABASE_PULL) == 0) {
        // Database update from server
        char json_payload[512];
        if (length < 512) {
            strncpy(json_payload, (char*)payload, length);
            json_payload[length] = '\0';
            
            // Parse and merge database
            UIDDatabase remoteDB;
            if (parseDatabaseFromJSON(json_payload, &remoteDB) == 0) {
                xSemaphoreTake(databaseMutex, pdMS_TO_TICKS(500));
                mergeDatabaseUpdates(&uidDatabase, &remoteDB);
                saveUIDsToFile(&uidDatabase, SPIFFS_UID_FILE);
                xSemaphoreGive(databaseMutex);
                
                // Log sync event
                EventLog syncEvent;
                syncEvent.type = EVENT_DATABASE_SYNCED;
                syncEvent.timestamp = millis();
                syncEvent.result = 1;
                strcpy(syncEvent.message, "Database synced from server");
                xQueueSend(eventLogQueue, &syncEvent, pdMS_TO_TICKS(10));
            }
        }
    }
}

int subscribeMQTTTopics() {
    if (!mqttClient.connected()) {
        return -1;
    }

    mqttClient.subscribe(TOPIC_DATABASE_PULL);
    
    return 0;
}

// ============ DATABASE SYNC ============

int parseDatabaseFromJSON(const char *json_payload, UIDDatabase *db) {
    if (json_payload == NULL || db == NULL) {
        return -1;
    }

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, json_payload);

    if (error) {
        return -1;
    }

    db->count = 0;
    if (doc.containsKey("entries")) {
        JsonArray entries = doc["entries"];
        for (JsonObject entry : entries) {
            if (db->count >= MAX_UID_ENTRIES) break;

            strlcpy(db->entries[db->count].uid, entry["uid"] | "", 9);
            strlcpy(db->entries[db->count].name, entry["name"] | "", 50);
            db->entries[db->count].timestamp_reg = entry["timestamp_reg"] | 0;
            db->count++;
        }
    }

    if (doc.containsKey("last_sync")) {
        db->last_sync = doc["last_sync"];
    } else {
        db->last_sync = millis();
    }

    return 0;
}

int databaseToJSON(UIDDatabase *db, char *json_buffer, int buffer_size) {
    if (db == NULL || json_buffer == NULL) {
        return -1;
    }

    StaticJsonDocument<4096> doc;
    JsonArray entries = doc.createNestedArray("entries");

    for (int i = 0; i < db->count; i++) {
        JsonObject entry = entries.createNestedObject();
        entry["uid"] = db->entries[i].uid;
        entry["name"] = db->entries[i].name;
        entry["timestamp_reg"] = db->entries[i].timestamp_reg;
    }

    doc["count"] = db->count;
    doc["last_sync"] = db->last_sync;

    int json_len = serializeJson(doc, json_buffer, buffer_size);
    return (json_len > 0) ? 0 : -1;
}

int mergeDatabaseUpdates(UIDDatabase *local_db, UIDDatabase *remote_db) {
    if (local_db == NULL || remote_db == NULL) {
        return -1;
    }

    // Simple merge: if remote is newer, use remote
    if (remote_db->last_sync > local_db->last_sync) {
        memcpy(local_db, remote_db, sizeof(UIDDatabase));
    }

    return 0;
}

// ============ WIFI UTILITIES ============

int initWiFi(const char *ssid, const char *password) {
    if (ssid == NULL || password == NULL) {
        return -1;
    }

    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    return (WiFi.status() == WL_CONNECTED) ? 0 : -1;
}

int isWiFiConnected() {
    return (WiFi.status() == WL_CONNECTED) ? 1 : 0;
}

int getWiFiSignalStrength() {
    if (WiFi.status() != WL_CONNECTED) {
        return -100;  // Very weak
    }
    return WiFi.RSSI();
}

const char* getESP32IPAddress() {
    static String ip_string;
    ip_string = WiFi.localIP().toString();
    return ip_string.c_str();
}
