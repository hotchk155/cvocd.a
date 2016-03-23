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

// PIC CONFIG BITS
// - RESET INPUT DISABLED
// - WATCHDOG TIMER OFF
// - INTERNAL OSC
#pragma DATA _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _MCLRE_OFF &_CLKOUTEN_OFF
#pragma DATA _CONFIG2, _WRT_OFF & _PLLEN_OFF & _STVREN_ON & _BORV_19 & _LVP_OFF
#pragma CLOCK_FREQ 8000000

//
// TYPE DEFS
//
typedef unsigned char byte;

//
// MACRO DEFS
//

#define P_LED1		lata.2
#define P_LED2		latc.2

#define P_SRDAT1	lata.0
#define P_SRDAT2	lata.1
#define P_SRCLK		lata.4
#define P_SRLAT		lata.5


// MIDI beat clock messages
#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc

#define SRB_GATA	0x0004
#define SRB_GATB 	0x0002
#define SRB_CLKA	0x0008
#define SRB_CLKB	0x0001
#define SRB_DRM1 	0x0100
#define SRB_DRM2 	0x0200
#define SRB_DRM3 	0x0400
#define SRB_DRM4 	0x0800
#define SRB_DRM5 	0x1000
#define SRB_DRM6 	0x2000
#define SRB_DRM7 	0x4000
#define SRB_DRM8 	0x8000

enum {
	GATE_A = 0,
	GATE_B,
	GATE_DRM1,
	GATE_DRM2,
	GATE_DRM3,
	GATE_DRM4,
	GATE_DRM5,
	GATE_DRM6,
	GATE_DRM7,
	GATE_DRM8,
	GATE_CLKA,
	GATE_CLKB,
	GATE_MAX
};

#define CHANNEL_A 	0
#define CHANNEL_B 	1
#define CHANNEL_DRUMS 	9

#define NOTE_DRUM1	0x3c
#define NOTE_DRUM2	0x3e
#define NOTE_DRUM3	0x40
#define NOTE_DRUM4	0x41
#define NOTE_DRUM5	0x42
#define NOTE_DRUM6	0x43
#define NOTE_DRUM7	0x44
#define NOTE_DRUM8	0x45

#define VEL_ACCENT 100

#define TRIG_PULSE 5 //ms
#define LED_TIME 10 //ms
//
// GLOBAL DATA
//

#define TIMER_0_INIT_SCALAR		5	// Timer 0 is an 8 bit timer counting at 250kHz

// define the buffer used to receive MIDI input
#define SZ_RXBUFFER 20
volatile byte rxBuffer[SZ_RXBUFFER];
volatile byte rxHead = 0;
volatile byte rxTail = 0;

volatile byte beatcount = 0;


// state variables
unsigned int srData = 0;
byte midiInRunningStatus;
byte midiNumParams;
byte midiParams[2];
char midiParamIndex;


char gateTime[GATE_MAX] = {0};
char ledTime = 0;

int cvPitchA = 0;
int cvPitchB = 0;
int cvVolA = 0;
int cvVolB = 0;
enum {
	CHAN_OMNI	= 0xFF,
};
enum {
	MODE_NOTE,
};


/*
Note stacking... allow four notes

2 x NOTE, 2 x VEL, 1 x CHAN
4 x NOTE, 1 x CHAN



2 x NOTE, 2 x VEL, 2 x CHAN
4 x NOTE, 4 x CHAN 



GATE VS TRIG
PITCH BEND
ACCENT




*/


typedef struct {
	byte chan;
	byte mode;
	byte param;
} CV_CONFIG;

typedef struct {
	byte chan;
	byte mode;
	byte param;
} GATE_CONFIG;


#define NUM_CV	4
#define NUM_GATE	12
CV_CONFIG CVConfig[NUM_CV];
GATE_CONFIG GateConfig[NUM_GATE];


volatile byte msTick = 0;
void interrupt( void )
{
	// TIMER0 OVERFLOW
	// Timer 0 overflow is used to 
	// create a once per millisecond
	// count
	if(intcon.2)
	{
		tmr0 = TIMER_0_INIT_SCALAR;
		msTick = 1;
		intcon.2 = 0;		
	}		
	// serial rx ISR
	if(pir1.5)
	{	
		// get the byte
		byte b = rcreg;

		// calculate next buffer head
		byte nextHead = (rxHead + 1);
		if(nextHead >= SZ_RXBUFFER) 
		{
			nextHead -= SZ_RXBUFFER;
		}
		
		// if buffer is not full
		if(nextHead != rxTail)
		{
			// store the byte
			rxBuffer[rxHead] = b;
			rxHead = nextHead;
		}		
		pir1.5 = 0;
	}
	
}


////////////////////////////////////////////////////////////
// INITIALISE SERIAL PORT FOR MIDI
void initUSART()
{
	pir1.1 = 1;		//TXIF 		
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
	spbrg = 15;		// brg low byte (31250)	
	
}

////////////////////////////////////////////////////////////
// RUN MIDI IN
byte midiIn()
{
	// loop until there is no more data or
	// we receive a full message
	for(;;)
	{
		// buffer overrun error?
		if(rcsta.1)
		{
			rcsta.4 = 0;
			rcsta.4 = 1;
		}
		// any data in the buffer?
		if(rxHead == rxTail)
		{
			// no data ready
			return 0;
		}
		
		// read the character out of buffer
		byte ch = rxBuffer[rxTail];
		if(++rxTail >= SZ_RXBUFFER) 
			rxTail -= SZ_RXBUFFER;

		// REALTIME MESSAGE
		if((ch & 0xf0) == 0xf0)
		{
			switch(ch)
			{
			case MIDI_SYNCH_TICK:
			case MIDI_SYNCH_START:
			case MIDI_SYNCH_CONTINUE:
			case MIDI_SYNCH_STOP:
			break;
			}
		}      
		// CHANNEL STATUS MESSAGE
		else if(!!(ch & 0x80))
		{
			midiParamIndex = 0;
			midiInRunningStatus = ch; 
			switch(ch & 0xF0)
			{
			case 0xA0: //  Aftertouch  1  key  touch  
			case 0xC0: //  Patch change  1  instrument #   
			case 0xD0: //  Channel Pressure  1  pressure  
				midiNumParams = 1;
				break;    
			case 0x80: //  Note-off  2  key  velocity  
			case 0x90: //  Note-on  2  key  veolcity  
			case 0xB0: //  Continuous controller  2  controller #  controller value  
			case 0xE0: //  Pitch bend  2  lsb (7 bits)  msb (7 bits)  
			default:
				midiNumParams = 2;
				break;        
			}
		}    
		else if(midiInRunningStatus)
		{
			// gathering parameters
			midiParams[midiParamIndex++] = ch;
			if(midiParamIndex >= midiNumParams)
			{
				midiParamIndex = 0;
				switch(midiInRunningStatus & 0xF0)
				{
				case 0x80: // note off
				case 0x90: // note on
					return midiInRunningStatus; // return to the arp engine
				default:
					break;
				}
			}
		}
	}
	return 0;
}

////////////////////////////////////////////////////////////
// I2C MASTER
////////////////////////////////////////////////////////////
void i2cInit() {
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
void i2cWrite(byte data) {
	ssp1buf = data;
	while((ssp1con2 & 0b00011111) || // SEN, RSEN, PEN, RCEN or ACKEN
		(ssp1stat.2)); // data transmit in progress	
}
void i2cBeginWrite(byte address) {
	pir1.3 = 0; // clear SSP1IF
	ssp1con2.0 = 1; // signal start condition
	while(!pir1.3); // wait for it to complete
	i2cWrite(address<<1); // address + WRITE(0) bit
}

void i2cEndMsg() {
	pir1.3 = 0; // clear SSP1IF
	ssp1con2.2 = 1; // signal stop condition
	while(!pir1.3); // wait for it to complete
}

void sendDAC(int n0, int v0, int n1, int v1) {

  i2cBeginWrite(0b1100000);
  i2cWrite((v0>>8) & 0xF);
  i2cWrite(v0 & 0xFF);
  i2cWrite((v1>>8) & 0xF);
  i2cWrite(v1 & 0xFF);
  i2cWrite((n1>>8) & 0xF);
  i2cWrite(n1 & 0xFF);
  i2cWrite((n0>>8) & 0xF);
  i2cWrite(n0 & 0xFF);
  i2cEndMsg();
}
void load_sr(unsigned int d) {
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
void startNote(byte which, byte note, byte vel) {
	long dacNote = ((long)note * 500)/12;
	long dacVol = ((long)vel * 2500)/127;
	if(dacNote < 0) dacNote = 0;
	if(dacNote > 4095) dacNote = 4095;
	if(dacVol< 0) dacVol = 0;
	if(dacVol > 4095) dacVol = 4095;
	if(which == 1) {
		cvPitchB = dacNote;
		cvVolB = dacVol;
		srData |= SRB_GATB;
		load_sr(srData);
	}
	else {
		cvPitchA = dacNote;
		cvVolA = dacVol;
		srData |= SRB_GATA;
		load_sr(srData);
	}
	sendDAC(cvPitchA, cvVolA, cvPitchB, cvVolB);
}

void stopNote(byte which) {
	if(which == 1) {
		cvVolB = 0;
		srData &= ~SRB_GATB;
		load_sr(srData);
	}
	else {
		cvVolA = 0;
		srData &= ~SRB_GATA;
		load_sr(srData);
	}
	sendDAC(cvPitchA, cvVolA, cvPitchB, cvVolB);
}

void initTimer0() {
	// Configure timer 0 (controls systemticks)
	// 	timer 0 runs at 2MHz
	// 	prescaled 1/8 = 250kHz
	// 	rollover at 250 = 1kHz
	// 	1ms per rollover	
	option_reg.5 = 0; // timer 0 driven from instruction cycle clock
	option_reg.3 = 0; // timer 0 is prescaled
	option_reg.2 = 0; // }
	option_reg.1 = 1; // } 1/16 prescaler
	option_reg.0 = 0; // }
	intcon.5 = 1; 	  // enabled timer 0 interrrupt
	intcon.2 = 0;     // clear interrupt fired flag	
}

void trigPulse(int which, int ms) {
	unsigned int d = srData;
	switch(which) {
		case GATE_A: 	srData |= SRB_GATA; break;
		case GATE_B: 	srData |= SRB_GATB; break;
		case GATE_DRM1: srData |= SRB_DRM1; break;
		case GATE_DRM2: srData |= SRB_DRM2; break;
		case GATE_DRM3: srData |= SRB_DRM3; break;
		case GATE_DRM4: srData |= SRB_DRM4; break;
		case GATE_DRM5: srData |= SRB_DRM5; break;
		case GATE_DRM6: srData |= SRB_DRM6; break;
		case GATE_DRM7: srData |= SRB_DRM7; break;
		case GATE_DRM8: srData |= SRB_DRM8; break;
		case GATE_CLKA: srData |= SRB_CLKA; break;
		case GATE_CLKB: srData |= SRB_CLKB; break;
		default: return;
	}	
	if(d != srData) {
		load_sr(srData);
	}
	gateTime[which] = ms;
}



////////////////////////////////////////////////////////////
// MAIN
void main()
{ 	
	// osc control / 8MHz / internal
	osccon = 0b01110010;


	// configure io
	trisa = 0b00000000;              	
    trisc = 0b00110000;   	
	ansela = 0b00000000;
	anselc = 0b00000000;
	porta=0;
	portc=0;


	// initialise MIDI comms
	initUSART();
initTimer0();

i2cInit();
//load_sr(0);
	
	// enable interrupts	
	intcon.7 = 1; //GIE
	intcon.6 = 1; //PEIE


//	unsigned int a=0x8000;
//	for(;;) {
		//load_sr(a);
		//delay_ms(100);
		//load_sr(0);
		//delay_ms(100);
	//}

	// App loop
	int i=0;
	for(;;)
	{
		if(msTick) {
			unsigned int d = srData;
			for(int g = 0; g<GATE_MAX; ++g) {
				if(gateTime[g]) {
					if(--gateTime[g] == 0) {
						switch(g) {
							case GATE_A: srData &= ~SRB_GATA; break;
							case GATE_B: srData &= ~SRB_GATB; break;
							case GATE_DRM1: srData &= ~SRB_DRM1; break;
							case GATE_DRM2: srData &= ~SRB_DRM2; break;
							case GATE_DRM3: srData &= ~SRB_DRM3; break;
							case GATE_DRM4: srData &= ~SRB_DRM4; break;
							case GATE_DRM5: srData &= ~SRB_DRM5; break;
							case GATE_DRM6: srData &= ~SRB_DRM6; break;
							case GATE_DRM7: srData &= ~SRB_DRM7; break;
							case GATE_DRM8: srData &= ~SRB_DRM8; break;
							case GATE_CLKA: srData &= ~SRB_CLKA; break;
							case GATE_CLKB: srData &= ~SRB_CLKB; break;
						}
					}
				}
			}
			if(d != srData) {
				load_sr(srData);
			}
			if(ledTime && !--ledTime) {
				P_LED1 = 0;
			}
			msTick = 0;
		}
		
		
		byte msg = midiIn();		
		switch(msg & 0xF0) {
			case 0x90:			
				P_LED1 = 1;
				ledTime = LED_TIME;
			case 0x80:
				if((msg & 0x0F) == CHANNEL_A) {
					if(midiParams[1] && (msg & 0xF0) == 0x90) {
						startNote(0, midiParams[0], midiParams[1]);							
					}
					else {
						stopNote(0);							
					}
				}
				else
				if((msg & 0x0F) == CHANNEL_B) {
					if(midiParams[1] && (msg & 0xF0) == 0x90) {
						startNote(1, midiParams[0], midiParams[1]);							
					}
					else {
						stopNote(1);							
					}
				}
				else
				if((msg & 0x0F) == CHANNEL_DRUMS) {
					if(midiParams[1] && (msg & 0xF0) == 0x90) {					
						switch(midiParams[0]) {
							case NOTE_DRUM1: trigPulse(GATE_DRM1, TRIG_PULSE); break;
							case NOTE_DRUM2: trigPulse(GATE_DRM2, TRIG_PULSE); break;
							case NOTE_DRUM3: trigPulse(GATE_DRM3, TRIG_PULSE); break;
							case NOTE_DRUM4: trigPulse(GATE_DRM4, TRIG_PULSE); break;
							case NOTE_DRUM5: trigPulse(GATE_DRM5, TRIG_PULSE); break;
							case NOTE_DRUM6: trigPulse(GATE_DRM6, TRIG_PULSE); break;
							case NOTE_DRUM7: trigPulse(GATE_DRM7, TRIG_PULSE); break;
							case NOTE_DRUM8: trigPulse(GATE_DRM8, TRIG_PULSE); break;
						}
						//if(midiParams[1] >= VEL_ACCENT) {
//							trigPulse(GATE_SYNC, TRIG_PULSE); 
//						}
						break;
					}
				}
				break;
				
				
				
				if(midiParams[1] > 0) {
					switch(midiParams[0]) {
						case NOTE_DRUM1:
						case NOTE_DRUM2:
						case NOTE_DRUM3:
						case NOTE_DRUM4:
							break;
						default:
							startNote(0, midiParams[0], midiParams[1]);							
							break;
					}
					break;
				}
		}
		
		
		//i+=1;
		//if(i>4095)
//			i=0;
	//	P_LED = !!(cmout.0); // comparator output
	}

}



