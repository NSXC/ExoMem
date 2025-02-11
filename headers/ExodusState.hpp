#pragma once

#include <windows.h>
#include <string>
#include <chrono>

enum class ExodusState {
    NotRunning,
    Starting,
    LoginPage,
    WalletPage
};

class ExodusStateDetector {
public:
    ExodusStateDetector();
    ~ExodusStateDetector();
    ExodusState DetectExodusState();

private:
    DWORD FindExodusProcess();
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    static VOID CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);
    bool IsProcessRunning(DWORD processId);
    void ResetState();

    DWORD m_exodusProcessId;
    HWND m_mainWindow;
    HWINEVENTHOOK m_hook;
    bool m_stateChanged;
    ExodusState m_lastState;
    std::chrono::steady_clock::time_point m_lastCheckTime;
    std::chrono::steady_clock::time_point m_startTime;
};
