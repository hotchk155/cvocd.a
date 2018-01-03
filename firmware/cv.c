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
// CV OUTPUT MODULE
//
//////////////////////////////////////////////////////////////

//
// INCLUDE FILES
//
#include <system.h>
#include <memory.h>
#include "cvocd.h"


//
// MACRO DEFS
//
#define I2C_ADDRESS 0b1100000

//
// TYPE DEFS
//

// different modes CV output can operate in 
enum {
	CV_DISABLE = 0,	// disabled
	CV_NOTE,	// mapped to note input
	CV_VEL,		// mapped to note input velocity
	CV_MIDI_BEND,	// mapped to MIDI pitch bend
	CV_MIDI_TOUCH, // mapped to aftertouch
	CV_MIDI_CC,	// mapped to midi CC
	CV_MIDI_BPM, // mapped to midi CC
	CV_TEST,			// mapped to test voltage	
	CV_NOTE_HZV, // mapped to Hz/Volt note
	CV_NOTE_12VO // mapped to 1.2V/oct
};

typedef struct {
	byte mode;	// CV_xxx enum
	byte volts;	
	byte ofs;
	byte scale;
	byte stack_id;
	byte out;
	byte transpose;  
} T_CV_EVENT;

typedef struct {
	byte mode;	// CV_xxx enum
	byte volts;	
	byte ofs;
	byte scale;
	byte chan;
	byte cc;
} T_CV_MIDI;

typedef union {
	T_CV_EVENT 				event;
	T_CV_MIDI 				midi;
} CV_OUT;

//
// LOCAL DATA
//

// cache of raw DAC data
int l_dac[CV_MAX] = {0};

// CV config 
CV_OUT l_cv[CV_MAX];

// cache of the notes playing on each output
int l_note[CV_MAX];

//
// LOCAL FUNCTIONS
//

////////////////////////////////////////////////////////////
// CONFIGURE THE DAC
static void cv_config_dac() {
	i2c_begin_write(I2C_ADDRESS);
	i2c_send(0b10001111); // set each channel to use internal vref
	i2c_end();	

	i2c_begin_write(I2C_ADDRESS);
	i2c_send(0b11001111); // set x2 gain on each channel
	i2c_end();	
}

////////////////////////////////////////////////////////////
// STORE AN OUTPUT VALUE READY TO SEND TO DAC
static void cv_update(byte which, int value) {
	
	if(l_cv[which].event.scale) {
		long scale = (long)l_cv[which].event.scale - 64;
		long ofs = (long)l_cv[which].event.ofs - 64;
		value = (((long)value * (4096 + scale))/4096) + ofs;
	}
	
	if(value < 0) 
		value = 0;
	if(value > 4095) 
		value = 4095;
		
	// check the value has actually changed
	if(value != l_dac[which]) {
		l_dac[which] = value;
		g_cv_dac_pending = 1;
	}
}	

////////////////////////////////////////////////////////////
// WRITE A NOTE VALUE TO A CV OUTPUT
// pitch_bend units = MIDI note * 256
static void cv_write_note(byte which, long note, int pitch_bend, long dacs_per_oct) {
	note <<= 8;
	note += pitch_bend;
	note *= dacs_per_oct;
	note /= 12;	
	note >>= 8;
	cv_update(which, note);
}


////////////////////////////////////////////////////////////
// WRITE A NOTE VALUE TO A CV OUTPUT
// pitch_bend units = MIDI note * 256
static void cv_write_note_hzvolt(byte which, long note, int pitch_bend) {

	// convert pitch bend to whole notes and fractional (1/256) notes
	note <<= 8;
	note += pitch_bend;
	pitch_bend = note & 0xFF;
	note >>= 8;

	// use a hard coded lookup table to get the
	// the DAC value for note in top octave
	int dac;
	if(note == 72)
		dac = 4000;	// we can just about manage a C6!
	else switch((byte)note % 12) {	
		case 0: dac = 2000; break;
		case 1: dac = 2119; break;
		case 2: dac = 2245; break;
		case 3: dac = 2378; break;
		case 4: dac = 2520; break;
		case 5: dac = 2670; break;
		case 6: dac = 2828; break;
		case 7: dac = 2997; break;
		case 8: dac = 3175; break;
		case 9: dac = 3364; break;
		case 10: dac = 3564; break;
		case 11: dac = 3775; break;	
	}

	/*
		next dac note will be dac*(2^(1/12))
		approximately equal to dac * (1 + 244/(16*256))
		pitch_bend contains 256 * fractional note (signed)
		so dac offset for fractional bend = 
			dac * (pitch_bend/256)*(244/(16*256))
		
			= (dac * 244 * pitch_bend) / (16*256*256)
			= (dac * 244 * pitch_bend) / 1048576 		(0x100000L)
			
		
		NB: this results in linear interpolation between the notes, 
		which is a bad approximation of a smooth bend since the 
		mapping of V to Hz is logarithmic not linear!
		
		It'll have to do for now...
	*/
	dac += (int)(((long)dac*244*pitch_bend)/0x100000L);
	
	// transpose to the requested octave by 
	// right shifting
	byte octave = ((byte)note)/12;
	if(octave > 5) octave = 5;
	dac >>= (5-octave);
		
	// finally update dac
	cv_update(which, dac);
}

////////////////////////////////////////////////////////////
// WRITE A 7-BIT CC VALUE TO A CV OUTPUT
static void cv_write_7bit(byte which, byte value, byte volts) {
	if(value > 127) 
		value = 127;
	// 1 volt is 500 clicks on the DAC
	// So (500 * volts) is the full range for the 7-bit value (127)
	// DAC value = (value / 127) * (500 * volts)
	// = 3.937 * value * volts
	// ~ 4 * value * volts
	cv_update(which, ((int)value * volts)<<2);
}

////////////////////////////////////////////////////////////
// WRITE PITCH BEND VALUE TO A CV OUTPUT
// receive raw 14bit value 
static void cv_write_bend(byte which, int value, byte volts) {	
	// 1 volt is 500 clicks on the DAC
	// So (500 * volts) is the full range for the 14-bit bend value (16384)
	// DAC value = (value / 16384) * (500 * volts)
	// = (value / 32.768) * volts
	// ~ (volts * value)/32
	cv_update(which, (((long)value * volts) >> 5));
}

////////////////////////////////////////////////////////////
// WRITE VOLTS
static void cv_write_volts(byte which, byte value) {
	cv_update(which, (int)value * 500);
}

//
// GLOBAL FUNCTIONS
//

////////////////////////////////////////////////////////////
// COPY CURRENT OUTPUT VALUES TO TRANSMIT BUFFER FOR DAC
void cv_dac_prepare() {	
	g_i2c_tx_buf[0] = I2C_ADDRESS<<1;
	g_i2c_tx_buf[1] = ((l_dac[1]>>8) & 0xF);
	g_i2c_tx_buf[2] = (l_dac[1] & 0xFF);
	g_i2c_tx_buf[3] = ((l_dac[3]>>8) & 0xF);
	g_i2c_tx_buf[4] = (l_dac[3] & 0xFF);
	g_i2c_tx_buf[5] = ((l_dac[2]>>8) & 0xF);
	g_i2c_tx_buf[6] = (l_dac[2] & 0xFF);
	g_i2c_tx_buf[7] = ((l_dac[0]>>8) & 0xF);
	g_i2c_tx_buf[8] = (l_dac[0] & 0xFF);
	g_i2c_tx_buf_len = 9;
	g_i2c_tx_buf_index = 0;
}

////////////////////////////////////////////////////////////
// HANDLE AN EVENT FROM A NOTE STACK
void cv_event(byte event, byte stack_id) {
	byte output_id;
	int note;
	NOTE_STACK *pstack;
	
	// for each CV output
	for(byte which_cv=0; which_cv<CV_MAX; ++which_cv) {
		CV_OUT *pcv = &l_cv[which_cv];
		if(pcv->event.mode == CV_DISABLE)
			continue;
		
		// is it listening to the stack sending the event?
		if(pcv->event.stack_id != stack_id)
			continue;

		// get pointer to note stack
		pstack = &g_stack[stack_id];
		
		// check the mode			
		switch(pcv->event.mode) {

		/////////////////////////////////////////////
		// CV OUTPUT TIED TO INPUT NOTE
		case CV_NOTE:	
		case CV_NOTE_HZV:
		case CV_NOTE_12VO:
			switch(event) {
				case EV_NOTE_A:
				case EV_NOTE_B:
				case EV_NOTE_C:
				case EV_NOTE_D:
					output_id = event - EV_NOTE_A;
					if(pcv->event.out == output_id) {			
						note = (int)pstack->out[output_id] + ((int)pcv->event.transpose - TRANSPOSE_NONE) - 24;
						while(note < 0) note += 12; 	
						while(note > 120) note -= 12; 	
						l_note[which_cv] = note;
					}
					// fall through
				case EV_BEND:
					if(pcv->event.mode == CV_NOTE_HZV) {
						cv_write_note_hzvolt(which_cv, l_note[which_cv], pstack->bend);
					}
					else if(pcv->event.mode == CV_NOTE_12VO) {
						cv_write_note(which_cv, l_note[which_cv], pstack->bend, 600);
					}
					else {
						cv_write_note(which_cv, l_note[which_cv], pstack->bend, 500);
					}
					break;
			}
			break;
		/////////////////////////////////////////////
		// CV OUTPUT TIED TO INPUT VELOCITY
		case CV_VEL:	
			switch(event) {
				case EV_NOTE_A:
				case EV_NOTE_B:
				case EV_NOTE_C:
				case EV_NOTE_D:
					cv_write_7bit(which_cv, pstack->vel, pcv->event.volts);
					break;
			}
			break;
		}
	}
}

////////////////////////////////////////////////////////////
// HANDLE A MIDI CC
void cv_midi_cc(byte chan, byte cc, byte value) {
	for(byte which_cv=0; which_cv<CV_MAX; ++which_cv) {
		CV_OUT *pcv = &l_cv[which_cv];
	
		// is this CV output configured for CC?
		if(pcv->event.mode != CV_MIDI_CC) {
			continue;
		}		
		// does the CC number match?
		if(cc != pcv->midi.cc) {
			continue;
		}
		// does MIDI channel match
		if(!IS_CHAN(pcv->midi.chan,chan)) {
			continue;
		}		
		// OK update the output
		cv_write_7bit(which_cv, value, pcv->event.volts);
	}
}

////////////////////////////////////////////////////////////
// HANDLE MIDI AFTERTOUCH
void cv_midi_touch(byte chan, byte value) {
	for(byte which_cv=0; which_cv<CV_MAX; ++which_cv) {
		CV_OUT *pcv = &l_cv[which_cv];
		
		// is this CV output configured for CC?
		if(pcv->event.mode != CV_MIDI_TOUCH) {
			continue;
		}		
		// does MIDI channel match
		if(!IS_CHAN(pcv->midi.chan,chan)) {
			continue;
		}		
		// OK update the output
		cv_write_7bit(which_cv, value, pcv->event.volts);
	}
}

////////////////////////////////////////////////////////////
// HANDLE PITCH BEND
void cv_midi_bend(byte chan, int value)
{
	for(byte which_cv=0; which_cv<CV_MAX; ++which_cv) {
		CV_OUT *pcv = &l_cv[which_cv];
		if(pcv->event.mode != CV_MIDI_BEND) {
			continue;
		}		
		// does MIDI channel match
		if(!IS_CHAN(pcv->midi.chan,chan)) {
			continue;
		}		
		cv_write_bend(which_cv, value, pcv->event.volts);		
	}
}					

////////////////////////////////////////////////////////////
// HANDLE BPM
// BPM is upscaled by 256
/*
void cv_midi_bpm(long value) {
	if(value & (long)0xFFFF0000)
		value = 0xFFFF;
	for(byte which_cv=0; which_cv<CV_MAX; ++which_cv) {
		CV_OUT *pcv = &l_cv[which_cv];
		if(pcv->event.mode != CV_MIDI_BPM) {
			continue;
		}				
		value *= (500 * pcv->event.volts);
		cv_update(which_cv, value>>16);
	}
}
*/
////////////////////////////////////////////////////////////
// CONFIGURE A CV OUTPUT
// return nonzero if any change was made
byte cv_nrpn(byte which_cv, byte param_lo, byte value_hi, byte value_lo) 
{
	if(which_cv>CV_MAX)
		return 0;
	CV_OUT *pcv = &l_cv[which_cv];
	
	switch(param_lo) {
	// SELECT SOURCE
	case NRPNL_SRC:
		switch(value_hi) {				
		case NRPVH_SRC_DISABLE:	// DISABLE
			cv_write_volts(which_cv, 0); 
			pcv->event.mode = CV_DISABLE;
			return 1;
		case NRPVH_SRC_TESTVOLTAGE:	// REFERENCE VOLTAGE
			pcv->event.mode = CV_TEST;
			pcv->event.volts = DEFAULT_CV_TEST_VOLTS;
			return 1;
		case NRPVH_SRC_MIDITICK: // BPM
			pcv->event.mode = CV_MIDI_BPM;
			pcv->event.volts = DEFAULT_CV_BPM_MAX_VOLTS;
			return 1;
		case NRPVH_SRC_MIDICC: // CC
			pcv->event.mode = CV_MIDI_CC;
			pcv->midi.chan = CHAN_GLOBAL;
			pcv->midi.cc = value_lo;
			pcv->midi.volts = DEFAULT_CV_CC_MAX_VOLTS;
			return 1;					
		case NRPVH_SRC_MIDITOUCH: // AFTERTOUCH
			pcv->event.mode = CV_MIDI_TOUCH;
			pcv->midi.chan = CHAN_GLOBAL;
			pcv->midi.volts = DEFAULT_CV_TOUCH_MAX_VOLTS;
			return 1;					
		case NRPVH_SRC_MIDIBEND: // PITCHBEND
			pcv->event.mode = CV_MIDI_BEND;
			pcv->midi.chan = CHAN_GLOBAL;
			pcv->midi.volts = DEFAULT_CV_PB_MAX_VOLTS;
			return 1;					
		case NRPVH_SRC_STACK1: // NOTE STACK 
		case NRPVH_SRC_STACK2:
		case NRPVH_SRC_STACK3:
		case NRPVH_SRC_STACK4:
			pcv->event.stack_id = value_hi - NRPVH_SRC_STACK1;		
			switch(value_lo) {
			case NRPVL_SRC_NOTE1:	// NOTE PITCH
			case NRPVL_SRC_NOTE2:
			case NRPVL_SRC_NOTE3:
			case NRPVL_SRC_NOTE4:
				pcv->event.mode = CV_NOTE;
				pcv->event.out = value_lo - NRPVL_SRC_NOTE1;
				pcv->event.transpose = TRANSPOSE_NONE;
				return 1;				
			case NRPVL_SRC_VEL:		// NOTE VELOCITY
				pcv->event.mode = CV_VEL;
				pcv->event.volts = DEFAULT_CV_VEL_MAX_VOLTS;
				return 1;
		}
		}
		break;
		
	////////////////////////////////////////////////////////////////
	// SELECT MIDI CHANNEL
	case NRPNL_CHAN:	
		switch(value_hi) {
		case NRPVH_CHAN_SPECIFIC:
			if(value_lo >= 1 && value_lo <= 16) {
				pcv->midi.chan = value_lo - 1; // relies on alignment of chan member in cc too
				return 1;
			}
			break;
		case NRPVH_CHAN_OMNI:
			pcv->midi.chan = CHAN_OMNI;
			return 1;
		case NRPVH_CHAN_GLOBAL:
			pcv->midi.chan = CHAN_GLOBAL;
			return 1;
		}
		break;		
		
	// SELECT TRANSPOSE AMOUNT
	case NRPNL_TRANSPOSE:
		pcv->event.transpose = value_lo; 
		return 1;

	// SELECT VOLTAGE RANGE
	case NRPNL_VOLTS:
		if(value_lo >= 0 && value_lo <= 8) {
			pcv->event.volts = value_lo;
			return 1;
		}
		break;	

	// SELECT PITCH SCHEME
	case NRPNL_PITCH_SCHEME:
		if(value_lo == NRPVH_PITCH_HZV) {
			pcv->event.mode = CV_NOTE_HZV;
		}
		else if(value_lo == NRPVH_PITCH_12VO) {
			pcv->event.mode = CV_NOTE_12VO;
		}
		else {
			pcv->event.mode = CV_NOTE;
		}
		return 1;		
		
	// CALIBRATION
	case NRPNL_CAL_SCALE:
		pcv->event.scale = value_lo; // zero here is "off"
		return 1;
	case NRPNL_CAL_OFS:
		pcv->event.ofs = value_lo;
		return 1;		
	}
	return 0;
}

////////////////////////////////////////////////////////////
// GET CV CONFIG
byte *cv_storage(int *len) {
	*len = sizeof(l_cv);
	return (byte*)&l_cv;
}

////////////////////////////////////////////////////////////
// INITIALISE CV MODULE
void cv_init() {
	memset(l_cv, 0, sizeof(l_cv));
	memset(l_dac, 0, sizeof(l_dac));
	memset(l_note, 0, sizeof(l_note));
	cv_config_dac();
	
	/*l_cv[0].event.mode = CV_NOTE;
	l_cv[0].event.out = 0;	
	l_cv[1].event.mode = CV_NOTE;
	l_cv[1].event.out = 1;	
	l_cv[2].event.mode = CV_NOTE;
	l_cv[2].event.out = 2;	
	l_cv[3].event.mode = CV_NOTE;
	l_cv[3].event.out = 3;		
	*/
}

////////////////////////////////////////////////////////////
void cv_reset() {
	for(byte which=0; which < CV_MAX; ++which) {
		switch(l_cv[which].event.mode) {				
		case CV_TEST:	
			cv_write_volts(which, l_cv[which].event.volts); // set test volts
			break;
		case CV_MIDI_BEND: 
			cv_write_bend(which, 8192, l_cv[which].event.volts); // set half full volts
			break;
		default:
			cv_write_volts(which, 0); 
			break;
		}
	}
	g_cv_dac_pending = 1;
}

//
// END
//