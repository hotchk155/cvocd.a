<%

' =================================================================
Function RenderChannel(o, bIncludeDefault)
	Response.Write "<select name=""" & o.Key & ".chan"" id=""" & o.Key & ".chan"">"		
	If bIncludeDefault Then
		Response.Write "<option value=""default"""
		If o.ChanType = 2 Then Response.Write " selected"
		Response.Write ">(default)</option>"
	End If
		
	
	For i = 1 to 16
		Response.Write "<option value=""" & i & """"
		If o.ChanType = 0 And o.ChanNumber = i Then Response.Write " selected"
		Response.Write ">Chan." & i & "</option>"
	Next
	
	If bIncludeDefault Then	
		Response.Write "<option value=""omni"""
		If o.ChanType = 1 Then Response.Write " selected"
		Response.Write ">(OMNI)</option>"	
	End If

	Response.Write "</select>"
	
End Function 

'====================================================================
Function RenderMinNote(o)
	response.Write "<select name=""" & o.Key & ".min_note"" id=""" & o.Key & ".min_note"">"
	For i = 0 to 127	
		strNote = mid(" CC# DD# E FF# GG# AA# B", 1 + 2 * (i mod 12), 2)
		strNote = trim(strNote & (int(i/12)-1))	
	
		response.Write "<option value=""" & i & """"
		if o.MinNote = i then Response.write " selected"
		response.Write ">" & i & " " & strNote &  "</option>"	
	next
	Response.Write "</select>"
end function

'====================================================================
Function RenderMaxNote(o)
	response.Write "<select name=""" & o.Key & ".max_note"" id=""" & o.Key & ".max_note"">"
	For i = 0 to 127	
		strNote = mid(" CC# DD# E FF# GG# AA# B", 1 + 2 * (i mod 12), 2)
		strNote = trim(strNote & (int(i/12)-1))	
		
		response.Write "<option value=""" & i & """"
		if o.MaxNote = i then Response.write " selected"
		if i = 0 then 
			response.Write ">(exact)</option>"
		else
			response.Write ">" & i & " " & strNote &  "</option>"	
		end if
	next
	Response.Write "</select>"
end function

'====================================================================
Function RenderMinVel(o)
	response.Write "<select name=""" & o.Key & ".min_vel"" id=""" & o.Key & ".min_vel"">"
	For i = 1 to 127	
		response.Write "<option value=""" & i & """"
		if o.MinVel = i then Response.write " selected"
		if i = 1 then 
			response.Write ">(any)</option>"
		else
			response.Write ">" & i & "</option>"
		end if
	next
	Response.Write "</select>"
end function

'====================================================================
Function RenderCC(o)
	response.Write "<select name=""" & o.Key & ".cc"" id=""" & o.Key & ".cc"">"
	For i = 1 to 127	
		response.Write "<option value=""" & i & """"
		if o.CC = i then Response.write " selected"
		
		Select Case i
			Case 1 
				response.Write ">Mod Wheel</option>"
			Case 6 
				response.Write " disabled>Data Entry MSB</option>"
			Case 38 
				response.Write " disabled>Data Entry LSB</option>"
			Case 98 
				response.Write " disabled>NRPN LSB</option>"
			Case 99		
				response.Write " disabled>NRPN MSB</option>"
			Case Else
				response.Write ">cc#" & i & "</option>"
		End Select
	next
	Response.Write "</select>"
end function

'====================================================================
Function RenderThreshold(o)
	response.Write "<select name=""" & o.Key & ".thrs"" id=""" & o.Key & ".thrs"">"
	For i = 1 to 127	
		response.Write "<option value=""" & i & """"
		if o.Threshold = i then Response.write " selected"
		response.Write ">" & i & "</option>"
	next
	Response.Write "</select>"
end function

'====================================================================
Function RenderClockOffset(o)
	response.Write "<select name=""" & o.Key & ".ofs"" id=""" & o.Key & ".ofs"">"
	For i = 0 to 127	
		response.Write "<option value=""" & i & """"
		if o.Offset = i then Response.write " selected"
		if i = 0 then 
			response.Write ">(none)</option>"
		else
			response.Write ">+" & i & "</option>"
		end if
	next
	Response.Write "</select>"
End Function


'====================================================================
Function RenderGateDuration(o, bIncludeDefault)

	response.Write "<select name=""" & o.Key & ".dur"" id=""" & o.Key & ".dur"">"



	If bIncludeDefault then
	
		response.Write "<option value=""inf"""
		If o.DurationType = 0 then response.Write " selected"	
		response.Write ">(gate)</option>"

		response.Write "<option value=""retrig"""
		If o.DurationType = 3 then response.Write " selected"	
		response.Write ">(retrig)</option>"

		response.Write "<option value=""default"""
		If o.DurationType = 2 then response.Write " selected"	
		response.Write ">(trigger)</option>"		
	end if
	
	i = 1
	while i < 125
		response.Write "<option value=""" & i & """"
		if o.DurationType = 1 and o.Duration = i then Response.write " selected"
		response.Write ">" & i & "ms</option>"
		if i<15 then i=i+1 else i=i + 5					
	wend
	Response.Write "</select>"
End Function

'====================================================================
Function RenderSelection(name, default, dict)
	response.Write "<select name=""" & name & """ id=""" & name & """>"
	arrKeys = dict.Keys
	arrItems = dict.Items
	For i = 0 to dict.Count - 1
		response.Write "<option value=""" & arrKeys(i) & """"
		if CStr(default) = CStr(arrKeys(i)) then Response.write " selected"
		response.Write ">" & arrItems(i) & "</option>"
	next
	Response.Write "</select>"
End Function

'====================================================================
Function RenderInputSource(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "0", "(disable)"
	dict.add "1", "ENABLE"
	RenderSelection o.Key & ".src", o.Source, dict
end function

'====================================================================
Function RenderPriority(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "0", "Last note priority"
	dict.add "1", "Lowest note priority"
	dict.add "3", "Highest note priority"
	dict.add "6", "2 note cycle"
	dict.add "7", "3 note cycle"
	dict.add "8", "4 note cycle"
	dict.add "9", "2 note chord"
	dict.add "10", "3 note chord"
	dict.add "11", "4 note chord"
	RenderSelection o.Key & ".prty", o.Priority, dict
end function

'====================================================================
Function RenderBendRange(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	For i = 0 to 24
		dict.add CStr(i), "+/-" & i
	next
	RenderSelection o.Key & ".bend", o.BendRange, dict
end function

'====================================================================
Function RenderCVSource(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "0", "(disable)"
	dict.add "11", "Note input #1"
	dict.add "12", "Note input #2"
	dict.add "13", "Note input #3"
	dict.add "14", "Note input #4"
	dict.add "2", "MIDI CC"
	dict.add "4", "Pitch Bend"
	dict.add "5", "Channel Aftertouch"
	dict.add "20", "BPM to CV"
	dict.add "127", "Fixed Voltage"
	RenderSelection o.Key & ".src", o.Source, dict
end function

'====================================================================
Function RenderCVEvent(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "1", "First note pitch"
	dict.add "2", "Second note pitch"
	dict.add "3", "Third note pitch"
	dict.add "4", "Fourth note pitch"
	dict.add "20", "Most recent note velocity"
	RenderSelection o.Key & ".event", o.InputEvent, dict
end function

'====================================================================
Function RenderTranspose(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "100", "+3oct"
	dict.add "88", "+2oct"
	dict.add "76", "+1oct"
	for i=11 to 1 step -1
		dict.add cstr(64+i), cstr(i)
	Next
	dict.add "64", "(none)"
	for i=-1 to -11 step -1
		dict.add cstr(64+i), cstr(i)
	Next
	dict.add "52", "-1oct"
	dict.add "40", "-2oct"
	dict.add "28", "-3oct"
	RenderSelection o.Key & ".trans", o.Transpose, dict
end function

'====================================================================
Function RenderGateSource(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "0", "(disable)"
	dict.add "11", "Note input #1"
	dict.add "12", "Note input #2"
	dict.add "13", "Note input #3"
	dict.add "14", "Note input #4"
	dict.add "1", "MIDI note"
	dict.add "2", "CC above threshold"
	dict.add "3", "CC below threshold"
	dict.add "20", "Clock Tick"
	dict.add "21", "Clock Tick+Run"
	dict.add "22", "Transport Run"
	dict.add "23", "Transport Restart"
	dict.add "25", "Transport Stop"
	dict.add "26", "Trans. Start/Stop &dagger;"
	RenderSelection o.Key & ".src", o.Source, dict
end function

'====================================================================
Function RenderGateEvent(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "1", "First note on"
	dict.add "2", "Second note on"
	dict.add "3", "Third note on"
	dict.add "4", "Fourth note on"
	dict.add "5", "Any note on"
	dict.add "0", "All notes off"
	RenderSelection o.Key & ".event", o.InputEvent, dict
end function

'====================================================================
Function RenderClockDivider(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")	
	dict.add "3", "1/32"
	dict.add "4", "1/16T"
	dict.add "6", "1/16"
	dict.add "8", "1/8T"
	dict.add "9", "1/16D"
	dict.add "12", "1/8"
	dict.add "16", "1/4T"
	dict.add "18", "1/8D"
	dict.add "24", "1/4"
	dict.add "32", "1/2T"
	dict.add "36", "1/4D"
	dict.add "48", "1/2"
	dict.add "72", "1/2D"
	dict.add "96", "1"
	dict.add "1", "24ppqn"
	RenderSelection o.Key & ".div", o.Divider, dict	
end function

'====================================================================
Function RenderVolts(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "1", "1V"
	dict.add "2", "2V"
	dict.add "3", "3V"
	dict.add "4", "4V"
	dict.add "5", "5V"
	dict.add "6", "6V"
	dict.add "7", "7V"
	dict.add "8", "8V"
	RenderSelection o.Key & ".volts", o.Volts, dict
end function

'====================================================================
Function RenderPitchScheme(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	dict.add "0", "V/Oct"
	dict.add "1", "Hz/V &dagger;"
	dict.add "2", "1.2V/Oct &dagger;"
	RenderSelection o.Key & ".scheme", o.PitchScheme, dict
end function

'====================================================================
Function RenderCVScale(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	for i=63 to -63 step -1
		if i>0 then
			dict.add CStr(i+64), "+" & CStr(i)
		else
			dict.add CStr(i+64), CStr(i)
		end if
	next 
	RenderSelection o.Key & ".cvscale", o.CVScale, dict
End Function

'====================================================================
Function RenderCVOfs(o)
	Set dict = Server.CreateObject("Scripting.Dictionary")
	for i=63 to -63 step -1
		if i>0 then
			dict.add CStr(i+64), "+" & CStr(i)
		else
			dict.add CStr(i+64), CStr(i)
		end if
	next 
	RenderSelection o.Key & ".cvofs", o.CVOfs, dict
End Function

'====================================================================
Function RenderCVCal(o)
	response.Write "<input type=""checkbox"""
	If o.CVCal Then response.Write " checked "
	response.Write " name=""" & o.Key & ".cvcal"" id=""" & o.Key & ".cvcal"">"
End Function

%>