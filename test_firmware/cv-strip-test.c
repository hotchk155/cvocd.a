/*
	CV STRIP
	BASIC HARDWARE TEST

	REVISION 1
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

void test_vg() 
{
	byte count = 0;
	unsigned int index = 0;
	unsigned int a = 0;
	unsigned int b = 1024;
	unsigned int c = 2048;
	unsigned int d = 3072;
	while(1) {
		if(!a) {
				if(++index > 3) {
					index = 0;
				}
				sr_write(x[index]|x[4+index]|x[8+index]);
		}
		
		dac_send(a, b, c, d);
		a+=8;
		b+=8;
		c+=8;
		d+=8;
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
	test_vg();
}



