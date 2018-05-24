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

//////////////////////////////////////////////////////////////
//
// GLOBAL DEFS
//
//////////////////////////////////////////////////////////////

//
// MACRO DEFS
//

// Pin definitions
#define P_LED1		lata.2
#define P_LED2		latc.2
#define P_SRDAT1	lata.0
#define P_SRDAT2	lata.1
#define P_SRCLK		lata.4
#define P_SRLAT		lata.5
#define P_SWITCH 	portc.3
#define TRIS_A		0b11001000
#define TRIS_C		0b11111011

// constants
#define CV_MAX		4					// number of cv outs
#define GATE_MAX	12					// number of gate outs
#define SZ_NOTE_STACK 5					// max notes in a single stack
#define NUM_NOTE_STACKS 4				// number of stacks supported
#define NO_NOTE_OUT 0xFF 				// special "no note" value
#define I2C_TX_BUF_SZ 12				// size of i2c transmit buffer

// Defaults
#define DEFAULT_GATE_NOTE 			60
#define DEFAULT_GATE_CC 			1
#define DEFAULT_GATE_CC_THRESHOLD 	64
#define DEFAULT_GATE_DIV			6
#define DEFAULT_GATE_DURATION 		10
#define DEFAULT_ACCENT_VELOCITY 	127
#define DEFAULT_MIDI_CHANNEL 		0
#define DEFAULT_CV_BPM_MAX_VOLTS 	5
#define DEFAULT_CV_CC_MAX_VOLTS 	5
#define DEFAULT_CV_PB_MAX_VOLTS 	5
#define DEFAULT_CV_VEL_MAX_VOLTS 	5
#define DEFAULT_CV_TOUCH_MAX_VOLTS 	5
#define DEFAULT_CV_TEST_VOLTS 		5

// Millisecond timings
#define SHORT_BUTTON_PRESS 40
#define LONG_BUTTON_PRESS 2000
#define LED_PULSE_MIDI_IN 2
#define LED_PULSE_MIDI_TICK 10
#define LED_PULSE_MIDI_BEAT 100
#define LED_PULSE_PARAM 255

// MIDI message bytes
#define MIDI_MTC_QTR_FRAME 		0xf1
#define MIDI_SPP 				0xf2
#define MIDI_SONG_SELECT 		0xf3 
#define MIDI_SYNCH_TICK     	0xf8
#define MIDI_SYNCH_START    	0xfa
#define MIDI_SYNCH_CONTINUE 	0xfb
#define MIDI_SYNCH_STOP     	0xfc
#define MIDI_SYSEX_BEGIN     	0xf0
#define MIDI_SYSEX_END     		0xf7

#define MIDI_CC_NRPN_HI 		99
#define MIDI_CC_NRPN_LO 		98
#define MIDI_CC_DATA_HI 		6
#define MIDI_CC_DATA_LO 		38

// Sysex ID
#define MY_SYSEX_ID0	0x00
#define MY_SYSEX_ID1	0x7f
#define MY_SYSEX_ID2	0x15 // CVOCD patch

// Utility macros to flash an LED
#define LED_1_PULSE(ms) { P_LED1 = 1; g_led_1_timeout = ms; }
#define LED_2_PULSE(ms) { P_LED2 = 1; g_led_2_timeout = ms; }

// Check if MIDI channel mychan matches chan - taking into account GLOBAL and OMNI modes
#define IS_CHAN(mychan, chan) (((chan) == (mychan)) || (CHAN_OMNI == (mychan)) || \
 ((CHAN_GLOBAL == (mychan)) && (g_global.chan == (chan))))

// Check if a note matches a min-max range. If max==0 then it must exactly equal min
#define IS_NOTE_MATCH(mymin, mymax, note) \
 (!(mymax)?((note)==(mymin)):((note)>=(mymin) && (note)<=(mymax)))

//
// ENUMS
//

// special gate duration
enum {
	GATE_DUR_INFINITE = 0x00,	// "gate mode"
	GATE_DUR_GLOBAL = 0x80		// use global setting
};

// special channels
enum {
	CHAN_OMNI = 0x80,			// omni
	CHAN_GLOBAL = 0x81,			// use global setting
	CHAN_DISABLE = 0xFF			// disable channel
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
	EV_NOTE_ON,
	EV_BEND
};

// note stack note priority orders
enum {
	PRIORITY_LAST			= 0,	// gives priority to newest note
	PRIORITY_LOW			= 1,	// gives priority to lowest note
	PRIORITY_HIGH			= 3,	// gives priority to highest note

	PRIORITY_CYCLE2			= 6,	// 2 note cycle
	PRIORITY_CYCLE3			= 7,	// 3 note cycle
	PRIORITY_CYCLE4			= 8,	// 4 note cycle
	
	PRIORITY_CHORD2			= 9,	
	PRIORITY_CHORD3			= 10,	
	PRIORITY_CHORD4			= 11,	
	
	PRIORITY_MAX			= 12
};

enum {
	TRANSPOSE_NONE			= 64	// transpose value for no transpose
};

// Parameter Number High Byte 
enum {
	// global settings
	NRPNH_GLOBAL 	= 1,	
	// note stacks
	NRPNH_STACK1 	= 11,
	NRPNH_STACK2 	= 12,
	NRPNH_STACK3 	= 13,
	NRPNH_STACK4 	= 14,
	// cv's
	NRPNH_CV1		= 21,
	NRPNH_CV2		= 22,
	NRPNH_CV3		= 23,
	NRPNH_CV4		= 24,
	// gates
	NRPNH_GATE1 	= 31,
	NRPNH_GATE2 	= 32,
	NRPNH_GATE3 	= 33,
	NRPNH_GATE4 	= 34,
	NRPNH_GATE5 	= 35,
	NRPNH_GATE6 	= 36,
	NRPNH_GATE7 	= 37,
	NRPNH_GATE8 	= 38,
	NRPNH_GATE9 	= 39,
	NRPNH_GATE10	= 40,
	NRPNH_GATE11	= 41,
	NRPNH_GATE12	= 42
};

// Parameter Number Low Byte 
enum {
	NRPNL_SRC			= 1,
	NRPNL_CHAN			= 2,
	NRPNL_NOTE_MIN  	= 3,
	NRPNL_NOTE			= NRPNL_NOTE_MIN,
	NRPNL_NOTE_MAX  	= 4,
	NRPNL_VEL_MIN  		= 5,
	NRPNL_PB_RANGE		= 7,
	NRPNL_PRIORITY		= 8,	
	NRPNL_TICK_OFS		= 11,
	NRPNL_GATE_DUR		= 12,
	NRPNL_THRESHOLD		= 13,
	NRPNL_TRANSPOSE		= 14,
	NRPNL_VOLTS			= 15,
	NRPNL_PITCH_SCHEME  = 16,
	NRPNL_CAL_SCALE  	= 98,
	NRPNL_CAL_OFS  		= 99,
	NRPNL_SAVE			= 100
};

// Parameter Value High Byte
enum {
	NRPVH_SRC_DISABLE		= 0,

	NRPVH_SRC_MIDINOTE		= 1,
	NRPVH_SRC_MIDICC		= 2,
	NRPVH_SRC_MIDICC_NEG	= 3,
	NRPVH_SRC_MIDIBEND		= 4,
	NRPVH_SRC_MIDITOUCH		= 5,

	NRPVH_SRC_STACK1		= 11,
	NRPVH_SRC_STACK2		= 12,
	NRPVH_SRC_STACK3		= 13,
	NRPVH_SRC_STACK4		= 14,

	NRPVH_SRC_MIDITICK		= 20,
	NRPVH_SRC_MIDITICKRUN	= 21,
	NRPVH_SRC_MIDIRUN		= 22,
	NRPVH_SRC_MIDISTART		= 23,
	NRPVH_SRC_MIDISTOP		= 25,
	NRPVH_SRC_MIDISTARTSTOP	= 26,

	NRPVH_SRC_TESTVOLTAGE	= 127,
	
	NRPVH_CHAN_SPECIFIC		= 0,
	NRPVH_CHAN_OMNI			= 1,
	NRPVH_CHAN_GLOBAL		= 2,
	
	NRPVH_DUR_INF			= 0,
	NRPVH_DUR_MS			= 1,
	NRPVH_DUR_GLOBAL		= 2,
	NRPVH_DUR_RETRIG		= 3,

	NRPVH_PITCH_VOCT		= 0,
	NRPVH_PITCH_HZV			= 1,
	NRPVH_PITCH_12VO		= 2
};

// Parameter Value Low Byte
enum {
	NRPVL_SRC_NO_NOTES			= 0,
	NRPVL_SRC_NOTE1				= 1,
	NRPVL_SRC_NOTE2				= 2,
	NRPVL_SRC_NOTE3				= 3,
	NRPVL_SRC_NOTE4				= 4,
	NRPVL_SRC_ANY_NOTES			= 5,

	NRPVL_SRC_VEL				= 20,
	//NRPVL_SRC_AFTERTOUCH		= 22
};

//
// TYPE DEFS
//
typedef unsigned char byte;

// global config
typedef struct {
	byte chan;
	byte gate_duration;
} GLOBAL_CFG;

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
	long bend;					// pitch bend
	byte vel;					// note velocity	
	byte index;					// index for note cycling
} NOTE_STACK;

//
// GLOBAL DATA DECLARATIONS
//
extern char g_led_1_timeout;
extern char g_led_2_timeout;
extern GLOBAL_CFG g_global;
extern NOTE_STACK g_stack[NUM_NOTE_STACKS];
extern NOTE_STACK_CFG g_stack_cfg[NUM_NOTE_STACKS];
extern byte g_cv_dac_pending;
extern volatile byte g_i2c_tx_buf[I2C_TX_BUF_SZ];
extern volatile byte g_i2c_tx_buf_index;
extern volatile byte g_i2c_tx_buf_len;
extern volatile unsigned int g_sr_data;
extern volatile unsigned int g_sr_retrigs;
extern volatile unsigned int g_sync_sr_data;
extern volatile unsigned int g_sync_sr_mask;
extern volatile byte g_sync_sr_data_pending;
extern volatile byte g_sr_data_pending;

//
// GLOBAL FUNCTION DECLARATIONS
//

// EXPORTED FUNCTIONS FROM MAIN FILE
void i2c_send(byte data);
void i2c_begin_write(byte address);
void i2c_end();
void nrpn(byte param_hi, byte param_lo, byte value_hi, byte value_lo);

// EXPORTED FUNCTIONS FROM GLOBAL MODULE
void global_init();
byte global_nrpn(byte param_lo, byte value_hi, byte value_lo);
byte *global_storage(int *len);

// EXPORTED FUNCTIONS FROM NOTE STACK MODULE
void stack_midi_note(byte chan, byte note, byte vel);
void stack_midi_bend(byte chan, int bend);
void stack_midi_aftertouch(byte chan, byte value);
byte stack_nrpn(byte which_stack, byte param_lo, byte value_hi, byte value_lo);
void stack_init();
void stack_reset();
byte *stack_storage(int *len);

// PUBLIC FUNCTIONS FROM GATES MODULE
void gate_event(byte event, byte stack_id);
void gate_midi_note(byte chan, byte note, byte vel);
void gate_midi_cc(byte chan, byte cc, byte value);
void gate_midi_clock(byte msg);
void gate_run();
void gate_init();
void gate_reset();
void gate_trigger(byte which_gate, byte trigger_enabled);
byte gate_nrpn(byte which_gate, byte param_lo, byte value_hi, byte value_lo);
void gate_update();
void gate_write();
byte *gate_storage(int *len);

// PUBLIC FUNCTIONS FROM CV MODULE
void cv_event(byte event, byte stack_id);
void cv_midi_cc(byte chan, byte cc, byte value);
void cv_midi_touch(byte chan, byte value);
void cv_midi_bend(byte chan, int bend);
//void cv_midi_bpm(long value);
void cv_init(); 
void cv_reset();
byte cv_nrpn(byte which_cv, byte param_lo, byte value_hi, byte value_lo);
void cv_dac_prepare();
byte *cv_storage(int *len);

// STORAGE
void storage_read_patch();
void storage_write_patch();

//
// END
//