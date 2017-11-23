#include <Windows.h>
#include "PIGeneral.h"

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

extern "C" __declspec(dllexport) SPAPI SPErr AutoPluginMain(const char* caller, const char* selector, void* message)
{
	return kSPUnimplementedError;
}
