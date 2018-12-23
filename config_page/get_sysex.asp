<%
	Dim strData, i, arrData

	response.addheader "content-type", "application/octet-stream"
	response.addheader "content-disposition", "attachment; filename=cvocd.syx"
	
	response.binarywrite chrb(&HF0) ' START OF SYSEX
	response.binarywrite chrb(&H00) ' MANUF ID
	response.binarywrite chrb(&H7F) ' MANUF ID
	response.binarywrite chrb(&H15) ' CVOCD PATCH
		
	' Convert the comma delimited list of sysex bytes into actual sysex
	' data which can be downloaded to a file to be saved on the client
	strData = Request.Form("sysex_data")
	arrData = Split(strData, ",")
	For i=LBound(arrData) to UBound(arrData)
		response.binarywrite chrb(CInt(arrData(i)))
	Next 
	
	response.binarywrite chrb(&HF7) ' END OF SYSEX
%>


