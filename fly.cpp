#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>


DWORD GetProcessIdByName(const char* procName) noexcept {
	DWORD PID = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (hSnap != NULL) {
		PROCESSENTRY32 procEntry;
		procEntry.dwSize = sizeof(PROCESSENTRY32);

		Process32First(hSnap, &procEntry);

		do {
			if (strcmp(procName, procEntry.szExeFile) == 0) {
				PID = procEntry.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnap, &procEntry));
	}

	return PID;
}


uintptr_t GetModuleBaseAddress(DWORD PID) noexcept {
	uintptr_t mod_base_addr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, PID);

	if (hSnap != NULL) {
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(MODULEENTRY32);

		Module32First(hSnap, &modEntry);

		do {
			if (PID == modEntry.th32ProcessID) {
				mod_base_addr = (uintptr_t)modEntry.modBaseAddr;
				break;
			}
		} while (Module32Next(hSnap, &modEntry));
	}

	return mod_base_addr;
}


uintptr_t GetPointerAddress(HANDLE processHandle, uintptr_t baseAddress, const std::vector<int>& offsets) noexcept
{
	uintptr_t address = baseAddress;
	for (size_t i = 0; i < offsets.size(); i++)
	{
		uintptr_t value = 0;
		SIZE_T bytesRead = 0;

		ReadProcessMemory(processHandle, reinterpret_cast<LPCVOID>(address), &value, sizeof(value), &bytesRead);

		address = value + offsets[i];
	}
	return address;
}


namespace ADDRESS {
	uintptr_t ModuleBaseAddress = 0;
	uintptr_t LocalPlayer = 0x0017E0A8;
	uintptr_t Pos_Y = 0x30;
}

int main(void) {
	DWORD PID = GetProcessIdByName("ac_client.exe");
	if (PID == 0) {
		return -1;
	}

	ADDRESS::ModuleBaseAddress = GetModuleBaseAddress(PID);
	if (ADDRESS::ModuleBaseAddress == 0) {
		return -1;
	}

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, PID);
	if (hProcess == NULL) {
		return -1;
	}
	ADDRESS::LocalPlayer += ADDRESS::ModuleBaseAddress;

	float value;
	float newValue;
	uintptr_t Address = GetPointerAddress(hProcess, ADDRESS::LocalPlayer, { ADDRESS::Pos_Y });

	while (true) {
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			unsigned char buffer[sizeof(float)];

			ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(Address), &value, sizeof(value), nullptr);
			newValue = value + 5.0f;
			memcpy(buffer, &newValue, sizeof(buffer));
			WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(Address), buffer, sizeof(buffer), nullptr);
		}

		if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) {
			unsigned char buffer[sizeof(float)];

			ReadProcessMemory(hProcess, reinterpret_cast<LPCVOID>(Address), &value, sizeof(value), nullptr);
			newValue = value - 5.0f;
			memcpy(buffer, &newValue, sizeof(buffer));
			WriteProcessMemory(hProcess, reinterpret_cast<LPVOID>(Address), buffer, sizeof(buffer), nullptr);
		}

		if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
			break;
		}

		Sleep(100);
	}
	
	return 0;
}