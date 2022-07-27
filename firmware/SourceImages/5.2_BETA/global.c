//////////////////////////////////////////////////////////////
//
//       ///// //   //          /////    /////  /////
//     //     //   //         //    // //      //   //
//    //      // //    //    //    // //      //   //
//   //      // //   ////   //    // //      //   //
//   /////   ///     //     //////   /////  //////
//
// CV.OCD MIDI-TO-CV CONVERTER
// hotchk155/2016
// Sixty Four Pixels Limited
//
// This work is distibuted under terms of Creative Commons 
// License BY-NC-SA (Attribution, Non-commercial, Share-Alike)
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//
//////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//
// GLOBAL DEFAULTS MODULE
//
//////////////////////////////////////////////////////////////

// INCLUDE FILES
#include <system.h>
#include <memory.h>
#include <eeprom.h>
#include "cvocd.h"



//
// GLOBAL DATA
//
GLOBAL_CFG g_global;

//
// GLOBAL FUNCTIONS
//

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
	
	////////////////////////////////////////////////////////////////
	// SAVE
	case NRPNL_SAVE:
		storage_write_patch();	// store to EEPROM 
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////
// GET PATCH STORAGE INFO
byte *global_storage(int *len) {
	*len = sizeof(g_global);
	return (byte*)&g_global;
}

////////////////////////////////////////////////////////////
// INITIALISE GLOBALS
void global_init() {
	g_global.chan = DEFAULT_MIDI_CHANNEL; // default MIDI channel
	g_global.gate_duration = DEFAULT_GATE_DURATION; // default gate duration
}

//
// END
//