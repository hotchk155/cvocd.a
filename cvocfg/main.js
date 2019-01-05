const SYSEX_BEGIN 		= 0xF0;
const SYSEX_END 		= 0xF7;
const MANUF_ID_0 		= 0x00;
const MANUF_ID_1 		= 0x7F;
const MANUF_ID_2 		= 0x15;		
const productName       = "CV.OCD";

const electron = require('electron');
const url = require('url');
const path = require('path');
const fs = require('fs');
const {app, dialog, BrowserWindow, Menu, ipcMain } = electron;
let mainWindow = null;
let midiLinkWindow = null;
let currentFileName = null;
let midiOutputTag = null;

///////////////////////////////////////////////////////////////////////////////
function setCurrentFileName(name) {
    currentFileName = name;
    if(name == null) {
        mainWindow.setTitle(productName + " Configuration");    
    }
    else {
        mainWindow.setTitle(productName + " Configuration" + ' - ' +name);    
    }
}


///////////////////////////////////////////////////////////////////////////////
function doMidiLink() {
    ipcMain.removeAllListeners('syxify-rs');
    ipcMain.once('syxify-rs', function(e, mode, data) {
        showMidiLinkWindow(data);
    });
    mainWindow.webContents.send('syxify-rq', null);
}

///////////////////////////////////////////////////////////////////////////////
function showMidiLinkWindow(data) {
    midiLinkWindow= new BrowserWindow({parent:mainWindow, frame:false, modal:true, resizable:false});
    midiLinkWindow.setMenu(null);
    midiLinkWindow.webContents.on('did-finish-load', function() {
        midiLinkWindow.webContents.send('midi-init-rq', data, midiOutputTag);
    });
    midiLinkWindow.loadURL(url.format({
        pathname: path.join(__dirname, 'midilink.html'),
        protocol:'file:',
        slashes:true
    }));
    midiLinkWindow.webContents.on('destroyed', function() {
        midiLinkWindow = null;
    });
}


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
                    onSaveSysex(currentFileName);
                }
            },
            {
                label: 'Save As...',
                click() {                    
                    onSaveSysex(null);
                }
            },
            {
                label: 'Clear',
                click() {
                    setCurrentFileName(null);
                    mainWindow.webContents.send('init-rq');
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
                label: 'Interface...',
                click() {
                    doMidiLink(); 
                }
            },
            {
                label: 'Send Sysex'
            }
        ]
    }
];

if(process.env.NODE_ENV !== 'production'){
    mainMenuTemplate.push({
        label: 'DevTools',
        click(item, focusedWindow) {
            focusedWindow.toggleDevTools();
        }
    });
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
            if(data.length < 5 ||
                data[0] !=SYSEX_BEGIN ||
                data[1] != MANUF_ID_0 ||
                data[2] != MANUF_ID_1 ||
                data[3] != MANUF_ID_2 ||
                data[data.length - 1] != SYSEX_END) {
                    dialog.showErrorBox("Invalid File", "The selected file is not a valid " + productName + " configuration");
                    return;
            }
            mainWindow.webContents.send('init-rq', data);
            setCurrentFileName(file[0]);
        }
        catch(e) {
            dialog.showErrorBox("Failed To Load", e.message);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
function onSaveSysex(fileName) {
    ipcMain.removeAllListeners('syxify-rs');
    ipcMain.once('syxify-rs', function(e, saveAs, data) {
        saveSysexFile(saveAs, data);
    });
    mainWindow.webContents.send('syxify-rq', fileName);
}

///////////////////////////////////////////////////////////////////////////////
function saveSysexFile(saveAs, data) 
{
    if(saveAs == null) {
        let file = dialog.showSaveDialog( mainWindow, {
            filters: [
                { name: 'System Exclusive Dump', extensions: ['syx'] },
                { name: 'All Files', extensions: ['*'] }
            ]
        });
        if(file == undefined || !file.length) {
            return;
        }
        saveAs = file;        
    }
    
    try {        
        let syx = [SYSEX_BEGIN, MANUF_ID_0, MANUF_ID_1, MANUF_ID_2];
        syx = syx.concat(data);
        syx.push(SYSEX_END);
        fs.writeFileSync(saveAs, new Buffer(syx));
        setCurrentFileName(saveAs);
    }
    catch(e) {
        dialog.showErrorBox("Failed To Save", e.message);
    }
}


///////////////////////////////////////////////////////////////////////////////
ipcMain.on('midi-hide-rq', function() {
    midiLinkWindow.hide();
    midiLinkWindow = null;    
});

///////////////////////////////////////////////////////////////////////////////
ipcMain.on('midi-select-rq', function(e, tag) {
    midiOutputTag = tag;
});

///////////////////////////////////////////////////////////////////////////////
app.on('ready', function() {
    mainWindow = new BrowserWindow({width: 1150, height: 700});

    mainWindow.webContents.on('did-finish-load', function() {
        mainWindow.webContents.send('init-rq');
    });

    mainWindow.loadURL(url.format({
        pathname: path.join(__dirname, 'patch.html'),
        protocol:'file:',
        slashes:true
    }));

    const mainMenu = Menu.buildFromTemplate(mainMenuTemplate);
    Menu.setApplicationMenu(mainMenu);

    setCurrentFileName(null);
});

