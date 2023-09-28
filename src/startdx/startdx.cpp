#include "startdx.hpp"

#include <windows.h>
#include <iostream>
#include <thread>

#include "helpers.hpp"

#include "minhook/include/MinHook.h"

#if defined _M_X64
#pragma comment(lib, "libMinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "libMinHook.x86.lib")
#endif

template <typename T>
inline MH_STATUS MH_CreateHookEx(LPVOID pTarget, LPVOID pDetour, T** ppOriginal)
{
	return MH_CreateHook(pTarget, pDetour, reinterpret_cast<LPVOID*>(ppOriginal));
}


typedef DWORD* (*getPlayer)();
getPlayer GetPlayer = NULL;
typedef int (*getSide)();
getSide GetSide = NULL;
typedef void(__thiscall* setDarkLane)(DWORD* pThis, DWORD* edxArg, unsigned int value, int side);
setDarkLane SetDarkLane = NULL;

typedef void (__thiscall* setDarkLaneSafe)(DWORD* pThis, DWORD* edxArg, unsigned int value);
setDarkLaneSafe SetDarkLaneSafe = NULL;

void __fastcall OnSetDarkLaneSafe(DWORD* pThis, DWORD* edxArg, unsigned int value)
{
	if (value > 5 && value <= 10)
	{
		SetDarkLane(GetPlayer(), edxArg, value, GetSide());
		return;
	}

	SetDarkLaneSafe(pThis, edxArg, value);
}

typedef int(__thiscall* getDarkLane)(DWORD* pThis, DWORD* edxArg, int side);
getDarkLane GetDarkLane = NULL;

typedef int(__thiscall* getDarkLaneSafe)(DWORD* pThis, DWORD* edxArg);
getDarkLaneSafe GetDarkLaneSafe = NULL;

int __fastcall OnGetDarkLaneSafe(DWORD* pThis, DWORD* edxArg)
{
	int darkVal = GetDarkLane(GetPlayer(), edxArg, GetSide());
	if (darkVal > 5 && darkVal <= 10)
	{
		return darkVal;
	}

	return GetDarkLaneSafe(pThis, edxArg);
}

float* darkLaneOptMaxIdx = NULL;
int* darkLaneOptIdxCount = NULL;

//// Pointer for calling original MessageBoxW.
//MESSAGEBOXW fpMessageBoxW = NULL;
//
//// Detour function which overrides MessageBoxW.
//int WINAPI DetourMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
//{
//	return fpMessageBoxW(hWnd, L"Hooked!", lpCaption, uType);
//}

void SetVariables()
{
	while (!CanAccess((uintptr_t)darkLaneOptIdxCount) || !CanAccess((uintptr_t)darkLaneOptMaxIdx))
	{
		DWORD oldProtect;
		VirtualProtect(darkLaneOptMaxIdx, sizeof(*darkLaneOptMaxIdx), PAGE_READWRITE, &oldProtect);
		VirtualProtect(darkLaneOptIdxCount, sizeof(*darkLaneOptIdxCount), PAGE_READWRITE, &oldProtect);
		Sleep(500);
	}
	*darkLaneOptMaxIdx = 10.f;
	*darkLaneOptIdxCount = 11;
}

int startdx::init(HMODULE hModule)
{
	uintptr_t modBm2dx = (uintptr_t)GetModuleHandle("bm2dx.dll");

	GetPlayer = (getPlayer)(modBm2dx + 0x913220);
	GetSide = (getSide)(modBm2dx + 0x83C390);
	SetDarkLane = (setDarkLane)(modBm2dx + 0x9449F0);
	GetDarkLane = (getDarkLane)(modBm2dx + 0x93FAA0);

	darkLaneOptMaxIdx = (float*)(modBm2dx + 0x12FDD00);
	darkLaneOptIdxCount = (int*)(modBm2dx + 0x1185758);

	std::thread setVariables(SetVariables);
	setVariables.detach();

	if (MH_Initialize() != MH_OK)
	{
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)(modBm2dx + 0x7F2130), &OnSetDarkLaneSafe, &SetDarkLaneSafe) != MH_OK)
	{
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)(modBm2dx + 0x7F0A90), &OnGetDarkLaneSafe, &GetDarkLaneSafe) != MH_OK)
	{
		return 1;
	}

	if (MH_QueueEnableHook(MH_ALL_HOOKS) || MH_ApplyQueued() != MH_OK)
	{
		return 1;
	}

	return 0;
}

int startdx::exit()
{
	*darkLaneOptMaxIdx = 5.f;
	*darkLaneOptIdxCount = 6;
	// Disable the hook for MessageBoxW.
	if (MH_QueueDisableHook(MH_ALL_HOOKS) || MH_ApplyQueued() != MH_OK)
	{
		return 1;
	}

	if (MH_Uninitialize() != MH_OK)
	{
		return 1;
	}

	return 0;
}

