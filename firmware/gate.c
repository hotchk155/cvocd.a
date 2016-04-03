////////////////////////////////////////////////////////////
//
// MINI MIDI CV
//
// GATE OUTPUTS
//
////////////////////////////////////////////////////////////

// INCLUDE FILES
#include <system.h>
#include "cv-strip.h"

//
// CONSTANTS
//

// Define bits for the shift register outputs
#define SRB_NOTE1	0x0004
#define SRB_NOTE2	0x0008
#define SRB_NOTE3 	0x0002
#define SRB_NOTE4	0x0001
#define SRB_DRM1 	0x0100
#define SRB_DRM2 	0x0200
#define SRB_DRM3 	0x0400
#define SRB_DRM4 	0x0800
#define SRB_DRM5 	0x1000
#define SRB_DRM6 	0x2000
#define SRB_DRM7 	0x4000
#define SRB_DRM8 	0x8000

// initial CC "last value" 
#define NO_VALUE 	0xFF

// List of modes for a gate output to be triggered
enum {
	GATE_DISABLE,
	
	// respond to events from a note stack
	GATE_NOTE_ON,			// when any note on is received
	GATE_NOTE_ACCENT,		// when a note is received above accent threshold
	GATE_NOTES_OFF,			// when no notes are pressed		
	GATE_NOTE_GATEA,		// when note out A is playing
	GATE_NOTE_GATEB,		// when note out B is playing
	GATE_NOTE_GATEC,		// when note out C is playing
	GATE_NOTE_GATED,		// when note out D is playing
		
	// Respond to raw MIDI events
	GATE_MIDI_NOTE,				// arbitrary note mapping
	GATE_MIDI_CC,				// arbitrary CC mapping
	GATE_MIDI_CLOCK_TICK,		// clock tick
	GATE_MIDI_CLOCK_RUN_TICK,	// clock tick if clock running
	GATE_MIDI_CLOCK_RUN,		// clock running
	GATE_MIDI_CLOCK_START,		// start message
	GATE_MIDI_CLOCK_STARTCONT,	// continue OR start message
	GATE_MIDI_CLOCK_STOP		// stop message
};

#define GATE_FLAG_INVERT 0x01

//
// STRUCT DEFS
//

// Structure to hold mapping of a note stack event to a gate
typedef struct {
	byte mode;			// type of trigger - GATE_xxx enum
	byte flags;			// Mode flags
	byte counter;		// (STATE) pulse duration counter
	byte duration;		// gate pulse duration in ms (or 0 for "as long as active")
	byte stack_id;	// index of the note stack
} T_GATE_EVENT;

// Structure to hold mapping of a raw MIDI note to a gate
typedef struct {
	byte mode;			// type of trigger - GATE_xxx enum
	byte flags;			// Mode flags
	byte counter;		// (STATE) pulse duration counter
	byte duration;		// gate pulse duration in ms (or 0 for "as long as active")
	byte chan;			// midi channel
	byte note;			// note range: lowest note
	byte note_max;		// note range: highest note (0 if there is only one note)
	byte vel_min;		// minimum velocity that will trigger the gate
} T_GATE_MIDI_NOTE;

// Structure to hold mapping of a raw MIDI CC to a gate
typedef struct {
	byte mode;			// type of trigger - GATE_xxx enum
	byte flags;			// Mode flags
	byte counter;		// (STATE) pulse duration counter
	byte duration;		// gate pulse duration in ms (or 0 for "as long as active")
	byte chan;			// midi channel
	byte cc;			// CC number
	byte threshold;		// threshold for gate ON
	byte last_value;	// (STATE) last CC value
} T_GATE_MIDI_CC;

// Structure to hold mapping of MIDI clock to a gate
typedef struct {
	byte mode;			// type of trigger - GATE_xxx enum
	byte flags;			// Mode flags
	byte counter;		// (STATE) pulse duration counter
	byte duration;		// gate pulse duration in ms (or 0 for "as long as active")
	byte div;			// clock divider (@24ppqn)
	byte ticks;			// (STATE) number of ticks counted
} T_GATE_MIDI_CLOCK;

// The gate out structure which combines the above
typedef union {
	T_GATE_EVENT 		event;
	T_GATE_MIDI_NOTE 	note;
	T_GATE_MIDI_CC 		cc;
	T_GATE_MIDI_CLOCK	clock;
} GATE_OUT;

//
// FILE SCOPE DATA
//

// Whether MIDI clock is running
static byte midi_clock_running = 0;

// Shift register status
static unsigned int g_sr_data = 0;

// Gate status
static GATE_OUT g_gate[GATE_MAX];

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////
// LOAD GATE SHIFT REGISTER
static void load_gates(unsigned int d) {
	unsigned int m1 = 0x0080;
	unsigned int m2 = 0x8000;
	P_SRLAT = 0;
	while(m1) {
		P_SRCLK = 0;
		P_SRDAT1 = !!(d&m1);
		P_SRDAT2 = !!(d&m2);
		P_SRCLK = 1;
		m1>>=1;
		m2>>=1;
	}
	P_SRLAT = 1;
}

////////////////////////////////////////////////////////////
// TRIGGER OR UNTRIGGER A GATE
static void trigger(GATE_OUT *pgate, byte which_gate, byte trigger_enabled)
{
	// get appropriate shift register bit
	unsigned int gate_bit = 0;
	switch(which_gate) {
		case 0:  gate_bit = SRB_NOTE1; break;
		case 1:  gate_bit = SRB_NOTE2; break;
		case 2:  gate_bit = SRB_NOTE3; break;
		case 3:  gate_bit = SRB_NOTE4; break;
		case 4:  gate_bit = SRB_DRM1; break;
		case 5:  gate_bit = SRB_DRM2; break;
		case 6:  gate_bit = SRB_DRM3; break;
		case 7:  gate_bit = SRB_DRM4; break;
		case 8:  gate_bit = SRB_DRM5; break;
		case 9:  gate_bit = SRB_DRM6; break;
		case 10: gate_bit = SRB_DRM7; break;
		case 11: gate_bit = SRB_DRM8; break;
		default: return;
	}	
			
	// apply bit change to shift register data
	unsigned int new_data = g_sr_data;
	if(trigger_enabled) {
		if(pgate->event.flags & GATE_FLAG_INVERT) {
			new_data = g_sr_data & ~gate_bit;
		}
		else {
			new_data = g_sr_data | gate_bit;
		}
		if(GATE_DUR_GLOBAL == pgate->event.duration) {
			pgate->event.counter = g_gate_duration;
		}
		else {
			pgate->event.counter = pgate->event.duration;
		}
	}
	else {
		if(pgate->event.flags & GATE_FLAG_INVERT) {
			new_data = g_sr_data | gate_bit;
		}
		else {
			new_data = g_sr_data & ~gate_bit;
		}
		pgate->event.counter = 0;
	}
	
	// reload shift registers if data changed
	if(new_data != g_sr_data) {
		g_sr_data = new_data;
		load_gates(g_sr_data);
	}
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// HANDLE EVENT FROM A NOTE STACK
void gate_event(byte event, byte stack_id)
{

	// iterate thru gate outputs
	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {	
		GATE_OUT *pgate = &g_gate[which_gate];
		
		// check this output is watching this note stack
		if(pgate->event.stack_id != stack_id)
			continue;
			
		// check the mode of this gate against the event
		switch(pgate->event.mode) {
			case GATE_NOTE_ON: // Any note on/changed
				if(EV_NOTE_ON == event) {
					trigger(pgate, which_gate, true);
				}
				else if(EV_NOTES_OFF == event) {
					trigger(pgate, which_gate, false);
				}
				break;
			case GATE_NOTES_OFF: // All notes off
				if(EV_NOTES_OFF == event) {
					trigger(pgate, which_gate, true);
				}
				else if(EV_NOTE_ON == event) {
					trigger(pgate, which_gate, false);
				}
				break;
			case GATE_NOTE_ACCENT: // Any note on with velocity above threshold
				if(EV_NOTE_ON == event) {
					trigger(pgate, which_gate, (g_stack[stack_id].vel >= g_accent_vel));
				}
				else if(EV_NOTES_OFF == event) {					
					trigger(pgate, which_gate, false);
				}
				break;
			case GATE_NOTE_GATEA: // Note present at output A
				if(event == EV_NOTE_A) {
					trigger(pgate, which_gate, true);
				}
				else if(event == EV_NO_NOTE_A) {
					trigger(pgate, which_gate, false);
				}
				break;
			case GATE_NOTE_GATEB: // Note present at output B
				if(event == EV_NOTE_B) {
					trigger(pgate, which_gate, true);
				}
				else if(event == EV_NO_NOTE_B) {
					trigger(pgate, which_gate, false);
				}
				break;
			case GATE_NOTE_GATEC: // Note present at output C
				if(event == EV_NOTE_C) {
					trigger(pgate, which_gate, true);
				}
				else if(event == EV_NO_NOTE_C) {
					trigger(pgate, which_gate, false);
				}
				break;
			case GATE_NOTE_GATED: // Note present at output D
				if(event == EV_NOTE_D) {
					trigger(pgate, which_gate, true);
				}
				else if(event == EV_NO_NOTE_D) {
					trigger(pgate, which_gate, false);
				}
				break;
		}
	}
}

////////////////////////////////////////////////////////////
// HANDLE EVENT FROM RAW MIDI NOTE
// Note on has velocity > 0
void gate_midi_note(byte chan, byte note, byte vel) 
{
	// for each gate output
	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {
		GATE_OUT *pgate = &g_gate[which_gate];
		
		// does this gate respond to midi note?
		if(pgate->event.mode != GATE_MIDI_NOTE)
			continue;
			
		// does the MIDI channel match?
		if(!IS_CHAN(pgate->note.chan, chan))
			continue;			
		// Does the note match?
		if(!IS_NOTE_MATCH(pgate->note.note, pgate->note.note_max, note))
			continue;
			
		// is this a note off or note on with velocity above threshold?
		if(vel) {
			if(VEL_ACC_GLOBAL == pgate->note.vel_min) {
				if(vel < g_accent_vel)
					continue;
			}
			else if( vel < pgate->note.vel_min) {			
				continue;
			}
		}		
		
		// trigger (for note on) or untrigger (for note off)
		trigger(pgate, which_gate, !!vel);
	}			
}

////////////////////////////////////////////////////////////
// HANDLE EVENT FROM RAW MIDI CC
void gate_midi_cc(byte chan, byte cc, byte value) 
{
	// for each gate output
	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {
		GATE_OUT *pgate = &g_gate[which_gate];
		
		// does this gate respond to CC?
		if(pgate->event.mode != GATE_MIDI_CC)
			continue;
		
		// is this the correct CC?	
		if(cc != pgate->cc.cc) {
			continue;
		}		
		// does the MIDI channel match?
		if(!IS_CHAN(pgate->cc.chan, chan))
			continue;

				
		// has the value just gone above threshold?
		if(value >= pgate->cc.threshold &&
			( pgate->cc.last_value < pgate->cc.threshold || 
				pgate->cc.last_value == NO_VALUE)) {
			
			// trigger gate
			trigger(pgate, which_gate, true);
			pgate->cc.last_value = value;		
		}
		// has the value just gone below threshold?
		else if(value < pgate->cc.threshold &&
			( pgate->cc.last_value >= pgate->cc.threshold || 
				pgate->cc.last_value == NO_VALUE)) {
			
			// untrigger gate
			trigger(pgate, which_gate, false);
			pgate->cc.last_value = value;		
		}
	}			
}

////////////////////////////////////////////////////////////
// HANDLE EVENT FROM RAW MIDI CLOCK MESSAGE
void gate_midi_clock(byte msg) {
	byte which_gate;
	GATE_OUT *pgate;
	switch(msg) {
	// CLOCK TICK
	case MIDI_SYNCH_TICK: 
		for(which_gate=0; which_gate<GATE_MAX; ++which_gate) {
			pgate = &g_gate[which_gate];			
			//is this gate tied to clock ticks?
			if((GATE_MIDI_CLOCK_TICK == pgate->event.mode) || (GATE_MIDI_CLOCK_RUN_TICK == pgate->event.mode)) {
				// does it need the clock to be running?
				if((GATE_MIDI_CLOCK_RUN_TICK == pgate->event.mode) && !midi_clock_running) {
					continue;
				}
				trigger(pgate, which_gate, true);
			}
		}
		break;
	// CLOCK START AND CONTINUE
	case MIDI_SYNCH_START:
	case MIDI_SYNCH_CONTINUE:
		midi_clock_running = 1;
		for(which_gate=0; which_gate<GATE_MAX; ++which_gate) {
			pgate = &g_gate[which_gate];			
			switch(pgate->event.mode) {				
			case GATE_MIDI_CLOCK_START:
				if(msg != MIDI_SYNCH_START) {
					break;
				}// else fall through				
			case GATE_MIDI_CLOCK_RUN:
			case GATE_MIDI_CLOCK_STARTCONT:
				trigger(pgate, which_gate, true);
				break;
			case GATE_MIDI_CLOCK_STOP:
				trigger(pgate, which_gate, false);
				break;
			}
		}
		break;
	// CLOCK STOP
	case MIDI_SYNCH_STOP:
		midi_clock_running = 0;
		for(which_gate=0; which_gate<GATE_MAX; ++which_gate) {
			pgate = &g_gate[which_gate];			
			switch(pgate->event.mode) {				
			case GATE_MIDI_CLOCK_RUN:
				trigger(pgate, which_gate, false);
				break;
			case GATE_MIDI_CLOCK_STOP:
				trigger(pgate, which_gate, true);
				break;
			}
		}
		break;
	}
}

////////////////////////////////////////////////////////////
// MANAGE GATE TIMEOUTS
void gate_run() {
	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {
		GATE_OUT *pgate = &g_gate[which_gate];
		if(pgate->event.counter) {
			if(!--pgate->event.counter) {
				trigger(pgate, which_gate, false);
			}
		}
	}
}

////////////////////////////////////////////////////////////
// TRIGGER A GATE DIRECTLY (testing)
void gate_trigger(byte which_gate, byte trigger_enabled)
{
	if(which_gate < GATE_MAX) {
		trigger(&g_gate[which_gate], which_gate, trigger_enabled);
	}
}



////////////////////////////////////////////////////////////
// SET DEFAULT GATE CONFIG
void gate_init() {

	// initialise state info
	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {
		GATE_OUT *pgate = &g_gate[which_gate];
		pgate->event.mode = GATE_DISABLE;
		pgate->event.flags = 0;
		pgate->event.duration = DEFAULT_GATE_DURATION;
	}
	gate_reset();
	
	g_gate[0].event.mode = GATE_NOTE_GATEA;	
	g_gate[0].event.duration = 0;	
}

////////////////////////////////////////////////////////////
// SET DEFAULT GATE STATE
void gate_reset() {

	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {
		GATE_OUT *pgate = &g_gate[which_gate];
		pgate->event.counter = 0;
		switch(pgate->event.mode) {
		case GATE_MIDI_CC:
			pgate->cc.last_value = NO_VALUE;
			break;
		case GATE_MIDI_CLOCK_TICK:
		case GATE_MIDI_CLOCK_RUN_TICK:
		case GATE_MIDI_CLOCK_RUN:
		case GATE_MIDI_CLOCK_START:
		case GATE_MIDI_CLOCK_STARTCONT:
		case GATE_MIDI_CLOCK_STOP:
			pgate->clock.ticks = 0;
			break;
		case GATE_MIDI_NOTE:
		default:
			break;
		}
		trigger(pgate, which_gate, false);
	}

}

/*
////////////////////////////////////////////////////////////
// CONFIGURE A GATE OUTPUT
// return nonzero if any change was made
byte gate_cfg(byte which_gate, byte param, byte value) {
	byte note, note_max; 
	if(which_gate >= GATE_MAX)
		return 0;
		
	GATE_OUT *pgate = &g_gate[which_gate];
	switch(param) {
	//////////////////////////////////////////////////////////
	// DISABLE A CV OUTPUT
	case P_DISABLE:
		pgate->event.mode = GATE_DISABLE;
		return 1;
	//////////////////////////////////////////////////////////
	// CONFIGURE GATE OUTPUT FOR INPUT STACK
	case P_INPUT1:
	case P_INPUT2:
	case P_INPUT3:
	case P_INPUT4:	
		switch(value) {
		case P_INPUT_NOTEA:			pgate->event.mode = GATE_NOTE_GATEA; 	break;
		case P_INPUT_NOTEB:			pgate->event.mode = GATE_NOTE_GATEB; 	break;
		case P_INPUT_NOTEC:			pgate->event.mode = GATE_NOTE_GATEC; 	break;
		case P_INPUT_NOTED:			pgate->event.mode = GATE_NOTE_GATED; 	break;
		case P_INPUT_NOTE_ON:		pgate->event.mode = GATE_NOTE_ON; 		break;
		case P_INPUT_NOTE_ACCENT:	pgate->event.mode = GATE_NOTE_ACCENT; 	break;
		case P_INPUT_NOTES_OFF:		pgate->event.mode = GATE_NOTES_OFF; 	break;
		default: return 0;
		}
		pgate->event.stack_id = param - P_INPUT1;		
		pgate->event.counter = 0;
		pgate->event.duration = GATE_DUR_GLOBAL;
		return 1;
		
	//////////////////////////////////////////////////////////
	// CONFIGURE GATE OUTPUT FOR MIDI
	case P_MIDI:
		switch(value) {	
			case P_MIDI_NOTE:
				pgate->event.mode = GATE_MIDI_NOTE;
				pgate->note.chan = CHAN_GLOBAL;
				pgate->note = DEFAULT_GATE_NOTE;
				pgate->note_max = 0;
				pgate->vel_min = 0;
				break;
			case P_MIDI_CC:
				pgate->event.mode = GATE_MIDI_CC;
				pgate->note.chan = CHAN_GLOBAL;
				pgate->cc.cc = DEFAULT_GATE_CC;
				pgate->cc.threshold = DEFAULT_GATE_THRESHOLD;
				pgate->cc.last_value = NO_VALUE;
				break;
			case P_MIDI_TICK:
			case P_MIDI_RUN_TICK:
			case P_MIDI_RUN:
			case P_MIDI_START:
			case P_MIDI_STOP:
			case P_MIDI_STARTCONT:
				switch(value) {
				case P_MIDI_TICK: 		pgate->event.mode = GATE_MIDI_CLOCK_TICK; 		break;
				case P_MIDI_RUN_TICK:	pgate->event.mode = GATE_MIDI_CLOCK_RUN_TICK;	break;
				case P_MIDI_RUN: 		pgate->event.mode = GATE_MIDI_CLOCK_RUN;		break;
				case P_MIDI_START:		pgate->event.mode = GATE_MIDI_CLOCK_START;		break;
				case P_MIDI_STOP:		pgate->event.mode = GATE_MIDI_CLOCK_STOP;		break;
				case P_MIDI_STARTCONT:	pgate->event.mode = GATE_MIDI_CLOCK_STARTCONT;	break;
				}
				pgate->clock.div = DEFAULT_GATE_DIV;
				pgate->clock.ticks = 0;
				break;
			}
		}
		pgate->event.counter = 0;
		pgate->event.duration = GATE_DUR_GLOBAL;
		break;
		
	/////////////////////////////////////////////////////////////
	// SETUP MIDI CHANNEL
	case P_CHAN_OMNI:
		value = CHAN_OMNI;
		goto set_chan:
	case P_CHAN_GLOBAL:
		value = CHAN_GLOBAL;
		goto set_chan:
	case P_CHAN:
		if(value < 1 || value > 16)
			return 0;
		--value;
set_chan:				
		switch(pgate->event.mode) {
			case GATE_MIDI_NOTE:
				pgate->note.chan = value;
				return 1;
			case GATE_MIDI_CC:
				pgate->cc.chan = value;
				return 1;
		}
		return 0;
		
	/////////////////////////////////////////////////////////////
	// SETUP MIDI NOTE RANGE
	case P_NOTE_SINGLE:
		if(pgate->event.mode == GATE_MIDI_NOTE && value < 128) {
			pgate->note.note = value;
			pgate->note.note_max = 0;
			return 1;
		}
		return 0;
	case P_NOTE_RANGE_FROM:
		if(pgate->event.mode == GATE_MIDI_NOTE && value < 128) {
			pgate->note.note = value;
			return 1;
		}
		return 0;
	case P_NOTE_RANGE_TO:
		if(pgate->event.mode == GATE_MIDI_NOTE && value < 128) {
			pgate->note.note_max = value;
			return 1;
		}		
		return 0;		

	/////////////////////////////////////////////////////////////
	// SETUP MIDI VELOCITY	
	case P_VEL_MIN:
		if(pgate->event.mode == P_MIDI_NOTE && value < 128) {
			pgate->note.vel_min = value;
			return 1;
		}
		return 0;		

	/////////////////////////////////////////////////////////////
	// SETUP MIDI CC
	case P_CC:
		if(pgate->event.mode == GATE_MIDI_CC && value < 128) {
			pgate->cc.cc = value;
			return 1;
		}
		return 0;
		
	/////////////////////////////////////////////////////////////
	// SETUP MIDI CC THRESHOLD
	case P_CC_THRESHOLD:
		if(pgate->event.mode == GATE_MIDI_CC && value < 128) {
			pgate->cc.threshold = value;
			return 1;
		}
		return 0;

	/////////////////////////////////////////////////////////////
	// SETUP MIDI CLOCK DIVIDER
	case P_DIV:
		if(!value) {
			return 0;
		}
		switch(pgate->event.mode) {
		case P_MIDI_TICK:
		case P_MIDI_RUN_TICK:
		case P_MIDI_RUN:
		case P_MIDI_START:
		case P_MIDI_STOP:
		case P_MIDI_STARTCONT:
			pgate->clock.div = value;
			return 1;
		}
		return 0;

	/////////////////////////////////////////////////////////////
	// SETUP DURATION
	case P_DURATION:
		pgate->event.duration = value;
		return 1;
	case P_DURATION_GLOBAL:
		pgate->event.duration = GATE_DUR_GLOBAL;
		return 1;
	}
}		


*/