// connections.cpp
#include "connections.h"
#include <iostream>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Initialize static members
I KDBConnection::instance_handle = 0;
std::unique_ptr<KDBConnection::Cleanup> KDBConnection::cleanup_handler;

// KDBConnection implementation
KDBConnection::Cleanup::~Cleanup() {
    KDBConnection::disconnect();
}

bool KDBConnection::connect(const std::string& host, int port) {
    if (instance_handle > 0) return true;  // Already connected
    
    instance_handle = khp((S)host.c_str(), port);
    if (instance_handle > 0 && !cleanup_handler) {
        cleanup_handler = std::make_unique<Cleanup>();
    }
    return instance_handle > 0;
}

void KDBConnection::disconnect() {
    if (instance_handle > 0) {
        kclose(instance_handle);
        instance_handle = 0;
    }
}

I KDBConnection::getHandle() {
    if (instance_handle <= 0) {
        throw std::runtime_error("Not connected to KDB+ server");
    }
    return instance_handle;
}

// Connection function implementations
bool is_connection_successful(I handle) {
    if (handle <= 0) {
        return false;
    }
    
    K result = k(handle, (S)".z.P", (K)0);
    if (!result) {
        return false;
    }
    if (result->t == -128) {
        r0(result);
        return false;
    }
    r0(result);
    return true;
}

I connect(const std::string& host, int port) {
    I handle = khp((S)host.c_str(), port);
    if (!is_connection_successful(handle)) {
        std::cerr << "Failed to connect to KDB+ server at " << host << ":" << port << std::endl;
        if (handle > 0) {
            kclose(handle);
        }
        return -1;
    }
    std::cout << "Connected to KDB+ server at " << host << ":" << port << std::endl;
    return handle;
}

I create_connection(const std::string& host, int port) {
    // First, attempt to connect
    I handle = connect(host, port);
    if (handle > 0) {
        return handle;
    }

    // Server is not running; attempt to start a new KDB+ server
    std::cout << "Attempting to start KDB+ server at port " << port << std::endl;

    std::string q_executable = "/path/to/q"; // Modify this path
    std::string command = q_executable + " -p " + std::to_string(port);

#ifdef _WIN32
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    
    if (!CreateProcess(NULL, (LPSTR)command.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        std::cerr << "Failed to start KDB+ server process." << std::endl;
        return -1;
    }
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    pid_t pid = fork();
    if (pid == 0) {
        execlp(q_executable.c_str(), q_executable.c_str(), "-p", std::to_string(port).c_str(), (char*)NULL);
        std::cerr << "Failed to start KDB+ server process." << std::endl;
        exit(1);
    } else if (pid < 0) {
        std::cerr << "Failed to fork process to start KDB+ server." << std::endl;
        return -1;
    }
#endif

    std::this_thread::sleep_for(std::chrono::seconds(2));

    handle = connect(host, port);
    if (handle > 0) {
        return handle;
    } else {
        std::cerr << "Failed to connect to KDB+ server after starting it." << std::endl;
        return -1;
    }
}
