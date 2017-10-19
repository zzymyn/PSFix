#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <memory>
#include <exception>
#include "ScopeExit.h"
#include "Utils.h"

namespace
{
	static const auto s_InjectedDll{ L"PSFixInjected.dll" };
	static const auto s_PhotoshopExe{ L"\\Photoshop.exe" };
	static const auto s_PhotoshopRegPath{ L"SOFTWARE\\Adobe\\Photoshop" };

	template <typename F>
	void EnumRegistryKeys(HKEY key, F&& f)
	{
		const DWORD buffSize{ 1024 };
		const auto buff = std::make_unique<wchar_t[]>(buffSize);

		for (DWORD index = 0; ; ++index)
		{
			const auto enumResult = RegEnumKeyW(key, index, buff.get(), buffSize);
			if (enumResult == ERROR_SUCCESS)
			{
				f(std::wstring{ buff.get() });
			}
			else if (enumResult == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else
			{
				throw std::exception("Error enumerating registry keys.");
			}
		}
	}

	std::wstring GetRegistryString(HKEY key, const std::wstring& path)
	{
		const DWORD buffSize{ 1024 };
		const auto buff = std::make_unique<char[]>(buffSize);
		auto outBuffSize = buffSize;

		if (RegGetValueW(key, path.c_str(), NULL, RRF_RT_REG_SZ, NULL, buff.get(), &outBuffSize) != ERROR_SUCCESS)
		{
			throw std::exception("Failed to get registry value.");
		}

		return std::wstring{ reinterpret_cast<const wchar_t*>(buff.get()) };
	}

	std::wstring FindPhotoshopPath()
	{
		HKEY photoshopKey{};

		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, s_PhotoshopRegPath, 0, KEY_READ, &photoshopKey) != ERROR_SUCCESS)
		{
			throw std::exception("Couldn't find Photoshop path in registry.");
		}

		const auto photoshopKeyDeleter = OnScopeExit([=]()
		{
			RegCloseKey(photoshopKey);
		});

		std::wstring highestVersion;

		EnumRegistryKeys(photoshopKey, [&](const auto& subkey)
		{
			if (subkey >= highestVersion)
			{
				highestVersion = subkey;
			}
		});

		if (highestVersion.empty())
		{
			throw std::exception("Couldn't find Photoshop path in registry.");
		}

		return GetRegistryString(photoshopKey, highestVersion + L"\\ApplicationPath");
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	try
	{
		const auto photoshopPath = FindPhotoshopPath();
		const auto photoshopExe = photoshopPath + s_PhotoshopExe;
		const auto injectDllFullPath = Utils::GetSiblingPath(s_InjectedDll);

		STARTUPINFO si{};
		PROCESS_INFORMATION pi{};

		if (CreateProcessW(photoshopExe.c_str(), lpCmdLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, photoshopPath.c_str(), &si, &pi) == 0)
			throw std::exception("Failed to open Photoshop.exe");

		auto createProcessDeleter = OnScopeExit([=]()
		{
			TerminateProcess(pi.hProcess, 1);
		});

		Utils::InjectDll(pi.hProcess, injectDllFullPath);

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