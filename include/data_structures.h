#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <stdint.h>
#include <time.h>

// ============ UID ENTRY STRUCTURE ============
typedef struct {
    char uid[9];              // "12345678" (4 bytes hex as string)
    char name[50];            // User full name
    uint32_t timestamp_reg;   // Registration timestamp (millis)
    uint8_t rolling_state;    // Current rolling token state (LSB)
} UIDEntry;

// ============ UID DATABASE STRUCTURE ============
typedef struct {
    UIDEntry entries[50];     // Max 50 registered users
    int count;                // Current number of entries
    uint32_t last_sync;       // Last sync timestamp with MQTT
    char version[16];         // Database version for sync
} UIDDatabase;

// ============ RFID DATA MESSAGE (for queue) ============
typedef struct {
    uint8_t uid[4];           // 4-byte UID
    uint32_t timestamp;       // millis() when detected
} RFIDData;

// ============ ROLLING TOKEN UPDATE MESSAGE (for queue) ============
typedef struct {
    uint8_t old_uid[4];       // Previous UID
    uint8_t new_uid[4];       // New UID after rolling
    char user_id[20];         // User identifier
    uint32_t timestamp;       // Update timestamp
} RollingTokenUpdate;

// ============ EVENT LOG TYPES ============
typedef enum {
    EVENT_ACCESS_GRANTED = 0,
    EVENT_ACCESS_DENIED = 1,
    EVENT_SPOOFING_DETECTED = 2,
    EVENT_SYSTEM_LOCKOUT = 3,
    EVENT_UID_REGISTERED = 4,
    EVENT_UID_DELETED = 5,
    EVENT_DATABASE_SYNCED = 6,
    EVENT_ERROR = 7
} EventType;

// ============ EVENT LOG STRUCTURE ============
typedef struct {
    EventType type;           // Event type
    uint32_t timestamp;       // Event timestamp (millis)
    uint8_t uid[4];           // Associated UID (if any)
    char user_name[50];       // User name
    uint8_t result;           // Result code (success=1, fail=0)
    char message[100];        // Additional message
} EventLog;

// ============ SECURITY STATE STRUCTURE ============
typedef struct {
    int failed_attempts;      // Current failed attempt count
    uint32_t lockout_until;   // Timestamp when lockout expires
    uint32_t last_attempt;    // Timestamp of last attempt
    uint8_t is_locked;        // Is system currently locked
} SecurityState;

// ============ HTTP REQUEST STRUCTURE ============
typedef enum {
    HTTP_GET_DASHBOARD = 0,
    HTTP_POST_REGISTER = 1,
    HTTP_DELETE_UID = 2,
    HTTP_GET_DATABASE = 3,
    HTTP_POST_RESET = 4
} HTTPRequestType;

typedef struct {
    HTTPRequestType type;
    char uid[9];
    char name[50];
    char extra_data[256];
} HTTPRequest;

// ============ MQTT MESSAGE STRUCTURE ============
typedef struct {
    char topic[64];
    char payload[512];
    uint32_t timestamp;
} MQTTMessage;

#endif // DATA_STRUCTURES_H
