#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <atomic>
#include <thread>
#include <vector>

#pragma comment(lib, "psapi.lib")

// ============================================================
// StreamMode - Nvidia ShadowPlay bypass (cross-process)
//
// Patches nvd3dumx.dll inside nvcontainer.exe to disable the
// Widevine L1 DRM content check. Without this patch, ShadowPlay
// detects WDA_EXCLUDEFROMCAPTURE on our overlay and STOPS
// recording entirely.
//
// After patching, ShadowPlay ignores the DRM flag and keeps
// recording. Windows still respects WDA_EXCLUDEFROMCAPTURE
// and excludes our overlay from the captured frames.
//
// IMPORTANT: We do NOT hook GetWindowDisplayAffinity here.
// That would make ShadowPlay see WDA_NONE on our overlay,
// which would cause it to INCLUDE the overlay in recordings.
// We WANT ShadowPlay to see WDA_EXCLUDEFROMCAPTURE so it
// excludes our overlay from the capture output.
// ============================================================

class StreamModePatcher
{
public:
	static void Enable()
	{
		if (s_enabled.exchange(true))
			return;

		// Try to patch nvcontainer.exe immediately
		PatchNvContainer();

		// Start monitor thread
		s_monitoring = true;
		s_monitorThread = std::thread(MonitorThread);
		s_monitorThread.detach();
	}

	static void Disable()
	{
		if (!s_enabled.exchange(false))
			return;

		s_monitoring = false;

		// Unpatch nvcontainer.exe
		UnpatchNvContainer();
	}

	static bool IsEnabled() { return s_enabled.load(); }
	static bool IsPatched() { return s_patched.load(); }

private:
	// ── Widevine L1 flag check patterns in nvd3dumx.dll ──

	// Pattern 1: mov r8d,[rdx+00000170] ; test r8d,r8d
	//   This reads the Widevine L1 DRM flag from the surface descriptor.
	//   Patch zeroes r8d so the test always sees 0 (no DRM).
	static inline BYTE s_original1[] = { 0x44, 0x8B, 0x82, 0x70, 0x01, 0x00, 0x00, 0x45, 0x85, 0xC0 };
	static inline BYTE s_patched1[]  = { 0x45, 0x31, 0xC0, 0x90, 0x90, 0x90, 0x90, 0x45, 0x85, 0xC0 };

	// Pattern 2: mov ecx,[rax+00000170] ; test ecx,ecx
	//   Same check, different register/code path.
	static inline BYTE s_original2[] = { 0x8B, 0x88, 0x70, 0x01, 0x00, 0x00, 0x85, 0xC9 };
	static inline BYTE s_patched2[]  = { 0x31, 0xC9, 0x90, 0x90, 0x90, 0x90, 0x85, 0xC9 };

	// ── State ──
	static inline std::atomic<bool> s_enabled{ false };
	static inline std::atomic<bool> s_monitoring{ false };
	static inline std::atomic<bool> s_patched{ false };
	static inline std::thread s_monitorThread;

	// Patched process tracking
	static inline DWORD s_patchedPID = 0;
	static inline std::vector<BYTE*> s_patchAddrs1;
	static inline std::vector<BYTE*> s_patchAddrs2;

	// ─────────────────────────────────────────────────────────
	// Find all nvcontainer.exe PIDs
	// ─────────────────────────────────────────────────────────
	static std::vector<DWORD> FindNvContainerPIDs()
	{
		std::vector<DWORD> pids;
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap == INVALID_HANDLE_VALUE)
			return pids;

		PROCESSENTRY32W pe = {};
		pe.dwSize = sizeof(pe);

		if (Process32FirstW(hSnap, &pe))
		{
			do
			{
				if (_wcsicmp(pe.szExeFile, L"nvcontainer.exe") == 0)
					pids.push_back(pe.th32ProcessID);
			} while (Process32NextW(hSnap, &pe));
		}

		CloseHandle(hSnap);
		return pids;
	}

	// ─────────────────────────────────────────────────────────
	// Find module base + size in a remote process
	// ─────────────────────────────────────────────────────────
	static bool FindRemoteModule(HANDLE hProcess, const wchar_t* moduleName,
		BYTE*& outBase, SIZE_T& outSize)
	{
		HMODULE hMods[1024];
		DWORD cbNeeded = 0;

		if (!EnumProcessModulesEx(hProcess, hMods, sizeof(hMods), &cbNeeded, LIST_MODULES_ALL))
			return false;

		DWORD numModules = cbNeeded / sizeof(HMODULE);
		for (DWORD i = 0; i < numModules; i++)
		{
			wchar_t modName[MAX_PATH] = { 0 };
			if (GetModuleBaseNameW(hProcess, hMods[i], modName, MAX_PATH))
			{
				if (_wcsicmp(modName, moduleName) == 0)
				{
					MODULEINFO modInfo = {};
					if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo)))
					{
						outBase = (BYTE*)modInfo.lpBaseOfDll;
						outSize = modInfo.SizeOfImage;
						return true;
					}
				}
			}
		}
		return false;
	}

	// ─────────────────────────────────────────────────────────
	// Remote pattern scan + patch
	// ─────────────────────────────────────────────────────────
	static bool RemoteScanAndPatch(HANDLE hProcess, BYTE* moduleBase, SIZE_T moduleSize,
		BYTE* search, BYTE* replace, size_t patternSize,
		std::vector<BYTE*>& patchedAddrs)
	{
		std::vector<BYTE> buffer(moduleSize);
		SIZE_T bytesRead = 0;
		if (!ReadProcessMemory(hProcess, moduleBase, buffer.data(), moduleSize, &bytesRead))
			return false;

		if (bytesRead < patternSize)
			return false;

		bool found = false;
		for (SIZE_T i = 0; i <= bytesRead - patternSize; i++)
		{
			if (memcmp(buffer.data() + i, search, patternSize) == 0)
			{
				BYTE* remoteAddr = moduleBase + i;

				DWORD oldProtect = 0;
				if (VirtualProtectEx(hProcess, remoteAddr, patternSize, PAGE_EXECUTE_READWRITE, &oldProtect))
				{
					SIZE_T written = 0;
					if (WriteProcessMemory(hProcess, remoteAddr, replace, patternSize, &written))
					{
						patchedAddrs.push_back(remoteAddr);
						found = true;
					}
					VirtualProtectEx(hProcess, remoteAddr, patternSize, oldProtect, &oldProtect);
				}
			}
		}
		return found;
	}

	// ─────────────────────────────────────────────────────────
	// Restore original bytes at a remote address
	// ─────────────────────────────────────────────────────────
	static bool RemoteRestore(HANDLE hProcess, BYTE* remoteAddr, BYTE* original, size_t size)
	{
		DWORD oldProtect = 0;
		if (VirtualProtectEx(hProcess, remoteAddr, size, PAGE_EXECUTE_READWRITE, &oldProtect))
		{
			SIZE_T written = 0;
			WriteProcessMemory(hProcess, remoteAddr, original, size, &written);
			VirtualProtectEx(hProcess, remoteAddr, size, oldProtect, &oldProtect);
			return written == size;
		}
		return false;
	}

	// ─────────────────────────────────────────────────────────
	// Main patch - find nvcontainer.exe with nvd3dumx.dll
	// and patch the Widevine L1 DRM checks
	// ─────────────────────────────────────────────────────────
	static void PatchNvContainer()
	{
		auto pids = FindNvContainerPIDs();

		for (DWORD pid : pids)
		{
			HANDLE hProcess = OpenProcess(
				PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
				FALSE, pid);

			if (!hProcess)
				continue;

			BYTE* nvdBase = nullptr;
			SIZE_T nvdSize = 0;

			if (FindRemoteModule(hProcess, L"nvd3dumx.dll", nvdBase, nvdSize))
			{
				s_patchAddrs1.clear();
				s_patchAddrs2.clear();

				// Patch both Widevine L1 flag check patterns
				RemoteScanAndPatch(hProcess, nvdBase, nvdSize,
					s_original1, s_patched1, sizeof(s_original1), s_patchAddrs1);

				RemoteScanAndPatch(hProcess, nvdBase, nvdSize,
					s_original2, s_patched2, sizeof(s_original2), s_patchAddrs2);

				s_patchedPID = pid;
				s_patched = true;

				CloseHandle(hProcess);
				return;
			}

			CloseHandle(hProcess);
		}
	}

	// ─────────────────────────────────────────────────────────
	// Unpatch - restore original bytes
	// ─────────────────────────────────────────────────────────
	static void UnpatchNvContainer()
	{
		if (!s_patched || s_patchedPID == 0)
			return;

		HANDLE hProcess = OpenProcess(
			PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
			FALSE, s_patchedPID);

		if (!hProcess)
		{
			ResetState();
			return;
		}

		for (auto addr : s_patchAddrs1)
			RemoteRestore(hProcess, addr, s_original1, sizeof(s_original1));

		for (auto addr : s_patchAddrs2)
			RemoteRestore(hProcess, addr, s_original2, sizeof(s_original2));

		CloseHandle(hProcess);
		ResetState();
	}

	static void ResetState()
	{
		s_patched = false;
		s_patchedPID = 0;
		s_patchAddrs1.clear();
		s_patchAddrs2.clear();
	}

	// ─────────────────────────────────────────────────────────
	// Monitor thread
	// - Keeps trying to patch if not yet done
	// - Verifies patched process/DLL is still alive
	// - Re-patches automatically if ShadowPlay is toggled
	// ─────────────────────────────────────────────────────────
	static void MonitorThread()
	{
		while (s_monitoring)
		{
			Sleep(3000);

			if (!s_monitoring)
				break;

			if (!s_patched)
			{
				PatchNvContainer();
			}
			else
			{
				// Verify process and DLL still exist
				HANDLE hProcess = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					FALSE, s_patchedPID);

				if (hProcess)
				{
					BYTE* nvdBase = nullptr;
					SIZE_T nvdSize = 0;
					bool stillLoaded = FindRemoteModule(hProcess, L"nvd3dumx.dll", nvdBase, nvdSize);
					CloseHandle(hProcess);

					if (!stillLoaded)
					{
						// DLL unloaded (ShadowPlay toggled) - will re-patch next iteration
						ResetState();
					}
				}
				else
				{
					// Process died - will re-patch next iteration
					ResetState();
				}
			}
		}
	}
};
