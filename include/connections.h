#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include "k.h"
#include <string>
#include <memory>

/**
 * @brief Checks if a connection to a KDB+ server is successful.
 *
 * Sends a `.z.P` query to validate the connection.
 *
 * @param handle Connection handle to the KDB+ server.
 * @return bool True if the connection is successful, false otherwise.
 */
bool is_connection_successful(I handle);

/**
 * @brief Establishes a connection to a KDB+ server.
 *
 * Attempts to connect to the server at the specified host and port.
 * If the connection fails, the handle is closed.
 *
 * @param host Hostname or IP address of the KDB+ server.
 * @param port Port number of the KDB+ server.
 * @return I Handle to the KDB+ server, or -1 if the connection fails.
 */
I connect(const std::string& host, int port);

/**
 * @brief Creates a new connection to a KDB+ server.
 *
 * If the server is not running, attempts to start a new KDB+ server
 * on the specified port and connects to it.
 *
 * @param host Hostname or IP address of the KDB+ server.
 * @param port Port number of the KDB+ server.
 * @return I Handle to the KDB+ server, or -1 if the connection fails.
 */
I create_connection(const std::string& host, int port);

/**
 * @class KDBConnection
 * @brief Manages a singleton connection to a KDB+ server.
 *
 * Provides static methods to connect, disconnect, and access the connection handle.
 * Automatically cleans up the connection on application exit.
 */
class KDBConnection {
private:
    /**
     * @class Cleanup
     * @brief Handles cleanup of the KDB+ connection on application exit.
     */
    class Cleanup {
    public:
        ~Cleanup();
    };

    static I instance_handle; ///< Singleton connection handle.
    static std::unique_ptr<Cleanup> cleanup_handler; ///< Cleanup handler instance.

public:
    /**
     * @brief Establishes a singleton connection to a KDB+ server.
     *
     * If already connected, returns true immediately.
     *
     * @param host Hostname or IP address of the KDB+ server (default: "localhost").
     * @param port Port number of the KDB+ server (default: 5001).
     * @return bool True if the connection is successful, false otherwise.
     */
    static bool connect(const std::string& host = "localhost", int port = 5001);

    /**
     * @brief Disconnects the singleton connection from the KDB+ server.
     *
     * Closes the connection and resets the handle to 0.
     */
    static void disconnect();

    /**
     * @brief Retrieves the connection handle for the singleton connection.
     *
     * @return I Connection handle to the KDB+ server.
     * @throws std::runtime_error If not connected to the server.
     */
    static I getHandle();

private:
    KDBConnection() = delete; ///< Prevent instantiation of the class.
};

#endif // CONNECTIONS_H
