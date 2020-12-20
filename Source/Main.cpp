#include "Main.h"
//https://github.com/maniacTM
//This is for learning purposes only.


int main(void)
{
	//Get Process ID
	hProcID = FindProcessId(szStrings::szCSGO);

	//Check if is valid
	if (hProcID == 0)
		return 0;

	//Open handle to CSGO
	hProc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, false, hProcID);

	//Wait for Client.DLL
	while (GetModuleBaseAddress(hProcID, szStrings::szClientMod) == 0)
		Sleep(100);

	//Wait for Engine.DLL
	while (GetModuleBaseAddress(hProcID, szStrings::szEngineMod) == 0)
		Sleep(100);

	//Set our client & engine module base locations.
	mClient = GetModuleBaseAddress(hProcID, szStrings::szClientMod);
	mEngine = GetModuleBaseAddress(hProcID, szStrings::szEngineMod);
	dwClientStatePtr = read<uintptr_t>(mEngine + offsets::dwClientState);

	while (true)
	{
		runRCS();
		Sleep(1);
	}

	return 0;
}

inline void runRCS()
{
	//read client state to check we are in game and check playerstate to see if ur alive.
	const auto dwClientState_State = read<uintptr_t>(dwClientStatePtr + offsets::dwClientState_State);
	const auto dwClientStatePtr = read<uintptr_t>(mEngine + offsets::dwClientState);

	if (dwClientState_State == 6)
	{
		const auto dwlocal = read<uintptr_t>(mClient + offsets::dwLocalPlayer);

		if (dwlocal != 0)
		{
			const auto dwPunch = read<Vector3>(dwlocal + offsets::m_aimPunchAngle);
			const auto dwViewAngles = read<Vector3>(dwClientStatePtr + offsets::dwClientState_ViewAngles);
			const float totalPunch = dwPunch.x + dwPunch.y;

			if (totalPunch != 0.f) //Check for punch from our yaw and pitch
			{
				const auto compensatedAngle = Vector3{ ((dwViewAngles.x + dwOldPunchAngle.x) - (dwPunch.x * 2.f)),((dwViewAngles.y + dwOldPunchAngle.y) - (dwPunch.y * 2.f)),0.f };
				const auto AimAngle = ClampAngle(compensatedAngle);
				write<Vector3>(dwClientStatePtr + offsets::dwClientState_ViewAngles, AimAngle);
				dwOldPunchAngle = Vector3{ dwPunch.x * 2.f, dwPunch.y * 2.f, 0.f };
			}
			else //If totalPunch Returns 0 (meaning no recoil) we set dwoldpunchangle back to null for next time.
			{
				dwOldPunchAngle = Vector3{ 0.f,0.f,0.f };
			}
		}
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