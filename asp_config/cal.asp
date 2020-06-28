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
<h1>CV.OCD Calibration</h1>
<%
	InitMappingsCal
	
	If Request.QueryString("file") <> "" Then
		If Not ReadSysex Then
%><b style="color:red">ERROR IN SYSEX FILE - NOT LOADED!!</b><%		
		End If
	End If
%>

<hr>
<table width=800><tr><td>
<form name="sysex_file" action="cal.asp?file=true" enctype="multipart/form-data" method="post">
<p>You can load an existing calibration sysex into this form, or create a new one from scratch <input type="file" name="file" value="File">
<input type="submit" value="Load SysEx Into Form"></p>
</form>
<form name="sysex" action="get_cal_sysex.asp" method="post">
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
<td>Cal</td>
<td>Scale</td>
<td>Ofs</td>
</tr>
<%
for count = 1 to 4
	Set o = dictMappings.item(20 + count)
	Response.Write "<tr><td>"
	Response.Write "CV." & Chr(64+count)
	Response.Write "</td><td>"
	RenderCVCal o
	Response.Write "</td><td>"
	RenderCVScale o
	Response.Write "</td><td>"
	RenderCVOfs o
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
<hr>
<input type="submit" value="I WANT YOUR SYSEX!">


</td></tr></table>

</form>



</body>