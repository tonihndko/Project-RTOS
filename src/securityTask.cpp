#include "tasks.h"
#include "config.h"
#include <Arduino.h>

/**
 * @brief Security Task - Monitor failed attempts and lockout
 * Priority: MEDIUM (2)
 * Period: 1000ms
 * 
 * Workflow:
 * 1. Monitor securityState for failed attempts
 * 2. Check if lockout is active
 * 3. If lockout has expired, unlock system
 * 4. Log security events
 * 5. Optional: Send alerts if needed
 */
void vSecurityTask(void *pvParameters) {
    BaseType_t xStatus;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    static uint32_t last_log_time = 0;
    static int last_attempt_count = 0;

    xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
    Serial.println("[Security] Task started - monitoring failed attempts...");
    xSemaphoreGive(serialMutex);

    while (1) {
        uint32_t current_time = millis();
        int current_attempts = securityState.failed_attempts;

        // Check if new failed attempt occurred
        if (current_attempts != last_attempt_count) {
            last_attempt_count = current_attempts;
            
            xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
            Serial.printf("[Security] Failed attempts: %d/%d\n",
                          current_attempts, MAX_FAILED_ATTEMPTS);
            xSemaphoreGive(serialMutex);

            // If we're approaching max attempts, prepare warning
            if (current_attempts >= MAX_FAILED_ATTEMPTS - 1) {
                EventLog warningEvent;
                warningEvent.type = EVENT_ERROR;
                warningEvent.timestamp = current_time;
                warningEvent.result = 0;
                snprintf(warningEvent.message, 100, 
                         "WARNING: %d failed attempts!", current_attempts);
                xQueueSend(eventLogQueue, &warningEvent, pdMS_TO_TICKS(10));
            }
        }

        // Check if system is locked
        if (securityState.is_locked) {
            // Check if lockout duration has expired
            if (current_time >= securityState.lockout_until) {
                // Lockout expired - unlock system
                clearFailedAttempts(&securityState);

                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.println("[Security] System unlocked - lockout expired");
                xSemaphoreGive(serialMutex);

                EventLog unlockEvent;
                unlockEvent.type = EVENT_SYSTEM_LOCKOUT;
                unlockEvent.timestamp = current_time;
                unlockEvent.result = 1;
                strcpy(unlockEvent.message, "System unlocked");
                xQueueSend(eventLogQueue, &unlockEvent, pdMS_TO_TICKS(10));
            } else {
                // Still locked - log remaining time every 5 seconds
                uint32_t remaining_ms = securityState.lockout_until - current_time;
                if (current_time - last_log_time > 5000) {
                    last_log_time = current_time;
                    
                    xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                    Serial.printf("[Security] System locked - %lu ms remaining\n", 
                                  remaining_ms);
                    xSemaphoreGive(serialMutex);
                }
            }
        } else {
            // System is unlocked
            if (current_time - last_log_time > 10000) {
                last_log_time = current_time;
                
                xSemaphoreTake(serialMutex, pdMS_TO_TICKS(100));
                Serial.printf("[Security] System operational - failed attempts: %d/%d\n",
                              securityState.failed_attempts, MAX_FAILED_ATTEMPTS);
                xSemaphoreGive(serialMutex);
            }
        }

        // Periodic delay 1000ms
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SECURITY_TASK_PERIOD));
    }
}
