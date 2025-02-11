#include "ExodusState.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include "ExodusPhrase.hpp"

int main() {
    ExodusStateDetector detector;
    ExodusState previousState = ExodusState::NotRunning;
    bool initialOutput = true;

    std::wcout << L"Monitoring Exodus..." << std::endl;

    while (true) {
        ExodusState currentState = detector.DetectExodusState();

        if (currentState != previousState || initialOutput) {
            switch (currentState) {
            case ExodusState::NotRunning:
                std::wcout << L"Exodus is not running." << std::endl;
                break;
            case ExodusState::Starting:
                std::wcout << L"Exodus is starting..." << std::endl;
                break;
            case ExodusState::LoginPage:
                std::wcout << L"Exodus is on the login page." << std::endl;
                break;
            case ExodusState::WalletPage:
                std::wcout << L"Exodus is on the wallet page." << std::endl;
                const char* processName = "Exodus.exe";
                ProcessMemoryReader reader(processName);

                if (!reader.initialize()) {
                    std::cerr << "Failed to initialize process reader. Error: " << GetLastError() << std::endl;
                    return 1;
                }

                reader.scanForStrings();
                break;
            }
            previousState = currentState;
            initialOutput = false;
        }

    }

    return 0;
}
