#include "PIGeneral.h"

#define plugInName "PSFix"
#define plugInCopyrightYear	"2017"
#define plugInDescription "Stop crashing."

resource 'PiPL' ( 16000, plugInName, purgeable)
	{
		{
		Kind { Extension },
		Name { plugInName },
		Category { "**Hidden**" },
		Version { (latestExtensionVersion << 16) | latestExtensionSubVersion },

		Component { ComponentNumber, plugInName },

		CodeWin64X86 { "AutoPluginMain" },

		EnableInfo { "false" },
		
		}
	};
