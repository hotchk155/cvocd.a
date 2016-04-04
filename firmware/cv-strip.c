////////////////////////////////////////////////////
//
// MIDI CV STRIP
//
// hotchk155/2016
//
////////////////////////////////////////////////////

/*
TODO: 
S-Gate support
CV tuning
*/

//
// HEADER FILES
//
#include <system.h>
#include <rand.h>
#include <eeprom.h>
#include "cv-strip.h"

// PIC CONFIG BITS
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 16000000


//
// MACRO DEFS
//

#define TIMER_0_INIT_SCALAR		5		// Timer 0 is an 8 bit timer counting at 250kHz
#define SZ_RXBUFFER 			64		// size of MIDI receive buffer (power of 2)
#define SZ_RXBUFFER_MASK 		0x3F	// mask to keep an index within range of buffer

//
// LOCAL DATA
//

// define the buffer used to receive MIDI input
volatile byte rx_buffer[SZ_RXBUFFER];
volatile byte rx_head = 0;
volatile byte rx_tail = 0;

// MIDI input state
byte midi_status = 0;
byte midi_num_params = 0;
byte midi_params[2];
char midi_param = 0;

byte midi_ticks = 0;

// once per millisecond tick flag
volatile byte ms_tick = 0;
volatile int millis = 0;

//
// GLOBAL DATA
//
char g_led_1_timeout = 0;
char g_led_2_timeout = 0;




////////////////////////////////////////////////////////////
// INTERRUPT SERVICE ROUTINE
void interrupt( void )
{
	/////////////////////////////////////////////////////
	// TIMER0 OVERFLOW
	// once per millisecond
	if(intcon.2)
	{
		tmr0 = TIMER_0_INIT_SCALAR;
		ms_tick = 1;
		++millis;
		intcon.2 = 0;		
	}		
	
	/////////////////////////////////////////////////////
	// SERIAL PORT RECEIVE
	if(pir1.5)
	{	
		byte b = rcreg;
		byte next_head = (rx_head + 1)&SZ_RXBUFFER_MASK;
		if(next_head != rx_tail) {
			rx_buffer[rx_head] = b;
			rx_head = next_head;
		}
		LED_1_PULSE(LED_PULSE_MIDI_IN);
		pir1.5 = 0;
	}
	
}

////////////////////////////////////////////////////////////
// INITIALISE TIMER
void timer_init() {
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 4MHz
	// 	prescaled 1/16 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	
	option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	option_reg.3 = 0; // timer 0 is prescaled
	option_reg.2 = 0; // }
	option_reg.1 = 1; // } 1/16 prescaler
	option_reg.0 = 1; // }
	intcon.5 = 1; 	  // enabled timer 0 interrrupt
	intcon.2 = 0;     // clear interrupt fired flag
}

////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void uart_init()
{
	pir1.1 = 0;		//TXIF 		
	pir1.5 = 0;		//RCIF
	
	pie1.1 = 0;		//TXIE 		no interrupts
	pie1.5 = 1;		//RCIE 		enable
	
	baudcon.4 = 0;	// SCKP		synchronous bit polarity 
	baudcon.3 = 1;	// BRG16	enable 16 bit brg
	baudcon.1 = 0;	// WUE		wake up enable off
	baudcon.0 = 0;	// ABDEN	auto baud detect
		
	txsta.6 = 0;	// TX9		8 bit transmission
	txsta.5 = 0;	// TXEN		transmit enable
	txsta.4 = 0;	// SYNC		async mode
	txsta.3 = 0;	// SEDNB	break character
	txsta.2 = 0;	// BRGH		high baudrate 
	txsta.0 = 0;	// TX9D		bit 9

	rcsta.7 = 1;	// SPEN 	serial port enable
	rcsta.6 = 0;	// RX9 		8 bit operation
	rcsta.5 = 1;	// SREN 	enable receiver
	rcsta.4 = 1;	// CREN 	continuous receive enable
		
	spbrgh = 0;		// brg high byte
	spbrg = 31;		// brg low byte (31250)	
	
}

////////////////////////////////////////////////////////////
// GET MESSAGES FROM MIDI INPUT
byte midi_in()
{
	// loop until there is no more data or
	// we receive a full message
	for(;;)
	{
		// usart buffer overrun error?
		if(rcsta.1)
		{
			rcsta.4 = 0;
			rcsta.4 = 1;
		}
		
		// check for empty receive buffer
		if(rx_head == rx_tail)
			return 0;
		
		// read the character out of buffer
		byte ch = rx_buffer[rx_tail];
		++rx_tail;
		rx_tail&=SZ_RXBUFFER_MASK;

		// REALTIME MESSAGE
		if((ch & 0xf0) == 0xf0)
		{
			// only clock messages are of interest
			switch(ch)
			{
			case MIDI_SYNCH_TICK:
			case MIDI_SYNCH_START:
			case MIDI_SYNCH_CONTINUE:
			case MIDI_SYNCH_STOP:
				return ch;			
			}
		}      
		// CHANNEL STATUS MESSAGE
		else if(!!(ch & 0x80))
		{
			midi_param = 0;
			midi_status = ch; 
			switch(ch & 0xF0)
			{
			case 0xA0: //  Aftertouch  1  key  touch  
			case 0xC0: //  Patch change  1  instrument #   
			case 0xD0: //  Channel Pressure  1  pressure  
				midi_num_params = 1;
				break;    
			case 0x80: //  Note-off  2  key  velocity  
			case 0x90: //  Note-on  2  key  veolcity  
			case 0xB0: //  Continuous controller  2  controller #  controller value  
			case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
			default:
				midi_num_params = 2;
				break;        
			}
		}    
		else if(midi_status)
		{
			// gathering parameters
			midi_params[midi_param++] = ch;
			if(midi_param >= midi_num_params)
			{
				// we have a complete message.. is it one we care about?
				midi_param = 0;
				switch(midi_status&0xF0)
				{
				case 0x80: // note off
				case 0x90: // note on
				case 0xE0: // pitch bend
				case 0xB0: // cc
					return midi_status; 
				}
			}
		}
	}
	// no message ready yet
	return 0;
}

////////////////////////////////////////////////////////////
// CONFIGURATION BY NRPN
void nrpn(byte param_hi, byte param_lo, byte value_hi, byte value_lo) {
	byte result = 0;
	switch(param_hi) {
		case NRPNH_GLOBAL:
			result = global_nrpn(param_lo, value_hi, value_lo);
			break;
		case NRPNH_STACK1:
		case NRPNH_STACK2:
		case NRPNH_STACK3:
		case NRPNH_STACK4:
			result = stack_nrpn(param_hi-NRPNH_STACK1, param_lo, value_hi, value_lo);
			break;
		case NRPNH_GATE1:
		case NRPNH_GATE2:
		case NRPNH_GATE3:
		case NRPNH_GATE4:
		case NRPNH_GATE5:
		case NRPNH_GATE6:
		case NRPNH_GATE7:
		case NRPNH_GATE8:
		case NRPNH_GATE9:
		case NRPNH_GATE10:
		case NRPNH_GATE11:
		case NRPNH_GATE12:
			result = gate_nrpn(param_hi-NRPNH_GATE1, param_lo, value_hi, value_lo);
			break;
		case NRPNH_CV1:
		case NRPNH_CV2:
		case NRPNH_CV3:
		case NRPNH_CV4:
			result = cv_nrpn(param_hi-NRPNH_CV1, param_lo, value_hi, value_lo);
			break;
	}
}

////////////////////////////////////////////////////////////
// MAIN
void main()
{ 	
	int bend;
	byte nrpn_hi = 0;
	byte nrpn_lo = 0;
	byte nrpn_value_hi = 0;
	
	// osc control / 16MHz / internal
	osccon = 0b01111010;

	trisa 	= TRIS_A;              	
    trisc 	= TRIS_C;   	
	ansela 	= 0b00000000;
	anselc 	= 0b00000000;
	porta 	= 0b00000000;
	portc 	= 0b00000000;

	// initialise the various modules
	uart_init();
	timer_init();	
	stack_init();
	gate_init();	
	cv_init(); 
	preset1();	

	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE


	g_led_1_timeout = 200;
	g_led_2_timeout = 200;
	//P_LED1 = 1;
	//P_LED2 = 1;

	// App loop
	long tick_time = 0; // milliseconds between ticks x 256
	for(;;)
	{
		// run the gate timeouts every millisecond
		if(ms_tick) {
			ms_tick = 0;
			gate_run();
			if(g_led_1_timeout) {
				if(!--g_led_1_timeout) {
					P_LED1 = 0;
				}
			}
			if(g_led_2_timeout) {
				if(!--g_led_2_timeout) {
					P_LED2 = 0;
				}
			}
		//	P_LED1 = !!(x&0x80);
//			++x;
		}
		
		// poll for incoming MIDI data
		byte msg = midi_in();		
		switch(msg & 0xF0) {
			// REALTIME MESSAGE
			case 0xF0:
				switch(msg) {
				case MIDI_SYNCH_TICK:
					if(millis) {						 
						tick_time *= 7;
						tick_time >>= 3; // divide by 8
						// tick_time = 7/8 * tick_time + millis
						// .. crude smoothing of values. The result
						// is upscaled x 8
						tick_time += millis;
						millis = 0;
					}
					if(!midi_ticks) {
						LED_2_PULSE(LED_PULSE_MIDI_BEAT);						
						if(tick_time) {
							// bpm = 2500/tick period(ms)
							// tick_time is upscaled x 8
							// parameter needs to be upscale x 256
							cv_midi_bpm(((long)2500*8*256)/tick_time);
						}
					}
					if(++midi_ticks>=24) {
						midi_ticks = 0;
					}
					gate_midi_clock(msg);
					break;
				case MIDI_SYNCH_START:
					midi_ticks = 0;
					// fall thru
				case MIDI_SYNCH_CONTINUE:
				case MIDI_SYNCH_STOP:
					gate_midi_clock(msg);
					break;	
				}
				break;
					
			// MIDI NOTE OFF
			case 0x80:
				stack_midi_note(msg&0x0F, midi_params[0], 0);
				//gate_midi_note(msg&0x0F, midi_params[0], 0);
				break;
			// MIDI NOTE ON
			case 0x90:
				stack_midi_note(msg&0x0F, midi_params[0], midi_params[1]);
				//gate_midi_note(msg&0x0F, midi_params[0], midi_params[1]);
				break;
				
			// CONTINUOUS CONTROLLER
			case 0xB0: 
				switch(midi_params[0]) {
					case MIDI_CC_NRPN_HI:
						nrpn_hi = midi_params[1];
						nrpn_lo = 0;
						nrpn_value_hi = 0;
						break;
					case MIDI_CC_NRPN_LO:
						nrpn_lo = midi_params[1];
						nrpn_value_hi = 0;
						break;
					case MIDI_CC_DATA_HI:
						nrpn_value_hi = midi_params[1];
						break;
					case MIDI_CC_DATA_LO:
						nrpn(nrpn_hi, nrpn_lo, nrpn_value_hi, midi_params[1]);
						break;
					default:
						cv_midi_cc(msg&0x0F, midi_params[0], midi_params[1]);
						gate_midi_cc(msg&0x0F, midi_params[0], midi_params[1]);
						break;
				}
				break;
				
			// PITCH BEND
			case 0xE0: 
				bend = (int)midi_params[0]<<7|(midi_params[1]&0x7F)-8192;	
				stack_midi_bend(msg&0x0F, bend);
				break;
		}
	}
}



