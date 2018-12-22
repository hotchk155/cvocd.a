const SYSEX_BEGIN 		= 0xF0;
const SYSEX_END 		= 0xF7;
const MANUF_ID_0 		= 0x00;
const MANUF_ID_1 		= 0x7F;
const MANUF_ID_2 		= 0x15;		
const NUM_NOTE_INPUTS 	= 4;
const NUM_CV_OUTPUTS 	= 4;
const NUM_GATE_OUTPUTS 	= 12;

const NRPNH_GLOBAL 	= 1;
const NRPNH_STACK1 	= 11;
const NRPNH_STACK2 	= 12;
const NRPNH_STACK3 	= 13;
const NRPNH_STACK4 	= 14;
const NRPNH_CV1		= 21;
const NRPNH_CV2		= 22;
const NRPNH_CV3		= 23;
const NRPNH_CV4		= 24;
const NRPNH_GATE1 	= 31;
const NRPNH_GATE2 	= 32;
const NRPNH_GATE3 	= 33;
const NRPNH_GATE4 	= 34;
const NRPNH_GATE5 	= 35;
const NRPNH_GATE6 	= 36;
const NRPNH_GATE7 	= 37;
const NRPNH_GATE8 	= 38;
const NRPNH_GATE9 	= 39;
const NRPNH_GATE10	= 40;
const NRPNH_GATE11	= 41;
const NRPNH_GATE12	= 42;

const NRPNL_SRC			= 1;
const NRPNL_CHAN		= 2;
const NRPNL_NOTE_MIN  	= 3;
const NRPNL_NOTE		= 3;
const NRPNL_NOTE_MAX  	= 4;
const NRPNL_VEL_MIN  	= 5;
const NRPNL_PB_RANGE	= 7;
const NRPNL_PRIORITY	= 8;
const NRPNL_SPLIT  		= 9;
const NRPNL_TICK_OFS	= 11;
const NRPNL_GATE_DUR	= 12;
const NRPNL_THRESHOLD	= 13;
const NRPNL_TRANSPOSE	= 14;
const NRPNL_VOLTS		= 15;
const NRPNL_PITCHSCHEME	= 16;
const NRPNL_GLIDETIME	= 17;
const NRPNL_CAL_SCALE  	= 98;
const NRPNL_CAL_OFS  	= 99;
const NRPNL_SAVE		= 100;

const NRPVH_SRC_DISABLE		= 0;
const NRPVH_SRC_MIDINOTE	= 1;
const NRPVH_SRC_MIDICC		= 2;
const NRPVH_SRC_MIDICC_NEG	= 3;
const NRPVH_SRC_MIDIBEND	= 4;
const NRPVH_SRC_MIDITOUCH	= 5;
const NRPVH_SRC_STACK1		= 11;
const NRPVH_SRC_STACK2		= 12;
const NRPVH_SRC_STACK3		= 13;
const NRPVH_SRC_STACK4		= 14;
const NRPVH_SRC_MIDITICK	= 20;
const NRPVH_SRC_MIDITICKRUN	= 21;
const NRPVH_SRC_MIDIRUN		= 22;
const NRPVH_SRC_MIDISTART	= 23;
const NRPVH_SRC_MIDISTOP	= 25;
const NRPVH_SRC_MIDISTARTSTOP	= 26;
const NRPVH_SRC_TESTVOLTAGE = 127;
const NRPVH_CHAN_SPECIFIC	= 0;
const NRPVH_CHAN_OMNI		= 1;
const NRPVH_CHAN_GLOBAL		= 2;
const NRPVH_DUR_INF			= 0;
const NRPVH_DUR_MS			= 1;
const NRPVH_DUR_GLOBAL		= 2;
const NRPVH_DUR_RETRIG		= 3;

const TAG_CHAN_GLOBAL		= "@";
const TAG_CHAN_OMNI			= "*";

class CfgPage {

	static renderBlankCell(row) {
		let cell = document.createElement("TD");
		row.appendChild(cell);	
	}

	static renderSelection(row, opts, value) {
		let cell = document.createElement("TD");
		let el = document.createElement("SELECT");
		for(let o of opts) {
			let ch = document.createElement("OPTION");
			if(o[0] == null) {
				ch.disabled = true;
			}
			else {
				ch.value = o[0];
			}			
			ch.innerHTML = o[1];
			if(o[0] == value) {
				ch.selected = true;
			}
			el.options.add(ch);
		}		
		cell.appendChild(el);
		row.appendChild(cell);
		return el;
	}
	
	
	static renderMidiChannel(row, includeDefault, chanType, chanNumber) {
		let opts = []
		if(includeDefault) {
			opts.push([(256*2), "(default)"])
		}
		for(let i=1; i<=16; ++i) {
			opts.push([i, "Chan." + i])
		}
		if(includeDefault) {
			opts.push([(256*1), "(OMNI)"])
		}				
		return CfgPage.renderSelection(row, opts, 256*chanType+chanNumber);
	}

	static renderGateDuration(row, includeDefault, durationType, duration) {	
		let opts = []
		opts.push([0, "(gate)"])
		opts.push([(256*3), "(retrig)"])
		if(includeDefault) {
			opts.push([(256*2), "(default)"])
		}
		let i=1;
		while(i<125) {
			opts.push([256+i, i + "ms"]);
			if(i<15) {
				++i;
			}
			else {
				i+=5;
			}
		}
		return CfgPage.renderSelection(row, opts, 256*durationType+duration);
	}
	
	static renderNote(row, value) {
		const notes = ["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"];
		let opts = []
		for(let i = 0; i<=127; ++i) {			
			opts.push([i, i + " " + notes[i%12] + (Math.trunc(i/12)-1)]);
		}
		return CfgPage.renderSelection(row, opts, value);
	}
	
	static render7Bit(row, value) {
		let opts = []
		for(let i = 1; i<=127; ++i) {			
			opts.push([i, i.toString()]);
		}
		return CfgPage.renderSelection(row, opts, value);
	}

	static renderBendRange(row, value) {
		let opts = []
		for(let i = 0; i<=24; ++i) {			
			opts.push([i, "+/-" + i]);
		}
		return CfgPage.renderSelection(row, opts, value);
	}
	
	static renderTranspose(row, value) {
		let opts = []
		opts.push([100, "+3oct"]);
		opts.push([88, "+2oct"]);
		opts.push([76, "+1oct"]);
		for(let i=11; i>=-11; --i) {
			if(i==0) {
				opts.push([64, "(none)"]);
			}
			else {
				opts.push([64+i, i.toString()]);
			}
		}
		opts.push([52, "-1oct"]);
		opts.push([40, "-2oct"]);
		opts.push([28, "-3oct"]);
		return CfgPage.renderSelection(row, opts, value);
	}
		
	static renderCC(row, value) {
		let opts = []
		for(let i=1; i<=127; ++i) {
			switch(i) {
				case 1:
					opts.push([i, "Mod Wheel"]);
					break;
				case 6:
					opts.push([null, "Data Entry MSB"]);
					break;
				case 38:
					opts.push([null, "Data Entry LSB"]);
					break;
				case 98:
					opts.push([null, "NRPN LSB"]);
					break;
				case 99:
					opts.push([null, "NRPN MSB"]);
					break;
				default:
					opts.push([i, "CC#" + i]);
			}
		}
		return CfgPage.renderSelection(row, opts, value);
	}
	
	static renderClockOffset(row, value) {
		let opts = [];
		opts.push([0, "(none)"]);		
		for(let i = 1; i<=127; ++i) {			
			opts.push([i, "+" + i]);
		}
		return CfgPage.renderSelection(row, opts, value);
	}

	
};

///////////////////////////////////////////////////////////////////////////////////
// Object to hold configuration of a CV.OCD Note Input	
class NoteInput {

	///////////////////////////////////////////////////////////////////////////////
	constructor(i) {
		this.Index 			= NRPNH_STACK1 + i;
		this.Source 		= NRPVH_SRC_DISABLE;
		this.ChanType  		= NRPVH_CHAN_GLOBAL;
		this.ChanNumber		= 0;
		this.Priority  		= 3;
		this.MinNote 		= 0;
		this.MaxNote		= 127;
		this.MinVel 		= 0;
		this.BendRange		= 3;
	}

	///////////////////////////////////////////////////////////////////////////////
	syxify() {
		return [
			this.Index, NRPNL_SRC, this.Source, 0,
			this.Index, NRPNL_CHAN, this.ChanType, this.ChanNumber,
			this.Index, NRPNL_NOTE_MIN, 0, this.MinNote,
			this.Index, NRPNL_NOTE_MAX, 0, this.MaxNote,
			this.Index, NRPNL_VEL_MIN, 0, this.MinVel,
			this.Index, NRPNL_PB_RANGE, 0, this.BendRange,
			this.Index, NRPNL_PRIORITY, 0, this.Priority
		];
	}
	
	///////////////////////////////////////////////////////////////////////////////
	unsyxify(data) {
		switch(data[1]) {
			case NRPNL_SRC:
				this.Source = data[2];
				break;
			case NRPNL_CHAN:
				this.ChanType = data[2];
				this.ChanNumber = data[3];
				break;
			case NRPNL_NOTE_MIN:
				this.MinNote = data[3];
				break;
			case NRPNL_NOTE_MAX:
				this.MaxNote = data[3];
				break;
			case NRPNL_VEL_MIN:
				this.MinVel = data[3];
				break;
			case NRPNL_PB_RANGE:
				this.BendRange = data[3];
				break;
			case NRPNL_PRIORITY:
				this.Priority = data[3];
				break;
		}
	}	
	
	xable() {
		let dis = (this.Source == 0);
		this.ctrlChan.disabled = dis;
		this.ctrlPriority.disabled = dis;
		this.ctrlMinNote.disabled = dis;
		this.ctrlMaxNote.disabled = dis;
		this.ctrlMinVel.disabled = dis;
		this.ctrlBendRange.disabled = dis;
	}	
	render(row) {
		let ctrl;
		let obj = this;
		
		// INPUT SOURCE
		ctrl = CfgPage.renderSelection(row, [
			[0, "(disable)"],
			[1, "ENABLE"]
		], this.Source);		
		ctrl.onchange = function() {obj.Source = this.value; obj.xable();}
		this.ctrlSource = ctrl;
		
		// MIDI CHANNEL
		ctrl = CfgPage.renderMidiChannel(row, true, this.ChanType, this.ChanNumber);
		ctrl.onchange = function() {obj.ChanType = this.value>>8; obj.ChanNumber = this.value&0xFF; }
		this.ctrlChan = ctrl;
		
		// PRIORITY
		ctrl = CfgPage.renderSelection(row, [		
			[0, "Last note priority"],
			[1, "Lowest note priority"],
			[3, "Highest note priority"],
			[6, "2 note cycle"],
			[7, "3 note cycle"],
			[8, "4 note cycle"],
			[9, "2 note chord"],
			[10, "3 note chord"],
			[11, "4 note chord"]
		], this.Priority);		
		ctrl.onchange = function() {obj.Priority = this.value; }
		this.ctrlPriority = ctrl;

		// MIN NOTE
		ctrl = CfgPage.renderNote(row, this.MinNote);
		ctrl.onchange = function() {obj.MinNote = this.value; }
		this.ctrlMinNote = ctrl;

		// MAX NOTE
		ctrl = CfgPage.renderNote(row, this.MaxNote);
		ctrl.onchange = function() {obj.MaxNote = this.value; }
		this.ctrlMaxNote = ctrl;
	
		// MIN VELOCITY
		ctrl = CfgPage.render7Bit(row, this.MinVel);
		ctrl.onchange = function() {obj.MinVel = this.value; }
		this.ctrlMinVel = ctrl;
	
		// BEND RANGE
		ctrl = CfgPage.renderBendRange(row, this.BendRange);
		ctrl.onchange = function() {obj.BendRange = this.value; }
		this.ctrlBendRange = ctrl;
	
		this.xable();
	}
	
	
	
	
};

///////////////////////////////////////////////////////////////////////////////////
// Object to hold configuration of a CV.OCD CV Output
class CVOutput {
	
	///////////////////////////////////////////////////////////////////////////////
	constructor(i) {
		this.Index 			= NRPNH_CV1 + i;
		this.Source 		= NRPVH_SRC_DISABLE;
		this.InputEvent		= 0;
		this.Transpose		= 64;
		this.ChanType		= NRPVH_CHAN_GLOBAL;
		this.ChanNumber		= 0;
		this.CC				= 1;
		this.Volts			= 5;
		this.PitchScheme	= 0;
	}
		
	///////////////////////////////////////////////////////////////////////////////
	syxify(data) {
		let d;
		switch(this.Source) {
			case NRPVH_SRC_STACK1:
			case NRPVH_SRC_STACK2: 
			case NRPVH_SRC_STACK3: 
			case NRPVH_SRC_STACK4:
				d = [this.Index, NRPNL_SRC, this.Source, this.InputEvent];
				switch(this.InputEvent) {
					case 1: case 2: case 3: case 4:
						return d.concat([
							this.Index, NRPNL_PITCHSCHEME, 0, this.PitchScheme,
							this.Index, NRPNL_TRANSPOSE, 0, this.Transpose
						]);
					case 20:
						return d.concat([
							this.Index, NRPNL_VOLTS, 0, this.Volts
						]);
					default:
						return d;
				}
			case NRPVH_SRC_MIDICC:
			case NRPVH_SRC_MIDICC_NEG:
				return [
					this.Index, NRPNL_SRC, this.Source, this.CC,
					this.Index, NRPNL_CHAN, this.ChanType, this.ChanNumber,
					this.Index, NRPNL_VOLTS, 0, this.Volts
				];
			case NRPVH_SRC_MIDIBEND:
			case NRPVH_SRC_MIDITOUCH:
				return [
					this.Index, NRPNL_SRC, this.Source, 0,
					this.Index, NRPNL_CHAN, this.ChanType, this.ChanNumber,
					this.Index, NRPNL_VOLTS, 0, this.Volts
				];
			default:
				return [
					this.Index, NRPNL_SRC, this.Source, 0,
				];
		}
	}

	///////////////////////////////////////////////////////////////////////////////
	unsyxify(data) {
		switch(data[1]) {
			case 1:
				switch(data[2]) {				
					case NRPVH_SRC_STACK1:
					case NRPVH_SRC_STACK2: 
					case NRPVH_SRC_STACK3: 
					case NRPVH_SRC_STACK4:
						this.Source = data[2];
						this.InputEvent = data[3]
						break;
					case NRPVH_SRC_MIDICC:
					case NRPVH_SRC_MIDICC_NEG:
						this.Source = data[2];
						this.CC = data[3];
						break;
					default:
						this.Source = data[2];
				}
				break;
			case NRPNL_CHAN:
				this.ChanType = data[2];
				this.ChanNumber = data[3];
				break;
			case NRPNL_VOLTS:
				this.Volts = data[3];
				break;
			case NRPNL_PITCHSCHEME:
				this.PitchScheme = data[3];
				break;
			case NRPNL_TRANSPOSE:
				this.Transpose = data[3];
				break;
		}
	}		
	
	
	xable() {
		this.ctrlInputEvent.disabled = !(this.Source>=11 &&  this.Source<=14);
		this.ctrlTranspose.disabled = !(this.Source>=11 &&  this.Source<=14);
		this.ctrlPitchScheme.disabled = !(this.Source>=11 &&  this.Source<=14);
		this.ctrlChan.disabled = !(this.Source==2||this.Source==4||this.Source==5);
		this.ctrlCC.disabled = !(this.Source==2);
		this.ctrlVolts.disabled = !(this.Source==2||this.Source==4||this.Source==5||this.Source==20||this.Source==127);
		
	}	
	render(row) {
		let ctrl;
		let obj = this;
		
		// CV SOURCE
		ctrl = CfgPage.renderSelection(row, [
			[0, "(disable)"],
			[11, "Note input #1"],
			[12, "Note input #2"],
			[13, "Note input #3"],
			[14, "Note input #4"],
			[2, "MIDI CC"],
			[4, "Pitch Bend"],
			[5, "Channel Aftertouch"],
			[20, "BPM to CV &dagger;"],
			[127, "Fixed Voltage"]
		], this.Source);		
		ctrl.onchange = function() {obj.Source = this.value; obj.xable();}
		this.ctrlSource = ctrl;

		///
		CfgPage.renderBlankCell(row);
		
		// NOTE INPUT EVENT
		ctrl = CfgPage.renderSelection(row, [		
			[1, "First note pitch"],
			[2, "Second note pitch"],
			[3, "Third note pitch"],
			[4, "Fourth note pitch"],
			[20, "Most recent note velocity"]
		], this.InputEvent);		
		ctrl.onchange = function() {obj.InputEvent = this.value; }
		this.ctrlInputEvent = ctrl;
	
		// TRANSPOSITION
		ctrl = CfgPage.renderTranspose(row, this.Transpose);		
		ctrl.onchange = function() {obj.Transpose = this.value; }
		this.ctrlTranspose = ctrl;

		// PITCH SCHEME
		ctrl = CfgPage.renderSelection(row, [		
			[0, "V/Oct"],
			[1, "Hz/V &dagger;"],
			[2, "1.2V/Oct &dagger;"]
		], this.PitchScheme);
		ctrl.onchange = function() {obj.PitchScheme = this.value; }
		this.ctrlPitchScheme = ctrl;

		///
		CfgPage.renderBlankCell(row);
		
		// MIDI CHANNEL
		ctrl = CfgPage.renderMidiChannel(row, true, this.ChanType, this.ChanNumber);
		ctrl.onchange = function() {obj.ChanType = this.value>>8; obj.ChanNumber = this.value&0xFF; }
		this.ctrlChan = ctrl;
		
		// CC
		ctrl = CfgPage.renderCC(row, this.CC);		
		ctrl.onchange = function() {obj.CC = this.value; }
		this.ctrlCC = ctrl;
		
		// VOLTS
		ctrl = CfgPage.renderSelection(row, [		
			[1, "1V"],
			[2, "2V"],
			[3, "3V"],
			[4, "4V"],
			[5, "5V"],
			[6, "6V"],
			[7, "7V"],
			[8, "8V"]
		], this.Volts);
		ctrl.onchange = function() {obj.Volts = this.value; }
		this.ctrlVolts = ctrl;
			
		this.xable();
	}
};

///////////////////////////////////////////////////////////////////////////////////
// Object to hold configuration of a CV.OCD Gate Output
class GateOutput {
	
	///////////////////////////////////////////////////////////////////////////////
	constructor(i) {
		this.Index 			= NRPNH_GATE1 + i;
		this.Source			= NRPVH_SRC_DISABLE;
		this.InputEvent		= 1;
		this.ChanType		= NRPVH_CHAN_GLOBAL;
		this.ChanNumber		= 0;
		this.MinNote		= 0;
		this.MaxNote		= 127;
		this.MinVel			= 0;
		this.CC				= 1;
		this.Threshold		= 64;
		this.Divider		= 6;
		this.Offset			= 0;
		this.DurationType	= NRPVH_DUR_GLOBAL;
		this.Duration		= 0;
	}
	
	///////////////////////////////////////////////////////////////////////////////
	syxify() {
		let d;
		switch(this.Source) {		
			case NRPVH_SRC_STACK1:
			case NRPVH_SRC_STACK2:
			case NRPVH_SRC_STACK3:
			case NRPVH_SRC_STACK4:
				d = [
					this.Index, NRPNL_SRC, this.Source, this.InputEvent
				];
				break;
			case NRPVH_SRC_MIDINOTE:
				d = [
					this.Index, NRPNL_SRC, NRPVH_SRC_MIDINOTE, this.MinNote,
					this.Index, NRPNL_CHAN, this.ChanType, this.ChanNumber,
					this.Index, NRPNL_NOTE_MAX, 0, this.MaxNote,
					this.Index, NRPNL_VEL_MIN, 0, this.MinVel
				];		
				break;				
			case NRPVH_SRC_MIDICC:
			case NRPVH_SRC_MIDICC_NEG:
				d = [
					this.Index, NRPNL_SRC, this.Source, this.CC,
					this.Index, NRPNL_CHAN, this.ChanType, this.ChanNumber,
					this.Index, NRPNL_THRESHOLD, 0, this.Threshold
				];
				break;
			case NRPVH_SRC_MIDITICK:
			case NRPVH_SRC_MIDITICKRUN:
				d = [
					this.Index, NRPNL_SRC, this.Source, this.Divider,
					this.Index, NRPNL_TICK_OFS, 0, this.Offset			
				];
				break;
			default:
				d = [
					this.Index, NRPNL_SRC, this.Source, 0
				];
				break;
		}
		return d.concat([
			this.Index, NRPNL_GATE_DUR, this.DurationType, this.Duration	
		]);
	}
	
	///////////////////////////////////////////////////////////////////////////////
	unsyxify(data) {
		switch(data[1]) {
			case NRPNL_SRC:
				switch(data[2]) {
					case NRPVH_SRC_STACK1:
					case NRPVH_SRC_STACK2: 
					case NRPVH_SRC_STACK3: 
					case NRPVH_SRC_STACK4:
						this.Source = data[2];
						this.InputEvent = data[3];
						break;
					case NRPVH_SRC_MIDINOTE:
						this.Source = data[2];
						this.MinNote = data[3];
						break;
					case NRPVH_SRC_MIDICC:
					case NRPVH_SRC_MIDICC_NEG:
						this.Source = data[2];
						this.CC = data[3];
						break;
					case NRPVH_SRC_MIDITICK:
					case NRPVH_SRC_MIDITICKRUN:
						this.Source = data[2];
						this.Divider = data[3];			
						break;
					default:
						this.Source = data[2];
						break;
				}
				break;
			case NRPNL_CHAN:
				this.ChanType = data[2];
				this.ChanNumber = data[3];
				break;
			case NRPNL_NOTE_MAX:
				this.MaxNote = data[3];
				break;
			case NRPNL_VEL_MIN:
				this.MinVel = data[3];
				break;
			case NRPNL_GATE_DUR:
				this.DurationType = data[2];
				this.Duration = data[3];
				break;
			case NRPNL_TICK_OFS:
				this.Offset = data[3];
				break;
			case NRPNL_THRESHOLD:
				this.Threshold = data[3];
				break;
		}
	}
	xable() {
	}	
	render(row) {
		let ctrl;
		let obj = this;
		
		// GATE SOURCE
		ctrl = CfgPage.renderSelection(row, [
			[0, "(disable)"],
			[11, "Note input #1"],
			[12, "Note input #2"],
			[13, "Note input #3"],
			[14, "Note input #4"],
			[1, "MIDI note"],
			[2, "CC above threshold"],
			[3, "CC below threshold"],
			[20, "Clock Tick"],
			[21, "Clock Tick+Run"],
			[22, "Transport Run"],
			[23, "Transport Restart"],
			[25, "Transport Stop"],
			[26, "Trans. Start/Stop &dagger;"]
		], this.Source);		
		ctrl.onchange = function() {obj.Source = this.value; obj.xable();}
		this.ctrlSource = ctrl;
		
		///
		CfgPage.renderBlankCell(row);
		
		// GATE EVENT
		ctrl = CfgPage.renderSelection(row, [
			[1, "First note on"],
			[2, "Second note on"],
			[3, "Third note on"],
			[4, "Fourth note on"],
			[5, "Any note on"],
			[0, "All notes off"]
		], this.InputEvent);		
		ctrl.onchange = function() {obj.InputEvent = this.value; obj.xable();}
		this.ctrlInputEvent = ctrl;

		///
		CfgPage.renderBlankCell(row);
		
		// MIDI CHANNEL
		ctrl = CfgPage.renderMidiChannel(row, true, this.ChanType, this.ChanNumber);
		ctrl.onchange = function() {obj.ChanType = this.value>>8; obj.ChanNumber = this.value&0xFF; }
		this.ctrlChan = ctrl;
		
		// MIN NOTE
		ctrl = CfgPage.renderNote(row, this.MinNote);
		ctrl.onchange = function() {obj.MinNote = this.value; }
		this.ctrlMinNote = ctrl;

		// MAX NOTE
		ctrl = CfgPage.renderNote(row, this.MaxNote);
		ctrl.onchange = function() {obj.MaxNote = this.value; }
		this.ctrlMaxNote = ctrl;
	
		// MIN VELOCITY
		ctrl = CfgPage.render7Bit(row, this.MinVel);
		ctrl.onchange = function() {obj.MinVel = this.value; }
		this.ctrlMinVel = ctrl;

		///
		CfgPage.renderBlankCell(row);
		
		// CC
		ctrl = CfgPage.renderCC(row, this.CC);		
		ctrl.onchange = function() {obj.CC = this.value; }
		this.ctrlCC = ctrl;
		
		// CC THRESHOLD
		ctrl = CfgPage.render7Bit(row, this.Threshold);		
		ctrl.onchange = function() {obj.Threshold = this.value; }
		this.ctrlThreshold = ctrl;

		///
		CfgPage.renderBlankCell(row);

		// CLOCK DIVIDER
		ctrl = CfgPage.renderSelection(row, [
			[3, "1/32"],
			[4, "1/16T"],
			[6, "1/16"],
			[8, "1/8T"],
			[9, "1/16D"],
			[12, "1/8"],
			[16, "1/4T"],
			[18, "1/8D"],
			[24, "1/4"],
			[32, "1/2T"],
			[36, "1/4D"],
			[48, "1/2"],
			[72, "1/2D"],
			[96, "1"],
			[1, "24ppqn"],
		], this.Divider);		
		ctrl.onchange = function() {obj.Divider = this.value; obj.xable();}
		this.ctrlDivider = ctrl;

		// CLOCK OFFSET
		ctrl = CfgPage.renderClockOffset(row, this.Offset);		
		ctrl.onchange = function() {obj.Offset = this.value; }
		this.ctrlOffset = ctrl;

		///
		CfgPage.renderBlankCell(row);
		
		// GATE DURATION
		ctrl = CfgPage.renderGateDuration(row, true, this.DurationType, this.Duration);
		ctrl.onchange = function() {obj.DurationType = this.value>>8; obj.Duration = this.value&0xFF; }
		this.ctrlDuration = ctrl;

		this.xable();
	}
};

///////////////////////////////////////////////////////////////////////////////////
// Object to hold complete CV.OCD configuration patch
class Patch {
	
	///////////////////////////////////////////////////////////////////////////////
	constructor() {
		this.ChanType 		= NRPVH_CHAN_SPECIFIC;
		this.ChanNumber		= 1;
		this.DurationType	= NRPVH_CHAN_SPECIFIC;
		this.Duration		= 15;
		this.AutoSave		= 1;
		this.NoteInputs		= [];
		this.CVOutputs		= [];
		this.GateOutputs	= [];
		for(let i=0;i<NUM_NOTE_INPUTS; ++i) {
			this.NoteInputs.push(new NoteInput(i));
		}
		for(let i=0;i<NUM_CV_OUTPUTS; ++i) {
			this.CVOutputs.push(new CVOutput(i));
		}
		for(let i=0;i<NUM_GATE_OUTPUTS; ++i) {
			this.GateOutputs.push(new GateOutput(i));
		}
	}
	
	///////////////////////////////////////////////////////////////////////////////
	syxify() {
		let sysex = [ 
			SYSEX_BEGIN, MANUF_ID_0, MANUF_ID_1, MANUF_ID_2,		
			NRPNH_GLOBAL, NRPNL_CHAN, this.ChanType, this.ChanNumber,
			NRPNH_GLOBAL, NRPNL_GATE_DUR, this.DurationType, this.Duration		
		];		
		for(let o of this.NoteInputs) {
			sysex = sysex.concat(o.syxify());
		}
		for(let o of this.CVOutputs) {
			sysex = sysex.concat(o.syxify());
		}
		for(let o of this.GateOutputs) {
			sysex = sysex.concat(o.syxify());
		}
		return sysex.concat([SYSEX_END]);
	}
	
	///////////////////////////////////////////////////////////////////////////////
	unsyxify(data) {
		if((data[0] != SYSEX_BEGIN) ||
			(data[1] != MANUF_ID_0) ||
			(data[2] != MANUF_ID_1) ||
			(data[3] != MANUF_ID_2)) {
			return false;
		}
		while(data.length >= 4) {
			if(data[0] == NRPNH_GLOBAL) {
				switch(data[1]) {
					case NRPNL_CHAN:
						this.ChanType = data[2];
						this.ChanNumber = data[3];
						break;
					case NRPNL_GATE_DUR:
						this.DurationType = data[2];
						this.Duration = data[3];
						break;
					default:
						return false;
				}
			}
			else if(data[0] >= NRPNH_STACK1 && data[0] <= NRPNH_STACK4) {				 
				this.NoteInputs[data[0]-NRPNH_STACK1].unsyxify(data);
			}
			else if(data[0] >= NRPNH_CV1 && data[0] <= NRPNH_CV4) {
				this.CVOutputs[data[0]-NRPNH_CV1].unsyxify(data);
			}
			else if(data[0] >= NRPNH_GATE1 && data[0] <= NRPNH_GATE12) {
				this.GateOutputs[data[0]-NRPNH_GATE1].unsyxify(data);
			}
			data = data.slice(4);
		}	
		if(data.length != 1 || data[0] != SYSEX_END) {
			return false;
		}
		return true;
	}
	
	render() {
		let table, ctrl, tr;
		let obj = this;		

		table = document.getElementById("global_settings");		
		tr = document.createElement("TR");

		CfgPage.renderBlankCell(tr);
		
		ctrl = CfgPage.renderMidiChannel(tr, false, this.ChanType, this.ChanNumber);
		ctrl.onchange = function() {obj.ChanType = this.value>>8; obj.ChanNumber = this.value&0xFF; }
		this.ctrlChan = ctrl;

		CfgPage.renderBlankCell(tr);
		
		ctrl = CfgPage.renderGateDuration(tr, false, this.DurationType, this.Duration);
		ctrl.onchange = function() {obj.DurationType = this.value>>8; obj.Duration = this.value&0xFF; }
		this.ctrlDuration = ctrl;

		table.appendChild(tr);
	
		table = document.getElementById("note_inputs");		
		for(let i=0; i<NUM_NOTE_INPUTS; ++i) {
			let tr = document.createElement("TR");
			let cell = document.createElement("TD");
			cell.innerHTML = "Input." + (1+i);
			tr.appendChild(cell);
			this.NoteInputs[i].render(tr);
			table.appendChild(tr);
		}

		table = document.getElementById("cv_outputs");
		for(let i=0; i<NUM_CV_OUTPUTS; ++i) {
			let tr = document.createElement("TR");
			let cell = document.createElement("TD");
			cell.innerHTML = "CV." + String.fromCharCode(65+i);
			tr.appendChild(cell);
			this.CVOutputs[i].render(tr);
			table.appendChild(tr);
		}

		table = document.getElementById("gate_outputs");
		for(let i=0; i<NUM_CV_OUTPUTS; ++i) {
			let tr = document.createElement("TR");
			let cell = document.createElement("TD");
			cell.innerHTML = "Gate." + (1+i);
			tr.appendChild(cell);
			this.GateOutputs[i].render(tr);
			table.appendChild(tr);
		}
		
	}
	
};