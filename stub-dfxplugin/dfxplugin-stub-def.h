// This file includes many important #defines for making a DfxPlugin.  
// Make this a "prefix" or "entry" header file of your project, 
// or somehow define this stuff another way (in compiler settings, or whatever).


// the name of the plugin
#define PLUGIN_NAME_STRING	"DfxPlugin stub"
// the name of the plugin prepended with "Destroy FX: "   (for Audio Unit)
#define PLUGIN_DOUBLE_NAME_STRING	"Destroy FX: DfxPlugin stub"
// a four-byte ID for the plugin
#define PLUGIN_ID	'stub'
// the version expressed in hex with four bytes:  ?-major-minor-bugfix
#define PLUGIN_VERSION	0x00010000
// Audio Unit entry point:  base class name appended with "Entry"
#define PLUGIN_ENTRY_POINT	"DfxStubEntry"
// 0 or 1
#define TARGET_PLUGIN_USES_MIDI	1
// 0 or 1	(1 implied 1 is also true for TARGET_PLUGIN_USES_MIDI)
#define TARGET_PLUGIN_IS_INSTRUMENT	0
// 0 or 1	(whether or not the plugin uses a DSP processing core class, 1-in/1-out)
#define TARGET_PLUGIN_USES_DSPCORE	0
// 0 or 1	(whether or not the plugin has its own custom graphical interface)
#define TARGET_PLUGIN_HAS_GUI	1

// only necessary if using a custom GUI
#if TARGET_PLUGIN_HAS_GUI
	// the name of the plugin GUI prepended with "Destroy FX: "   (for Audio Unit, not important)
	#define PLUGIN_EDITOR_DOUBLE_NAME_STRING	"Destroy FX: DfxPlugin stub editor"
	// GUI component entry point (for Audio Unit)
	#define PLUGIN_EDITOR_ENTRY_POINT	"DfxStubEditorEntry"
#endif

// optional
// a description of the effect (for Audio Unit)
#define PLUGIN_DESCRIPTION_STRING	"demonstration of how to use DfxPlugin"
// music plugin component resource ID (for Audio Unit)
#define PLUGIN_RES_ID	3000
#if TARGET_PLUGIN_HAS_GUI
	// a four-byte ID for the GUI component of the plugin (for Audio Unit)
	#define PLUGIN_EDITOR_ID	'stuV'
	// a description of the GUI component of the plugin (for Audio Unit, not important)
	#define PLUGIN_EDITOR_DESCRIPTION_STRING	"graphics for DfxPlugin stub"
	// GUI component resource ID (for Audio Unit)
	#define PLUGIN_EDITOR_RES_ID	9000
#endif


// 0 or 1
// whether or not we want to support loading old-style (pre-DfxPlugin) VST settings
// definitely not necessary if you are writing a new plugin
#define DFX_SUPPORT_OLD_VST_SETTINGS 0


// only define one of these with 1, the rest with 0
#define TARGET_API_AUDIOUNIT	1
#define TARGET_API_VST	0


#if TARGET_API_VST
	#include "vstplugscarbon.h"	// Carbon for classic Mac OS or Mac OS X
//	#include "vstplugsmac.h"	// classic Mac OS
	// the number of ins and outs in VST must be fixed
	#define NUM_INPUTS 2	// change as needed
	#define NUM_OUTPUTS 2	// change as needed
#endif
