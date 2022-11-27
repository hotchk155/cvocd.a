
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

// PIC16F1825 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = OFF       // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable (PWRT disabled)
#pragma config MCLRE = OFF      // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP = OFF         // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = OFF        // Low-Voltage Programming Enable (High-voltage on MCLR/VPP must be used for programming)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define _XTAL_FREQ 16000000
#include <xc.h>
//////////////////////////////////////////////////////////////
//
// MAIN MODULE
//
//////////////////////////////////////////////////////////////

//
// HEADER FILES
//
#include "cvocd.h"


// PIC CONFIG BITS
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
//// - INTERNAL OSC
//#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
//#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
//#pragma CLOCK_FREQ 16000000

//
// TYPES
//

// States for sysex loading
enum {
	SYSEX_NONE,		// no sysex
	SYSEX_IGNORE,	// sysex in progress, but not for us
	SYSEX_ID0,		// expect first byte of id
	SYSEX_ID1,		// expect second byte of id
	SYSEX_ID2,		// expect third byte of id
	SYSEX_PARAMH,	// expect high byte of a param number
	SYSEX_PARAML,	// expect low byte of a param number
	SYSEX_VALUEH,	// expect high byte of a param value
	SYSEX_VALUEL	// expect low byte of a param value
};

//
// LOCAL DATA
//

// define the buffer used to receive MIDI input
#define SZ_RXBUFFER 			64		// size of MIDI receive buffer (power of 2)
#define SZ_RXBUFFER_MASK 		0x3F	// mask to keep an index within range of buffer
volatile byte rx_buffer[SZ_RXBUFFER];	// the MIDI receive buffer
volatile byte rx_head = 0;				// buffer data insertion index
volatile byte rx_tail = 0;				// buffer data retrieval index

// State flags used while receiving MIDI data
byte midi_status = 0;					// current MIDI message status (running status)
byte midi_num_params = 0;				// number of parameters needed by current MIDI message
byte midi_params[2];					// parameter values of current MIDI message
char midi_param = 0;					// number of params currently received
byte midi_ticks = 0;					// number of MIDI clock ticks received
byte sysex_state = SYSEX_NONE;			// whether we are currently inside a sysex block

// Timer related stuff
#define TIMER_0_INIT_SCALAR		5		// Timer 0 initialiser to overlow at 1ms intervals
volatile byte ms_tick = 0;				// once per millisecond tick flag used to synchronise stuff
//volatile int millis = 0;				// millisecond counter

byte nrpn_hi = 0;						// value of last NRPN param high byte			
byte nrpn_lo = 0;						// value of last NRPN param low byte
byte nrpn_value_hi = 0;					// value of last NRPN value high byte

//
// GLOBAL DATA
//
volatile byte g_cv_dac_pending;				// flag to say whether dac data is pending
volatile unsigned int g_sr_data = 0;		// gate data to load to shift registers
volatile unsigned int g_sr_retrigs = 0;		// shift register bits to send low before next load
volatile byte g_sr_data_pending = 0;		// indicates if any gate data is pending
volatile unsigned int g_sync_sr_data = 0;	// additional gate bits, synced to CV load
volatile byte g_sync_sr_data_pending = 0;	// indicates if any synched gate data is pending

volatile byte g_i2c_tx_buf[I2C_TX_BUF_SZ];	// transmit buffer for i2c
volatile byte g_i2c_tx_buf_index = 0;		// index of next byte to send over i2c
volatile byte g_i2c_tx_buf_len = 0;			// total number of bytes in buffer

byte g_led_1_timeout = 0;					// ms after which LED1 is turned off
byte g_led_2_timeout = 0;					// ms after which LED1 is turned off

////////////////////////////////////////////////////////////
// INTERRUPT SERVICE ROUTINE
void __interrupt() ISR()
{
	/////////////////////////////////////////////////////
	// TIMER0 OVERFLOW
	// once per millisecond
    
	//if(intcon.2)
	if(INTCONbits.TMR0IF)
	{
		//tmr0 = TIMER_0_INIT_SCALAR;
		TMR0 = TIMER_0_INIT_SCALAR;
		ms_tick = 1;
//		++millis;
		//intcon.2 = 0;	
        INTCONbits.TMR0IF = 0;
	}		
	
	/////////////////////////////////////////////////////
	// UART RECEIVE
//	if(pir1.5)
	if(PIR1bits.RCIF)
	{	
		byte b = RCREG;
		//byte b = rcreg;
		byte next_head = (rx_head + 1)&SZ_RXBUFFER_MASK;
		if(next_head != rx_tail) {
			rx_buffer[rx_head] = b;
			rx_head = next_head;
		}
		LED_1_PULSE(LED_PULSE_MIDI_IN);
		//pir1.5 = 0;
        //PIR1bits.RCIF = 0; read only
	}

	/////////////////////////////////////////////////////
	// I2C INTERRUPT
	//if(pir1.3) 
	if(PIR1bits.SSP1IF) 
	{
		//pir1.3 = 0;
        PIR1bits.SSP1IF = 0;
		if(g_i2c_tx_buf_index < g_i2c_tx_buf_len) {
			// send next data byte
			//ssp1buf = g_i2c_tx_buf[g_i2c_tx_buf_index++];
			SSP1BUF = g_i2c_tx_buf[g_i2c_tx_buf_index++];
		}
		else if(g_i2c_tx_buf_index == g_i2c_tx_buf_len) {
			++g_i2c_tx_buf_index;			
			//ssp1con2.2 = 1; // send stop condition
            SSP1CON2bits.PEN = 1;
		}
		else {			
			// check if there is any synchronised gate data (This mechanism is designed to trigger
			// a gate associated with a note only after the CV has been output to the DAC, so the 
			// gate does not open before the note CV sweeps to the new value)
			if(g_sync_sr_data_pending) {
				g_sr_data |= g_sync_sr_data;	// set the new gates
				g_sync_sr_data = 0;				// no syncronised data pending now..
				g_sync_sr_data_pending = 0;		
				g_sr_data_pending = 1;			// but we do need to load the new info to shift regs
			}			
			//pie1.3 = 0; // we're done - disable the I2C interrupt
            PIE1bits.SSP1IE = 0;
		}
	}
}

////////////////////////////////////////////////////////////
// I2C MASTER INIT
static void i2c_init() {
	// disable output drivers on i2c pins
	//trisc.0 = 1;
	//trisc.1 = 1;
    TRISCbits.TRISC0 = 1;
    TRISCbits.TRISC1 = 1;
	
    SSP1CON1bits.SSPEN = 1;
    SSP1CON1bits.CKP = 1;
    SSP1CON1bits.SSPM3 = 1;
    SSP1CON1bits.SSPM2 = 0;
    SSP1CON1bits.SSPM1 = 0;
    SSP1CON1bits.SSPM0 = 0;
    
	//ssp1con1.7 = 
	//ssp1con1.6 = 
	//ssp1con1.5 = 1; // Enable synchronous serial port
	//ssp1con1.4 = 1; // Enable SCL
	//ssp1con1.3 = 1; // }
	//ssp1con1.2 = 0; // }
	//ssp1con1.1 = 0; // }
	//ssp1con1.0 = 0; // } I2C Master with clock = Fosc/(4(SSPxADD+1))

    //ssp1stat.7 = 1;	// slew rate disabled	
    SSP1STATbits.SMP = 1;
	//ssp1add = 19;	// 100kHz baud rate
    SSP1ADD = 19;
}

////////////////////////////////////////////////////////////
// I2C WRITE BYTE TO BUS
void i2c_send(byte data) {
	//ssp1buf = data;
	SSP1BUF = data;
	//while((ssp1con2 & 0b00011111) || // SEN, RSEN, PEN, RCEN or ACKEN
	//	(ssp1stat.2)); // data transmit in progress	
	while((SSP1CON2 & 0b00011111) || // SEN, RSEN, PEN, RCEN or ACKEN
		(SSP1STATbits.R_nW)); // data transmit in progress	
}

////////////////////////////////////////////////////////////
// I2C START WRITE MESSAGE TO A SLAVE
void i2c_begin_write(byte address) {
	//pir1.3 = 0; // clear SSP1IF
	PIR1bits.SSP1IF = 0;
	//ssp1con2.0 = 1; // signal start condition
    SSP1CON2bits.SEN = 1;
	//while(!pir1.3); // wait for it to complete
	while(!PIR1bits.SSP1IF); // wait for it to complete
	i2c_send((byte)(address<<1)); // address + WRITE(0) bit
}

////////////////////////////////////////////////////////////
// I2C FINISH MESSAGE
void i2c_end() {
	//pir1.3 = 0; // clear SSP1IF
	PIR1bits.SSP1IF = 0;
	//ssp1con2.2 = 1; // signal stop condition
    SSP1CON2bits.PEN = 1;
	//while(!pir1.3); // wait for it to complete
	while(!PIR1bits.SSP1IF); // wait for it to complete
}

////////////////////////////////////////////////////////////
// I2C ASYNC SEND
void i2c_send_async() {
	//pir1.3 = 0; // clear interrupt flag
	PIR1bits.SSP1IF = 0;
	//pie1.3 = 1; // enable the interrupt
	PIE1bits.SSP1IE = 0;
	//ssp1con2.0 = 1; // signal start condition					
    SSP1CON2bits.SEN = 1;
}

////////////////////////////////////////////////////////////
// INITIALISE TIMER
void timer_init() {
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 4MHz
	// 	prescaled 1/16 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	
	//option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	//option_reg.3 = 0; // timer 0 is prescaled
	//option_reg.2 = 0; // }
	//option_reg.1 = 1; // } 1/16 prescaler
	//option_reg.0 = 1; // }
	//intcon.5 = 1; 	  // enabled timer 0 interrrupt
	//intcon.2 = 0;     // clear interrupt fired flag
    
    OPTION_REGbits.TMR0CS = 0;
    OPTION_REGbits.PSA = 0;
    OPTION_REGbits.PS2 = 0;
    OPTION_REGbits.PS1 = 1;
    OPTION_REGbits.PS0 = 1;
    
    INTCONbits.T0IE = 1;
    INTCONbits.T0IF = 0;
}

////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void uart_init()
{

	//pir1.1 = 0;		//TXIF 		
    PIR1bits.TXIF = 0;
	//pir1.5 = 0;		//RCIF
    PIR1bits.RCIF = 0;
	
	//pie1.1 = 0;		//TXIE 		no interrupts
    PIE1bits.TXIE = 0;
	//pie1.5 = 1;		//RCIE 		enable
    PIE1bits.RCIE = 1;
	
	//baudcon.4 = 0;	// SCKP		synchronous bit polarity 
    BAUDCONbits.SCKP = 0;
	//baudcon.3 = 1;	// BRG16	enable 16 bit brg
    BAUDCONbits.BRG16 = 1;
	//baudcon.1 = 0;	// WUE		wake up enable off
    BAUDCONbits.WUE = 0;
	//baudcon.0 = 0;	// ABDEN	auto baud detect
    BAUDCONbits.ABDEN = 0;
		
	//txsta.6 = 0;	// TX9		8 bit transmission
    TXSTAbits.TX9 = 0;
	//txsta.5 = 0;	// TXEN		transmit enable
    TXSTAbits.TXEN = 0;
	//txsta.4 = 0;	// SYNC		async mode
    TXSTAbits.SYNC = 0;
	//txsta.3 = 0;	// SEDNB	break character
    TXSTAbits.SENDB = 0;
	//txsta.2 = 0;	// BRGH		high baudrate 
    TXSTAbits.BRGH = 0;
	//txsta.0 = 0;	// TX9D		bit 9
    TXSTAbits.TX9D = 0;

	//rcsta.7 = 1;	// SPEN 	serial port enable
    RCSTAbits.SPEN = 1;
	//rcsta.6 = 0;	// RX9 		8 bit operation
    RCSTAbits.RX9 = 0;
	//rcsta.5 = 1;	// SREN 	enable receiver
    RCSTAbits.SREN = 1;
	//rcsta.4 = 1;	// CREN 	continuous receive enable
    RCSTAbits.CREN = 1;
		
	//spbrgh = 0;		// brg high byte
    SPBRGH = 0;
	//spbrg = 31;		// brg low byte (31250)	
    SPBRG = 31;
	
}

////////////////////////////////////////////////////////////
// LOAD GATE SHIFT REGISTER
void sr_write(unsigned int nmask) {
	unsigned int d = g_sr_data & ~nmask; // nmask is a bit set to force to LOW
	unsigned int m1 = 0x0080;
	unsigned int m2 = 0x8000;
	P_SRLAT = 0;
	while(m1) {
		P_SRCLK = 0;
		P_SRDAT1 = !!(d&m1);
		P_SRDAT2 = !!(d&m2);
		P_SRCLK = 1;
		m1>>=1;
		m2>>=1;
	}
	P_SRLAT = 1;
}

////////////////////////////////////////////////////////////
// RESET STATES
void all_reset()
{
	gate_reset();
	cv_reset();
	stack_reset();
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
		//if(rcsta.1)
		//{
		//	rcsta.4 = 0;
		//	rcsta.4 = 1;
		//}
		if(RCSTAbits.OERR)
		{
            RCSTAbits.CREN = 0;
            RCSTAbits.CREN = 1;
			//rcsta.4 = 0;
			//rcsta.4 = 1;
		}
		
		// check for empty receive buffer
		if(rx_head == rx_tail)
			return 0;
		
		// read the character out of buffer
		byte ch = rx_buffer[rx_tail];
		++rx_tail;
		rx_tail&=SZ_RXBUFFER_MASK;

		// SYSTEM MESSAGE
		if((ch & 0xf0) == 0xf0)
		{
			switch(ch)
			{
			// RELEVANT REALTIME MESSAGE 
			case MIDI_SYNCH_TICK:
			case MIDI_SYNCH_START:
			case MIDI_SYNCH_CONTINUE:
			case MIDI_SYNCH_STOP:
				return ch;		
			// SYSTEM COMMON MESSAGES WITH PARAMETERS
			case MIDI_MTC_QTR_FRAME:	// 1 param byte follows
			case MIDI_SONG_SELECT:		// 1 param byte follows			
			case MIDI_SPP:				// 2 param bytes follow
				midi_param = 0;
				midi_status = ch; 
				midi_num_params = (ch==MIDI_SPP)? 2:1;
				break;
			// START OF SYSEX	
			case MIDI_SYSEX_BEGIN:
				sysex_state = SYSEX_ID0; 
				break;
			// END OF SYSEX	
			case MIDI_SYSEX_END:
				switch(sysex_state) {
				case SYSEX_IGNORE: // we're ignoring a syex block
				case SYSEX_NONE: // we weren't even in sysex mode!					
					break;			
				case SYSEX_PARAMH:	// the state we'd expect to end in
					P_LED1 = 1; 			
                    P_LED2 = 1; 
                    __delay_ms(250);
                    __delay_ms(250);
                    __delay_ms(250);
                    __delay_ms(250);
					//delay_ms(250); 
					//delay_ms(250); 
					//delay_ms(250); 
					//delay_ms(250); 
					P_LED1 = 0; 
					P_LED2 = 0; 
					storage_write_patch();	// store to EEPROM 
					all_reset();
					break;
				default:	// any other state would imply bad sysex data
					P_LED1 = 0; 
					for(char i=0; i<10; ++i) {
						P_LED2 = 1; 
                        __delay_ms(100);
						//delay_ms(100);
						P_LED2 = 0; 
                        __delay_ms(100);
						//delay_ms(100);
					}
					all_reset();
					break;
				}
				sysex_state = SYSEX_NONE; 
				break;
			}
			// Ignoring....			
			//  0xF4	RESERVED
			//  0xF5	RESERVED
			//  0xF6	TUNE REQUEST
			//	0xF9	RESERVED
			//	0xFD	RESERVED
			//	0xFE	ACTIVE SENSING
			//	0xFF	RESET
		}    
		// STATUS BYTE
		else if(!!(ch & 0x80))
		{
			// a status byte cancels sysex state
			sysex_state = SYSEX_NONE;
		
			midi_param = 0;
			midi_status = ch; 
			switch(ch & 0xF0)
			{
			case 0xC0: //  Patch change  1  instrument #   
			case 0xD0: //  Channel Pressure  1  pressure  
				midi_num_params = 1;
				break;    
			case 0xA0: //  Polyphonic aftertouch  2  key  touch  
			case 0x80: //  Note-off  2  key  velocity  
			case 0x90: //  Note-on  2  key  veolcity  
			case 0xB0: //  Continuous controller  2  controller #  controller value  
			case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
			default:
				midi_num_params = 2;
				break;        
			}
		}    
		else 
		{
			switch(sysex_state) // are we inside a sysex block?
			{
			// SYSEX MANUFACTURER ID
			case SYSEX_ID0: sysex_state = (ch == MY_SYSEX_ID0)? SYSEX_ID1 : SYSEX_IGNORE; break;
			case SYSEX_ID1: sysex_state = (ch == MY_SYSEX_ID1)? SYSEX_ID2 : SYSEX_IGNORE; break;
			case SYSEX_ID2: sysex_state = (ch == MY_SYSEX_ID2)? SYSEX_PARAMH : SYSEX_IGNORE; break;
			// CONFIG PARAM DELIVERED BY SYSEX
			case SYSEX_PARAMH: nrpn_hi = ch; ++sysex_state; break;
			case SYSEX_PARAML: nrpn_lo = ch; ++sysex_state;break;
			case SYSEX_VALUEH: nrpn_value_hi = ch; ++sysex_state;break;
			case SYSEX_VALUEL: nrpn(nrpn_hi, nrpn_lo, nrpn_value_hi, ch); sysex_state = SYSEX_PARAMH; break;
			case SYSEX_IGNORE: break;			
			// MIDI DATA
			case SYSEX_NONE: 
				if(midi_status)
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
						case 0xD0: // channel pressure
							return midi_status; 
						}
					}
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
	if(result) {
		LED_2_PULSE(LED_PULSE_PARAM);						
	}
}

////////////////////////////////////////////////////////////
// MAIN
void main()
{ 		
	// osc control / 16MHz / internal
	//osccon = 0b01111010;
    OSCCON = 0b01111010;

	//trisa 	= TRIS_A;              	
    TRISA = TRIS_A;
    //trisc 	= TRIS_C;   	
    TRISC = TRIS_C;
	//ansela 	= 0b00000000;
    ANSELA = 0b00000000;
	//anselc 	= 0b00000000;
    ANSELC = 0b00000000;
	//porta 	= 0b00000000;
    PORTA = 0b00000000;
	//portc 	= 0b00000000;
    PORTC = 0b00000000;

	// enable interrupts	
	//intcon.7 = 1; //GIE
    INTCONbits.GIE = 1;
	//intcon.6 = 1; //PEIE
    INTCONbits.PEIE = 1;

	g_cv_dac_pending = 0;
	nrpn_hi = 0;
	nrpn_lo = 0;
	nrpn_value_hi = 0;

	unsigned int button_press = 0;

	// flash both LEDs at startup
	LED_1_PULSE(255);
	LED_2_PULSE(255);

	// initialise the various modules
	uart_init();
	i2c_init();	
	timer_init();	
	global_init();
	stack_init();
	gate_init();	
	cv_init(); 
	storage_read_patch();	
	
	// reset them
	all_reset();

	// App loop
	int bend;
	long tick_time = 0; // milliseconds between ticks x 256
	for(;;)
	{	
		// once per millisecond tick event
		if(ms_tick) {
			ms_tick = 0;
			
			// update the gates...
			gate_run();
			
			// update LED1
			if(g_led_1_timeout) {
				if(!--g_led_1_timeout) {
					P_LED1 = 0;
				}
			}
			
			// update LED2
			if(g_led_2_timeout) {
				if(!--g_led_2_timeout) {
					P_LED2 = 0;
				}
			}
			
			if(!P_SWITCH) {
				++button_press;
				if(button_press == SHORT_BUTTON_PRESS) {
					all_reset();
					LED_2_PULSE(100);				
				}
				else if(button_press == LONG_BUTTON_PRESS) {
					P_LED2 = 1;
					storage_write_patch();
					LED_2_PULSE(255);				
				}					
			}
			else {
				button_press = 0;
			}
		}
		
		// poll for incoming MIDI data
		byte msg = midi_in();		
		switch(msg & 0xF0) {
		// REALTIME MESSAGE
		case 0xF0:
			switch(msg) {
			case MIDI_SYNCH_TICK:
				if(!midi_ticks) {
					LED_2_PULSE(LED_PULSE_MIDI_BEAT);				
//					if(millis>0) {		
//						cv_midi_bpm(((long)256*60000)/millis);					
//						millis = 0;
//					}
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
			gate_midi_note(msg&0x0F, midi_params[0], 0);
			break;
		// MIDI NOTE ON
		case 0x90:
			stack_midi_note(msg&0x0F, midi_params[0], midi_params[1]);
			gate_midi_note(msg&0x0F, midi_params[0], midi_params[1]);
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

		// AFTERTOUCH
		case 0xD0: 
			cv_midi_touch(msg&0x0F, midi_params[0]);
			break;

		// PITCH BEND
		case 0xE0: 
			bend = (int)midi_params[1]<<7|(midi_params[0]&0x7F);	
			stack_midi_bend(msg&0x0F, bend);
			cv_midi_bend(msg&0x0F, bend);
			break;
		}
				
		// check if there is any CV data to send out and no i2c transmit in progress
		//if(!pie1.3 && g_cv_dac_pending) {
		if(!PIE1bits.SSP1IE && g_cv_dac_pending) {
			cv_dac_prepare(); 
			i2c_send_async();
			g_cv_dac_pending = 0; 
		}				
		// check for retrigs.. if so all retrig bits will be sent low
		if(g_sr_retrigs) {
			sr_write(g_sr_retrigs);
			g_sr_retrigs = 0;
		}
		// check if there is any shift register data pending		
		if(g_sr_data_pending) {
			g_sr_data_pending = 0;
			sr_write(0);
		}			
	}
}

//
// END
//