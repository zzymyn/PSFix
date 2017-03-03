#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <exception>
#include "ScopeExit.h"

namespace
{
	static const std::wstring s_InjectedDll = L"PSFixInjected.dll";
	static const std::wstring s_PhotoshopPath = L"C:\\Program Files\\Adobe\\Adobe Photoshop CC 2017";
	static const std::wstring s_PhotoshopExe = s_PhotoshopPath + L"\\Photoshop.exe";

	std::wstring GetPSFixInjectedDll()
	{
		wchar_t* wpgmptr{ nullptr };

		if (_get_wpgmptr(&wpgmptr) != 0)
			throw std::exception("Failed to get injected DLL path.");

		std::wstring r{ wpgmptr };

		const auto lastSlash = r.find_last_of(L'\\');

		if (lastSlash == std::wstring::npos)
			throw std::exception("Failed to get injected DLL path.");

		r.resize(lastSlash + 1);
		r += s_InjectedDll;

		return r;
	}

	FARPROC GetProcAddress(LPCSTR moduleName, LPCSTR procName)
	{
		const auto moduleHandle = GetModuleHandleA(moduleName);

		if (moduleHandle == 0)
			throw std::exception("Failed to get module handle.");

		const auto procAddress = GetProcAddress(moduleHandle, procName);

		if (procAddress == 0)
			throw std::exception("Failed to get LoadLibraryW address.");

		return procAddress;
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	try
	{
		const auto injectDllFullPath = GetPSFixInjectedDll();
		const auto injectDllFullPathSize = sizeof(wchar_t) * (injectDllFullPath.size() + 1);
		const auto loadLibraryWAddress = GetProcAddress("Kernel32", "LoadLibraryW");

		STARTUPINFO si{};
		PROCESS_INFORMATION pi{};

		if (CreateProcessW(s_PhotoshopExe.c_str(), lpCmdLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, s_PhotoshopPath.c_str(), &si, &pi) == 0)
			throw std::exception("Failed to open Photoshop.exe");

		auto createProcessDeleter = OnScopeExit([=]()
		{
			TerminateProcess(pi.hProcess, 1);
		});

		const auto remoteMemory = VirtualAllocEx(pi.hProcess, NULL, injectDllFullPathSize, MEM_COMMIT, PAGE_READWRITE);

		if (!remoteMemory)
			throw std::exception("Failed to alloc remote memory.");

		const auto remoteMemoryDeleter = OnScopeExit([=]()
		{
			VirtualFreeEx(pi.hProcess, remoteMemory, injectDllFullPathSize, 0);
		});

		if (WriteProcessMemory(pi.hProcess, remoteMemory, injectDllFullPath.c_str(), injectDllFullPathSize, NULL) == 0)
			throw std::exception("Failed to write process memory.");

		const auto remoteThread = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryWAddress, remoteMemory, 0, NULL);

		if (remoteThread == 0)
			throw std::exception("Failed to create remote thread.");

		const auto remoteThreadDeleter = OnScopeExit([=]()
		{
			CloseHandle(remoteThread);
		});

		if (WaitForSingleObject(remoteThread, INFINITE) == WAIT_FAILED)
			throw std::exception("Failed to wait for remote thread.");

		if (ResumeThread(pi.hThread) == -1)
			throw std::exception("Failed to resume remote thread.");

		createProcessDeleter.Cancel();

		return 0;
	}
	catch (std::exception& ex)
	{
		MessageBeep(MB_ICONEXCLAMATION);
		MessageBoxA(NULL, ex.what(), "Error", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
}