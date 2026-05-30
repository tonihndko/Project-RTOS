#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <stdint.h>
#include "data_structures.h"

// ============ WEB SERVER INITIALIZATION ============

/**
 * @brief Initialize AsyncWebServer
 * Sets up HTTP routes and handlers
 * 
 * @return 0 if success
 */
int initWebServer();

/**
 * @brief Start web server
 * 
 * @return 0 if success
 */
int startWebServer();

/**
 * @brief Stop web server
 * 
 * @return 0 if success
 */
int stopWebServer();

/**
 * @brief Check if web server is running
 * 
 * @return 1 if running, 0 if stopped
 */
int isWebServerRunning();

// ============ HTTP ROUTE HANDLERS ============

/**
 * @brief Handle GET /admin
 * Serves HTML dashboard with registration form
 */
void handleAdminDashboard();

/**
 * @brief Handle GET /api/database
 * Returns current UID database as JSON
 */
void handleGetDatabase();

/**
 * @brief Handle POST /api/register
 * Registers new UID to database
 * 
 * Expected JSON payload:
 * {
 *   "uid": "12345678",
 *   "name": "User Name"
 * }
 */
void handleRegisterUID();

/**
 * @brief Handle DELETE /api/uid/{uid}
 * Deletes UID from database
 * 
 * Example: DELETE /api/uid/12345678
 */
void handleDeleteUID();

/**
 * @brief Handle POST /api/reset
 * Reset system (optional dangerous operation)
 */
void handleResetSystem();

/**
 * @brief Handle 404 Not Found
 */
void handleNotFound();

// ============ HTML ASSET GENERATION ============

/**
 * @brief Generate HTML dashboard content
 * 
 * @param buffer Output buffer
 * @param buffer_size Maximum buffer size
 * @param db Pointer to current database (for display)
 * @return 0 if success
 */
int generateDashboardHTML(char *buffer, int buffer_size, UIDDatabase *db);

/**
 * @brief Generate database JSON response
 * 
 * @param buffer Output buffer
 * @param buffer_size Maximum buffer size
 * @param db Pointer to database
 * @return 0 if success
 */
int generateDatabaseJSON(char *buffer, int buffer_size, UIDDatabase *db);

// ============ REQUEST VALIDATION ============

/**
 * @brief Validate UID format (hex string "12345678")
 * 
 * @param uid_str UID as string
 * @return 1 if valid, 0 if invalid
 */
int validateUIDFormat(const char *uid_str);

/**
 * @brief Validate name format (alphanumeric + space)
 * 
 * @param name User name
 * @return 1 if valid, 0 if invalid
 */
int validateNameFormat(const char *name);

/**
 * @brief Check for duplicate UID in database
 * 
 * @param uid_str UID as hex string
 * @param db Pointer to database
 * @return 1 if duplicate found, 0 if unique
 */
int checkDuplicateUID(const char *uid_str, UIDDatabase *db);

#endif // WEB_SERVER_H
