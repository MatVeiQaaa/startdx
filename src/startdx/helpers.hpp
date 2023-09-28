#include <Windows.h>
#include <string>

static void Log(const std::string format, ...) {
    //EnterCriticalSection(&logMutex);

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    WORD originalAttributes = consoleInfo.wAttributes;
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
    printf("[+] ");
    SetConsoleTextAttribute(hConsole, originalAttributes);
    va_list args;
    va_start(args, format);
    vprintf(format.c_str(), args);
    va_end(args);
    printf("\n");
    fflush(stdout);

    //LeaveCriticalSection(&logMutex);
}

static BOOL CanAccess(uintptr_t ptr)
{
    LPVOID addressToCheck = reinterpret_cast<LPVOID>(ptr);
    MEMORY_BASIC_INFORMATION memInfo;
    SIZE_T memInfoSize = sizeof(memInfo);
    SIZE_T queryResult = VirtualQuery(addressToCheck, &memInfo, memInfoSize);

    if (queryResult == 0) {
        return 0;
    }

    if (memInfo.State == MEM_COMMIT && (memInfo.Protect == PAGE_READWRITE || memInfo.Protect == PAGE_EXECUTE_READWRITE || 
                                        memInfo.Protect == PAGE_WRITECOPY || memInfo.Protect == PAGE_EXECUTE_WRITECOPY)) {
        return 1;
    }
    return 0;
}