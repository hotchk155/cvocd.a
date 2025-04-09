<html>
<head>
<style>
body {font-family: sans-serif}
td {vertical-align: top}
.cal-settings {font-weight: bold; font-size: 18pt}
.scale-text { font-weight: bold; font-size: 14pt}
.instruction { color: gray; font-size: 10pt; font-style: italic}
.pg-heading {vertical-align: middle; font-size: 14pt; font-weight: bold; color: white; background: black; text-align: center}
.cv-heading {vertical-align: middle; font-size: 14pt}
.tp-main { color:black; background: #ccffcc }
.tp-mid { color:black; background: #cccccc }
.tp-outer { color:black; background: #eeeeee }
</style>
</head>
<body>

<form name="sysex_file" action="patch.asp?file=true" enctype="multipart/form-data" method="post">
<table style="width: 640; border:thin solid black; background:#ffffee; cellpadding=5">
<tr><td>1. Select sysex file</td>
<td>:</td>
<td><input type="file" name="file" value="File"></td>
<td><input type="submit" value="Load SysEx Into Form"></td>
<tr>
</table>
</form>

<table style="width: 640; border:thin solid black; background:#ffffee; cellpadding=5">
<tr><td class="pg-heading">CONVERT SYSEX FILE TO NRPN</td></tr>
<tr><td class="instruction">
This page uses the WebMIDI API and requires Chrome (probably)
</td></tr>
</table>

<br>


<table style="width: 640; border:thin solid black; background:#ffffee; cellpadding=5">
<tr><td class="cv-heading">2. Select your MIDI output device</td></tr>
<tr><td class="instruction">
Please select the MIDI interface to which the device is attached. If you need to plug in a USB MIDI interface please connect it and reload the page.
</td></tr>
<tr><td><select id="midi-outputs"><option></option></select></td></tr>
</table>

<br>

<table style="width: 640; border:thin solid black; background:#ffffee; cellpadding=5">
<tr><td class="cv-heading">3. Send the patch</td></tr>
<tr><td class="instruction">
..
</td></tr>
<tr><td><input type='button' value="Prepare" onclick='javascript:onClickSendPatch();'></td></tr>
</table>

<script>
var g_nrpnList = null;
var g_nrpnNumber = 0;
function sendMidiNrpnListAsync2() {
	if(g_nrpnNumber < g_nrpnList.length) {
		sendMidiNrpn(
			g_nrpnList[g_nrpnNumber][0],
			g_nrpnList[g_nrpnNumber][1],
			g_nrpnList[g_nrpnNumber][2],
			g_nrpnList[g_nrpnNumber][3]
		);
		g_nrpnNumber++;
		setTimeout(sendMidiNrpnListAsync2, 20);
	}
	else {
		g_nrpnList = null;
		g_nrpnNumber = 0;
	}
}
function sendMidiNrpnListAsync(nrpnList) {
	if(midiOutput != null) {
		g_nrpnList = nrpnList;
		g_nrpnNumber = 0;
		sendMidiNrpnListAsync2();
	}
}


function onClickSendPatch() {
	let patch = [
<%
Const MANUF_ID_0 = &H00
Const MANUF_ID_1 = &H7F 
Const MANUF_ID_2 = &H15	
function ReadSysEx
	Dim data
	ReadSysEx = false
	data = request.BinaryRead(request.TotalBytes)
	
	index = 0
	for i=1 to lenb(data) - 4
		if midb(data,i,4) = (chrb(13) & chrb(10) & chrb(13) & chrb(10)) then
			index = i + 4
			exit for
		end if
	next	
	if index = 0 then
		exit function 
	end if
    if (midb(data,index,4) <> (ChrB(&HF0) & ChrB(MANUF_ID_0) & ChrB(MANUF_ID_1) & ChrB(&H12))) and _   
		(midb(data,index,4) <> (ChrB(&HF0) & ChrB(MANUF_ID_0) & ChrB(MANUF_ID_1) & ChrB(MANUF_ID_2))) then
		exit function 
	end if
	index = index + 4
	
	do while index <= lenb(data) 
		if midb(data,index,1) = chrb(&hF7) then exit do
		if index + 4 >= lenb(data) then exit function
%>[<%=ascb(midb(data,index,1))%>, <%=ascb(midb(data,index+1,1))%>, <%=ascb(midb(data,index+2,1))%>, <%=ascb(midb(data,index+3,1))%>],
<%
		index = index + 4		
	loop
	ReadSysEx = true
end function	


%>
	];
	sendMidiNrpnListAsync(patch);
}
</script>
</body>
</html>