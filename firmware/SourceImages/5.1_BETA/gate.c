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
// GATE OUTPUT MODULE
//
//////////////////////////////////////////////////////////////

// INCLUDE FILES
#include <system.h>
#include "cvocd.h"


//
// TYPE DEFS
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

// flag bits
#define GATE_FLAG_RETRIG 0x01

// List of modes for a gate output to be triggered
enum {
	GATE_DISABLE,
		
	// Respond to raw MIDI events
	GATE_MIDI_NOTE 				= NRPVH_SRC_MIDINOTE,	// arbitrary note mapping
	GATE_MIDI_CC 				= NRPVH_SRC_MIDICC,		// arbitrary CC mapping > threshold
	GATE_MIDI_CC_NEG			= NRPVH_SRC_MIDICC_NEG,	// arbitrary CC mapping < threshold
	GATE_MIDI_CLOCK_TICK 		= NRPVH_SRC_MIDITICK,	// clock tick
	GATE_MIDI_CLOCK_RUN_TICK	= NRPVH_SRC_MIDITICKRUN,// clock tick if clock running
	GATE_MIDI_CLOCK_RUN			= NRPVH_SRC_MIDIRUN,	// clock running
	GATE_MIDI_CLOCK_START		= NRPVH_SRC_MIDISTART,	// start message
	GATE_MIDI_CLOCK_STOP		= NRPVH_SRC_MIDISTOP,	// stop message
	GATE_MIDI_CLOCK_STARTSTOP	= NRPVH_SRC_MIDISTARTSTOP,	// start or stop message
	
	// respond to events from a note stack
	GATE_NOTE_EVENT_BASE		= 128,
	GATE_NOTE_ON,								// when any note on is received
	GATE_NOTES_OFF,								// when no notes are pressed		
	GATE_NOTE_GATEA,							// when note out A is playing
	GATE_NOTE_GATEB,							// when note out B is playing
	GATE_NOTE_GATEC,							// when note out C is playing
	GATE_NOTE_GATED								// when note out D is playing
	
};

//
// STRUCT DEFS 
//
typedef struct {
	byte counter;		
	byte value;
} GATE_OUT;

// Structure to hold mapping of a note stack event to a gate
typedef struct {
	byte mode;			// type of trigger - GATE_xxx enum
	byte flags;		
	byte duration;		// gate pulse duration in ms (or 0 for "as long as active")
	byte stack_id;	// index of the note stack
} T_GATE_EVENT;

// Structure to hold mapping of a raw MIDI note to a gate
typedef struct {
	byte mode;			// type of trigger - GATE_xxx enum
	byte flags;		
	byte duration;		// gate pulse duration in ms (or 0 for "as long as active")
	byte chan;			// midi channel
	byte note;			// note range: lowest note
	byte note_max;		// note range: highest note (0 if there is only one note)
	byte vel_min;		// minimum velocity that will trigger the gate
} T_GATE_MIDI_NOTE;

// Structure to hold mapping of a raw MIDI CC to a gate
typedef struct {
	byte mode;			// type of trigger - GATE_xxx enum
	byte flags;		
	byte duration;		// gate pulse duration in ms (or 0 for "as long as active")
	byte chan;			// midi channel
	byte cc;			// CC number
	byte threshold;		// threshold for gate ON
} T_GATE_MIDI_CC;

// Structure to hold mapping of MIDI clock to a gate
typedef struct {
	byte mode;			// type of trigger - GATE_xxx enum
	byte flags;		
	byte duration;		// gate pulse duration in ms (or 0 for "as long as active")
	byte div;			// clock divider (@24ppqn)
	byte tick_ofs;			// initial clock count
} T_GATE_MIDI_CLOCK;

// The gate out structure which combines the above
typedef union {
	T_GATE_EVENT		event;
	T_GATE_MIDI_NOTE	note;
	T_GATE_MIDI_CC		cc;
	T_GATE_MIDI_CLOCK	clock;
} GATE_OUT_CFG;

//
// LOCAL DATA
//

// Whether MIDI clock is running
static byte midi_clock_running = 0;

// gate config
static GATE_OUT_CFG l_gate_cfg[GATE_MAX];

// gate status
static GATE_OUT l_gate[GATE_MAX];

//
// LOCAL FUNCTIONS
//

////////////////////////////////////////////////////////////
// TRIGGER OR UNTRIGGER A GATE
static void trigger(GATE_OUT *pgate, GATE_OUT_CFG *pcfg, byte which_gate, byte trigger_enabled, byte sync)
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

	
	if(trigger_enabled) { // trigger ON
	
		// if this gate is set to retrigger, then set
		// up the retrig mask to include the bit for 
		// this gate
		if(pcfg->event.flags & GATE_FLAG_RETRIG) {
			g_sr_retrigs |= gate_bit;
		}
		
		if(sync && g_cv_dac_pending) {
			// synchronised trigger - set trigger bits to be 
			// actioned after CV has been updated
			if(!(g_sync_sr_data & gate_bit)) {
				g_sync_sr_data |= gate_bit;
				g_sync_sr_data_pending = 1;	
			}
		}
		else 
		{
			// only need to refresh the gates if the bit has changed (or if
			// we are retriggering)
			if((pcfg->event.flags & GATE_FLAG_RETRIG) || !(g_sr_data & gate_bit)) {
				g_sr_data |= gate_bit;
				g_sr_data_pending = 1;	
			}
		}
		// set duration counter
		if(GATE_DUR_GLOBAL == pcfg->event.duration) {
			pgate->counter = g_global.gate_duration;
		}
		else {
			pgate->counter = pcfg->event.duration;
		}
		
		// compensate for immediate decrement 
		if(pgate->counter) {
			++pgate->counter;
		}
	}
	// trigger OFF - no worries about synchronisation
	else
	{
		g_sync_sr_data &= ~gate_bit; // cancel any deferred trigger
		if(g_sr_data & gate_bit) {
			g_sr_data &= ~gate_bit;			
			g_sr_data_pending = 1;	
		}
		pgate->counter = 0;
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
		GATE_OUT *pgate = &l_gate[which_gate];
		GATE_OUT_CFG *pcfg = &l_gate_cfg[which_gate];
		
		// check if this output is possibly watching the note stack
		// that sourced the event
		if(pcfg->event.stack_id != stack_id)
			continue;
			
		// check the mode of this gate against the event. only modes
		// which watch note stacks are matched
		switch(pcfg->event.mode) {
			case GATE_NOTE_ON: // Any note on/changed
				if(EV_NOTE_ON == event) {
					trigger(pgate, pcfg, which_gate, true, true);
				}
				else if(EV_NOTES_OFF == event) {
					trigger(pgate, pcfg, which_gate, false, false);
				}
				break;
			case GATE_NOTES_OFF: // All notes off
				if(EV_NOTES_OFF == event) {
					trigger(pgate, pcfg, which_gate, true, true);
				}
				else if(EV_NOTE_ON == event) {
					trigger(pgate, pcfg, which_gate, false, false);
				}
				break;
			case GATE_NOTE_GATEA: // Note present at output A
				if(event == EV_NOTE_A) {
					trigger(pgate, pcfg, which_gate, true, true);
				}
				else if(event == EV_NO_NOTE_A) {
					trigger(pgate, pcfg, which_gate, false, false);
				}
				break;
			case GATE_NOTE_GATEB: // Note present at output B
				if(event == EV_NOTE_B) {
					trigger(pgate, pcfg, which_gate, true, true);
				}
				else if(event == EV_NO_NOTE_B) {
					trigger(pgate, pcfg, which_gate, false, false);
				}
				break;
			case GATE_NOTE_GATEC: // Note present at output C
				if(event == EV_NOTE_C) {
					trigger(pgate, pcfg, which_gate, true, true);
				}
				else if(event == EV_NO_NOTE_C) {
					trigger(pgate, pcfg, which_gate, false, false);
				}
				break;
			case GATE_NOTE_GATED: // Note present at output D
				if(event == EV_NOTE_D) {
					trigger(pgate, pcfg, which_gate, true, true);
				}
				else if(event == EV_NO_NOTE_D) {
					trigger(pgate, pcfg, which_gate, false, false);
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
		GATE_OUT *pgate = &l_gate[which_gate];
		GATE_OUT_CFG *pcfg = &l_gate_cfg[which_gate];
		
		// does this gate respond to midi note?
		if(pcfg->event.mode != GATE_MIDI_NOTE)
			continue;			
		// does the MIDI channel match?
		if(!IS_CHAN(pcfg->note.chan, chan))
			continue;			
		// Does the note match?
		if(!IS_NOTE_MATCH(pcfg->note.note, pcfg->note.note_max, note))
			continue;			
		// is this a note off or note on with velocity above threshold?
		if(vel && vel < pcfg->note.vel_min) {			
			continue;
		}		
		
		// trigger (for note on) or untrigger (for note off)
		trigger(pgate, pcfg, which_gate, !!vel, false);
	}			
}

////////////////////////////////////////////////////////////
// HANDLE EVENT FROM RAW MIDI CC
void gate_midi_cc(byte chan, byte cc, byte value) 
{
	// for each gate output
	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {
		GATE_OUT *pgate = &l_gate[which_gate];
		GATE_OUT_CFG *pcfg = &l_gate_cfg[which_gate];
		
		// does this gate respond to CC?
		if(pcfg->event.mode != GATE_MIDI_CC && 
			pcfg->event.mode != GATE_MIDI_CC_NEG)
			continue;
		
		// is this the correct CC?	
		if(cc != pcfg->cc.cc) {
			continue;
		}		
		// does the MIDI channel match?
		if(!IS_CHAN(pcfg->cc.chan, chan))
			continue;

				
		// has the value just gone above threshold?
		if(value >= pcfg->cc.threshold &&
			( pgate->value < pcfg->cc.threshold || 
				pgate->value == NO_VALUE)) {
			
			// trigger gate
			trigger(pgate, pcfg, which_gate, !(pcfg->event.mode == GATE_MIDI_CC_NEG), false);
			pgate->value = value;		
		}
		// has the value just gone below threshold?
		else if(value < pcfg->cc.threshold &&
			( pgate->value >= pcfg->cc.threshold || 
				pgate->value == NO_VALUE)) {
			
			// untrigger gate
			trigger(pgate, pcfg, which_gate, (pcfg->event.mode == GATE_MIDI_CC_NEG), false);
			pgate->value = value;		
		}
	}			
}

////////////////////////////////////////////////////////////
// HANDLE EVENT FROM RAW MIDI CLOCK MESSAGE
void gate_midi_clock(byte msg) {
	byte which_gate;
	GATE_OUT *pgate;
	GATE_OUT_CFG *pcfg;
	switch(msg) {
	// CLOCK TICK
	case MIDI_SYNCH_TICK: 
		for(which_gate=0; which_gate<GATE_MAX; ++which_gate) {
			pcfg = &l_gate_cfg[which_gate];			
			//is this gate tied to clock ticks?
			if((GATE_MIDI_CLOCK_TICK == pcfg->event.mode) || (GATE_MIDI_CLOCK_RUN_TICK == pcfg->event.mode)) {
				// does it need the clock to be running?
				if((GATE_MIDI_CLOCK_RUN_TICK == pcfg->event.mode) && !midi_clock_running) {
					continue;
				}
				pgate = &l_gate[which_gate];			
				if(!pgate->value) {
					trigger(pgate, &l_gate_cfg[which_gate], which_gate, true, false);
				}
				if(++pgate->value >= pcfg->clock.div) {
					pgate->value = 0;
				}
			}
		}
		break;
	// CLOCK START AND CONTINUE
	case MIDI_SYNCH_START:		
	case MIDI_SYNCH_CONTINUE:
		midi_clock_running = 1;
		for(which_gate=0; which_gate<GATE_MAX; ++which_gate) {
			pcfg = &l_gate_cfg[which_gate];			
			pgate = &l_gate[which_gate];						
			switch(pcfg->event.mode) {	
			case GATE_MIDI_CLOCK_TICK:
			case GATE_MIDI_CLOCK_RUN_TICK:
				if(msg == MIDI_SYNCH_START) {
					pgate->value = pcfg->clock.tick_ofs;
				}
				break;
			case GATE_MIDI_CLOCK_START:
				if(msg != MIDI_SYNCH_START) {
					break;
				}// else fall through				
			case GATE_MIDI_CLOCK_STARTSTOP:
			case GATE_MIDI_CLOCK_RUN:
			//case GATE_MIDI_CLOCK_STARTCONT:
				trigger(pgate, pcfg, which_gate, true, false);
				break;
			case GATE_MIDI_CLOCK_STOP:
				trigger(pgate, pcfg, which_gate, false, false);
				break;
			}
		}
		break;
	// CLOCK STOP
	case MIDI_SYNCH_STOP:
		midi_clock_running = 0;
		for(which_gate=0; which_gate<GATE_MAX; ++which_gate) {
			pcfg = &l_gate_cfg[which_gate];			
			pgate = &l_gate[which_gate];			
			switch(pcfg->event.mode) {				
			case GATE_MIDI_CLOCK_RUN:
				trigger(pgate, pcfg, which_gate, false, false);
				break;
			case GATE_MIDI_CLOCK_STOP:
			case GATE_MIDI_CLOCK_STARTSTOP:
				trigger(pgate, pcfg, which_gate, true, false);
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
		GATE_OUT *pgate = &l_gate[which_gate];
		if(pgate->counter) {
			if(!--pgate->counter) {
				trigger(pgate, &l_gate_cfg[which_gate], which_gate, false, false);
			}
		}
	}
}

////////////////////////////////////////////////////////////
// TRIGGER A GATE DIRECTLY (testing)
void gate_trigger(byte which_gate, byte trigger_enabled)
{
	if(which_gate < GATE_MAX) {
		trigger(&l_gate[which_gate], &l_gate_cfg[which_gate], which_gate, trigger_enabled, false);
	}
}

////////////////////////////////////////////////////////////
// SET DEFAULT GATE STATE
void gate_reset() {
	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {
		GATE_OUT_CFG *pcfg = &l_gate_cfg[which_gate];
		GATE_OUT *pgate = &l_gate[which_gate];
		pgate->counter = 0;
		switch(pcfg->event.mode) {
			case GATE_MIDI_CLOCK_TICK:
			case GATE_MIDI_CLOCK_RUN_TICK:
				pgate->value = pcfg->clock.tick_ofs;
				break;
			default:
				pgate->value = NO_VALUE;
				break;
		}
		trigger(pgate, pcfg, which_gate, false, false);
	}
}
	
////////////////////////////////////////////////////////////
// SET DEFAULT GATE CONFIG
void gate_init() {

	// initialise state info
	for(byte which_gate=0; which_gate<GATE_MAX; ++which_gate) {
		GATE_OUT_CFG *pcfg = &l_gate_cfg[which_gate];
		GATE_OUT *pgate = &l_gate[which_gate];
		pcfg->event.mode = GATE_DISABLE;
		pcfg->event.flags = 0;
		pcfg->event.duration = DEFAULT_GATE_DURATION;
	}	
	gate_reset();
}


////////////////////////////////////////////////////////////
// CONFIGURE A GATE OUTPUT
// return nonzero if any change was made
byte gate_nrpn(byte which_gate, byte param_lo, byte value_hi, byte value_lo) {	
	if(which_gate >= GATE_MAX)
		return 0;		
	GATE_OUT_CFG *pcfg = &l_gate_cfg[which_gate];
	GATE_OUT *pgate = &l_gate[which_gate];
	
	// Check the target register
	switch(param_lo) {	
	
	////////////////////////////////////////////////////////////////
	// SELECT GATE SOURCE AND INITIALISE GATE
	case NRPNL_SRC:	
	
		// High byte of value is the gate event source
		switch(value_hi) {		

		// NO SOURCE - DISABLE
		case NRPVH_SRC_DISABLE:	
			pcfg->event.mode = GATE_DISABLE;
			return 1;

		// NOTE STACK SOURCE
		case NRPVH_SRC_STACK1: 
		case NRPVH_SRC_STACK2:
		case NRPVH_SRC_STACK3:
		case NRPVH_SRC_STACK4:
			pcfg->event.stack_id = value_hi - NRPVH_SRC_STACK1;		
			pcfg->event.duration = GATE_DUR_GLOBAL;
			pcfg->event.flags = 0;
			// NOTE STACK EVENT
			switch(value_lo) {
			case NRPVL_SRC_NOTE1:// NOTE GATES
			case NRPVL_SRC_NOTE2:
			case NRPVL_SRC_NOTE3:
			case NRPVL_SRC_NOTE4:
				pcfg->event.mode = GATE_NOTE_GATEA + (value_lo - NRPVL_SRC_NOTE1);
				return 1;
			case NRPVL_SRC_NO_NOTES: // ALL NOTES OFF
				pcfg->event.mode = GATE_NOTES_OFF;
				return 1;
			case NRPVL_SRC_ANY_NOTES: // ANY NOTE ON
				pcfg->event.mode = GATE_NOTE_ON;
				return 1;
			default: 
				break;
			}
			break;
			
		// MIDI NOTE SOURCE
		case NRPVH_SRC_MIDINOTE:
			pcfg->note.mode = GATE_MIDI_NOTE;
			pcfg->note.chan = CHAN_GLOBAL;
			pcfg->note.note = value_lo;
			pcfg->note.note_max = 0;
			pcfg->note.vel_min = 0;
			return 1;

		// MIDI CC SOURCE 
		case NRPVH_SRC_MIDICC:
		case NRPVH_SRC_MIDICC_NEG:
			pcfg->cc.mode = value_hi;
			pcfg->cc.chan = CHAN_GLOBAL;
			pcfg->cc.cc = value_lo;
			pcfg->cc.threshold = DEFAULT_GATE_CC_THRESHOLD;
			return 1;

		// MIDI CLOCK SOURCE
		case NRPVH_SRC_MIDITICK:
		case NRPVH_SRC_MIDITICKRUN:
		case NRPVH_SRC_MIDIRUN:
		case NRPVH_SRC_MIDISTART:
		//case NRPVH_SRC_MIDICONT:
		case NRPVH_SRC_MIDISTOP:
		case NRPVH_SRC_MIDISTARTSTOP:
			pcfg->clock.mode = value_hi; // relies on alignment of values!
			pcfg->clock.tick_ofs = 0;
			if(value_lo) {
				pcfg->clock.div = value_lo;
			}
			else {
				pcfg->clock.div = DEFAULT_GATE_DIV;
			}
			return 1;
		}
		break;

	////////////////////////////////////////////////////////////////
	// SELECT MIDI CHANNEL
	case NRPNL_CHAN:
		switch(value_hi) {
		case NRPVH_CHAN_SPECIFIC:
			if(value_lo >= 1 && value_lo <= 16) {
				pcfg->note.chan = value_lo - 1; // relies on alignment of chan member in cc too
				return 1;
			}
			break;
		case NRPVH_CHAN_OMNI:
			pcfg->note.chan = CHAN_OMNI;
			return 1;
		case NRPVH_CHAN_GLOBAL:
			pcfg->note.chan = CHAN_GLOBAL;
			return 1;
		}
		break;
		
	////////////////////////////////////////////////////////////////
	// SELECT MIDI NOTE 
	case NRPNL_NOTE_MIN:
		pcfg->note.note = value_lo;
		pcfg->note.note_max = 0;
		return 1;

	////////////////////////////////////////////////////////////////
	// SELECT MIDI NOTE RANGE
	case NRPNL_NOTE_MAX:
		pcfg->note.note_max = value_lo;
		return 1;

	////////////////////////////////////////////////////////////////
	// SELECT MIDI VELOCITY THRESHOLD
	case NRPNL_VEL_MIN:
		pcfg->note.vel_min = value_lo;
		return 1;

	////////////////////////////////////////////////////////////////
	// SELECT MIDI CC SWITCHING THRESHOLD
	case NRPNL_THRESHOLD:
		pcfg->cc.threshold = value_lo;
		return 1;
			
	////////////////////////////////////////////////////////////////
	// SELECT GATE DURATION
	case NRPNL_GATE_DUR:
		switch(value_hi) {
		case NRPVH_DUR_MS:
			pcfg->event.duration = value_lo;
			return 1;
		case NRPVH_DUR_INF:
			pcfg->event.duration = GATE_DUR_INFINITE;
			return 1;
		case NRPVH_DUR_GLOBAL:
			pcfg->event.duration = GATE_DUR_GLOBAL;
			return 1;
		case NRPVH_DUR_RETRIG:
			pcfg->event.duration = GATE_DUR_INFINITE;
			pcfg->event.flags |= GATE_FLAG_RETRIG;
			return 1;
		}
		break;
		
	////////////////////////////////////////////////////////////////
	// SELECT CLOCK COUNT INITIAL VALUE
	case NRPNL_TICK_OFS:
		pcfg->clock.tick_ofs = value_lo;
		return 1;
	}	
	return 0;
}		


////////////////////////////////////////////////////////////
// GET PATCH STORAGE INFO
byte *gate_storage(int *len) {
	*len = sizeof(l_gate_cfg);
	return (byte*)&l_gate_cfg;
}

//
// END
//