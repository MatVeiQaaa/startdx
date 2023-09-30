#include "startdx.hpp"

#include <windows.h>
#include <iostream>
#include <thread>
#include <psapi.h>

#include "helpers.hpp"

#include "minhook/include/MinHook.h"
#include "sigmatch/include/sigmatch/sigmatch.hpp"

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
	Sleep(10000);
	uintptr_t hBm2dx = (uintptr_t)GetModuleHandle("bm2dx.dll");
	MODULEINFO modBm2dx;
	GetModuleInformation(GetCurrentProcess(), (HMODULE)hBm2dx, &modBm2dx, sizeof(MODULEINFO));


	using namespace sigmatch_literals;
	sigmatch::this_process_target target;
	sigmatch::search_context context = target.in_module("bm2dx.dll");

	GetPlayer = (getPlayer)(context.search("B8 08 00 00 00 48 6B C0 01 48 8D 0D ?? ?? ?? ?? 48 8B 04 01 C3"_sig).matches().front());
	GetSide = (getSide)(context.search("48 83 EC 38 E8 ?? ?? ?? ?? 85 C0 74 0A C7 44 24 20 01 00 00 00 EB 08 C7 44 24 20 00 00 00 00 8B 44 24 20 48 83 C4 38 C3"_sig).matches().front());
	SetDarkLane = (setDarkLane)(context.search("E8 ?? ?? ?? ?? 8B 4C 24 40 89 88 8C 00 00 00"_sig).matches().front() - 0x25);
	GetDarkLane = (getDarkLane)(context.search("E8 ?? ?? ?? ?? 8B 80 8C 00 00 00 48 83 C4 28"_sig).matches().front() - 0x20);
	SetDarkLaneSafe = (setDarkLaneSafe)(context.search("8B 84 24 90 00 00 00 89 44 24 20 83 7C 24 20 05"_sig).matches().front() - 0x12);
	GetDarkLaneSafe = (getDarkLaneSafe)(context.search("48 03 C1 FF E0 33 C0 EB 25 B8 01 00 00 00 EB 1E B8 02 00 00 00 EB 17"_sig).matches().front() - 0x51);
	DWORD* maxIdxOffset = (DWORD*)(context.search("F3 0F 5E 05 ?? ?? ?? ?? F3 0F 2C C0 89 84 24 B8 00 00 00"_sig).matches().front() + 0x04);
	darkLaneOptMaxIdx = (float*)((long long)maxIdxOffset + *maxIdxOffset + sizeof(float));
	darkLaneOptIdxCount = (int*)(context.search("06 00 00 00 00 00 00 00 01 00 00 00 02 00 00 00 04 00 00 00 08 00 00 00 10 00 00 00 20"_sig).matches().front());
	
	std::thread setVariables(SetVariables);
	setVariables.detach();

	if (MH_Initialize() != MH_OK)
	{
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)SetDarkLaneSafe, &OnSetDarkLaneSafe, &SetDarkLaneSafe) != MH_OK)
	{
		return 1;
	}

	if (MH_CreateHookEx((LPVOID)GetDarkLaneSafe, &OnGetDarkLaneSafe, &GetDarkLaneSafe) != MH_OK)
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

