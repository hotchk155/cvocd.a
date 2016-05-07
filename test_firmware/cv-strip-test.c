/*
	CV STRIP
	BASIC HARDWARE TEST
	
	Press button to cycle
	1 - CV A test slope - outputs 1,2
	2 - CV B test slope - outputs 3,4
	3 - CV C test slope - outputs 5,6,7,8
	4 - CV D test slope - outputs 9,10,11,12
*/

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
#pragma CLOCK_FREQ 16000000


//
// MACRO DEFS
//
#define P_LED1		lata.2
#define P_LED2		latc.2

#define P_SRDAT1	lata.0
#define P_SRDAT2	lata.1
#define P_SRCLK		lata.4
#define P_SRLAT		lata.5

#define P_SWITCH 	portc.3

#define TRIS_A		0b11001000
#define TRIS_C		0b11111011

//
// MACRO DEFS
//
#define I2C_ADDRESS 0b1100000

int cv_a, cv_b, cv_c, cv_d;
typedef unsigned char byte;

void i2c_init() {
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
	//ssp1add = 19;	// 100kHz baud rate
	ssp1add =2;	// 100kHz baud rate
}

////////////////////////////////////////////////////////////
// I2C WRITE BYTE TO BUS
void i2c_send(byte data) {
	ssp1buf = data;
	while((ssp1con2 & 0b00011111) || // SEN, RSEN, PEN, RCEN or ACKEN
		(ssp1stat.2)); // data transmit in progress	
}

////////////////////////////////////////////////////////////
// I2C START WRITE MESSAGE TO A SLAVE
void i2c_begin_write(byte address) {
	pir1.3 = 0; // clear SSP1IF
	ssp1con2.0 = 1; // signal start condition
	while(!pir1.3); // wait for it to complete
	i2c_send(address<<1); // address + WRITE(0) bit
}

////////////////////////////////////////////////////////////
// I2C FINISH MESSAGE
void i2c_end() {
	pir1.3 = 0; // clear SSP1IF
	ssp1con2.2 = 1; // signal stop condition
	while(!pir1.3); // wait for it to complete
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


unsigned int x[12] = {
	0x0004,
	0x0008,
	0x0002,
	0x0001,
	0x0100,
	0x0200,
	0x0400,
	0x0800,
	0x1000,
	0x2000,
	0x4000,
	0x8000
};
void sr_write(unsigned int d) {
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
void gates(int i) {
	unsigned int q = 0;
	if(i&1) {
		q |= x[0];
		q |= x[4];
		q |= x[8];
	}
	if(i&2) {
		q |= x[1];
		q |= x[5];
		q |= x[9];
	}
	if(i&4) {
		q |= x[2];
		q |= x[6];
		q |= x[10];
	}
	if(i&8) {
		q |= x[3];
		q |= x[7];
		q |= x[11];
	}
	sr_write(q);
}




void dac_cfg() {
	i2c_begin_write(I2C_ADDRESS);
	i2c_send(0b10001111); // set each channel to use internal vref
	i2c_end();	

	i2c_begin_write(I2C_ADDRESS);
	i2c_send(0b11001111); // set x2 gain on each channel
	i2c_end();	
}

void dac_send(int a, int b, int c, int d) {		
	i2c_begin_write(I2C_ADDRESS);
	i2c_send((b>>8) & 0xF);
	i2c_send(b & 0xFF);
	i2c_send((d>>8) & 0xF);
	i2c_send(d & 0xFF);
	i2c_send((c>>8) & 0xF);
	i2c_send(c & 0xFF);
	i2c_send((a>>8) & 0xF);
	i2c_send(a & 0xFF);
	i2c_end();	
	
}



void test_v()
{
	unsigned int i=0;
	P_LED1 = 0;
	P_LED2 = 1;
	for(;;) {	
		int a = 1000*(i%4);
		int b = 1000*((i+1)%4);
		int c = 1000*((i+2)%4);
		int d = 1000*((i+3)%4);
		dac_send(a, b, c, d);
		gates(i);
		while(P_SWITCH);
		delay_ms(100);
		while(!P_SWITCH);
		delay_ms(100);
		++i;
		P_LED1 = !P_LED1;
		P_LED2 = !P_LED1;
	}
}

void test_ramp() 
{
	cv_a = 0;
	cv_b = 0;
	cv_c = 0;
	cv_d = 0;
	
	int *which = &cv_a;
	
	
	int i;
	
	for(;;) {
		P_LED1 = !P_LED1;
		P_LED2 = !P_LED1;

		*which = 0;
		for(i=0; i<2048; ++i) {
			dac_send(cv_a, cv_b, cv_c, cv_d);
		}
		for(i=0; i<2048; ++i) {
			*which=i;
			dac_send(cv_a, cv_b, cv_c, cv_d);
		}
		for(i=0; i<2048; ++i) {
			dac_send(cv_a, cv_b, cv_c, cv_d);
		}
		for(i=2048; i<4096; ++i) {
			*which=i;
			dac_send(cv_a, cv_b, cv_c, cv_d);
		}
	}

}


////////////////////////////////////////////////////////////
// MAIN
void main()
{ 	
	// osc control / 16MHz / internal
	osccon = 0b01111010;

	trisa 	= TRIS_A;              	
    trisc 	= TRIS_C;   	
	ansela 	= 0b00000000;
	anselc 	= 0b00000000;
	porta 	= 0b00000000;
	portc 	= 0b00000000;
	P_LED1 = 1;
	P_LED2 = 1;
	delay_ms(255);
	delay_ms(255);
	i2c_init();
	dac_cfg();
	test_v();
	P_LED1 = 0;
	P_LED2 = 0;
}



