////////////////////////////////////////////////////////////
//
// MINI MIDI CV
//
// NOTE STACK
//
////////////////////////////////////////////////////////////

//TODO ACCENT
//TODO LEGATO

//
// INCLUDES
//
#include <system.h>
#include <rand.h>
#include "cv-strip.h"


//
// PRIVATE DATA
//

// the note stacks
NOTE_STACK g_stack[NUM_NOTE_STACKS] = {0};
NOTE_STACK_CFG g_stack_cfg[NUM_NOTE_STACKS];

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// ADD A NOTE TO A STACK
static byte add_note(NOTE_STACK *pstack, byte note) {
	char i;

	// if the note is already at top of stack
	// there is nothing to do
	if(pstack->count>0 && note == pstack->note[0]) {
		return 0;
	}
	
	// check if note is already in the stack
	for(i = 0; i < pstack->count; ++i) {
		if(pstack->note[i] == note) { // found it
			// shuffle all lower indexed notes up one place
			for(; i > 0; --i) {
				pstack->note[i] = pstack->note[i-1];
			}
			// and place this note at the front
			pstack->note[0] = note;
			return 1;
		}
	}

	// is the note stack full?
	if(pstack->count == SZ_NOTE_STACK) { 
		// ok, we're going to lose the oldest note
		for(i = SZ_NOTE_STACK-1; i > 0; --i) {
			pstack->note[i] = pstack->note[i-1];
		}
	}
	else {
		// otherwise make space for the new note
		for(i = pstack->count; i > 0; --i) {
			pstack->note[i] = pstack->note[i-1];
		}
		++pstack->count;
	}
	// add the new note
	pstack->note[0] = note;	
	return 1;
}

///////////////////////////////////////////////////////////////
// REMOVE A NOTE FROM A STACK
static byte remove_note(NOTE_STACK *pstack, byte note) 
{
	char i;
	
	// search for the note
	for(i = 0; i < pstack->count; ++i) {
		if(pstack->note[i] == note) { 
			// remove the note by shufflng all later notes down
			--pstack->count;
			for(; i<pstack->count; ++i) {
				pstack->note[i] = pstack->note[i+1];
			}
			return 1;
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////
// RECALCULATE NOTE STACK OUTPUTS
static byte recalc_outputs(NOTE_STACK *pstack, NOTE_STACK_CFG *pcfg, byte stack_id)
{
	byte i,j,len,flag,pos = 0;
	byte new_out[4];
	byte result = 0;
	new_out[0] = NO_NOTE_OUT;
	new_out[1] = NO_NOTE_OUT;
	new_out[2] = NO_NOTE_OUT;
	new_out[3] = NO_NOTE_OUT;

	if(pstack->count == 0) {
		// no held notes		
	}
	else if(pstack->count == 1) {
		// only one held note
		new_out[0] = pstack->note[0];
	}
	else { // multiple held notes - need to prioritise
			
		// handle according to prioritization setting
		switch(pcfg->priority) {
		
		//////////////////////////////////////////////////////////
		// OLD NOTE PRIORITY - copy out back notes
		case PRIORITY_OLD:
			for(i=0; i<4 && i<pstack->count; ++i) {
				new_out[i] = pstack->note[pstack->count - 1 - i];
			}
			break;
		
		//////////////////////////////////////////////////////////
		// HIGHEST NOTE PRIORITY - copy out the highest held notes
		// in order from highest to lowest
		//////////////////////////////////////////////////////////
		case PRIORITY_HIGH:
		case PRIORITY_HIGH_SPREAD:
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
		case PRIORITY_LOW_SPREAD:
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
			for(i=0; i<4 && i<pstack->count; ++i) {
				flag = 1;
				while(flag) { // looking for a note position we've not used
					pos = rand()%pstack->count;
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
			for(i=0; i<4; ++i) {
				new_out[i] = pstack->note[new_out[i]];
			}			
			break;
			
		//////////////////////////////////////////////////////////
		// PLAY ORDER QUEUE
		// Just shuffle notes along the outputs without any
		// prioritisation
		case PRIORITY_PLAYORDER:
			new_out[3] = pstack->out[2];
			new_out[2] = pstack->out[1];
			new_out[1] = pstack->out[0];
			new_out[0] = pstack->note[0];					
			break;
	
	

		//////////////////////////////////////////////////////////
		// NEW NOTE PRIORITY - just copy out front notes
		case PRIORITY_NEW:
		default:
			for(i=0; i<4 && i<pstack->count; ++i) {
				new_out[i] = pstack->note[i];
			}			
			break;
		}
		
		// Spread out the notes if applicable
		if(pcfg->priority == PRIORITY_LOW_SPREAD || 
			pcfg->priority == PRIORITY_HIGH_SPREAD) {
			if(new_out[0] != NO_NOTE_OUT && new_out[1] != NO_NOTE_OUT) {
				if(new_out[2] == NO_NOTE_OUT) { // #3 will also be empty
					// AB.. -> A..B
					new_out[3] = new_out[1];
					new_out[1] = NO_NOTE_OUT;
				}						
				else if(new_out[3] == NO_NOTE_OUT) {
					// ABC. -> AB.C
					new_out[3] = new_out[2];
					new_out[2] = NO_NOTE_OUT;
				}					
			}
		}
	}

	// now update the note stack outputs based on the new_out array
	for(i=0; i<4; ++i) {
		// check for a change to output
		if(new_out[i] != pstack->out[i]) {
			pstack->out[i] = new_out[i];
			if(new_out[i] != NO_NOTE_OUT) {
				// new note triggered on output
				cv_event(EV_NOTE_A + i, stack_id);
				gate_event(EV_NOTE_A + i, stack_id);
			}
			else {
				// note untriggered on output
				gate_event(EV_NO_NOTE_A + i, stack_id);
			}
			result = 1; // record a change to outputs
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
		
		// note on message?
		if(vel) {			
		
			// velocity above threshold? 
			if(pcfg->vel_min && vel < pcfg->vel_min) 
				continue;
				
			// store note velocity as stack velocity
			pstack->vel = vel;
				
			// if note prioritisation is off then only
			// a single note needs to be tracked at a 
			// time...
			if(pcfg->priority == PRIORITY_OFF) {
				pstack->out[0] = note;
				cv_event(EV_NOTE_A, which_stack);
				gate_event(EV_NOTE_A, which_stack);
			}
			// otherwise add the note to the stack
			else if(add_note(pstack, note)) {
				recalc_outputs(pstack, pcfg, which_stack);
				
			}
			gate_event(EV_NOTE_ON, which_stack);
		}
		else {
			// no note prioritisation
			if(pcfg->priority == PRIORITY_OFF) {
				pstack->out[0] = NO_NOTE_OUT;
				gate_event(EV_NO_NOTE_A, which_stack);
				gate_event(EV_NOTES_OFF, which_stack);
			}
			// else remove note from the stack
			else if(remove_note(pstack, note)) {
				recalc_outputs(pstack, pcfg, which_stack);
				if(!pstack->count) {
					// all notes are off
					gate_event(EV_NOTES_OFF, which_stack);
				}
			}
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

		long new_bend = ((int)((pcfg->bend_range * (bend - 8192.0)/8192.0))) << 8;		
		if(pstack->bend != new_bend) {
			pstack->bend = new_bend;
			cv_event(EV_BEND, i);
		}
	}
}

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
	}
}
 

// INITIALISE NOTE STACK CONFIG
void stack_init()
{
	for(byte i=0; i<NUM_NOTE_STACKS; ++i) {
		g_stack_cfg[i].chan = CHAN_DISABLE;
		g_stack_cfg[i].note_min = 0;
		g_stack_cfg[i].note_max = 127;
		g_stack_cfg[i].vel_min = 0;
		g_stack_cfg[i].bend_range = 12;		
		g_stack_cfg[i].priority = PRIORITY_OFF;		
	}
	g_stack_cfg[0].priority = PRIORITY_NEW;		
	g_stack_cfg[1].priority = PRIORITY_NEW;		
	stack_reset();
//	g_stack_cfg[0].chan = 0;
}

////////////////////////////////////////////////////////////
// CONFIGURE A PARAMETER OF A NOTE STACK
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
			
	//////////////////////////////////////////////////
	// SET A SPLIT POINT
	case NRPNL_SPLIT: 
		if((which_stack == 0 || which_stack == 2) && (value_lo>0 && value_lo<127)) {
			g_stack_cfg[which_stack+1] = g_stack_cfg[which_stack];
			g_stack_cfg[which_stack].note_min = 0;
			g_stack_cfg[which_stack].note_max = value_lo - 1;
			g_stack_cfg[which_stack+1].note_min = value_lo;
			g_stack_cfg[which_stack+1].note_max = 127;
			return 1;
		}
		break;
	}
		
	return 0;
}

