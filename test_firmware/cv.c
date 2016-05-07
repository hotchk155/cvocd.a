////////////////////////////////////////////////////////////
//
// MINI MIDI CV
//
// CONTROL VOLTAGE OUTPUTS
//
//TODO: voltage scaling for CC  and velocity
//TODO: allow 4V double res mode
////////////////////////////////////////////////////////////

//
// INCLUDE FILES
//
#include <system.h>
#include <memory.h>
#include "cv-strip.h"

//
// MACRO DEFS
//
#define I2C_ADDRESS 0b1100000

//
// FILE SCOPE DATA
//

// different modes CV output can operate in 
enum {
	CV_DISABLE = 0,	// disabled
	CV_NOTE,	// mapped to note input
	CV_VEL,		// mapped to note input velocity
	CV_MIDI_BEND,	// mapped to MIID pitch bend
	CV_MIDI_TOUCH, // mapped to aftertouch
	CV_MIDI_CC,	// mapped to midi CC
	CV_MIDI_BPM, // mapped to midi CC
	CV_TEST			// mapped to test voltage	
};

typedef struct {
	byte mode;	// CV_xxx enum
	byte volts;	
	byte stack_id;
	byte out;
	char transpose;	// note offset 
} T_CV_EVENT;

typedef struct {
	byte mode;	// CV_xxx enum
	byte volts;	
	byte chan;
	byte cc;
} T_CV_MIDI;

typedef union {
	T_CV_EVENT 				event;
	T_CV_MIDI 				midi;
} CV_OUT;

// calibration constants
char g_cal_ofs[CV_MAX] = {0};
char g_cal_gain[CV_MAX] = {0};

// cache of raw DAC data
int g_dac[CV_MAX] = {0};

// CV config 
CV_OUT g_cv[CV_MAX];

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// CONFIGURE THE DAC
void cv_config_dac() {
	i2c_begin_write(I2C_ADDRESS);
	i2c_send(0b10001111); // set each channel to use internal vref
	i2c_end();	

	i2c_begin_write(I2C_ADDRESS);
	i2c_send(0b11001111); // set x2 gain on each channel
	i2c_end();	
}

////////////////////////////////////////////////////////////
// COPY CURRENT OUTPUT VALUES TO TRANSMIT BUFFER FOR DAC
void cv_dac_prepare() {	
	g_i2c_tx_buf[0] = I2C_ADDRESS<<1;
	g_i2c_tx_buf[1] = ((g_dac[1]>>8) & 0xF);
	g_i2c_tx_buf[2] = (g_dac[1] & 0xFF);
	g_i2c_tx_buf[3] = ((g_dac[3]>>8) & 0xF);
	g_i2c_tx_buf[4] = (g_dac[3] & 0xFF);
	g_i2c_tx_buf[5] = ((g_dac[2]>>8) & 0xF);
	g_i2c_tx_buf[6] = (g_dac[2] & 0xFF);
	g_i2c_tx_buf[7] = ((g_dac[0]>>8) & 0xF);
	g_i2c_tx_buf[8] = (g_dac[0] & 0xFF);
	g_i2c_tx_buf_len = 9;
	g_i2c_tx_buf_index = 0;
}

////////////////////////////////////////////////////////////
// STORE AN OUTPUT VALUE READY TO SEND TO DAC
void cv_update(byte which, int value) {

	/*
		want ability to tune gain by +/-5%
		0.95 .. 1.05 
		
		map +/-127 to +/-0.05
		.0004
	*/
	//if(g_cal_gain[which]) {
//		value *= (1.0 + (0.0004 * g_cal_gain[which]));
//	}
//	value += g_cal_ofs[which];
	if(value < 0) 
		value = 0;
	if(value > 4095) 
		value = 4095;
		
	// check the value has actually changed
	if(value != g_dac[which]) {
		g_dac[which] = value;
		g_cv_dac_pending = 1;
	}
}	

////////////////////////////////////////////////////////////
// WRITE A NOTE VALUE TO A CV OUTPUT
// pitch_bend units = MIDI note * 256
void cv_write_note(byte which, byte midi_note, int pitch_bend) {
	long value = (((long)midi_note)<<8 + pitch_bend);
	value *= 500;
	value /= 12;	
	cv_update(which, value>>8);
}

////////////////////////////////////////////////////////////
// WRITE A 7-BIT CC VALUE TO A CV OUTPUT
void cv_write_7bit(byte which, byte value, byte volts) {
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
void cv_write_bend(byte which, int value, byte volts) {	
	// 1 volt is 500 clicks on the DAC
	// So (500 * volts) is the full range for the 14-bit bend value (16384)
	// DAC value = (value / 16384) * (500 * volts)
	// = (value / 32.768) * volts
	// ~ (volts * value)/32
	cv_update(which, (((long)value * volts) >> 5));
}

////////////////////////////////////////////////////////////
// WRITE VOLTS
void cv_write_volts(byte which, byte value) {
	cv_update(which, (int)value * 500);
}

////////////////////////////////////////////////////////////
// HANDLE AN EVENT FROM A NOTE STACK
void cv_event(byte event, byte stack_id) {
	byte output_id;
	NOTE_STACK *pstack;
	
	// for each CV output
	for(byte which_cv=0; which_cv<CV_MAX; ++which_cv) {
		CV_OUT *pcv = &g_cv[which_cv];
		if(pcv->event.mode == CV_DISABLE)
			continue;
		
		// is it listening to the stack sending the event?
		if(pcv->event.stack_id == stack_id) {

			// get pointer to note stack
			pstack = &g_stack[stack_id];
			
			// check the mode			
			switch(pcv->event.mode) {

			/////////////////////////////////////////////
			// CV OUTPUT TIED TO INPUT NOTE
			case CV_NOTE:	
				switch(event) {
					case EV_NOTE_A:
					case EV_NOTE_B:
					case EV_NOTE_C:
					case EV_NOTE_D:
						output_id = event - EV_NOTE_A;
						if(pcv->event.out == output_id) {			
							int note = pstack->out[output_id] + pcv->event.transpose;
							while(note < 0) note += 12;
							while(note > 127) note -= 12;
							cv_write_note(which_cv, note, pstack->bend);
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
}

////////////////////////////////////////////////////////////
// HANDLE A MIDI CC
void cv_midi_cc(byte chan, byte cc, byte value) {
	for(byte which_cv=0; which_cv<CV_MAX; ++which_cv) {
		CV_OUT *pcv = &g_cv[which_cv];
		
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
		CV_OUT *pcv = &g_cv[which_cv];
		
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
		CV_OUT *pcv = &g_cv[which_cv];
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
void cv_midi_bpm(long value) {
	for(byte which_cv=0; which_cv<CV_MAX; ++which_cv) {
		CV_OUT *pcv = &g_cv[which_cv];
		if(pcv->event.mode != CV_MIDI_BPM) {
			continue;
		}		
		cv_update(which_cv, (int)(value/30)); //TODO
	}
}					


 
////////////////////////////////////////////////////////////
// CONFIGURE A CV OUTPUT
// return nonzero if any change was made
byte cv_nrpn(byte which_cv, byte param_lo, byte value_hi, byte value_lo) 
{
	if(which_cv>CV_MAX)
		return 0;
	CV_OUT *pcv = &g_cv[which_cv];
	
	switch(param_lo) {
	// SELECT SOURCE
	case NRPNL_SRC:
		switch(value_hi) {				
		case NRPVH_SRC_TESTVOLTAGE0:	// REFERENCE VOLTAGE
		case NRPVH_SRC_TESTVOLTAGE1:
		case NRPVH_SRC_TESTVOLTAGE2:
		case NRPVH_SRC_TESTVOLTAGE3:
		case NRPVH_SRC_TESTVOLTAGE4:
		case NRPVH_SRC_TESTVOLTAGE5:
		case NRPVH_SRC_TESTVOLTAGE6:
		case NRPVH_SRC_TESTVOLTAGE7:
		case NRPVH_SRC_TESTVOLTAGE8:
			pcv->event.mode = CV_TEST;
			pcv->event.volts = value_lo - NRPVH_SRC_TESTVOLTAGE0;
			cv_write_volts(which_cv, value_lo); 
			return 1;
		case NRPVH_SRC_DISABLE:	// DISABLE
			cv_write_volts(which_cv, 0); 
			pcv->event.mode = CV_DISABLE;
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
				pcv->event.transpose = 0;
				return 1;				
			case NRPVL_SRC_VEL:		// NOTE VELOCITY
				pcv->event.mode = CV_VEL;
				pcv->event.volts = DEFAULT_CV_VEL_MAX_VOLTS;
				return 1;
			}
		}
		break;
	// SELECT TRANSPOSE AMOUNT
	case NRPNL_TRANSPOSE:
		if(CV_NOTE == pcv->event.mode) {
			pcv->event.transpose = value_lo - 64;
			return 1;
		}
		break;

	// SELECT VOLTAGE RANGE
	case NRPNL_VOLTS:
		if(value_lo >= 0 && value_lo <= 8) {
			pcv->event.volts = value_lo;
			return 1;
		}
		break;
	
	case NRPNL_CV_OFFSET:
	case NRPNL_CV_GAIN:
		if(CV_TEST == pcv->event.mode) {			
			char val = value_hi? value_lo : -value_lo;
			if(param_lo == NRPNL_CV_OFFSET) {
				g_cal_ofs[which_cv] = val;
			}
			else {
				g_cal_gain[which_cv] = val;
			}
			//TODO: EEPROM
			cv_write_volts(which_cv, pcv->event.volts); 
		}
		break;		
	}
	return 0;
}

////////////////////////////////////////////////////////////
// INITIALISE CV MODULE
void cv_init() {
	memset(g_dac, 0, sizeof(g_dac));
	memset(g_cv, 0, sizeof(g_cv));
	cv_config_dac();
	
	//g_cv[0].event.mode = CV_NOTE;
	//g_cv[0].event.stack_id = 0;
	//g_cv[0].event.out = 0;	
	
	//g_cv[1].event.mode = CV_MIDI_BPM;
	
	cv_reset();
	
}

////////////////////////////////////////////////////////////
void cv_reset() {
}



