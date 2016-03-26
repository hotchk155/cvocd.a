
typedef unsigned char byte;

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

//
// CONSTANTS
//
enum {
	PRIORITY_NEW,		// gives priority to newest note
	PRIORITY_LOW,		// gives priority to lowest note
	PRIORITY_HIGH,		// gives priority to highest note
	PRIORITY_OLD,		// gives priority to oldest note
	PRIORITY_RANDOM		// randomly prioritises notes	
};

#define SZ_NOTE_STACK 12	// max notes in a single stack
#define NUM_NOTE_STACKS 4	// number of stacks supported
#define NO_NOTE_OUT 0xFF 	// special "no note" value
//
// STRUCT DEFS
//
typedef struct {
	byte chan;			// midi channel
	byte note_min;		// lowest note
	byte note_max;		// highest note
	byte vel_min;		// minimum velocity threshold
	byte bend_range;	// pitch bend range (+/- semitones)
	byte priority;		// how notes are prioritised when assigned to outputs
		
	// STATE
	byte note[SZ_NOTE_STACK];	// the notes held in the stack
	char count;					// number of held notes
	byte out[4];				// the stack output notes
	int bend;					// pitch bend
	byte vel;					// note velocity	
} NOTE_STACK;

enum {
	CHAN_OMNI = 0x80,
	CHAN_GLOBAL = 0x81
};

extern NOTE_STACK g_stack[NUM_NOTE_STACKS];
extern byte g_chan;
extern byte g_accent_vel;



// EXPORTED FUNCTIONS FROM NOTE STACK MODULE
void stack_midi_note(byte chan, byte note, byte vel);
void stack_bend(byte chan, byte hi, byte lo);
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
