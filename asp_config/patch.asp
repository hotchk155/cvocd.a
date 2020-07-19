<!--#include file ="sysex.inc"-->
<!--#include file ="render.inc"-->
<head>
<style>
body {
    font-family: arial;
}

table {
    font-family: arial;
    font-size: 12px;
    margin-top: 0px;
    margin-bottom: 0px;
    margin-right: 0px;
    margin-left: 0px;
    padding: 0px;
	line-height: 1;
}
</style>
</head>
<body>
<h1>CV.OCD SysEx Builder</h1>
<%
	InitMappings
	
	If Request.QueryString("file") <> "" Then
		If Not ReadSysex Then
%><b style="color:red">ERROR IN SYSEX FILE - NOT LOADED!!</b><%		
		End If
	End If
%>

<hr>
<table width=800><tr><td>
<center>
<img width=600 src="img/cvocd.gif">
</center>
<hr>
<form name="sysex_file" action="patch.asp?file=true" enctype="multipart/form-data" method="post">
<table>
<tr><td>You can load an existing CV.OCD sysex into this form, or create a new one from scratch<br>
Click <a href="https://github.com/hotchk155/cvocd.a/blob/master/patches/cvocd_default.syx?raw=true">here</a> to download the default patch (as loaded on a new CV.OCD)
</td>
<td>:</td>
<td><input type="file" name="file" value="File"></td>
<td><input type="submit" value="Load SysEx Into Form"></td>
<tr>
</table>
</form>
<form name="sysex" action="get_sysex.asp" method="post">
<%
' ================================================================================================
' GLOBALS
' ================================================================================================
%>
<hr>
<p><b>Globals</b> (TIP: hover mouse over field names for a description)</p>
<table>
<tr>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select the MIDI channel to use for all mapings that use MIDI channel '(default)'">Default Channel</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select the millsecond duration for gate outputs that are set as '(trig)'">Default Trigger</td>
</tr>
<%
	Set o = dictMappings.item(1)
	Response.Write "<tr><td>"
	Response.Write "</td><td>"
	RenderChannel o, False
	Response.Write "</td><td>"
	Response.Write "</td><td>"
	RenderGateDuration o, False
	Response.Write "</td><tr>"
%>
</table>
<%
' ================================================================================================
' NOTE INPUTS
' ================================================================================================
%>
<hr>
<p><b>Note Inputs</b></p>
<table>
<tr>
<td></td>
<td title="Select whether this note input is enabled and listening to MIDI.&#10;You should disable any inputs you don't need.">Enable</td>
<td title="Select the MIDI channel that this note input will listen to">Channel</td>
<td title="When multiple notes are held, this setting determines which of the notes should be be played on the outputs">Mode</td>
<td title="You can limit the range of notes that this note input will play (for example to split the keyboard between two outputs)&#10;Select min note = 0 and max note = 127 to play any note on the selected MIDI channel">Min.Note</td>
<td title="Maximum note of the range. If you select 'single' then only the single note selected in min note will play.">Max.Note</td>
<td title="You can filter notes that will play on the input based on their MIDI note velocity.&#10;Only notes that are equal or higher in velocity can play.">Min Vel.</td>
<td title="Select the pitch bend range for this note input.">Pitch Bend</td>
</tr>
<%
for count = 1 to 4
	Set o = dictMappings.item(10 + count)
	Response.Write "<tr><td>"
	Response.Write "Input." & count
	Response.Write "</td><td>"
	RenderInputSource o
	Response.Write "</td><td>"
	RenderChannel o, True
	Response.Write "</td><td>"
	RenderPriority o
	Response.Write "</td><td>"
	RenderMinNote o
	Response.Write "</td><td>"
	RenderMaxNote o
	Response.Write "</td><td>"
	RenderMinVel o
	Response.Write "</td><td>"
	RenderBendRange o
	Response.Write "</td></tr>"	
next	
%>
<script language="javascript">
var o_form = document.forms[1];
function XAble_Input(id) {
	var d = (document.getElementById("in" + id + ".src").value != "1");
	document.getElementById("in" + id + ".chan").disabled = d;
	document.getElementById("in" + id + ".prty").disabled = d;
	document.getElementById("in" + id + ".min_note").disabled = d;
	document.getElementById("in" + id + ".max_note").disabled = d;
	document.getElementById("in" + id + ".min_vel").disabled = d;
	document.getElementById("in" + id + ".bend").disabled = d;
}
<% For count = 1 to 4 %>
document.getElementById("in<%=count%>.src").onchange=function(){XAble_Input(<%=count%>);}
XAble_Input(<%=count%>);
<% Next %>	
</script>

</table>

<%
' ================================================================================================
' CV OUTPUTS
' ================================================================================================
%>
<hr>
<p><b>CV outputs</b></p>
<table>
<tr>
<td></td>
<td title="Select the source of the note or voltage level that this CV output will play.">CV Source</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="If you select a note input, this decides which note (or other parameter)&#10;from that note input will be mapped to the CV output">Note input</td>
<td title="If you select a note input, you can transpose the output note CV.&#10;This only works for notes (not velocity, bend etc)">Transpose</td>
<td title="For note pitch CV this is the voltage scaling scheme">Scheme</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="If you select a MIDI CC as input, you specify the channel here">Chan</td>
<td title="If you select a MIDI CC as input, you specify the CC number here">CC#</td>
<td title="For non-musical note CV this is the full voltage range">Range</td>
</tr>
<%
for count = 1 to 4
	Set o = dictMappings.item(20 + count)
	Response.Write "<tr><td>"
	Response.Write "CV." & Chr(64+count)
	Response.Write "</td><td>"
	RenderCVSource o
	Response.Write "</td><td>"
	Response.Write "</td><td>"
	RenderCVEvent o
	Response.Write "</td><td>"
	RenderTranspose o
	Response.Write "</td><td>"
	RenderPitchScheme o
	Response.Write "</td><td>"
	Response.Write "</td><td>"
	RenderChannel o, True
	Response.Write "</td><td>"
	RenderCC o
	Response.Write "</td><td>"
	RenderVolts o
	Response.Write "</td></tr>"
next	
%>
<script language="javascript">
var o_form = document.forms[1];
function XAble_CVOutput(id) {
	var val = (document.getElementById("cv" + id + ".src").value);
	var val2 = (document.getElementById("cv" + id + ".event").value);
	var d_ev = !(val == "11" || val == "12" || val == "13" || val == "14");
	var d_vel = (val2 == "20");
	var d_ch = !(val == "2" || val=="3" || val=="4" || val == "5");
	var d_cc = !(val == "2");
	var d_volts = !(val == "2" || val=="4" || val == "5" || val == "20" || val=="127" || (!d_ev && val2=="20"));
	document.getElementById("cv" + id + ".event").disabled = d_ev;
	document.getElementById("cv" + id + ".trans").disabled = d_ev||d_vel;
	document.getElementById("cv" + id + ".scheme").disabled = d_ev||d_vel;
	document.getElementById("cv" + id + ".chan").disabled = d_ch;
	document.getElementById("cv" + id + ".cc").disabled = d_cc;
	document.getElementById("cv" + id + ".volts").disabled = d_volts;
}
<% For count = 1 to 4 %>
document.getElementById("cv<%=count%>.src").onchange=function(){XAble_CVOutput(<%=count%>);}
document.getElementById("cv<%=count%>.event").onchange=function(){XAble_CVOutput(<%=count%>);}
XAble_CVOutput(<%=count%>);
<% Next %>	
</script>

</table>
<p>&dagger;&nbsp;Some settings have specific firmware requirements. Make sure you have an <a href="firmwares.html">appropriate firmware version</a> to use settings marked with the &dagger; symbol</p>
<%
' ================================================================================================
' GATE OUTPUTS
' ================================================================================================
%>
<hr>
<p><b>Gate Outputs</b></p>
<table>
<tr>
<td></td>
<td title="Select the source of event that will activate this gate output">Source</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="If you select a note input, the gate can activate when&#10;a note is played from that input">Note input event</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="If you select MIDI note or MIDI CC to activate the gate output&#10;then you can select the MIDI channel here">Chan</td>
<td title="If you select MIDI note to activate the output, select the note value here">Min.Note</td>
<td title="Select a single note, or upper limit of a range of notes to activate the output when received">Max.Note</td>
<td title="The output can activate only when notes are received that are equal or higher than this velocity">Min Vel.</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="If you select MIDI CC to activate the output, select the CC number here">CC#</td>
<td title="If you select MIDI CC to activate the output, select the switching threshold here">Switch@</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="If you select MIDI clock to activate the output, select the clock rate (divider) here">Ck.Rate</td>
<td title="You can also offset MIDI clock outputs so they activate 'late' by a number of ticks (There are 24 ticks per beat)">Tk.Ofs</td>
<td>&nbsp;&nbsp;&nbsp;</td>
<td title="Select (trig) to pulse output with the default trigger time or&#10;select (gate) to have the output switch on until the activating condition is over (e.g. note released)&#10;You can also set a specific trigger time for this output">Trig</td>
</tr>
<%
for count = 1 to 12
	Set o = dictMappings.item(30 + count)
	Response.Write "<tr><td>"
	Response.Write "Gate." & count
	Response.Write "</td><td>"
	RenderGateSource o
	Response.Write "</td><td>"
	Response.Write "</td><td>"
	RenderGateEvent o
	Response.Write "</td><td>"
	Response.Write "</td><td>"
	RenderChannel o, True
	Response.Write "</td><td>"
	RenderMinNote o
	Response.Write "</td><td>"
	RenderMaxNote o
	Response.Write "</td><td>"
	RenderMinVel o
	Response.Write "</td><td>"
	Response.Write "</td><td>"
	RenderCC o
	Response.Write "</td><td>"
	RenderThreshold o
	Response.Write "</td><td>"
	Response.Write "</td><td>"		
	RenderClockDivider o
	Response.Write "</td><td>"
	RenderClockOffset o
	Response.Write "</td><td>"
	Response.Write "</td><td>"
	RenderGateDuration o, True
	Response.Write "</td></tr>"
next	
%>
</table>
<script language="javascript">
var o_form = document.forms[1];
function XAble_GateOutput(id) {
	var val = (document.getElementById("gt" + id + ".src").value);	
	var e_any = (val != "0");
	var e_ev = (val == "11" || val == "12" || val == "13" || val == "14");
	var e_note = (val == "1");
	var e_cc = (val == "2"||val == "3");
	var e_clk = (val == "20"||val == "21");
	document.getElementById("gt" + id + ".event").disabled = !e_ev;
	document.getElementById("gt" + id + ".chan").disabled =  !(e_cc||e_note);
	document.getElementById("gt" + id + ".min_note").disabled = !e_note;
	document.getElementById("gt" + id + ".max_note").disabled = !e_note;
	document.getElementById("gt" + id + ".min_vel").disabled = !e_note;
	document.getElementById("gt" + id + ".cc").disabled = !e_cc;
	document.getElementById("gt" + id + ".thrs").disabled = !e_cc;
	document.getElementById("gt" + id + ".div").disabled = !e_clk;
	document.getElementById("gt" + id + ".ofs").disabled = !e_clk;
	document.getElementById("gt" + id + ".dur").disabled = !e_any;
}
<% For count = 1 to 12 %>
document.getElementById("gt<%=count%>.src").onchange=function(){XAble_GateOutput(<%=count%>);}
XAble_GateOutput(<%=count%>);
<% Next %>	
</script>
<p>&dagger;&nbsp;Some settings have specific firmware requirements. Make sure you have an <a href="firmwares.html">appropriate firmware version</a> to use settings marked with the &dagger; symbol</p>

<hr>
<input type="submit" value="I WANT YOUR SYSEX!">


</td></tr></table>

</form>



</body>