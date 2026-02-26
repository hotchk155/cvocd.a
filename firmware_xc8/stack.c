//////////////////////////////////////////////////////////////
//
//       ///// //   //          /////    /////  /////
//     //     //   //         //    // //      //   //
//    //      // //    //    //    // //      //   //
//   //      // //   ////   //    // //      //   //
//   /////   ///     //     //////   /////  //////
//
// CV.OCD MIDI-TO-CV CONVERTER
// hotchk155/2019
// Sixty Four Pixels Limited
//
// This work is distibuted under terms of Creative Commons 
// License BY-NC-SA (Attribution, Non-commercial, Share-Alike)
// https://creativecommons.org/licenses/by-nc-sa/4.0/
//
//////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////
//
// NOTE STACK MODULE
//
//////////////////////////////////////////////////////////////

//
// INCLUDES
//
#include <xc.h>
#include <string.h>
#include "cvocd.h"

//
// DEFS
//

typedef unsigned int NOTE_VEL;

// note stack config
typedef struct {
	byte chan;			// midi channel
	byte note_min;		// lowest note
	byte note_max;		// highest note
	byte vel_min;		// minimum velocity threshold
	byte bend_range;	// pitch bend range (+/- semitones)
	byte priority;		// how notes are prioritised when assigned to outputs
} NOTE_STACK_CFG;

// note stack state
typedef struct {
	NOTE_VEL note[SZ_NOTE_STACK];	// the notes held in the stack
	char count;						// number of held notes
	NOTE_VEL out[4];				// the stack output notes (velocity in top 8 bits)
	int bend;						// pitch bend
//	byte vel;						// note velocity	
	byte index;						// index for note cycling
} NOTE_STACK;

// assign_notes() flags
const byte ASSIGN_ALL_OUTPUTS	= 0x01;	// every output must be assigned valid note
const byte REBUILD_ON_RELEASE	= 0x02;	// reassign outputs when a note is released
const byte MUTE_ALL_ON_RELEASE	= 0x04;	// keep assignments but mute all outputs when note released
const byte TB_PITCH_GLIDE		= 0x08;	// TB style glide on tied notes

//
// GLOBAL DATA
//

// the note stacks
NOTE_STACK g_stack[NUM_NOTE_STACKS] = {0};
NOTE_STACK_CFG g_stack_cfg[NUM_NOTE_STACKS];

//
// PRIVATE FUNCTIONS
//

///////////////////////////////////////////////////////////////
// MAINTAIN SORTED LIST OF ACTIVE MIDI NOTES
static void update_held_notes(NOTE_STACK *pstack, byte note, byte vel, byte priority) {
	int i,pos;
	// If this is a note on message, the note needs to be added into the buffer
	if (vel) { 
	
		// determine the insertion point for the new note based on the 
		// note prioritisation order
		for (pos = 0; pos < pstack->count; ++pos) {
			if ((note > (byte)pstack->note[pos] && priority == PRIORITY_HIGH) ||
				(note < (byte)pstack->note[pos] && priority == PRIORITY_LOW) || 
				priority == PRIORITY_LAST) {
				break;
			}
		}

		// increase count of notes in the buffer if there is space
		if (pstack->count < SZ_NOTE_STACK) {
			++pstack->count;
		}

		// can the new note be inserted in the buffer? (lower priority notes 
		// will be shifted along if there is space, otherwise the lowest
		// note will drop of the buffer)
		if (pos < pstack->count) {
		
			// shift down along which are after the insertion point
			for (i = pstack->count - 2; i >= pos; --i) {
				pstack->note[i + 1] = pstack->note[i];
			}
			// insert the new note in the buffer
			pstack->note[pos] = ((NOTE_VEL)vel)<<8|note;
		}
	}
	else { // note off - remove from the buffer
	
		// search for the note
		for(i = 0; i < pstack->count; ++i) {
			if((byte)pstack->note[i] == note) { 
				// remove the note by shufflng all later notes down
				--pstack->count;
				for(; i<pstack->count; ++i) {
					pstack->note[i] = pstack->note[i+1];
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////
// ASSIGN NOTES TO OUTPUTS
static void assign_notes(NOTE_STACK *pstack, byte which_stack, byte chord_size, byte priority, byte flags, byte note, byte vel) {
	
	byte out_idx;
	
	// add/remove notes from sorted note list
	update_held_notes(pstack, note, vel, priority);
	
	// assign new outputs
	byte outputs_to_assign = chord_size;
	if(pstack->count && (vel || (flags & REBUILD_ON_RELEASE))) {
		outputs_to_assign = pstack->count; 
		if((flags & ASSIGN_ALL_OUTPUTS) || (outputs_to_assign > chord_size)) {
			outputs_to_assign = chord_size; // number of outputs to assign always 1..chord_size
		}
		for(out_idx=0; out_idx<outputs_to_assign; ++out_idx) { 
			NOTE_VEL note_to_assign = pstack->note[out_idx%pstack->count]; // source note for output
			if((byte)pstack->out[out_idx] != (byte)note_to_assign) { // change of note?
				pstack->out[out_idx] = note_to_assign;
				if((flags & TB_PITCH_GLIDE) && (pstack->count>1))  {
					cv_note_event(EV_NOTE_A + out_idx, which_stack, (byte)pstack->out[out_idx], pstack->out[out_idx]>>8, pstack->bend, EVF_TB_GLIDE);
					gate_note_event(EV_NOTE_A + out_idx, which_stack, EVF_TB_GLIDE); 
				}
				else {
					cv_note_event(EV_NOTE_A + out_idx, which_stack, (byte)pstack->out[out_idx], pstack->out[out_idx]>>8, pstack->bend, 0);
					gate_note_event(EV_NOTE_A + out_idx, which_stack, 0); 
				}
			}
			else if(vel && (flags & ASSIGN_ALL_OUTPUTS)) {
				gate_note_event(EV_NOTE_A + out_idx, which_stack, 0); // trig note gate
			}
		}
		if(vel) {
			//pstack->vel = vel;	// store most recent note on velocity
			gate_note_event(EV_NOTE, which_stack, 0);
		}
	}
	
	// mute outputs	
	byte any_notes_muted = 0;
	for(out_idx = 0; out_idx<chord_size; ++out_idx) { 			
		if(	pstack->out[out_idx] && (out_idx >= outputs_to_assign || 
			(!vel && (((byte)pstack->out[out_idx] == note) || (flags & MUTE_ALL_ON_RELEASE))))) {
			gate_note_event(EV_NO_NOTE_A + out_idx, which_stack, 0);
			pstack->out[out_idx] = 0;
			any_notes_muted = 1;
		}
	}
	if(any_notes_muted && !pstack->count) {
		gate_note_event(EV_NO_NOTE, which_stack, 0); // signal final note off 
	}
}

///////////////////////////////////////////////////////////////
// HANDLE NOTE CYCLING
static void cycle_note(NOTE_STACK *pstack, byte which_stack, byte cycle_size, byte note, byte vel) 
{
	if(vel) {
		pstack->out[pstack->index] = ((NOTE_VEL)vel)<<8|note;
		cv_note_event(EV_NOTE_A + pstack->index, which_stack, note, vel, pstack->bend, 0);
		gate_note_event(EV_NOTE_A + pstack->index, which_stack, 0);
		gate_note_event(EV_NOTE, which_stack, 0);
		if(++pstack->index >= cycle_size ) {
			pstack->index = 0;
		}
	}
	else {
		byte any_notes = 0;
		for(byte i=0; i<4; ++i) {		
			if(pstack->out[i] == note) {
				any_notes |= pstack->out[i];
				pstack->out[i] = 0;
				gate_note_event(EV_NO_NOTE_A + i, which_stack, 0);
			}			
		}
		if(!any_notes) {
			gate_note_event(EV_NO_NOTE, which_stack, 0); 
		}
	}	
}

//
// GLOBAL FUNCTIONS
//

////////////////////////////////////////////////////////////
// HANDLE A MIDI NOTE
void stack_midi_note(byte chan, byte note, byte vel) 
{
	// for each note stack
	for(byte which_stack=0; which_stack<NUM_NOTE_STACKS; ++which_stack) {
		NOTE_STACK *pstack = &g_stack[which_stack];		
		NOTE_STACK_CFG *pcfg = &g_stack_cfg[which_stack];		

		// channel matches?
		if(!IS_CHAN(pcfg->chan, chan))
			continue;
		// note matches?
		if(!IS_NOTE_MATCH(pcfg->note_min, pcfg->note_max, note))
			continue;
		
		// for a note on message, velocity must be abve threshold
		if(vel && pcfg->vel_min && vel<pcfg->vel_min) {
			continue;			
		}

		// pass the note to the appropriate handler
		switch(pcfg->priority) {		
			case PRIORITY_LAST:
			case PRIORITY_LOW:
			case PRIORITY_HIGH:
				assign_notes(pstack, which_stack, 1, pcfg->priority, REBUILD_ON_RELEASE, note, vel);
				break;
			case PRIORITY_GLIDE:
				assign_notes(pstack, which_stack, 1, PRIORITY_LAST, REBUILD_ON_RELEASE|TB_PITCH_GLIDE, note, vel);
				break;
			case PRIORITY_CHORD2:
			case PRIORITY_CHORD3:
			case PRIORITY_CHORD4:
				assign_notes(pstack, which_stack, 4, PRIORITY_LOW, 0, note, vel);
				break;	
			case PRIORITY_PARA:
				assign_notes(pstack, which_stack, 4, PRIORITY_LOW, ASSIGN_ALL_OUTPUTS|REBUILD_ON_RELEASE, note, vel);
				break;
			case PRIORITY_PARA_RELEASE:
				assign_notes(pstack, which_stack, 4, PRIORITY_LOW, ASSIGN_ALL_OUTPUTS|MUTE_ALL_ON_RELEASE, note, vel);
				break;
			case PRIORITY_CYCLE2:
			case PRIORITY_CYCLE3:
			case PRIORITY_CYCLE4:
				cycle_note(pstack, which_stack, (2 + pcfg->priority - PRIORITY_CYCLE2), note, vel);
				break;	
		}
	}
}

////////////////////////////////////////////////////////////
// HANDLE MIDI PITCH BEND
// bend is the raw unscaled midi value
void stack_midi_bend(byte chan, int bend) 
{	
	char i;
	for(i=0; i<NUM_NOTE_STACKS; ++i) {
		NOTE_STACK_CFG *pcfg = &g_stack_cfg[i];		
		NOTE_STACK *pstack = &g_stack[i];		
		
		// does the MIDI channel match?
		if(!IS_CHAN(pcfg->chan, chan))
			continue;

		// pitch bend units are 256 * number of midi notes offset 
		// and can be positive or negative
		int new_bend = (int)((long)pcfg->bend_range * (bend - 8192))/32;
		if(pstack->bend != new_bend) {
			pstack->bend = new_bend;
			cv_note_event(EV_BEND, i, 0, 0, pstack->bend, 0);
		}
	}
}

////////////////////////////////////////////////////////////
// CONFIGURE NOTE STACK
byte stack_nrpn(byte which_stack, byte param_lo, byte value_hi, byte value_lo)
{
	if(which_stack >= NUM_NOTE_STACKS) 
		return 0;		
	NOTE_STACK_CFG *pcfg = &g_stack_cfg[which_stack];		
	
	// Check the config parm
	switch(param_lo) {
	
	//////////////////////////////////////////////////
	// SELECT MIDI CHANNEL
	case NRPNL_CHAN:
		switch(value_hi) {
		case NRPVH_CHAN_OMNI:
			pcfg->chan = CHAN_OMNI;
			return 1;
		case NRPVH_CHAN_GLOBAL:
			pcfg->chan = CHAN_GLOBAL;
			return 1;
		default:
		case NRPVH_CHAN_SPECIFIC:
			if(value_lo >= 1 && value_lo <= 16) {
				pcfg->chan = value_lo-1;
				return 1;
			}		
			break;
		}
		break;	

	//////////////////////////////////////////////////
	// SELECT MIDI NOTE RANGE
	case NRPNL_NOTE_MIN:
		pcfg->note_min = value_lo;
		return 1;
	case NRPNL_NOTE_MAX:
		pcfg->note_max = value_lo;
		return 1;	

	//////////////////////////////////////////////////
	// SELECT MIN VELOCITY THRESHOLD
	case NRPNL_VEL_MIN:
		pcfg->vel_min = value_lo;
		return 1;	

	//////////////////////////////////////////////////
	// SELECT PITCH BEND RANGE
	case NRPNL_PB_RANGE:
		pcfg->bend_range = value_lo;
		return 1;	

	//////////////////////////////////////////////////
	// SELECT NOTE PRIORITY
	case NRPNL_PRIORITY:
		if(value_lo<PRIORITY_MAX) {
			pcfg->priority = value_lo;
			return 1;
		}
		break;			
	}
		
	return 0;
}

////////////////////////////////////////////////////////////
// GET NOTE STACK CONFIG 
byte *stack_storage(int *len) {
	*len = sizeof(g_stack_cfg);
	return (byte*)&g_stack_cfg;
}


////////////////////////////////////////////////////////////
// RESET NOTE STACK STATE
void stack_reset() {
	memset(g_stack, 0, sizeof(g_stack));
	gate_note_event(EV_NO_NOTE, 0, 0); 
	gate_note_event(EV_NO_NOTE, 1, 0); 
	gate_note_event(EV_NO_NOTE, 2, 0); 
	gate_note_event(EV_NO_NOTE, 3, 0); 
}
 
////////////////////////////////////////////////////////////
// INITIALISE NOTE STACK CONFIG
void stack_init() {
	memset(g_stack_cfg, 0, sizeof(g_stack_cfg));
}

//
// END
//
