#include "PIGeneral.h"

resource 'PiPL' (16000, "PSFix", purgeable)
{
	{
	    Kind { Extension },
	    Name { "PSFix" },
	    Version { (latestExtensionVersion << 16) | latestExtensionSubVersion },
	    CodeWin64X86 { "PluginMain" },
	}
};
