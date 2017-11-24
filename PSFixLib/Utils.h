#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>

namespace Utils
{
	HMODULE GetCurrentModule();
	HMODULE AddRefCurrentModule();

	std::wstring GetCurrentModulePath();

	std::wstring GetSiblingPath(const std::wstring& name);

	void InjectDll(HANDLE process, const std::wstring& injectDllFullPath);
};
