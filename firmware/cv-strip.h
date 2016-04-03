////////////////////////////////////////////////////////////
//
// MINI MIDI CV
//
// GLOBAL DEFINITIONS
//
////////////////////////////////////////////////////////////

//
// MACRO DEFS
//
#define P_LED1		lata.2
#define P_LED2		latc.2

#define P_SRDAT1	lata.0
#define P_SRDAT2	lata.1
#define P_SRCLK		lata.4
#define P_SRLAT		lata.5

#define TRIS_A		0b11001000
#define TRIS_C		0b11111011

#define VEL_ACCENT 100

#define LED_PULSE_MIDI_IN 2
#define LED_PULSE_MIDI_TICK 10

#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc

#define SZ_NOTE_STACK 5	// max notes in a single stack
#define NUM_NOTE_STACKS 4	// number of stacks supported
#define NO_NOTE_OUT 0xFF 	// special "no note" value

#define LED_1_PULSE(ms) { P_LED1 = 1; g_led_1_timeout = ms; }
#define LED_2_PULSE(ms) { P_LED2 = 1; g_led_2_timeout = ms; }

// Check if MIDI channel mychan matches chan - taking into account GLOBAL and OMNI modes
#define IS_CHAN(mychan, chan) (((chan) == (mychan)) || (CHAN_OMNI == (mychan)) || \
 ((CHAN_GLOBAL == (mychan)) && (g_chan == (chan))))

// Check if a note matches a min-max range. If max==0 then it must exactly equal min
#define IS_NOTE_MATCH(mymin, mymax, note) (!(mymax)?((note)==(mymin)):((note)>=(mymin) && (note)<=(mymax)))

#define DEFAULT_GATE_NOTE 60
#define DEFAULT_GATE_CC 1
#define DEFAULT_GATE_CC_THRESHOLD 64
#define DEFAULT_GATE_DIV	6
#define DEFAULT_GATE_DURATION 10
#define DEFAULT_ACCENT_VELOCITY 127
#define DEFAULT_MIDI_CHANNEL 0

//
// TYPE DEFS
//
typedef unsigned char byte;

enum {
	GATE_DUR_INFINITE = 0x00,
	GATE_DUR_GLOBAL = 0x80
};

enum {
	VEL_ACC_GLOBAL = 0x7F
};

// special channels
enum {
	CHAN_OMNI = 0x80,
	CHAN_GLOBAL = 0x81,
	CHAN_DISABLE = 0xFF
};

// Events from note stack
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

// note stack note priority orders
enum {
	PRIORITY_NEW,		// gives priority to newest note
	PRIORITY_LOW,		// gives priority to lowest note
	PRIORITY_HIGH,		// gives priority to highest note
	PRIORITY_OLD,		// gives priority to oldest note
	PRIORITY_RANDOM,	// randomly prioritises notes	
	PRIORITY_PLAYORDER	// work as queue 
};

/*
+---+---+---+---+	+---+---+---+---+
|CV0|CV1|GT0|GT1|	|GT4|GT5|GT6|GT7|
+---+---+---+---+	+---+---+---+---+
|CV2|CV3|GT2|GT3|	|GT8|GT9|G10|G11|
+---+---+---+---+	+---+---+---+---+
*/
#define CV_MAX		4
#define GATE_MAX	12

/////////////////////////////////////////////////////////////////////////////
// CONFIGURATION ID'S
enum {
	P_ID_GLOBAL,	// Global settings
	P_ID_INPUT1,	// Note stack settings
	P_ID_INPUT2,
	P_ID_INPUT3,
	P_ID_INPUT4,
	P_ID_CV1,		// CV output settings
	P_ID_CV2,
	P_ID_CV3,
	P_ID_CV4,
	P_ID_GATE1,		// Gate output settings
	P_ID_GATE2,
	P_ID_GATE3,
	P_ID_GATE4,
	P_ID_GATE5,
	P_ID_GATE6,
	P_ID_GATE7,
	P_ID_GATE8,
	P_ID_GATE9,
	P_ID_GATE10,
	P_ID_GATE11,
	P_ID_GATE12,
	P_ID_MAX
};



enum {
	// Parameters for configuring source of an output
	P_DISABLE,	// output is disabled
	P_INPUT1,	// output source is input stack #1
	P_INPUT2,	// output source is input stack #2
	P_INPUT3,	// output source is input stack #3
	P_INPUT4,	// output source is input stack #4
	P_MIDI,		// output source is MIDI
	
	// parameters for configuring output, input or global
	P_CHAN,				// midi channel (1-16)
	P_CHAN_OMNI,		// midi channel OMNI
	P_CHAN_GLOBAL,		// midi channel global 
	P_PRTY,				// note priority
	P_NOTE_SINGLE,		// midi note (single)
	P_NOTE_RANGE_FROM,	// midi note (range, min)
	P_NOTE_RANGE_TO,	// midi note (range, max)
	P_VEL_MIN,			// midi velocity (min)
	P_VEL_ACCENT,
	P_VEL_ACCENT_GLOBAL,
	P_CC,				// CC number
	P_CC_THRESHOLD,		// CC switching threshold
	P_PB_RANGE,			// pitch bend
	P_DURATION,			// gate duration (ms) or 0 for continuous
	P_DURATION_GLOBAL,	// 
	P_DIV				// clock divider		
};

// values for P_INPUTx
enum {
	P_INPUT_NOTEA,			// output is note A from input stack
	P_INPUT_NOTEB,			// output is note B from input stack
	P_INPUT_NOTEC,			// output is note C from input stack
	P_INPUT_NOTED,			// output is note D from input stack
	P_INPUT_VELOCITY,		// output is note velocity from input stack
	P_INPUT_NOTE_ON,		// output trigger is when any notes are active on stack
	P_INPUT_NOTE_ACCENT,	// output trigger is when note hit with accent velocity
	P_INPUT_NOTES_OFF,		// output trigger is when all notes are off
};

// values for P_MIDIx
enum {	
	P_MIDI_NOTE,			// tied to a MIDI note
	P_MIDI_CC,				// tied to a MIDI cc
	P_MIDI_PB,				// tied to MIDI pitch bend
	P_MIDI_TICK,			// tied to MIDI clock ticks
	P_MIDI_RUN_TICK,		// tied to gated MIDI clock ticks
	P_MIDI_RUN,				// tied to MIDI clock run state
	P_MIDI_START,			// tied to MIDI start event
	P_MIDI_STOP,			// tied to MIDI stop
	P_MIDI_STARTCONT		// tied to MIDI start/continue
};	
	
// Parameter values for P_PRTY
enum {
	P_PRTY_NEW,
	P_PRTY_LOW,
	P_PRTY_HIGH,
	P_PRTY_OLD,	
	P_PRTY_RANDOM,
	P_PRTY_PLAYORDER,
	P_PRTY_MAX
};

// Parameter values for P_DIV
enum {
  P_DIV_1    = 96,
  P_DIV_2D   = 72,
  P_DIV_2    = 48,
  P_DIV_4D   = 36,
  P_DIV_2T   = 32,  
  P_DIV_4    = 24,
  P_DIV_8D   = 18,
  P_DIV_4T   = 16,
  P_DIV_8    = 12,
  P_DIV_16D  = 9,
  P_DIV_8T   = 8,
  P_DIV_16   = 6,
  P_DIV_16T  = 4,
  P_DIV_32   = 3	
};

//
// STRUCT DEFS
//

// note stack config
typedef struct {
	byte chan;			// midi channel
	byte note_min;		// lowest note
	byte note_max;		// highest note
	byte vel_min;		// minimum velocity threshold
	byte bend_range;	// pitch bend range (+/- semitones)
	byte priority;		// how notes are prioritised when assigned to outputs
} NOTE_STACK_CFG;

// note stack state
typedef struct {
	byte note[SZ_NOTE_STACK];	// the notes held in the stack
	char count;					// number of held notes
	byte out[4];				// the stack output notes
	int bend;					// pitch bend
	byte vel;					// note velocity	
} NOTE_STACK;

extern char g_led_1_timeout;
extern char g_led_2_timeout;

// GLOBAL EXPORTED DATA
extern NOTE_STACK g_stack[NUM_NOTE_STACKS];
extern NOTE_STACK_CFG g_stack_cfg[NUM_NOTE_STACKS];

extern byte g_chan;
extern byte g_accent_vel;
extern byte g_gate_duration;

byte cfg(byte module, byte param, byte value);

// EXPORTED FUNCTIONS FROM GLOBAL MODULE
void global_init();
byte global_cfg(byte param, byte value);


// EXPORTED FUNCTIONS FROM NOTE STACK MODULE
void stack_midi_note(byte chan, byte note, byte vel);
void stack_midi_bend(byte chan, int bend);
byte stack_cfg(byte which_stack, byte param, byte value);
void stack_init();

// PUBLIC FUNCTIONS FROM GATES MODULE
void gate_event(byte event, byte stack_id);
void gate_midi_note(byte chan, byte note, byte vel);
void gate_midi_cc(byte chan, byte cc, byte value);
void gate_midi_clock(byte msg);
void gate_run();
void gate_init();
void gate_reset();
void gate_trigger(byte which_gate, byte trigger_enabled);
byte gate_cfg(byte which_gate, byte param, byte value);

// PUBLIC FUNCTIONS FROM CV MODULE
void cv_event(byte event, byte stack_id);
void cv_midi_cc(byte chan, byte cc, byte value);
void cv_midi_bend(byte chan, int bend);
void cv_init(); 
byte cv_cfg(byte which_cv, byte param, byte value);
void cv_write_dac(byte which, int value);
void cv_write_note(byte which, byte midi_note, int pitch_bend);
void cv_write_vel(byte which, long value);
void cv_write_cc(byte which, long value);
void cv_write_bend(byte which, long value);