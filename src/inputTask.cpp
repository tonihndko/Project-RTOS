#include "tasks.h"
#include "config.h"
#include "security.h"
#include <Arduino.h>

/**
 * @brief Input Task - Listen to RFID reader and queue detected UIDs
 * Priority: HIGH (3)
 * Period: 50ms
 * 
 * Workflow:
 * 1. Wait for rfidReadSemaphore (triggered by ISR)
 * 2. Read UID from RFID reader
 * 3. Queue UID to rfidDataQueue
 * 4. Delay 50ms for next cycle
 */
void vInputTask(void *pvParameters) {
    RFIDData rfidData;
    BaseType_t xStatus;
    TickType_t xLastWakeTime = xTaskGetTickCount();

    xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
    Serial.println("[Input] Task started - waiting for RFID data...");
    xSemaphoreGive(serialMutex);

    while (1) {
        // Wait for ISR semaphore with 50ms timeout
        if (xSemaphoreTake(rfidReadSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
            // Simulate RFID read (replace with actual SPI/hardware read)
            // For now, generate test UID
            static uint8_t test_uid[4] = {0x12, 0x34, 0x56, 0x78};
            
            // In production: read UID from RFID reader via SPI
            // uint8_t uid[4];
            // rfid_read_uid(uid);
            
            // Prepare data
            rfidData.timestamp = millis();
            rfidData.uid[0] = test_uid[0];
            rfidData.uid[1] = test_uid[1];
            rfidData.uid[2] = test_uid[2];
            rfidData.uid[3] = test_uid[3];

            // Queue to Auth Task
            xStatus = xQueueSend(rfidDataQueue, &rfidData, pdMS_TO_TICKS(10));

            if (xStatus == pdPASS) {
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.printf("[Input] UID detected: %02X%02X%02X%02X at %ld ms\n",
                              rfidData.uid[0], rfidData.uid[1],
                              rfidData.uid[2], rfidData.uid[3],
                              rfidData.timestamp);
                xSemaphoreGive(serialMutex);
            } else {
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.println("[Input] ERROR: Queue full, dropped UID");
                xSemaphoreGive(serialMutex);
            }
        }

        // Periodic delay 50ms
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(INPUT_TASK_PERIOD));
    }
}
