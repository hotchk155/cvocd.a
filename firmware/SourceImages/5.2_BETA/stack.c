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
#include <system.h>
#include <memory.h>
#include "cvocd.h"

#define PARA_CHORD	1

//
// GLOBAL DATA
//

// the note stacks
NOTE_STACK g_stack[NUM_NOTE_STACKS] = {0};
NOTE_STACK_CFG g_stack_cfg[NUM_NOTE_STACKS];

//
// PRIVATE FUNCTIONS
//

static void update_held_notes(NOTE_STACK *pstack, byte note, byte vel, byte priority) {
	int i,pos;
	// If this is a note on message, the note needs to be added into the buffer
	if (vel) { 
	
		// determine the insertion point for the new note based on the 
		// note prioritisation order
		for (pos = 0; pos < pstack->count; ++pos) {
			if ((note > pstack->note[pos] && priority == PRIORITY_HIGH) ||
				(note < pstack->note[pos] && priority == PRIORITY_LOW) || 
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
			pstack->note[pos] = note;
		}
	}
	else { // note off - remove from the buffer
	
		// search for the note
		for(i = 0; i < pstack->count; ++i) {
			if(pstack->note[i] == note) { 
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
// MONOPHONIC MODE
static void prioritize_note(NOTE_STACK *pstack, byte which_stack, byte priority, byte note, byte vel)
{
	update_held_notes(pstack, note, vel, priority);
	byte prev_out = pstack->out[0];
	if(!pstack->count) { // no notes held
		if(prev_out != NO_NOTE_OUT) { // a note was playing
			pstack->out[0] = NO_NOTE_OUT; // not any more!
			gate_event(EV_NO_NOTE_A, which_stack);
			gate_event(EV_NOTES_OFF, which_stack);
		}
	}
	else if(prev_out != pstack->note[0]) { 		// change in note to play?
		pstack->out[0] = pstack->note[0]; 
		cv_event(EV_NOTE_A, which_stack); 		// update CV out
		gate_event(EV_NOTE_A, which_stack); 	// event for change of top note
		if(prev_out == NO_NOTE_OUT) { 
			gate_event(EV_NOTE_ON, which_stack); // event for first note
		}		
	}
}

///////////////////////////////////////////////////////////////
// HANDLE NOTE CYCLING
static void cycle_note(NOTE_STACK *pstack, byte which_stack, byte cycle_size, byte note, byte vel) 
{
	byte i, any_note;
	if(vel) {
		pstack->out[pstack->index] = note;
		cv_event(EV_NOTE_A + pstack->index, which_stack);
		gate_event(EV_NOTE_A + pstack->index, which_stack);
		gate_event(EV_NOTE_ON, which_stack);
		if(++pstack->index >= cycle_size ) {
			pstack->index = 0;
		}
	}
	else {
		any_note = 0;
		for(i=0; i<4; ++i) {		
			if(pstack->out[i] == note) {
				pstack->out[i] = NO_NOTE_OUT;
				gate_event(EV_NO_NOTE_A + i, which_stack);
			}			
			else if(pstack->out[i] != NO_NOTE_OUT) {
				any_note = 1;
			}
		}
		if(!any_note) {
			gate_event(EV_NOTES_OFF, which_stack);			
		}
	}	
}


///////////////////////////////////////////////////////////////
// POLYPHONIC 
static void poly_chord_note(NOTE_STACK *pstack, byte which_stack, byte chord_size, byte note, byte vel) 
{
	byte i, any_note;
	if(vel) {	
	
		// note on... look for a space for it...
		for(i=0; i<chord_size; ++i) {		
			if(pstack->out[i] == NO_NOTE_OUT)  // free slot
				break;
		}
		if(i == chord_size) {			
			i = chord_size - 1; // no free slots, steal the last slot
		}
		pstack->out[i] = note;
		cv_event(EV_NOTE_A + i, which_stack);
		gate_event(EV_NOTE_A + i, which_stack);
		gate_event(EV_NOTE_ON, which_stack);
	}
	else {
		// note off - remove old note
		any_note = 0;
		for(i=0; i<4; ++i) {		
			if(pstack->out[i] == note) {
				pstack->out[i] = NO_NOTE_OUT; // but do not update CV
				gate_event(EV_NO_NOTE_A + i, which_stack);
			}		
			else if(pstack->out[i] != NO_NOTE_OUT) {
				any_note = 1;
			}
		}
		if(!any_note) {
			gate_event(EV_NOTES_OFF, which_stack);			
		}
	}
}	

///////////////////////////////////////////////////////////////
// PARAPHONIC
static void para_chord_note(NOTE_STACK *pstack, byte which_stack, byte chord_size, byte note, byte vel) 
{
	update_held_notes(pstack, note, vel, PRIORITY_LOW);
	if(vel) { 
		// 0 0 0 0
		// 0 1 0 1
		// 0 1 2 0
		// 0 1 2 3
		byte from_index = 0;
		for(int i=0; i<chord_size; ++i) {		
			if(pstack->note[from_index] != pstack->out[i]) {
				pstack->out[i] = pstack->note[from_index];
				cv_event(EV_NOTE_A+i, which_stack);
			}
			if(++from_index >= pstack->count) {
				from_index = 0;
			}
		}
		if(pstack->count <= chord_size) {
			gate_event(EV_NOTE_ON, which_stack);		// event if any audible note changed
		}
		if(pstack->count == 1) {
			gate_event(EV_NOTE_A, which_stack);	// event when first note goes on
		}
	}
	else {
		if(!pstack->count) {
			gate_event(EV_NO_NOTE_A, which_stack);	// events when all notes go off
			gate_event(EV_NOTES_OFF, which_stack);			
		}
	}
}	




/* 
///////////////////////////////////////////////////////////////
// HANDLE CHORDS
static void poly_chord_note(NOTE_STACK *pstack, byte which_stack, byte chord_size, byte note, byte vel) 
{
	byte i, any_note;
	if(vel) {	
	
		// note on... look for a space for it...
		for(i=0; i<chord_size; ++i) {		
			if(pstack->out[i] == NO_NOTE_OUT)  // free slot
				break;
		}
		if(i == chord_size) {			
			i = chord_size - 1; // no free slots, steal the last slot
		}
		pstack->out[i] = note;
		cv_event(EV_NOTE_A + i, which_stack);
		gate_event(EV_NOTE_A + i, which_stack);
		gate_event(EV_NOTE_ON, which_stack);
	}
	else {
		// note off - remove old note
		any_note = 0;
		for(i=0; i<4; ++i) {		
			if(pstack->out[i] == note) {
				pstack->out[i] = NO_NOTE_OUT;
				gate_event(EV_NO_NOTE_A + i, which_stack);
			}		
			else if(pstack->out[i] != NO_NOTE_OUT) {
				any_note = 1;
			}
		}
		if(!any_note) {
			gate_event(EV_NOTES_OFF, which_stack);			
		}
	}
}	
*/


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
		
		if(vel) {
			// for a note on message, velocity must be abve threshold
			if(pcfg->vel_min && vel < pcfg->vel_min) {
				continue;			
			}
			// store note velocity as stack velocity
			pstack->vel = vel;
		}

		// pass the note to the appropriate handler
		switch(pcfg->priority) {		
			case PRIORITY_LAST:
			case PRIORITY_LOW:
			case PRIORITY_HIGH:
				prioritize_note(pstack, which_stack, pcfg->priority, note, vel);
				break;
			case PRIORITY_CYCLE2:
			case PRIORITY_CYCLE3:
			case PRIORITY_CYCLE4:
				cycle_note(pstack, which_stack, (2 + pcfg->priority - PRIORITY_CYCLE2), note, vel);
				break;	
			case PRIORITY_CHORD2:
			case PRIORITY_CHORD3:
			case PRIORITY_CHORD4:
#ifdef PARA_CHORD			
				para_chord_note(pstack, which_stack, (2 + pcfg->priority - PRIORITY_CHORD2), note, vel);
#else
				poly_chord_note(pstack, which_stack, (2 + pcfg->priority - PRIORITY_CHORD2), note, vel);
#endif
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
		int new_bend = ((long)pcfg->bend_range * (bend - 8192))/32;
		if(pstack->bend != new_bend) {
			pstack->bend = new_bend;
			cv_event(EV_BEND, i);
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
	for(byte i=0; i<NUM_NOTE_STACKS; ++i) {
		g_stack[i].count = 0;
		g_stack[i].out[0] = NO_NOTE_OUT;
		g_stack[i].out[1] = NO_NOTE_OUT;
		g_stack[i].out[2] = NO_NOTE_OUT;
		g_stack[i].out[3] = NO_NOTE_OUT;
		g_stack[i].bend = 0;
		g_stack[i].vel = 0;		
		g_stack[i].index = 0;		
		gate_event(EV_NOTES_OFF, i);
	}
}
 
////////////////////////////////////////////////////////////
// INITIALISE NOTE STACK CONFIG
void stack_init()
{
	memset(g_stack_cfg, 0, sizeof(g_stack_cfg));
}

//
// END
//