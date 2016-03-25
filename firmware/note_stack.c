////////////////////////////////////////////////////////////
//
// MINI MIDI CV
//
// NOTE STACK
//
////////////////////////////////////////////////////////////

//
// INCLUDES
//
#include <system.h>
#include "cv_strip.h"

//
// CONSTANTS
//
enum {
	PRIORITY_NEW,		// gives priority to newest note
	PRIORITY_LOW,		// gives priority to lowest note
	PRIORITY_HIGH,		// gives priority to highest note
	PRIORITY_OLD,		// gives priority to oldest note
	PRIORITY_RANDOM		// randomly prioritises notes	
};

#define SZ_NOTE_STACK 12	// max notes in a single stack
#define NUM_NOTE_STACKS 4	// number of stacks supported

//
// STRUCT DEFS
//
typedef struct {
	byte priority;		// how notes are prioritised when assigned to outputs
	byte chan;			// midi channel
	byte note_min;		// lowest note
	byte note_max;		// highest note
	byte vel_min;		// minimum velocity threshold
	byte vel_accent;	// accent velocity level
	byte bend_range;	// pitch bend range (+/- semitones)
		
	// STATE
	byte note[SZ_NOTE_STACK];	// the notes held in the stack
	char count;					// number of held notes
	byte out[4];				// the stack output notes
	int bend;					// pitch bend
	byte vel;					// note velocity	
} NOTE_STACK;

//
// PRIVATE DATA
//

// the note stacks
static NOTE_STACK g_note_stack[NUM_NOTE_STACKS];

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// ADD A NOTE TO A STACK
static byte add_note(NOTE_STACK *pstack, byte note) {
	byte pos,i;
	byte note_added = 1;

	// if the note is already at top of stack
	// there is nothing to do
	if(pstack->count && note == pstack->note[0]) {
		return 0;
	}
	
	// check if note is already in the stack
	for(pos = 0; pos < pstack->count-1; ++pos) {
		if(pstack->note[pos] == note) { 
			for(i = pos; i > 0; --i) {
				pstack->note[i] = pstack->note[i-1];
			}
			pstack->note[0] = note;
			return;
		}
	}
	
	// note is not in stack so it will be added
	if(pstack->count < SZ_NOTE_STACK) {
		++pstack->count;
	}	
	for(i = pstack->count-1; i > 0; --i) {
		pstack->note[i] = pstack->note[i-1];
	}
	pstack->note[0] = note;	
	return 1;
}

///////////////////////////////////////////////////////////////
// REMOVE A NOTE FROM A STACK
static byte remove_note(NOTE_STACK *pstack, byte note) 
{
	byte pos,i;
	byte found = 0;
	
	// search for the note
	for(pos = 0; pos < pstack->count-1; ++pos) {
		if(pstack->note[pos] == note) { 
			found = 1;
			break;
		}
	}
	if(found) {	
		// remove note
		--pstack->count;
		for(i = pos; i < pstack->count-1; --i) {
			pstack->note[i] = pstack->note[i+1];
		}
	}
	return found;
}

///////////////////////////////////////////////////////////////
// RECALCULATE NOTE STACK OUTPUTS
static byte recalc_outputs(NOTE_STACK *pstack, byte stack_id)
{
	byte i,j,len,flag,pos;
	byte new_out[4];
	byte result = 0;
	new_out[0] = NO_NOTE_OUT;
	new_out[1] = NO_NOTE_OUT;
	new_out[2] = NO_NOTE_OUT;
	new_out[3] = NO_NOTE_OUT;

	// we need at least two notes for prioritisation to apply	
	if(pstack->count > 1) {
	
		// get minimum of held note count and output count
		len = pstack->count;
		if(len > NUM_NOTE_STACK_OUTS) {
			len = NUM_NOTE_STACK_OUTS
		}
		
		switch(pstack->priority) {
		
		//////////////////////////////////////////////////////////
		// NEW NOTE PRIORITY - just copy out front notes
		case PRIORITY_NEW;
			for(i=0; i<len; ++i) {
				new_out[i] = pstack->note[i];
			}
			break;

		//////////////////////////////////////////////////////////
		// OLD NOTE PRIORITY - copy out back notes
		case PRIORITY_OLD:
			for(i=0; i<len; ++i) {
				new_out[i] = pstack->note[pstack->count - 1 - i];
			}
			break;
		
		//////////////////////////////////////////////////////////
		// HIGHEST NOTE PRIORITY - copy out the highest held notes
		// in order from highest to lowest
		//////////////////////////////////////////////////////////
		case PRIORITY_HIGH:
			new_out[0] = pstack->note[0];
			for(i=1; i<pstack->count; ++i) {				
				if(pstack->note[i] > new_out[0]) {
					new_out[3] = new_out[2];
					new_out[2] = new_out[1];
					new_out[1] = new_out[0];
					new_out[0] = pstack->note[i];					
				}
				else if(new_out[1] == NO_NOTE_OUT || pstack->note[i] > new_out[1]) {
					new_out[3] = new_out[2];
					new_out[2] = new_out[1];
					new_out[1] = pstack->note[i];
				}
				else if(new_out[2] == NO_NOTE_OUT || pstack->note[i] > new_out[2]) {
					new_out[3] = new_out[2];
					new_out[2] = pstack->note[i];
				}
				else if(new_out[3] == NO_NOTE_OUT || pstack->note[i] > new_out[3]) {
					new_out[3] = pstack->note[i];
				}		
			}
			break;
		
		//////////////////////////////////////////////////////////
		// LOWEST NOTE PRIORITY - copy out the highest held notes
		// in order from highest to lowest
		//////////////////////////////////////////////////////////
		case PRIORITY_LOW:
			new_out[0] = pstack->note[0];
			for(i=1; i<pstack->count; ++i) {				
				if(pstack->note[i] < new_out[0]) {
					new_out[3] = new_out[2];
					new_out[2] = new_out[1];
					new_out[1] = new_out[0];
					new_out[0] = pstack->note[i];					
				}
				else if(new_out[1] == NO_NOTE_OUT || pstack->note[i] < new_out[1]) {
					new_out[3] = new_out[2];
					new_out[2] = new_out[1];
					new_out[1] = pstack->note[i];
				}
				else if(new_out[2] == NO_NOTE_OUT || pstack->note[i] < new_out[2]) {
					new_out[3] = new_out[2];
					new_out[2] = pstack->note[i];
				}
				else if(new_out[3] == NO_NOTE_OUT || pstack->note[i] < new_out[3]) {
					new_out[3] = pstack->note[i];
				}		
			}
			break;
			
		//////////////////////////////////////////////////////////
		// RANDOMISED PRIORITY - use random notes from list of held
		// notes, ensuring no note used no more that once
		case PRIORITY_RANDOM:
			for(i=0; i<len; ++i) {
				flag = 1;
				while(flag) { // looking for a note position we've not used
					pos = random(pstack->count)
					flag = 0;
					for(j=0; j<i; ++j) {
						if(new_out[j] == pos) { // already got this one
							flag = 1; 
							break;
						}
					}
				}
			}
			// replace the note positions with the note values
			for(i=0; i<NUM_NOTE_STACK_OUTS; ++i) {
				new_out[i] = pstack->note[new_out[i]];
			}			
			break;
		}
	}
	else if(pstack->count == 1) {
		// there is only one note..
		new_out[0] = pstack->note[0];
	}

	// Update the note stack output
	for(i=0; i<4; ++i) {
		if(new_out[i] != pstack->out[i]) {
			result = 1;
			pstack->out[i] = new_out[i];
			if(new_out[i] == NO_NOTE_OUT) {
				gate_event(EV_NO_NOTE_A + i, stack_id);
			}
			else {
				cv_event(EV_NOTE_A + i, stack_id);
				gate_event(EV_NOTE_A + i, stack_id);
			}
		}
	}
	
	return result;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// HANDLE A MIDI NOTE
void stack_midi_note(byte chan, byte note, byte vel) 
{
	char i;
	
	// iterate over the note stacks
	for(i=0; i<NUM_NOTE_STACKS; ++i) {
		NOTE_STACK pstack = &g_note_stack[i];		
		
		// Check the channel
		switch(pstack->chan) {
			case CHAN_OMNI:	
				break;
			case CHAN_GLOBAL:
				if(g_chan != CHAN_OMNI && chan != g_chan) {
					continue;
				}
				break;
			default:
				if(chan != pstack->chan) {
					continue;
				}
				break;
		}
		
		// Check note range
		if(pstack->note_min && note < pstack->note_min) {
			continue;
		}
		if(pstack->note_max && note > pstack->note_max) {
			continue;
		}

		// velocity filter (note on only)
		if(vel && pstack->vel_min && vel < pstack->vel_min) {
			continue;
		}
		
		if(vel) {			
			// NOTE ON
			// add to note stack and if there is any change
			// to the stack, recalculate the outputs
			pstack->vel = vel;
			if(add_note(pstack, note)) {
				recalc_outputs(pstack, i);
			}
			cv_event(EV_NOTE_ON, i);
			gate_event(EV_NOTE_ON, i);
		}
		else {
			// NOTE OFF
			if(remove_note(pstack, note)) {
				recalc_outputs(pstack, i);
				if(!pstack->count) {
					// all notes are off
					gate_event(EV_NOTES_OFF, i);
				}
			}
		}		
	}
}

////////////////////////////////////////////////////////////
// HANDLE MIDI PITCH BEND
void stack_bend(byte chan, byte hi, byte lo) 
{	
	char i;
	int bend = (int)hi<<7|(lo&0x7F)-8192;	
	for(i=0; i<NUM_NOTE_STACKS; ++i) {
		NOTE_STACK pstack = &g_note_stack[i];		
		
		// Check the channel
		switch(pstack->chan) {
			case CHAN_OMNI:	
				break;
			case CHAN_GLOBAL:
				if(g_chan != CHAN_OMNI && chan != g_chan) {
					continue;
				}
				break;
			default:
				if(chan != pstack->chan) {
					continue;
				}
				break;
		}		
		pstack->bend = ((int)((pstack->bend_range * (bend - 8192.0)/8192.0))) << 8;		
	}
}

////////////////////////////////////////////////////////////
// INITIALISE NOTE STACKS
void stack_init()
{
}