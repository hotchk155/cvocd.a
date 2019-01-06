///////////////////////////////////////////////////////////////////////////////////
//
// CONFIG PAGE EVENT HANDLING
//
///////////////////////////////////////////////////////////////////////////////////

const electron = require('electron');
const {ipcRenderer} = electron;
let midiInterface = null;

///////////////////////////////////////////////////////////////////////////////
// HANDLER init-midi-rq
// To be called just once - opens  up the WebMIDI interface and send back an 
// init-midi-rs response conrtaining an array of output device names
ipcRenderer.on('init-midi-rq', function(e) {
    midiInterface = null;
    if (navigator.requestMIDIAccess) {
        navigator.requestMIDIAccess({
        sysex: true
        }).then(
            function(handle) {
                midiInterface = handle;
                let devices = [];
                for (let output of midiInterface.outputs.values()) {
                    devices.push(output.name);
                }
                e.sender.send('init-midi-rs', devices);
            },
            function() {
                alert("Failed to open MIDI interface");
                e.sender.send('init-midi-rs', null);
            }
        )
    } else {
        alert("Your browser does not support webMIDI");
        e.sender.send('init-midi-rs', null);
    }    
});

///////////////////////////////////////////////////////////////////////////////
// HANDLER load-midi
// receives an array of MIDI SYSEX file bytes and uses the data to initialise 
// the form
ipcRenderer.on('load-midi', function(e, midi) {
    patch = new Patch(); 
    try {
        patch.put_midi(midi);
        patch.render();
    }
    catch(e) {
        alert(e.message);
    }
    patch.render();
});

///////////////////////////////////////////////////////////////////////////////
// HANDLER save-midi-rq
// fetches an array of MIDI SYSEX file bytes and sends them back to the main
// process in a save-midi-rs message
ipcRenderer.on('save-midi-rq', function(e) {
    let data = patch.get_midi();  
    e.sender.send('save-midi-rs', data)
});

///////////////////////////////////////////////////////////////////////////////
// HANDLER send-midi
// sends the contents of the form to a MIDI output based on the device name 
// parameter (a string)
ipcRenderer.on('send-midi', function(e, device) {
    let thisOutput = null;
    if(midiInterface != null) {
        try {
            for (let output of midiInterface.outputs.values()) {
                if(output.name == device) {
                    thisOutput = output;
                    break;  
                }
            }
            if(thisOutput == null) {
                alert("Please reselect MIDI output");
            }
            else {
                let midi = patch.get_midi();          
                thisOutput.send(midi);                                
            }
        }
        catch(e) {
            alert(e.message);
        }
    }
    else {
        alert("Please reselect MIDI output");
    }
});
