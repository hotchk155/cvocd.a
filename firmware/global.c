////////////////////////////////////////////////////////////
//
// MINI MIDI CV
//
// GLOBAL SETTINGS MODULE
//
////////////////////////////////////////////////////////////

// INCLUDE FILES
#include <system.h>
#include <memory.h>
#include <eeprom.h>
#include "cv-strip.h"


//
// GLOBAL DATA
//
GLOBAL_CFG g_global;

////////////////////////////////////////////////////////////
// CONFIGURE A GLOBAL SETTING
byte global_nrpn(byte param_lo, byte value_hi, byte value_lo)
{
	switch(param_lo) {
		// GLOBAL DEFAULT MIDI CHANNEL
	case NRPNL_CHAN:
		switch(value_hi) {
		default:
		case NRPVH_CHAN_SPECIFIC:
			if(value_lo >= 1 && value_lo <= 16) {
				g_global.chan = value_lo-1;
				return 1;
			}		
			break;
		}
		break;	

		
	////////////////////////////////////////////////////////////////
	// SELECT GATE DURATION
	case NRPNL_GATE_DUR:
		switch(value_hi) {
		case NRPVH_DUR_MS:
			g_global.gate_duration = value_lo;
			return 1;
		}
		break;
	}
	return 0;
}

////////////////////////////////////////////////////////////
// INITIALISE GLOBALS
void global_init() {
	g_global.chan = DEFAULT_MIDI_CHANNEL; // default MIDI channel
	g_global.gate_duration = DEFAULT_GATE_DURATION; // default gate duration
}

////////////////////////////////////////////////////////////
// GET PATCH STORAGE INFO
byte *global_storage(int *len) {
	*len = sizeof(g_global);
	return (byte*)&g_global;
}
