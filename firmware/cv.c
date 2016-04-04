////////////////////////////////////////////////////////////
//
// MINI MIDI CV
//
// CONTROL VOLTAGE OUTPUTS
//
//TODO: voltage scaling for CC 
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
	CV_PB,		// mapped to note input pitch bend
	CV_MIDI_CC,	// mapped to midi CC
	CV_MIDI_BPM, // mapped to midi CC
	CV_VOLTS	
};

typedef struct {
	byte mode;	// CV_xxx enum
	byte stack_id;
	byte out;
	char transpose;	// note offset 
} T_CV_EVENT;

typedef struct {
	byte mode;	// CV_xxx enum
	byte chan;
	byte cc;
} T_CV_MIDI_CC;

typedef struct {
	byte mode;	// CV_xxx enum
} T_CV_MIDI_PB;

typedef struct {
	byte mode;	// CV_xxx enum
} T_CV_MIDI_BPM;

typedef struct {
	byte mode;	// CV_xxx enum
	byte volts;	
} T_CV_VOLTS;

typedef union {
	T_CV_EVENT 		event;
	T_CV_MIDI_CC 	cc;
	T_CV_MIDI_PB 	pb;
	T_CV_MIDI_BPM	bpm;
	T_CV_VOLTS		volts;
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
// PRIVATE FUNCTIONS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// I2C MASTER INIT
static void i2c_init() {
	// disable output drivers on i2c pins
	trisc.0 = 1;
	trisc.1 = 1;
	
	//ssp1con1.7 = 
	//ssp1con1.6 = 
	ssp1con1.5 = 1; // Enable synchronous serial port
	ssp1con1.4 = 1; // Enable SCL
	ssp1con1.3 = 1; // }
	ssp1con1.2 = 0; // }
	ssp1con1.1 = 0; // }
	ssp1con1.0 = 0; // } I2C Master with clock = Fosc/(4(SSPxADD+1))
	
	ssp1stat.7 = 1;	// slew rate disabled	
	ssp1add = 19;	// 100kHz baud rate
}

////////////////////////////////////////////////////////////
// I2C WRITE BYTE TO BUS
static void i2c_send(byte data) {
	ssp1buf = data;
	while((ssp1con2 & 0b00011111) || // SEN, RSEN, PEN, RCEN or ACKEN
		(ssp1stat.2)); // data transmit in progress	
}

////////////////////////////////////////////////////////////
// I2C START WRITE MESSAGE TO A SLAVE
static void i2c_begin_write(byte address) {
	pir1.3 = 0; // clear SSP1IF
	ssp1con2.0 = 1; // signal start condition
	while(!pir1.3); // wait for it to complete
	i2c_send(address<<1); // address + WRITE(0) bit
}

////////////////////////////////////////////////////////////
// I2C FINISH MESSAGE
static void i2c_end() {
	pir1.3 = 0; // clear SSP1IF
	ssp1con2.2 = 1; // signal stop condition
	while(!pir1.3); // wait for it to complete
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
void cv_config_dac() {
	i2c_begin_write(I2C_ADDRESS);
	i2c_send(0b10001111); // set each channel to use internal vref
	i2c_end();	

	i2c_begin_write(I2C_ADDRESS);
	i2c_send(0b11001111); // set x2 gain on each channel
	i2c_end();	
}

////////////////////////////////////////////////////////////
// WRITE TO DAC
// TODO: Use single DAC write
void cv_write_dac(byte which, int value) {
	// constrain to valid range
	
	// 0x00000FFF
	// 0xFFF00000
	//
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
	if(value == g_dac[which]) 
		return;
		
	// store new channel value
	g_dac[which] = value;
	
	// send all channels to DAC chip - takes account
	// of mapping from logical CV 0-3 to actual DAC
	// channel wired to the appropriate CV output
	i2c_begin_write(I2C_ADDRESS);
	i2c_send((g_dac[1]>>8) & 0xF);
	i2c_send(g_dac[1] & 0xFF);
	i2c_send((g_dac[3]>>8) & 0xF);
	i2c_send(g_dac[3] & 0xFF);
	i2c_send((g_dac[2]>>8) & 0xF);
	i2c_send(g_dac[2] & 0xFF);
	i2c_send((g_dac[0]>>8) & 0xF);
	i2c_send(g_dac[0] & 0xFF);
	i2c_end();	
}

////////////////////////////////////////////////////////////
// WRITE A NOTE VALUE TO A CV OUTPUT
// pitch_bend units = MIDI note * 256
void cv_write_note(byte which, byte midi_note, int pitch_bend) {
	long value = (((long)midi_note)<<8 + pitch_bend);
	value *= 500;
	value /= 12;	
	cv_write_dac(which, value>>8);
}

////////////////////////////////////////////////////////////
// WRITE A 7-BIT VELOCITY VALUE TO A CV OUTPUT
void cv_write_vel(byte which, long value) {
//TODO: scaling
	value *= 500;
	value /= 12;	
	cv_write_dac(which, value);
}

////////////////////////////////////////////////////////////
// WRITE A 7-BIT CC VALUE TO A CV OUTPUT
void cv_write_cc(byte which, long value) {
	if(value > 127) 
		value = 127;
	if(value < 0) 
		value = 0;
	value *= 20;
	cv_write_dac(which, value);
}

////////////////////////////////////////////////////////////
// WRITE PITCH BEND VALUE TO A CV OUTPUT
// pitch bend value is MIDI notes * 256
void cv_write_bend(byte which, long value) {
//TODO: scaling
	value >>= 2;
	value += 2048;
	cv_write_dac(which, value);
}

////////////////////////////////////////////////////////////
// WRITE VOLTS
void cv_write_volts(byte which, byte value) {
	cv_write_dac(which, (int)value * 500);
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
						cv_write_vel(which_cv, pstack->vel);
						break;
				}
				break;
			
			/////////////////////////////////////////////
			// CV OUTPUT TIED TO PITCH BEND
			case CV_PB:	
				if(event == EV_BEND) {
					cv_write_bend(which_cv, pstack->bend);
				}
				break;
			};
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
		if(cc != pcv->cc.cc) {
			continue;
		}
		// does MIDI channel match
		if(!IS_CHAN(pcv->cc.chan,chan)) {
			continue;
		}		
		// OK update the output
		cv_write_cc(which_cv, value);
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
		cv_write_dac(which_cv, (int)(value/30));
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
		case NRPVH_SRC_VOLTS:	// REFERENCE VOLTAGE
			pcv->event.mode = CV_VOLTS;
			pcv->volts.volts = value_lo;
			cv_write_volts(which_cv, value_lo); 
			return 1;
		case NRPVH_SRC_DISABLE:	// DISABLE
			cv_write_volts(which_cv, 0); 
			pcv->event.mode = CV_DISABLE;
			return 1;
		case NRPVH_SRC_MIDITICK: // BPM
			pcv->event.mode = CV_MIDI_BPM;
			return 1;
		case NRPVH_SRC_MIDICC: // CC
			pcv->event.mode = CV_MIDI_CC;
			pcv->cc.chan = CHAN_GLOBAL;
			pcv->cc.cc = value_lo;
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
				return 1;
			case NRPVL_SRC_PB:		// PITCH BEND
				pcv->event.mode = CV_PB;
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
	case NRPNL_CV_OFFSET:
	case NRPNL_CV_GAIN:
		if(CV_VOLTS == pcv->event.mode) {			
			char val = value_hi? value_lo : -value_lo;
			if(param_lo == NRPNL_CV_OFFSET) {
				g_cal_ofs[which_cv] = val;
			}
			else {
				g_cal_gain[which_cv] = val;
			}
			//TODO: EEPROM
			cv_write_volts(which_cv, pcv->volts.volts); 
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
	i2c_init();
	cv_config_dac();
	
	g_cv[0].event.mode = CV_NOTE;
	g_cv[0].event.stack_id = 0;
	g_cv[0].event.out = 0;	
	
	g_cv[1].event.mode = CV_MIDI_BPM;
	
}



