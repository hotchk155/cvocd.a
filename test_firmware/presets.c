////////////////////////////////////////////////////
//
// MIDI CV STRIP
//
// hotchk155/2016
//
////////////////////////////////////////////////////

//
// HEADER FILES
//
#include <system.h>
#include <rand.h>
#include <eeprom.h>
#include "cv-strip.h"

#define CIRCUIT_DRUM1	0x3c
#define CIRCUIT_DRUM2	0x3e
#define CIRCUIT_DRUM3	0x40
#define CIRCUIT_DRUM4	0x41

/*
+---+---+---+---+	+---+---+---+---+
|CV1|CV2|GT1|GT2|	|GT5|GT6|GT7|GT8|
+---+---+---+---+	+---+---+---+---+
|CV3|CV4|GT3|GT4|	|GT9|G10|G11|G12|
+---+---+---+---+	+---+---+---+---+
*/

void preset1() 
{
	// Configure CVs
	// CV1 = Channel 1 note CV
	// CV2 = Channel 1 velocity CV
	// CV3 = Channel 2 note CV
	// CV4 = Channel 3 velocity CV
	nrpn(NRPNH_STACK1, NRPNL_CHAN, NRPVH_CHAN_SPECIFIC, 1);
	nrpn(NRPNH_STACK2, NRPNL_CHAN, NRPVH_CHAN_SPECIFIC, 2);	
	nrpn(NRPNH_CV1, NRPNL_SRC, NRPVH_SRC_STACK1, NRPVL_SRC_NOTE1);
	nrpn(NRPNH_CV2, NRPNL_SRC, NRPVH_SRC_STACK1, NRPVL_SRC_VEL);	
	nrpn(NRPNH_CV3, NRPNL_SRC, NRPVH_SRC_STACK2, NRPVL_SRC_NOTE1);
	nrpn(NRPNH_CV4, NRPNL_SRC, NRPVH_SRC_STACK2, NRPVL_SRC_VEL);
	// Gate 1 = Channel 1 note gate
	// Gate 2 = Channel 1 note trigger	
	// Gate 3 = Channel 2 note gate
	// Gate 4 = Channel 2 note trigger
	nrpn(NRPNH_GATE1, NRPNL_SRC, NRPVH_SRC_STACK1, NRPVL_SRC_NOTE1);
	nrpn(NRPNH_GATE2, NRPNL_SRC, NRPVH_SRC_STACK1, NRPVL_SRC_NOTE1);
	nrpn(NRPNH_GATE3, NRPNL_SRC, NRPVH_SRC_STACK2, NRPVL_SRC_NOTE1);
	nrpn(NRPNH_GATE4, NRPNL_SRC, NRPVH_SRC_STACK2, NRPVL_SRC_NOTE1);
	// Set gates 1 and 3 for "gate" mode
	nrpn(NRPNH_GATE1, NRPNL_GATE_DUR, 0, 0);
	nrpn(NRPNH_GATE3, NRPNL_GATE_DUR, 0, 0);

	// Gates 5,6,7,8 are specific notes corresponding to drums
	nrpn(NRPNH_GATE5, NRPNL_SRC, NRPVH_SRC_MIDINOTE, CIRCUIT_DRUM1);
	nrpn(NRPNH_GATE6, NRPNL_SRC, NRPVH_SRC_MIDINOTE, CIRCUIT_DRUM2);
	nrpn(NRPNH_GATE7, NRPNL_SRC, NRPVH_SRC_MIDINOTE, CIRCUIT_DRUM3);
	nrpn(NRPNH_GATE8, NRPNL_SRC, NRPVH_SRC_MIDINOTE, CIRCUIT_DRUM4);			
	nrpn(NRPNH_GATE5, NRPNL_CHAN, NRPVH_CHAN_SPECIFIC, 10);
	nrpn(NRPNH_GATE6, NRPNL_CHAN, NRPVH_CHAN_SPECIFIC, 10);	
	nrpn(NRPNH_GATE7, NRPNL_CHAN, NRPVH_CHAN_SPECIFIC, 10);	
	nrpn(NRPNH_GATE8, NRPNL_CHAN, NRPVH_CHAN_SPECIFIC, 10);			
	
	// Gate 9 is a drum accent trigger
	nrpn(NRPNH_GATE9, NRPNL_SRC, NRPVH_SRC_MIDINOTE, 0);
	nrpn(NRPNH_GATE9, NRPNL_NOTE_MAX, 0, 127);
	nrpn(NRPNH_GATE9, NRPNL_VEL_MIN, 0, 100);
	nrpn(NRPNH_GATE9, NRPNL_CHAN, NRPVH_CHAN_SPECIFIC, 10);	

	// Gate 10 is 4th note sync 
	nrpn(NRPNH_GATE10, NRPNL_SRC, NRPVH_SRC_MIDITICK, 24);
	
	// Gate 11 is 8th note sync (Volca)
	nrpn(NRPNH_GATE11, NRPNL_SRC, NRPVH_SRC_MIDITICK, 12);

	// Gate 12 is 16th note sync (beat clock)
	nrpn(NRPNH_GATE12, NRPNL_SRC, NRPVH_SRC_MIDITICK, 6);
	


}
/*
// PRESET FOR NOVATION CIRCUIT MODE
void preset_1()
{
	cfg(P_ID_GLOBAL, P_DURATION, 20);
	cfg(P_ID_GLOBAL, P_VEL_ACCENT, 127);

	// MIDI channel 1 note -> CV1
	// MIDI channel 1 velocity -> CV2
	// MIDI channel 1 gate -> GT1
	cfg(P_ID_INPUT1, P_CHAN, 1);
	cfg(P_ID_CV1, 	P_INPUT1, P_INPUT_NOTEA);
	cfg(P_ID_CV2, 	P_INPUT1, P_INPUT_VELOCITY);
	cfg(P_ID_GATE1, 	P_INPUT1, P_INPUT_NOTEA);
	
	// MIDI channel 2 note -> CV3
	// MIDI channel 2 velocity -> CV4
	// MIDI channel 2 gate -> GT3
	cfg(P_ID_INPUT2, P_CHAN, 2);
	cfg(P_ID_CV3, 	P_INPUT2, P_INPUT_NOTEA);
	cfg(P_ID_CV4, 	P_INPUT2, P_INPUT_VELOCITY);
	cfg(P_ID_GATE3, 	P_INPUT2, P_INPUT_NOTEA);

	// Gate 2 is MIDI step clock
	cfg(P_ID_GATE2, 	P_MIDI, P_MIDI_TICK);
	cfg(P_ID_GATE2, 	P_DIV, P_DIV_16);

	// Gate 4 is MIDI step clock/2 (Korg)
	cfg(P_ID_GATE4, 	P_MIDI, P_MIDI_TICK);
	cfg(P_ID_GATE4, 	P_DIV, P_DIV_8);

	cfg(P_ID_GATE5, 	P_CHAN, 10);
	cfg(P_ID_GATE6, 	P_CHAN, 10);
	cfg(P_ID_GATE7, 	P_CHAN, 10);
	cfg(P_ID_GATE8, 	P_CHAN, 10);
	cfg(P_ID_GATE9, 	P_CHAN, 10);
	cfg(P_ID_GATE10, P_CHAN, 10);
	cfg(P_ID_GATE11, P_CHAN, 10);
	cfg(P_ID_GATE12, P_CHAN, 10);

	cfg(P_ID_GATE5, 	P_MIDI, P_MIDI_NOTE);
	cfg(P_ID_GATE6, 	P_MIDI, P_MIDI_NOTE);
	cfg(P_ID_GATE7, 	P_MIDI, P_MIDI_NOTE);
	cfg(P_ID_GATE8, 	P_MIDI, P_MIDI_NOTE);
	cfg(P_ID_GATE9, 	P_MIDI, P_MIDI_NOTE);
	cfg(P_ID_GATE10,	P_MIDI, P_MIDI_NOTE);
	cfg(P_ID_GATE11,	P_MIDI, P_MIDI_NOTE);
	cfg(P_ID_GATE12,	P_MIDI, P_MIDI_NOTE);
		
	cfg(P_ID_GATE5, 	P_NOTE_SINGLE, 0x3C);
	cfg(P_ID_GATE6, 	P_NOTE_SINGLE, 0x3E);
	cfg(P_ID_GATE7, 	P_NOTE_SINGLE, 0x40);
	cfg(P_ID_GATE8, 	P_NOTE_SINGLE, 0x41);
	cfg(P_ID_GATE9, 	P_NOTE_SINGLE, 0x3C);
	cfg(P_ID_GATE10,	P_NOTE_SINGLE, 0x3E);
	cfg(P_ID_GATE11,	P_NOTE_SINGLE, 0x40);
	cfg(P_ID_GATE12,	P_NOTE_SINGLE, 0x41);
	
	cfg(P_ID_GATE9, 	P_VEL_MIN, VEL_ACC_GLOBAL);
	cfg(P_ID_GATE10, P_VEL_MIN, VEL_ACC_GLOBAL);
	cfg(P_ID_GATE11, P_VEL_MIN, VEL_ACC_GLOBAL);
	cfg(P_ID_GATE12, P_VEL_MIN, VEL_ACC_GLOBAL);
	
}

*/