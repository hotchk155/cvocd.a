const electron = require('electron');
const url = require('url');
const path = require('path');
const fs = require('fs');
const {app, dialog, BrowserWindow, Menu} = electron;
let mainWindow;

const SYSEX_BEGIN 		= 0xF0;
const SYSEX_END 		= 0xF7;
const MANUF_ID_0 		= 0x00;
const MANUF_ID_1 		= 0x7F;
const MANUF_ID_2 		= 0x15;		

app.on('ready', function() {
    mainWindow = new BrowserWindow({});

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
                label: 'Save As...'
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
        let buf  = fs.readFileSync(file[0]);
        let data = [...buf];
        if(data.length < 5 ||
            data[0] !=SYSEX_BEGIN ||
            data[1] != MANUF_ID_0 ||
            data[2] != MANUF_ID_1 ||
            data[3] != MANUF_ID_2 ||
            data[data.length - 1] != SYSEX_END) {
                alert("Bad");
                return;
        }
    }
}