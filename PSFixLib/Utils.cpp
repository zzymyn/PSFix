#include "Utils.h"
#include "ScopeExit.h"
#include <memory>

namespace
{
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

namespace Utils
{
	HMODULE GetCurrentModule()
	{
		HMODULE r;
		if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &r))
			return NULL;
		return r;
	}

	std::wstring GetCurrentModulePath()
	{
		const DWORD buffSize{ 1024 };
		const auto buff = std::make_unique<wchar_t[]>(buffSize);

		auto currentModule = GetCurrentModule();

		auto r = GetModuleFileNameW(currentModule, buff.get(), buffSize);
		if (r <= 0 || r >= buffSize)
		{
			throw std::exception("Failed to get current module path.");
		}

		return std::wstring{ buff.get() };
	}

	std::wstring GetSiblingPath(const std::wstring& name)
	{
		auto r = Utils::GetCurrentModulePath();

		const auto lastSlash = r.find_last_of(L'\\');

		if (lastSlash == std::wstring::npos)
			throw std::exception("Failed to sibling path.");

		r.resize(lastSlash + 1);
		r += name;

		return r;
	}

	void InjectDll(HANDLE process, const std::wstring& injectDllFullPath)
	{
		const auto injectDllFullPathSize = sizeof(wchar_t) * (injectDllFullPath.size() + 1);
		const auto loadLibraryWAddress = GetProcAddress("Kernel32", "LoadLibraryW");

		const auto remoteMemory = VirtualAllocEx(process, NULL, injectDllFullPathSize, MEM_COMMIT, PAGE_READWRITE);

		if (!remoteMemory)
			throw std::exception("Failed to alloc remote memory.");

		const auto remoteMemoryDeleter = OnScopeExit([=]()
		{
			VirtualFreeEx(process, remoteMemory, injectDllFullPathSize, 0);
		});

		if (WriteProcessMemory(process, remoteMemory, injectDllFullPath.c_str(), injectDllFullPathSize, NULL) == 0)
			throw std::exception("Failed to write process memory.");

		const auto remoteThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)loadLibraryWAddress, remoteMemory, 0, NULL);

		if (remoteThread == 0)
			throw std::exception("Failed to create remote thread.");

		const auto remoteThreadDeleter = OnScopeExit([=]()
		{
			CloseHandle(remoteThread);
		});

		if (WaitForSingleObject(remoteThread, INFINITE) == WAIT_FAILED)
			throw std::exception("Failed to wait for remote thread.");
	}
}