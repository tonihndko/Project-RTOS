#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ============ GPIO PIN DEFINITIONS ============
#define BUZZER_PIN          14
#define GREEN_LED           26
#define RED_LED             25
#define RELAY_PIN           27
#define RFID_INT_PIN        4    // RFID interrupt pin

// ============ I2C LCD CONFIGURATION ============
#define LCD_ADDR            0x27  // LCD I2C address
#define LCD_COLS            16
#define LCD_ROWS            2

// ============ FREERTOS TASK PERIODS (ms) ============
#define INPUT_TASK_PERIOD   50
#define AUTH_TASK_PERIOD    100
#define WEBSERVER_PERIOD    100
#define COMM_TASK_PERIOD    500
#define SECURITY_TASK_PERIOD 1000
#define DISPLAY_TASK_PERIOD 250

// ============ FREERTOS TASK STACK SIZES (bytes) ============
#define INPUT_TASK_STACK    2048
#define AUTH_TASK_STACK     3072
#define WEBSERVER_STACK     4096
#define COMM_TASK_STACK     3072
#define SECURITY_TASK_STACK 2048
#define DISPLAY_TASK_STACK  2048

// ============ FREERTOS TASK PRIORITIES ============
#define INPUT_PRIORITY      3    // HIGH
#define AUTH_PRIORITY       3    // HIGH
#define WEBSERVER_PRIORITY  2    // MEDIUM
#define COMM_PRIORITY       2    // MEDIUM
#define SECURITY_PRIORITY   2    // MEDIUM
#define DISPLAY_PRIORITY    1    // LOW

// ============ QUEUE & SEMAPHORE SIZES ============
#define RFID_DATA_QUEUE_SIZE    5
#define ROLLING_TOKEN_QUEUE_SIZE 5
#define EVENT_LOG_QUEUE_SIZE    10
#define HTTP_REQUEST_QUEUE_SIZE 3

// ============ ROLLING TOKEN CONFIGURATION ============
#define ROLLING_TOKEN_STEP  1
#define UID_MASK_LAST_3     0xFFF  // Mask untuk 3 digit terakhir

// ============ SECURITY CONFIGURATION ============
#define MAX_FAILED_ATTEMPTS 3
#define LOCKOUT_DURATION_MS 30000  // 30 seconds

// ============ MQTT CONFIGURATION ============
#define MQTT_BROKER         "test.mosquitto.org"
#define MQTT_PORT           1883
#define MQTT_CLIENT_ID      "esp32-rfid-01"
#define MQTT_RECONNECT_TIME 5000

// ============ MQTT TOPICS ============
#define TOPIC_UID_UPDATE    "attendance/rfid/uid_update"
#define TOPIC_EVENT_LOG     "attendance/rfid/event_log"
#define TOPIC_REGISTER      "attendance/rfid/register"
#define TOPIC_DATABASE_PULL "attendance/rfid/database/pull"

// ============ SPIFFS CONFIGURATION ============
#define SPIFFS_UID_FILE     "/spiffs/uids.json"
#define MAX_UID_ENTRIES     50

// ============ LED FEEDBACK TIMINGS (ms) ============
#define LED_SUCCESS_PULSE   50
#define LED_FAIL_PULSE      200
#define BUZZER_SUCCESS_MS   100
#define BUZZER_FAIL_MS      300

// ============ WIFI CONFIGURATION ============
#define WIFI_SSID           "your_ssid"         // TODO: Update
#define WIFI_PASSWORD       "your_password"     // TODO: Update
#define WIFI_TIMEOUT_MS     10000

// ============ WEB SERVER CONFIGURATION ============
#define WEB_SERVER_PORT     8080

// ============ DEBUG LOGGING ============
#define DEBUG_SERIAL        1

#endif // CONFIG_H
