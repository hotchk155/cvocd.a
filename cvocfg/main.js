const electron = require('electron');
const url = require('url');
const path = require('path');
const fs = require('fs');
const {app, dialog, BrowserWindow, Menu, ipcMain } = electron;
let mainWindow = null;
let midiDeviceWindow = null;
let currentFileName = null;
let midiOutputDevice = null;
let midiOutputDevices = null;


///////////////////////////////////////////////////////////////////////////////
const mainMenuTemplate = [
    {
        label: 'File',
        submenu:[
            {
                label: 'Open...',
                click() {
                    onOpenSysex();
                }
            },
            {
                label: 'Save',
                click() {    
                    onSaveAs(currentFileName);
                }
            },
            {
                label: 'Save As...',
                click() {                    
                    onSaveAs(null);
                }
            },
            {
                label: 'Quit',                
                click() {
                    app.quit();
                }
            },
        ],
    },
    {
        label: 'MIDI',
        submenu:[
            {
                label: 'Device...',
                click() {
                    onSelectMidiDevice(); 
                }
            },
            {
                label: 'Send Sysex',
                click() {
                    onSendSysex();
                }
            }
        ]
    }
];

///////////////////////////////////////////////////////////////////////////////
//if(process.env.NODE_ENV !== 'production'){
//    mainMenuTemplate.push({
//        label: 'DevTools',
//        click(item, focusedWindow) {
//            focusedWindow.toggleDevTools();
//        }
//    });
//}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION setCurrentFileName
// stores the current .SYX filename and updates the main window title
function setCurrentFileName(name) {
    currentFileName = name;
    let title = app.getName();
    if(name != null) {
        title += " - " + name;
    }
    mainWindow.setTitle(title);
}

///////////////////////////////////////////////////////////////////////////////
function onOpenSysex() {
    let file = dialog.showOpenDialog( mainWindow, {
        properties: ['openFile'],
        filters: [
            { name: 'System Exclusive Dump', extensions: ['syx'] },
            { name: 'All Files', extensions: ['*'] }
        ]            
    });
    if(file != undefined && file.length == 1) {
        try {
            let buf  = fs.readFileSync(file[0]);
            let data = [...buf];
            mainWindow.webContents.send('load-midi', data);
            setCurrentFileName(file[0]);
        }
        catch(e) {
            dialog.showErrorBox("Failed To Load", e.message);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION onSaveAs
// "Save As" command handler
function onSaveAs(name) {

    // do we have a current file name?
    if(name == null) {
        // no, so choose one
        let file = dialog.showSaveDialog( mainWindow, {
            filters: [
                { name: 'System Exclusive Dump', extensions: ['syx'] },
                { name: 'All Files', extensions: ['*'] }
            ]
        });
        if(file == undefined || !file.length) {
            return;
        }
        name = file;        
    }

    // prepare the handler to receive the serialised SYSEX
    // data from the form and save it to the file
    ipcMain.once('save-midi-rs', function(e, data) {
        try {        
            fs.writeFileSync(name, new Buffer(data));
            setCurrentFileName(name);
        }
        catch(e) {
            dialog.showErrorBox("Failed To Save", e.message);
            setCurrentFileName(null);
        }
    });

    // request the serialized SYSEX data from the form
    mainWindow.webContents.send('save-midi-rq');
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION onSelectMidiDevice
// 
function onSelectMidiDevice() {
    midiDeviceWindow.webContents.send('init-devices', midiOutputDevices, midiOutputDevice);    
    midiDeviceWindow.show();
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION onSendSysex
function onSendSysex() {
    if(midiOutputDevice == null) {
        onSelectMidiDevice();
    }
    else {
        mainWindow.webContents.send('send-midi', midiOutputDevice);
    }
}

///////////////////////////////////////////////////////////////////////////////
// MESSAGE init-midi-rs
// Response to init-midi-rq from the main window. It provides us with a list
// of available MIDI outputs in an array of strings
ipcMain.on('init-midi-rs', function(e, devices) {
    midiOutputDevices = devices;
});

///////////////////////////////////////////////////////////////////////////////
// MESSAGE select-device
// Called when the user closes the MIDI device window, with or without 
// selecting a device from the list. When cancelled the device parameter 
// is null
ipcMain.on('select-device', function(e, device) {
    if(device != null) {
        midiOutputDevice = device;
    }
    midiDeviceWindow.hide();
});

///////////////////////////////////////////////////////////////////////////////
// MESSAGE app ready
// When application is ready
app.on('ready', function() {
    mainWindow = new BrowserWindow({width: 1150, height: 700});

    mainWindow.webContents.on('did-finish-load', function() {
        mainWindow.webContents.send('init-midi-rq');
    });

    mainWindow.loadURL(url.format({
        pathname: path.join(__dirname, 'patch.html'),
        protocol:'file:',
        slashes:true
    }));

    const mainMenu = Menu.buildFromTemplate(mainMenuTemplate);
    Menu.setApplicationMenu(mainMenu);

    midiDeviceWindow = new BrowserWindow({parent:mainWindow, frame:true, modal:true, resizable:false, show: false});
    midiDeviceWindow.setMenu(null);
    midiDeviceWindow.loadURL(url.format({
        pathname: path.join(__dirname, 'midilink.html'),
        protocol:'file:',
        slashes:true
    }));

    setCurrentFileName(null);
});

