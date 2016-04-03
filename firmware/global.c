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
#include "cv-strip.h"


//
// GLOBAL DATA
//
byte g_chan = DEFAULT_MIDI_CHANNEL; // default MIDI channel
byte g_accent_vel = DEFAULT_ACCENT_VELOCITY; // accent velocity
byte g_gate_duration = DEFAULT_GATE_DURATION; // default gate duration

////////////////////////////////////////////////////////////
// CONFIGURE A GLOBAL SETTING
byte global_cfg(byte param, byte value) {
	switch(param) {
		// GLOBAL DEFAULT MIDI CHANNEL
		case P_CHAN:			
			if(value != CHAN_OMNI && (value < 1 || value > 16 ))
				return 0;
			g_chan = value;
			return 1;
		// GLOBAL DEFAULT ACCENT VELOCITY THRESHOLD
		case P_VEL_MIN:
			if(value > 127)
				return 0;
			g_accent_vel = value;
			return 1;
		// GLOBAL DEFAULT PULSE DURATION
		case P_DURATION:
			g_gate_duration = value;
			return 1;
	}
	return 0;
}

void global_init() {
}
