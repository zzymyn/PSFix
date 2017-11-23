#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdint>

BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle, IN DWORD nReason, IN LPVOID Reserved)
{
	BOOLEAN bSuccess = TRUE;

	switch (nReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hDllHandle);
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return bSuccess;
}

extern "C" __declspec(dllexport) std::int32_t PluginMain(const char* caller, const char* selector, void* message)
{
	return 0;
}
