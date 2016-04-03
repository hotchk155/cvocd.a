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


/*
+---+---+---+---+	+---+---+---+---+
|CV1|CV2|GT1|GT2|	|GT5|GT6|GT7|GT8|
+---+---+---+---+	+---+---+---+---+
|CV3|CV4|GT3|GT4|	|GT9|G10|G11|G12|
+---+---+---+---+	+---+---+---+---+
*/
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