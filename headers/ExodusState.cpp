#include "ExodusState.hpp"
#include <tlhelp32.h>
#include <psapi.h>
#include <thread>

ExodusStateDetector::ExodusStateDetector()
    : m_exodusProcessId(0), m_mainWindow(NULL), m_stateChanged(true), m_lastState(ExodusState::NotRunning),
    m_lastCheckTime(std::chrono::steady_clock::now()), m_startTime(std::chrono::steady_clock::now()) {
    m_hook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_DESTROY, NULL, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
}

ExodusStateDetector::~ExodusStateDetector() {
    if (m_hook) {
        UnhookWinEvent(m_hook);
    }
}

bool ExodusStateDetector::IsProcessRunning(DWORD processId) {
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (process == NULL) {
        return false;
    }

    DWORD exitCode;
    if (GetExitCodeProcess(process, &exitCode) == 0) {
        CloseHandle(process);
        return false;
    }

    CloseHandle(process);
    return (exitCode == STILL_ACTIVE);
}

void ExodusStateDetector::ResetState() {
    m_exodusProcessId = 0;
    m_mainWindow = NULL;
    m_stateChanged = true;
    m_lastState = ExodusState::NotRunning;
}

DWORD ExodusStateDetector::FindExodusProcess() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (wcscmp(processEntry.szExeFile, L"Exodus.exe") == 0) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    CloseHandle(snapshot);
    return 0;
}

BOOL CALLBACK ExodusStateDetector::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    ExodusStateDetector* detector = reinterpret_cast<ExodusStateDetector*>(lParam);
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == detector->m_exodusProcessId) {
        detector->m_mainWindow = hwnd;
        return FALSE;
    }
    return TRUE;
}

VOID CALLBACK ExodusStateDetector::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime) {
    ExodusStateDetector* detector = reinterpret_cast<ExodusStateDetector*>(GetProp(hwnd, L"ExodusStateDetector"));
    if (detector) {
        detector->m_stateChanged = true;
    }
}

ExodusState ExodusStateDetector::DetectExodusState() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastCheckTime).count();

    if (elapsed > 2000) {
        m_stateChanged = true;
        m_lastCheckTime = now;
    }

    if (!m_stateChanged && m_lastState != ExodusState::NotRunning && m_lastState != ExodusState::Starting) {
        return m_lastState;
    }

    DWORD exodusProcessId = FindExodusProcess();
    if (exodusProcessId == 0) {
        ResetState();
        return m_lastState;
    }

    if (exodusProcessId != m_exodusProcessId) {
        m_exodusProcessId = exodusProcessId;
        m_mainWindow = NULL;
        m_stateChanged = true;
        m_startTime = std::chrono::steady_clock::now();
        m_lastState = ExodusState::Starting;
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        return m_lastState;
    }

    if (m_lastState == ExodusState::Starting) {
        auto startElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime).count();
        if (startElapsed < 400) {
            return m_lastState;
        }
    }

    if (!m_mainWindow || !IsWindow(m_mainWindow)) {
        m_mainWindow = NULL;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this));
    }

    if (!m_mainWindow) {
        m_stateChanged = false;
        m_lastState = ExodusState::NotRunning;
        return m_lastState;
    }

    wchar_t windowText[256];
    GetWindowText(m_mainWindow, windowText, 256);
    std::wstring mainWindowText(windowText);

    if (mainWindowText.find(L"Enter Password") != std::wstring::npos) {
        m_stateChanged = false;
        m_lastState = ExodusState::LoginPage;
        return m_lastState;
    }

    // Check child windows only if necessary
    BOOL foundLoginPage = FALSE;
    EnumChildWindows(m_mainWindow, [](HWND hwnd, LPARAM lParam) -> BOOL {
        wchar_t windowText[256];
        GetWindowText(hwnd, windowText, 256);
        if (wcsstr(windowText, L"Enter Password") != NULL) {
            *reinterpret_cast<BOOL*>(lParam) = TRUE;
            return FALSE;
        }
        return TRUE;
        }, reinterpret_cast<LPARAM>(&foundLoginPage));

    m_stateChanged = false;
    m_lastState = foundLoginPage ? ExodusState::LoginPage : ExodusState::WalletPage;
    return m_lastState;
}
