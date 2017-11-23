#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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
