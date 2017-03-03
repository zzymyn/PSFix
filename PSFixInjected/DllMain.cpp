#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#include <dxgi.h>
#include <atlbase.h>
#include <utility>
#include "FixedDXGIFactory.h"
#include "MemUnlocker.h"

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dbghelp.lib")

namespace
{
	template <typename T>
	T GetModuleObject(HMODULE module, DWORD offset)
	{
		return reinterpret_cast<T>(reinterpret_cast<char*>(module)+offset);
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

		const auto targetModule = GetModuleHandleA(dllName);

		if (targetModule == NULL)
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

	HRESULT WINAPI MyCreateDXGIFactory(REFIID riid, void **ppFactory);
	HMODULE WINAPI MyLoadLibraryA(LPCSTR lpLibFileName);
	HMODULE WINAPI MyLoadLibraryW(LPWSTR lpLibFileName);
	HMODULE WINAPI MyLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
	HMODULE WINAPI MyLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

	void HookAll(HMODULE srcModule)
	{
		if (srcModule == NULL)
			return;

		DoHook(srcModule, "Kernel32.dll", "LoadLibraryA", &MyLoadLibraryA);
		DoHook(srcModule, "Kernel32.dll", "LoadLibraryW", &MyLoadLibraryW);
		DoHook(srcModule, "Kernel32.dll", "LoadLibraryExA", &MyLoadLibraryExA);
		DoHook(srcModule, "Kernel32.dll", "LoadLibraryExW", &MyLoadLibraryExW);
		DoHook(srcModule, "DXGI.dll", "CreateDXGIFactory", &MyCreateDXGIFactory);
	}

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
}

BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle, IN DWORD nReason, IN LPVOID Reserved)
{
	BOOLEAN bSuccess = TRUE;

	switch (nReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hDllHandle);
		HookAll(GetModuleHandleA(NULL));
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return bSuccess;
}