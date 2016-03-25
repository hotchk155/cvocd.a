////////////////////////////////////////////////////////////
//
// MINI MIDI CV
//
// CONTROL VOLTAGE OUTPUTS
//
////////////////////////////////////////////////////////////

// INCLUDE FILES
#include <system.h>
#include "cv-strip.h"

// MACRO DEFS
#define I2C_ADDRESS 0b1100000

// FILE SCOPE DATA
enum {
	CV_OFF,
	CV_NOTE,
	CV_VEL,
	CV_CC
};

typedef struct {
	byte stack_id;
	byte out;
} T_CV_EVENT;

typedef struct {
	byte chan;
	byte cc;
} T_CV_MIDI_CC;

typedef struct {
	byte mode;	// CV_xxx enum
	union {
		T_CV_EVENT 		event;
		T_CV_MIDI_CC 	cc;
	}
} CV_OUT;

// cache of raw DAC data
int g_dac[4] = {0};

// CV status
CV_OUT g_cv[4] = {0};

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
	i2cWrite(address<<1); // address + WRITE(0) bit
}

////////////////////////////////////////////////////////////
// I2C FINISH MESSAGE
static void i2c_end() {
	pir1.3 = 0; // clear SSP1IF
	ssp1con2.2 = 1; // signal stop condition
	while(!pir1.3); // wait for it to complete
}

////////////////////////////////////////////////////////////
// WRITE TO DAC
// TODO: Use single DAC write
static void write_dac(byte which, int value) 
{
	// constrain to valid range
	if(value < 0) 
		value = 0;
	if(value > 4095) 
		value = 4095;
		
	// store new channel value
	g_dac[which] = value;
	
	// send all channels to DAC chip
	i2c_begin_write(I2C_ADDRESS);
	i2c_send((g_dac[1]>>8) & 0xF);
	i2c_send(g_dac[1] & 0xFF);
	i2c_send((g_dac[2]>>8) & 0xF);
	i2c_send(g_dac[2] & 0xFF);
	i2c_send((g_dac[3]>>8) & 0xF);
	i2c_send(g_dac[3] & 0xFF);
	i2c_send((g_dac[0]>>8) & 0xF);
	i2c_send(g_dac[0] & 0xFF);
	i2c_end();	
}

////////////////////////////////////////////////////////////
// WRITE A NOTE VALUE TO A CV OUTPUT
// pitch_bend units = MIDI note * 256
static void write_note(byte which, byte midi_note, int pitch_bend) 
{
	long value = (((long)midi_note)<<8 + pitch_bend);
	value *= 500;
	value /= 12;	
	write_dac(which, value>>8);
}

////////////////////////////////////////////////////////////
// WRITE A 7-BIT VELOCITY VALUE TO A CV OUTPUT
static void write_vel(byte which, long value) 
{
//TODO: scaling
	value *= 500;
	value /= 12;	
	write_dac(which, value);
}

////////////////////////////////////////////////////////////
// WRITE A 7-BIT CC VALUE TO A CV OUTPUT
static void write_cc(byte which, long value) 
{
//TODO: scaling
	value *= 500;
	value /= 12;	
	write_dac(which, value);
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
// HANDLE AN EVENT FROM A NOTE STACK
void cv_event(byte event, byte stack_id)
{
	byte i, output_id;
	
	// for each CV output
	for(i=0; i<4; ++i) {
		CV_OUT *pcv = &g_cv[i];
		
		// is it listening to the stack sending the event?
		if(pcv->event.stack_id == stack_id) {
			NOTE_STACK *pstack = &g_stack[i];
			
			// check the mode			
			switch(pcv->mode)
			{
			case CV_NOTE:	// Note on will set output note CV
				switch(event) {
					case EV_NOTEA:
					case EV_NOTEB:
					case EV_NOTEC:
					case EV_NOTED:
						output_id = event - EV_NOTEA;
						if(pcv->event.out == output_id) {			
							write_note(i, pstack->out[output_id], pstack->bend);
						}
						break;
				}
				break;
			case CV_VEL:	// Note on will set output velocity CV
				switch(event) {
					case EV_NOTEA:
					case EV_NOTEB:
					case EV_NOTEC:
					case EV_NOTED:
						write_vel(i, pstack->vel);
						break;
				}
				break;
			}
	}
}

////////////////////////////////////////////////////////////
// HANDLE A MIDI CC
void cv_midi_cc(byte chan, byte cc, byte value)
{
	byte i
	for(i=0; i<4; ++i) {
		CV_OUT *pcv = &g_cv[i];
		
		// is this CV output configured for CC?
		if(pcv->mode != CV_CC) {
			continue;
		}		

		// does the CC number match?
		if(cc != pcv->cc.cc) {
			continue;
		}

		// does the channel match the MIDI message?
		switch(pcv->cc.chan) {
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
		
		// OK update the output
		write_cc(i, value);
	}
}

////////////////////////////////////////////////////////////
// INITIALISE CV MODULE
void cv_init() 
{
	memset(g_dac, 0, sizeof g_dac);
	i2c_init();
}