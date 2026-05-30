#ifndef SECURITY_H
#define SECURITY_H

#include <stdint.h>
#include "data_structures.h"

// ============ ROLLING TOKEN ALGORITHM ============

/**
 * @brief Calculate new UID based on rolling token algorithm
 * 
 * Algorithm:
 *  - Extract last 3 hex digits (LSB)
 *  - Add ROLLING_TOKEN_STEP
 *  - Apply modulo 0xFFF
 *  - Combine with prefix (first byte)
 * 
 * @param current_uid Pointer to current 4-byte UID
 * @param new_uid Pointer to store calculated new UID
 * @return 0 if success, -1 if error
 */
int calculateRollingToken(uint8_t *current_uid, uint8_t *new_uid);

/**
 * @brief Verify UID against database and get user info
 * 
 * @param scanned_uid 4-byte UID from RFID reader
 * @param db Pointer to UID database
 * @param out_entry Pointer to store matched entry (if found)
 * @return Index of matching entry, or -1 if not found
 */
int verifyUID(uint8_t *scanned_uid, UIDDatabase *db, UIDEntry *out_entry);

/**
 * @brief Update UID in database with new rolling token value
 * 
 * @param db Pointer to UID database
 * @param uid_index Index of UID to update
 * @param new_uid New UID value (4 bytes)
 * @return 0 if success, -1 if error
 */
int updateUIDInDatabase(UIDDatabase *db, int uid_index, uint8_t *new_uid);

/**
 * @brief Convert 4-byte binary UID to hex string
 * 
 * @param binary_uid 4-byte binary UID
 * @param hex_string Output buffer (minimum 9 bytes: "12345678\0")
 * @return 0 if success
 */
int uidBinaryToHex(uint8_t *binary_uid, char *hex_string);

/**
 * @brief Convert hex string UID to 4-byte binary
 * 
 * @param hex_string Hex string (e.g., "12345678")
 * @param binary_uid Output buffer (4 bytes)
 * @return 0 if success, -1 if invalid format
 */
int uidHexToBinary(const char *hex_string, uint8_t *binary_uid);

/**
 * @brief Compare two 4-byte UIDs
 * 
 * @param uid1 First UID
 * @param uid2 Second UID
 * @return 1 if equal, 0 if different
 */
int uidCompare(uint8_t *uid1, uint8_t *uid2);

/**
 * @brief Copy UID from source to destination
 * 
 * @param src Source UID (4 bytes)
 * @param dst Destination UID (4 bytes)
 */
void uidCopy(uint8_t *src, uint8_t *dst);

// ============ SECURITY STATE MANAGEMENT ============

/**
 * @brief Record a failed access attempt
 * 
 * @param state Pointer to security state
 * @return Current failed attempt count
 */
int recordFailedAttempt(SecurityState *state);

/**
 * @brief Check if system is currently locked
 * 
 * @param state Pointer to security state
 * @return 1 if locked, 0 if not locked
 */
int isSystemLocked(SecurityState *state);

/**
 * @brief Clear failed attempts and unlock system
 * 
 * @param state Pointer to security state
 */
void clearFailedAttempts(SecurityState *state);

/**
 * @brief Lock system for specified duration
 * 
 * @param state Pointer to security state
 * @param duration_ms Lock duration in milliseconds
 */
void lockSystem(SecurityState *state, uint32_t duration_ms);

// ============ HARDWARE FEEDBACK ============

/**
 * @brief Give success feedback (green LED + beep)
 */
void feedbackSuccess();

/**
 * @brief Give failure feedback (red LED + error beep)
 */
void feedbackFailure();

/**
 * @brief Lock system feedback (red LED pulse + long beep)
 */
void feedbackLockout();

#endif // SECURITY_H
