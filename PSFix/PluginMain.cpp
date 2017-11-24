#include <cstdint>

// The plugin does nothing, all the fun happens in DLLMain.

extern "C" __declspec(dllexport) std::int32_t PluginMain(const char* caller, const char* selector, void* message)
{
	return 1;
}
