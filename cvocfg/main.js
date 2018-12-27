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
let mainWindow;
let currentFileName = null;

function setCurrentFileName(name) {
    currentFileName = name;
    if(name == null) {
        mainWindow.setTitle(app.getName());    
    }
    else {
        mainWindow.setTitle(app.getName() + ' - ' +name);    
    }
}


app.on('ready', function() {
    mainWindow = new BrowserWindow({});

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

        
});


const mainMenuTemplate = [
    {
        label: 'File',
        submenu:[
            {
                label: 'Open...',
                click() {
                    loadSysex();
                }
            },
            {
                label: 'Save',
                click() {                    
                    mainWindow.webContents.send('save-rq', currentFileName);
                }
            },
            {
                label: 'Save As...',
                click() {                    
                    mainWindow.webContents.send('save-rq', null);
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

function loadSysex() {
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

ipcMain.on('save-rs', function(e, saveAs, data) {
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

});