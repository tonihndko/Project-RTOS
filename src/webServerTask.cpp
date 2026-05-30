#include "tasks.h"
#include "config.h"
#include "webServer.h"
#include <Arduino.h>

/**
 * @brief Web Server Task - Handle HTTP registration and management
 * Priority: MEDIUM (2)
 * Period: ~100ms (event-driven)
 * 
 * Workflow:
 * 1. Web server handles incoming HTTP requests (managed by ESP Async WebServer)
 * 2. This task can periodically check server status
 * 3. Or handle requests from httpRequestQueue if needed
 */
void vWebServerTask(void *pvParameters) {
    HTTPRequest httpRequest;
    BaseType_t xStatus;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
    Serial.println("[WebServer] Task started - HTTP handlers ready");
    xSemaphoreGive(serialMutex);

    while (1) {
        // Check for HTTP requests in queue (if using synchronous handling)
        xStatus = xQueueReceive(httpRequestQueue, &httpRequest, pdMS_TO_TICKS(100));

        if (xStatus == pdPASS) {
            xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
            Serial.printf("[WebServer] Processing HTTP request type: %d\n", httpRequest.type);
            xSemaphoreGive(serialMutex);

            switch (httpRequest.type) {
                case HTTP_POST_REGISTER:
                    // Register new UID
                    if (xSemaphoreTake(databaseMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                        UIDEntry newEntry;
                        strncpy(newEntry.uid, httpRequest.uid, 8);
                        strncpy(newEntry.name, httpRequest.name, 49);
                        newEntry.timestamp_reg = millis();
                        newEntry.rolling_state = 0;

                        // Save to SPIFFS
                        if (addUIDToFile(&newEntry, SPIFFS_UID_FILE) == 0) {
                            // Also update in-memory database
                            if (uidDatabase.count < MAX_UID_ENTRIES) {
                                memcpy(&uidDatabase.entries[uidDatabase.count], 
                                       &newEntry, sizeof(UIDEntry));
                                uidDatabase.count++;

                                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                                Serial.printf("[WebServer] ✓ UID registered: %s (%s)\n",
                                              newEntry.uid, newEntry.name);
                                xSemaphoreGive(serialMutex);

                                // Publish to MQTT
                                publishUIDRegistration(&newEntry);

                                // Log event
                                EventLog regEvent;
                                regEvent.type = EVENT_UID_REGISTERED;
                                regEvent.timestamp = millis();
                                regEvent.result = 1;
                                strcpy(regEvent.user_name, newEntry.name);
                                snprintf(regEvent.message, 100, "UID %s registered", newEntry.uid);
                                xQueueSend(eventLogQueue, &regEvent, pdMS_TO_TICKS(10));
                            }
                        }

                        xSemaphoreGive(databaseMutex);
                    }
                    break;

                case HTTP_DELETE_UID:
                    // Delete UID
                    if (xSemaphoreTake(databaseMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                        if (deleteUIDFromFile(httpRequest.uid, SPIFFS_UID_FILE) == 0) {
                            // Update in-memory database
                            for (int i = 0; i < uidDatabase.count; i++) {
                                if (strcmp(uidDatabase.entries[i].uid, httpRequest.uid) == 0) {
                                    // Remove by shifting
                                    for (int j = i; j < uidDatabase.count - 1; j++) {
                                        memcpy(&uidDatabase.entries[j],
                                               &uidDatabase.entries[j + 1],
                                               sizeof(UIDEntry));
                                    }
                                    uidDatabase.count--;
                                    break;
                                }
                            }

                            xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                            Serial.printf("[WebServer] ✓ UID deleted: %s\n", httpRequest.uid);
                            xSemaphoreGive(serialMutex);

                            // Log event
                            EventLog delEvent;
                            delEvent.type = EVENT_UID_DELETED;
                            delEvent.timestamp = millis();
                            delEvent.result = 1;
                            snprintf(delEvent.message, 100, "UID %s deleted", httpRequest.uid);
                            xQueueSend(eventLogQueue, &delEvent, pdMS_TO_TICKS(10));
                        }

                        xSemaphoreGive(databaseMutex);
                    }
                    break;

                default:
                    break;
            }
        }

        // Periodic delay 100ms
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(WEBSERVER_PERIOD));
    }
}
