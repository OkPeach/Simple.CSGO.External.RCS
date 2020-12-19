#include "Main.h"

int main(void)
{
	//Get Process ID
	hProcID = FindProcessId(szStrings::szCSGO);

	//Check if is valid
	if (hProcID == 0)
		return 0;

	//Open handle to CSGO
	hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, hProcID);

	//std::cout << hProc << std::endl;
	while (GetModuleBaseAddress(hProcID, szStrings::szClientMod) == 0)
		Sleep(100);

	while (GetModuleBaseAddress(hProcID, szStrings::szEngineMod) == 0)
		Sleep(100);

	mClient = GetModuleBaseAddress(hProcID, szStrings::szClientMod);
	mEngine = GetModuleBaseAddress(hProcID, szStrings::szEngineMod);

	while (FindProcessId(szStrings::szCSGO) != 0)
	{
		runRCS();
		Sleep(1);
	}

	return 0;
}

void runRCS()
{
	auto dwlocal = read<uintptr_t>(mClient + offsets::dwLocalPlayer);
	auto dwClientState = read<uintptr_t>(mEngine + offsets::dwClientState);
	auto dwPunch = read<Vector3>(dwlocal + offsets::m_aimPunchAngle);
	auto dwViewAngles = read<Vector3>(dwClientState + offsets::dwClientState_ViewAngles);

	auto totalPunch = dwPunch.x + dwPunch.y;
	if (totalPunch != 0.f) //Check for recoil, if detected then run control system
	{
		Vector3 compensatedAngle;

		compensatedAngle.x = ((dwViewAngles.x + dwOldPunchAngle.x) - (dwPunch.x * 2.f));
		compensatedAngle.y = ((dwViewAngles.y + dwOldPunchAngle.y) - (dwPunch.y * 2.f));

		auto AimAngle = ClampAngle(compensatedAngle);

		write<Vector3>(dwClientState + offsets::dwClientState_ViewAngles, AimAngle);

		dwOldPunchAngle.x = dwPunch.x * 2.f;
		dwOldPunchAngle.y = dwPunch.y * 2.f;
	}
	else //If no recoil detected we null vOldAimPunch
	{
		dwOldPunchAngle = Vector3{ 0,0,0 };
	}
}

inline uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
	uintptr_t modBaseAddr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (!_wcsicmp(modEntry.szModule, modName))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}

inline DWORD FindProcessId(const std::wstring& processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processesSnapshot, &processInfo);
	if (!processName.compare(processInfo.szExeFile))
	{
		CloseHandle(processesSnapshot);
		return processInfo.th32ProcessID;
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (!processName.compare(processInfo.szExeFile))
		{
			CloseHandle(processesSnapshot);
			return processInfo.th32ProcessID;
		}
	}

	CloseHandle(processesSnapshot);
	return 0;
}

inline Vector3 ClampAngle(Vector3 qaAng)
{
	if (qaAng.x > 89.0f)
		qaAng.x = 89.0f;
	if (qaAng.x < -89.0f)
		qaAng.x = -89.0f;
	while (qaAng.y > 180.0f)
		qaAng.y -= 360.0f;
	while (qaAng.y < -180.0f)
		qaAng.y += 360.0f;
	qaAng.z = 0;
	return qaAng;
}