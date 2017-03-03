#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class MemUnlocker
{
public:
	MemUnlocker(const MemUnlocker&) = delete;
	MemUnlocker(MemUnlocker&&) = delete;

	MemUnlocker(void* memory)
	{
		VirtualQuery(memory, &m_Mbi, sizeof(MEMORY_BASIC_INFORMATION));

		if (!VirtualProtect(m_Mbi.BaseAddress, m_Mbi.RegionSize, PAGE_READWRITE, &m_Mbi.Protect))
			return;
	}

	~MemUnlocker()
	{
		DWORD dwOldProtect;
		VirtualProtect(m_Mbi.BaseAddress, m_Mbi.RegionSize, m_Mbi.Protect, &dwOldProtect);
	}

	MemUnlocker& operator=(const MemUnlocker&) = delete;
	MemUnlocker& operator=(MemUnlocker&&) = delete;

private:
	MEMORY_BASIC_INFORMATION m_Mbi;
};