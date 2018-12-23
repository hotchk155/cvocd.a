<%	
	Dim strSysex
	strSysex = ""
	
	
	Const MANUF_ID_0 = &H00
	Const MANUF_ID_1 = &H7F 
	Const MANUF_ID_2 = &H15		
	
	function ReadSysEx
		Dim data
		ReadSysEx = ""
		data = request.BinaryRead(request.TotalBytes)
		
		index = 0
		for i=1 to lenb(data) - 4
			if midb(data,i,4) = (chrb(13) & chrb(10) & chrb(13) & chrb(10)) then
				index = i + 4
				exit for
			end if
		next	
		if index = 0 then
			ReadSysEx = "NO FILE, OR FILE IS EMPTY"
			exit function 
		end if
		if (midb(data,index,4) <> (ChrB(&HF0) & ChrB(MANUF_ID_0) & ChrB(MANUF_ID_1) & ChrB(&H12))) and _   
			(midb(data,index,4) <> (ChrB(&HF0) & ChrB(MANUF_ID_0) & ChrB(MANUF_ID_1) & ChrB(MANUF_ID_2))) then
			ReadSysEx = "NOT A CVOCD PATCH"
			exit function 
		end if
		index = index + 4
		
		do while index <= lenb(data) 
			if midb(data,index,1) = chrb(&hF7) then exit do
			if index + 4 >= lenb(data) then 
				ReadSysEx = "INCOMPLETE OR CORRUPTED SYSEX FILE"			
				exit function
			End If
			strSysex = strSysex & "," & ascb(midb(data,index,1))
			index = index + 1		
		loop
		strSysex = Mid(strSysex,2)
	end function

	
	If Request.QueryString("file") <> "" Then
		Dim strError
		strError = ReadSysex
		If strError <> "" Then
%><b style="color:red">ERROR IN SYSEX FILE - <%=strError%></b><%		
		End If
	End If
%>