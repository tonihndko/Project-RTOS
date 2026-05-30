#include "tasks.h"
#include "config.h"
#include "comm.h"
#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * @brief Communication Task - Sync with MQTT broker
 * Priority: MEDIUM (2)
 * Period: 500ms
 * 
 * Workflow:
 * 1. Check MQTT connection
 * 2. Receive rolling token updates from Auth Task
 * 3. Publish rolling token update to MQTT
 * 4. Receive event logs and publish them
 * 5. Periodically pull database updates from broker
 * 6. Update local database with remote changes
 */
void vCommTask(void *pvParameters) {
    RollingTokenUpdate rollingUpdate;
    EventLog eventLog;
    BaseType_t xStatus;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static uint32_t last_sync_time = 0;
    const uint32_t SYNC_INTERVAL = 5000;  // Sync every 5 seconds

    xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
    Serial.println("[Comm] Task started - MQTT communication ready");
    xSemaphoreGive(serialMutex);

    while (1) {
        uint32_t current_time = millis();

        // Ensure MQTT connection
        if (!isMQTTConnected()) {
            if (reconnectMQTT() != 0) {
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.println("[Comm] WARNING: MQTT not connected");
                xSemaphoreGive(serialMutex);
            }
        }

        // Process rolling token updates from Auth Task
        xStatus = xQueueReceive(rollingTokenQueue, &rollingUpdate, pdMS_TO_TICKS(10));
        if (xStatus == pdPASS && isMQTTConnected()) {
            // Publish rolling token update
            if (publishRollingTokenUpdate(&rollingUpdate) == 0) {
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.printf("[Comm] ✓ Published rolling token update for %s\n",
                              rollingUpdate.user_id);
                xSemaphoreGive(serialMutex);
            } else {
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.println("[Comm] WARNING: Failed to publish rolling token update");
                xSemaphoreGive(serialMutex);
            }
        }

        // Process event logs
        xStatus = xQueueReceive(eventLogQueue, &eventLog, pdMS_TO_TICKS(10));
        if (xStatus == pdPASS && isMQTTConnected()) {
            // Publish event log
            if (publishEventLog(&eventLog) == 0) {
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.printf("[Comm] ✓ Published event: %s\n", eventLog.message);
                xSemaphoreGive(serialMutex);
            }
        }

        // Periodically sync database with server
        if (current_time - last_sync_time > SYNC_INTERVAL) {
            last_sync_time = current_time;

            if (isMQTTConnected()) {
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.println("[Comm] Requesting database sync from server...");
                xSemaphoreGive(serialMutex);

                // Send sync request
                if (publishDatabaseSyncRequest() == 0) {
                    // In production: wait for incoming message to update database
                    // For now, just log the request
                    xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                    Serial.println("[Comm] Sync request sent");
                    xSemaphoreGive(serialMutex);
                }
            }
        }

        // Keep-alive: publish periodic status
        static uint32_t last_status_time = 0;
        if (current_time - last_status_time > 30000) {  // Every 30 seconds
            last_status_time = current_time;

            if (isMQTTConnected()) {
                // Optionally publish system status
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.println("[Comm] System status OK");
                xSemaphoreGive(serialMutex);
            }
        }

        // Periodic delay 500ms
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(COMM_TASK_PERIOD));
    }
}
