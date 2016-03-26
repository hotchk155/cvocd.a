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
// GLOBAL DATA
//
byte g_chan = 0; // default MIDI channel
byte g_accent_vel = VEL_ACCENT; // accent velocity

//
// LOCAL DATA
//

// define the buffer used to receive MIDI input
volatile byte rx_buffer[SZ_RXBUFFER];
volatile byte rx_head = 0;
volatile byte rx_tail = 0;

// MIDI input state
byte midi_status;
byte midi_num_params;
byte midi_params[2];
char midi_param;


char ledTime = 0;

// once per millisecond tick flag
volatile byte ms_tick = 0;

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
		rx_tail = (rx_tail + 1)%SZ_RXBUFFER_MASK;

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
// MAIN
void main()
{ 	
	// osc control / 16MHz / internal
	osccon = 0b01111010;

	// configure io
	trisa = 0b00000000;              	
    trisc = 0b00110000;   	
	ansela = 0b00000000;
	anselc = 0b00000000;
	porta=0;
	portc=0;

	// initialise the various modules
	uart_init();
	timer_init();	
	stack_init();
	gate_init();
	cv_init(); 

	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE


	// App loop
	for(;;)
	{
		// run the gate timeouts every millisecond
		if(ms_tick) {
			gate_run();
			ms_tick = 0;
		}
		
		// poll for incoming MIDI data
		byte msg = midi_in();		
		switch(msg & 0xF0) {
			// REALTIME CLOCK MESSAGE
			case MIDI_SYNCH_TICK:
			case MIDI_SYNCH_START:
			case MIDI_SYNCH_CONTINUE:
			case MIDI_SYNCH_STOP:
				gate_midi_clock(msg);
				break;		
			// MIDI NOTE ON
			case 0x80:
				stack_midi_note(msg&0x0F, midi_params[0], midi_params[1]);
				gate_midi_note(msg&0x0F, midi_params[0], midi_params[1]);
				break;
			// MIDI NOTE OFF
			case 0x90:
				stack_midi_note(msg&0x0F, midi_params[0], 0);
				gate_midi_note(msg&0x0F, midi_params[0], 0);
				break;
			// CONTINUOUS CONTROLLER
			case 0xB0: 
				cv_midi_cc(msg&0x0F, midi_params[0], midi_params[1]);
				gate_midi_cc(msg&0x0F, midi_params[0], midi_params[1]);
				break;
			// PITCH BEND
			case 0xE0: 
				stack_bend(msg&0x0F, midi_params[0], midi_params[1]);
				break;
		}				
	}
}



