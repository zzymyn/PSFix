#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma warning (push)
#pragma warning (disable: 4091)
#	include <DbgHelp.h>
#pragma warning (pop)
#include <dxgi.h>
#include <Psapi.h>
#include <atlbase.h>
#include <utility>
#include <vector>
#include "FixedDXGIFactory.h"
#include "MemUnlocker.h"
#include "Utils.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")

namespace
{
	template <typename T>
	T GetModuleObject(HMODULE module, DWORD offset)
	{
		return reinterpret_cast<T>(reinterpret_cast<char*>(module) + offset);
	}

	std::vector<HMODULE> GetProcessModules()
	{
		std::vector<HMODULE> modules;

		DWORD sizeNeeded;
		if (EnumProcessModulesEx(GetCurrentProcess(), NULL, 0, &sizeNeeded, LIST_MODULES_64BIT))
		{
			modules.resize(sizeNeeded / sizeof(HMODULE));
			EnumProcessModulesEx(GetCurrentProcess(), modules.data(), sizeNeeded, &sizeNeeded, LIST_MODULES_64BIT);
		}

		return modules;
	}

	PIMAGE_IMPORT_DESCRIPTOR FindImportDescriptor(HMODULE module, PIMAGE_IMPORT_DESCRIPTOR importDescriptor, LPCSTR moduleName)
	{
		for (; importDescriptor->Characteristics > 0 && importDescriptor->Name > 0; ++importDescriptor)
		{
			const auto name = GetModuleObject<const char*>(module, importDescriptor->Name);

			if (_stricmp(name, moduleName) == 0)
				return importDescriptor;
		}

		return NULL;
	}

	template <typename T>
	T** FindThunk(HMODULE srcModule, PIMAGE_IMPORT_DESCRIPTOR importDescriptor, T* targetFunc)
	{
		for (auto thunk = GetModuleObject<PIMAGE_THUNK_DATA>(srcModule, importDescriptor->FirstThunk); thunk->u1.Function > 0; ++thunk)
		{
			auto& thunkToFunc = reinterpret_cast<T*&>(thunk->u1.Function);

			if (targetFunc == thunkToFunc)
			{
				return &thunkToFunc;
			}
		}

		return nullptr;
	}

	template <typename T>
	void DoHook(HMODULE srcModule, LPCSTR dllName, LPCSTR functionName, T* newFunc)
	{
		if (srcModule == NULL)
			return;

		// never hook ourselves:
		if (srcModule == Utils::GetCurrentModule())
			return;

		const auto targetModule = GetModuleHandleA(dllName);

		if (targetModule == NULL)
			return;

		// never hook the original DLL:
		if (targetModule == srcModule)
			return;

		const auto targetFunc = reinterpret_cast<T*>(GetProcAddress(targetModule, functionName));

		if (targetFunc == NULL)
			return;

		ULONG size;
		const auto baseImportDescriptors = (PIMAGE_IMPORT_DESCRIPTOR)ImageDirectoryEntryToDataEx(srcModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &size, NULL);

		if (baseImportDescriptors == NULL)
			return;

		const auto importDescriptor = FindImportDescriptor(srcModule, baseImportDescriptors, dllName);

		if (importDescriptor == NULL)
			return;

		auto thunk = FindThunk<T>(srcModule, importDescriptor, targetFunc);

		if (thunk == NULL)
			return;

		{
			MemUnlocker memunlocker(thunk);
			*thunk = newFunc;
		}
	}

	void HookAll(HMODULE srcModule);

	// Hook all LoadLibrary variants so we infect every DLL loaded:
	HMODULE WINAPI MyLoadLibraryA(LPCSTR lpLibFileName)
	{
		auto r = LoadLibraryA(lpLibFileName);
		HookAll(r);
		return r;
	}

	HMODULE WINAPI MyLoadLibraryW(LPWSTR lpLibFileName)
	{
		auto r = LoadLibraryW(lpLibFileName);
		HookAll(r);
		return r;
	}

	HMODULE WINAPI MyLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
	{
		auto r = LoadLibraryExA(lpLibFileName, hFile, dwFlags);
		HookAll(r);
		return r;
	}

	HMODULE WINAPI MyLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
	{
		auto r = LoadLibraryExW(lpLibFileName, hFile, dwFlags);
		HookAll(r);
		return r;
	}

	// Intercept CreateProcess calls to also infect subprocesses:
	// However, at the moment, we only intercept calls to "sniffer.exe".
	// sniffer.exe is the tool that determines whether OpenCL is supported,
	// but it will also crash on ATI cards if you also have integrated graphics enabled.
	// We also don't care about CreateProcessA at the moment because sniffer.exe isn't launched with it.
	BOOL WINAPI MyCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
	{
		bool intercept = false;
		if (StrStrIW(lpCommandLine, L"Sniffer.exe") != NULL)
			intercept = true;

		if (!intercept)
			return CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

		// create suspended so we can inject ourselves, but remember what the caller wanted:
		const bool originalCreateSuspended = (dwCreationFlags & CREATE_SUSPENDED) != 0;
		dwCreationFlags |= CREATE_SUSPENDED;

		// always need to get PROCESS_INFORMATION, even if caller doesn't want:
		PROCESS_INFORMATION pi{};
		if (!lpProcessInformation)
			lpProcessInformation = &pi;

		auto r = CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

		if (r)
		{
			try
			{
				Utils::InjectDll(lpProcessInformation->hProcess, Utils::GetCurrentModulePath());
			}
			catch (...)
			{
				// can't really do much if injection fails, so just ignore the error...
			}

			// resume if the caller didn't use CREATE_SUSPENDED:
			if (!originalCreateSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
		}

		return r;
	}

	// Intercept CreateDXGIFactory to provide our wrapper:
	HRESULT WINAPI MyCreateDXGIFactory(REFIID riid, void** ppFactory)
	{
		CComPtr<IDXGIFactory> originalFactory;
		auto r = CreateDXGIFactory(riid, reinterpret_cast<void**>(&originalFactory));

		if (r == S_OK && originalFactory)
		{
			auto wrapperFactory = new FixedDXGIFactory(std::move(originalFactory));
			*ppFactory = wrapperFactory;
		}
		else
		{
			*ppFactory = nullptr;
		}

		return r;
	}

	void HookAll(HMODULE srcModule)
	{
		if (srcModule == NULL)
			return;

		DoHook(srcModule, "Kernel32.dll", "LoadLibraryA", &MyLoadLibraryA);
		DoHook(srcModule, "Kernel32.dll", "LoadLibraryW", &MyLoadLibraryW);
		DoHook(srcModule, "Kernel32.dll", "LoadLibraryExA", &MyLoadLibraryExA);
		DoHook(srcModule, "Kernel32.dll", "LoadLibraryExW", &MyLoadLibraryExW);
		DoHook(srcModule, "Kernel32.dll", "CreateProcessW", &MyCreateProcessW);
		DoHook(srcModule, "DXGI.dll", "CreateDXGIFactory", &MyCreateDXGIFactory);
	}
}

BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle, IN DWORD nReason, IN LPVOID Reserved)
{
	BOOLEAN bSuccess = TRUE;

	switch (nReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hDllHandle);

		// hook exe first:
		HookAll(GetModuleHandleA(NULL));

		// find all modules already loaded and hook them too:
		for (const auto& module : GetProcessModules())
		{
			HookAll(module);
		}

		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return bSuccess;
}