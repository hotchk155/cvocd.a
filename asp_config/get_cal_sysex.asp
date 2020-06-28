<!--#include file ="sysex.inc"-->
<%
'response.write request.form
	response.addheader "content-type", "application/octet-stream"
	response.addheader "content-disposition", "attachment; filename=cvocd_cal.syx"
	
	InitMappingsCal
	ReadForm
	WriteSysex
	
%>


