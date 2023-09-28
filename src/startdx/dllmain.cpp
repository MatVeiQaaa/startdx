// dllmain.cpp : Defines the entry point for the DLL application.
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <assert.h>
#include "helpers.hpp"
#include "startdx.hpp"

CRITICAL_SECTION logMutex;

DWORD WINAPI mainThread(HMODULE hModule)
{
    startdx::init(hModule);

    while (true)
    {
        if (GetAsyncKeyState(VK_INSERT))
        {
            Log("Unsafe detours left attached, exiting.");
            break;
        }
        Sleep(10);
    }

    startdx::exit();
    DeleteCriticalSection(&logMutex);
    FreeLibraryAndExitThread(hModule, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    HANDLE hThread;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        InitializeCriticalSection(&logMutex);
        Log("creating thread");
        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainThread, hModule, 0, NULL);
        assert(hThread != NULL);
        CloseHandle(hThread);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

