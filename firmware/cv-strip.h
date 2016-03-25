#define P_LED1		lata.2
#define P_LED2		latc.2

#define P_SRDAT1	lata.0
#define P_SRDAT2	lata.1
#define P_SRCLK		lata.4
#define P_SRLAT		lata.5

#define VEL_ACCENT 100

#define TRIG_PULSE 5 //ms
#define LED_TIME 10 //ms


#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc

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
enum {
	CHAN_OMNI	= 0xFF,
};

///////////////////////////////////////////////////////////////////////
// EVENTS WHICH A NOTE STACK CAN FIRE
enum {
	EV_NOTE_A = 1,
	EV_NOTE_B,
	EV_NOTE_C,
	EV_NOTE_D,
	EV_NO_NOTE_A,
	EV_NO_NOTE_B,
	EV_NO_NOTE_C,
	EV_NO_NOTE_D,
	EV_NOTES_OFF,
	EV_NOTE_ON
};

#define NO_NOTE_OUT 0xFF
#define NO_VALUE 	0xFF


enum {
	CHAN_OMNI = 0x80,
	CHAN_GLOBAL = 0x81
};
extern byte g_chan = 0;




// EXPORTED FUNCTIONS FROM NOTE STACK MODULE
void stack_midi_note(byte chan, byte note, byte vel);
void stack_init();

// PUBLIC FUNCTIONS FROM GATES MODULE
void gate_event(byte event, byte stack_id);
void gate_midi_note(byte chan, byte note, byte vel);
void gate_midi_cc(byte chan, byte cc, byte value);
void gate_midi_clock(byte msg);
void gate_run();
void gate_init();

// PUBLIC FUNCTIONS FROM CV MODULE
void cv_event(byte event, byte stack_id);
void cv_midi_cc(byte chan, byte cc, byte value);
void cv_init(); 
