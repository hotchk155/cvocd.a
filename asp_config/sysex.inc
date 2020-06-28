<%


Const MANUF_ID_0 = &H00
Const MANUF_ID_1 = &H7F 
Const MANUF_ID_2 = &H15		

Const NRPNH_GLOBAL 	= 1
Const NRPNH_STACK1 	= 11
Const NRPNH_STACK2 	= 12
Const NRPNH_STACK3 	= 13
Const NRPNH_STACK4 	= 14
Const NRPNH_CV1		= 21
Const NRPNH_CV2		= 22
Const NRPNH_CV3		= 23
Const NRPNH_CV4		= 24
Const NRPNH_GATE1 	= 31
Const NRPNH_GATE2 	= 32
Const NRPNH_GATE3 	= 33
Const NRPNH_GATE4 	= 34
Const NRPNH_GATE5 	= 35
Const NRPNH_GATE6 	= 36
Const NRPNH_GATE7 	= 37
Const NRPNH_GATE8 	= 38
Const NRPNH_GATE9 	= 39
Const NRPNH_GATE10	= 40
Const NRPNH_GATE11	= 41
Const NRPNH_GATE12	= 42

Const NRPNL_SRC			= 1
Const NRPNL_CHAN		= 2
Const NRPNL_NOTE_MIN  	= 3
Const NRPNL_NOTE		= 3
Const NRPNL_NOTE_MAX  	= 4
Const NRPNL_VEL_MIN  	= 5
Const NRPNL_PB_RANGE	= 7
Const NRPNL_PRIORITY	= 8	
Const NRPNL_SPLIT  		= 9
Const NRPNL_TICK_OFS	= 11
Const NRPNL_GATE_DUR	= 12
Const NRPNL_THRESHOLD	= 13
Const NRPNL_TRANSPOSE	= 14
Const NRPNL_VOLTS		= 15
Const NRPNL_PITCHSCHEME	= 16
Const NRPNL_CAL_SCALE  	= 98
Const NRPNL_CAL_OFS  	= 99
Const NRPNL_SAVE		= 100
	
Const NRPVH_SRC_DISABLE		= 0
Const NRPVH_SRC_MIDINOTE	= 1
Const NRPVH_SRC_MIDICC		= 2
Const NRPVH_SRC_MIDICC_NEG	= 3
Const NRPVH_SRC_MIDIBEND	= 4
Const NRPVH_SRC_MIDITOUCH	= 5
Const NRPVH_SRC_STACK1		= 11
Const NRPVH_SRC_STACK2		= 12
Const NRPVH_SRC_STACK3		= 13
Const NRPVH_SRC_STACK4		= 14
Const NRPVH_SRC_MIDITICK	= 20
Const NRPVH_SRC_MIDITICKRUN	= 21
Const NRPVH_SRC_MIDIRUN		= 22
Const NRPVH_SRC_MIDISTART	= 23
//Const NRPVH_SRC_MIDICONT	= 24
Const NRPVH_SRC_MIDISTOP	= 25
Const NRPVH_SRC_MIDISTARTSTOP	= 26
Const NRPVH_SRC_TESTVOLTAGE = 127
Const NRPVH_CHAN_SPECIFIC	= 0
Const NRPVH_CHAN_OMNI		= 1
Const NRPVH_CHAN_GLOBAL		= 2
Const NRPVH_DUR_INF			= 0
Const NRPVH_DUR_MS			= 1
Const NRPVH_DUR_GLOBAL		= 2
Const NRPVH_DUR_RETRIG		= 3
	
Dim dictMappings


' =================================================================
Function ReadChan(o)
	If Request.Form(o.Key & ".chan") = "omni" then
		o.ChanType = NRPVH_CHAN_OMNI
		o.ChanNumber = 0
	elseif Request.Form(o.Key & ".chan") = "default" then
		o.ChanType = NRPVH_CHAN_GLOBAL
		o.ChanNumber = 0
	else
		o.ChanType = NRPVH_CHAN_SPECIFIC
		o.ChanNumber = CLng(Request.Form(o.Key & ".chan"))
	end if
end function 
' =================================================================
Function ReadDuration(o)
	If request.form(o.Key & ".dur") = "inf" then
		o.DurationType = NRPVH_DUR_INF
		o.Duration = 0
	elseIf request.form(o.Key & ".dur") = "default" then
		o.DurationType = NRPVH_DUR_GLOBAL
		o.Duration = 0
	elseIf request.form(o.Key & ".dur") = "retrig" then
		o.DurationType = NRPVH_DUR_RETRIG
		o.Duration = 0
	else
		o.DurationType = NRPVH_DUR_MS
		o.Duration = CByte(Request.Form(o.Key & ".dur"))
	end if	
End Function 
' =================================================================
Function ReadPriority(o)
	o.Priority = CByte(Request.Form(o.Key & ".prty"))
End Function 
' =================================================================
Function ReadBendRange(o)
	o.BendRange = CByte(Request.Form(o.Key & ".bend"))
End Function 
' =================================================================
Function ReadMinNote(o)
	o.MinNote = CByte(Request.Form(o.Key & ".min_note"))
End Function 
' =================================================================
Function ReadMaxNote(o)
	o.MaxNote = CByte(Request.Form(o.Key & ".max_note"))
End Function 
' =================================================================
Function ReadMinVel(o)
	o.MinVel = CByte(Request.Form(o.Key & ".min_vel"))
End Function 
' =================================================================
Function ReadSource(o)
	o.Source = CByte(Request.Form(o.Key & ".src"))
End Function 
' =================================================================
Function ReadInputEvent(o)
	o.InputEvent = CByte(Request.Form(o.Key & ".event"))
End Function 
' =================================================================
Function ReadTranspose(o)
'response.write (o.Key & ".trans") & ">" & CByte(Request.Form(o.Key & ".trans"))
	o.Transpose = CByte(Request.Form(o.Key & ".trans"))
End Function 
' =================================================================
Function ReadCC(o)
	o.CC = CByte(Request.Form(o.Key & ".cc"))
End Function 
' =================================================================
Function ReadThreshold(o)
	o.Threshold = CByte(Request.Form(o.Key & ".thrs"))
End Function 
' =================================================================
Function ReadDivider(o)
	o.Divider = CByte(Request.Form(o.Key & ".div"))
End Function 
' =================================================================
Function ReadOffset(o)
	o.Offset = CByte(Request.Form(o.Key & ".ofs"))
End Function 
' =================================================================
Function ReadPitchScheme(o)
	o.PitchScheme = CByte(Request.Form(o.Key & ".scheme"))
End Function 
' =================================================================
Function ReadVolts(o)
	o.Volts = CByte(Request.Form(o.Key & ".volts"))
End Function 
' =================================================================
Function ReadCVScale(o)
	o.CVScale = CByte(Request.Form(o.Key & ".cvscale"))
End Function 
' =================================================================
Function ReadCVOfs(o)
	o.CVOfs = CByte(Request.Form(o.Key & ".cvofs"))
End Function 
' =================================================================
Function ReadCVCal(o)
	o.CVCal = (Request.Form(o.Key & ".cvcal") = "on")
End Function 
' =================================================================
Function ReadAutoSave(o)
	o.AutoSave = CByte(Request.Form(o.Key & ".autosave"))
End Function 
' =================================================================
Function WriteParam(a,b,c,d)
'	Response.Write a & ", " & b & ", " & c & ", " & d & "<br>"
	Response.BinaryWrite ChrB(a) & ChrB(b) & ChrB(c) & ChrB(d) 
End Function


' =================================================================
' Globals
' =================================================================
Class Globals
	Public Key 
	Public ChanType 
	Public ChanNumber
	Public DurationType
	Public Duration
	
	'--------------------------
	Public Sub Class_Initialize
		Key = ""	
		ChanType = NRPVH_CHAN_SPECIFIC	
		ChanNumber = 1
		DurationType = NRPVH_DUR_MS			
		Duration = 15
		AutoSave = 1
	End Sub
	'--------------------------
	Public Sub ReadForm
		ReadChan Me
		ReadDuration Me		
	End Sub	
	'--------------------------
	Public Sub WriteSysex(id)
		WriteParam id, NRPNL_CHAN, Me.ChanType, Me.ChanNumber
		WriteParam id, NRPNL_GATE_DUR, Me.DurationType, Me.Duration
	End Sub
	'--------------------------
	Public Sub ReadSysex(p1, p2, v1, v2)
		select case p2
			case NRPNL_CHAN
				Me.ChanType = v1
				Me.ChanNumber = v2
			case NRPNL_GATE_DUR
				Me.DurationType = v1
				Me.Duration = v2
		end select
	End Sub
End Class

' =================================================================
' NOTE INPUT
' =================================================================
Class NoteInput
	Public Key 
	Public Source 
	Public ChanType 
	Public ChanNumber
	Public Priority 
	Public MinNote 
	Public MaxNote
	Public MinVel 
	Public BendRange
	'--------------------------
	Public Sub Class_Initialize
		Key = ""	
		Source = NRPVH_SRC_DISABLE
		ChanType = NRPVH_CHAN_GLOBAL
		ChanNumber = 0	
		Priority = 3
		MinNote = 0
		MaxNote = 127
		MinVel = 0
		BendRange = 3
	End Sub	
	'--------------------------	
	Public Sub ReadForm
		ReadSource Me
		ReadChan Me
		ReadPriority Me
		ReadMinNote Me
		ReadMaxNote Me
		ReadMinVel Me
		ReadBendRange Me
	End Sub	
	'--------------------------		
	Public Sub WriteSysex(id)
		WriteParam id, NRPNL_SRC, Me.Source, 0
		WriteParam id, NRPNL_CHAN, Me.ChanType, Me.ChanNumber
		WriteParam id, NRPNL_NOTE_MIN, 0, Me.MinNote
		WriteParam id, NRPNL_NOTE_MAX, 0, Me.MaxNote
		WriteParam id, NRPNL_VEL_MIN, 0, Me.MinVel
		WriteParam id, NRPNL_PB_RANGE, 0, Me.BendRange
		WriteParam id, NRPNL_PRIORITY, 0, Me.Priority
	End Sub
	'--------------------------
	Public Sub ReadSysex(p1, p2, v1, v2)
		select case p2
			case NRPNL_SRC
				Me.Source = v1
			case NRPNL_CHAN
				Me.ChanType = v1
				Me.ChanNumber = v2
			case NRPNL_NOTE_MIN
				Me.MinNote = v2
			case NRPNL_NOTE_MAX
				Me.MaxNote = v2
			case NRPNL_VEL_MIN
				Me.MinVel = v2
			case NRPNL_PB_RANGE
				Me.BendRange = v2
			case NRPNL_PRIORITY
				Me.Priority = v2
		end select
	End Sub
End Class

' =================================================================
' CV OUTPUT
' =================================================================
Class CVOutput
	Public Key 
	Public Source 
	Public InputEvent
	Public Transpose
	Public ChanType
	Public ChanNumber
	Public CC
	Public Volts
	Public PitchScheme
	'--------------------------
	Public Sub Class_Initialize
		Key = ""
		Source = NRPVH_SRC_DISABLE
		InputEvent = 0
		Transpose = 64
		ChanType = 2 'default
		ChanNumber = 0	
		CC = 1
		Volts = 5
		PitchScheme = 0
	End Sub	
	'--------------------------
	Public Sub ReadForm
		ReadSource Me
		ReadInputEvent Me
		ReadTranspose Me
		ReadChan Me
		ReadCC Me		
		ReadPitchScheme Me		
		ReadVolts Me		
	End Sub	
	'--------------------------		
	Public Sub WriteSysex(id)
		Select Case Me.Source
			Case NRPVH_SRC_STACK1, NRPVH_SRC_STACK2, NRPVH_SRC_STACK3, NRPVH_SRC_STACK4
				WriteParam id, NRPNL_SRC, Me.Source, Me.InputEvent	
				Select Case Me.InputEvent	
					Case "1", "2", "3", "4"
						WriteParam id, NRPNL_PITCHSCHEME, 0, Me.PitchScheme
						WriteParam id, NRPNL_TRANSPOSE, 0, Me.Transpose
					Case "20"
						WriteParam id, NRPNL_VOLTS, 0, Me.Volts
				End Select
			Case NRPVH_SRC_MIDICC, NRPVH_SRC_MIDICC_NEG
				WriteParam id, NRPNL_SRC, Me.Source, Me.CC
				WriteParam id, NRPNL_CHAN, Me.ChanType, Me.ChanNumber
				WriteParam id, NRPNL_VOLTS, 0, Me.Volts
			Case NRPVH_SRC_MIDIBEND, NRPVH_SRC_MIDITOUCH
				WriteParam id, NRPNL_SRC, Me.Source, 0
				WriteParam id, NRPNL_CHAN, Me.ChanType, Me.ChanNumber
				WriteParam id, NRPNL_VOLTS, 0, Me.Volts
			case else
				WriteParam id, NRPNL_SRC, Me.Source, 0
		End select
	End Sub
	'--------------------------
	Public Sub ReadSysex(p1, p2, v1, v2)
		select case p2
			case 1
				select case v1
					Case NRPVH_SRC_STACK1, NRPVH_SRC_STACK2, NRPVH_SRC_STACK3, NRPVH_SRC_STACK4
						Me.Source = v1
						Me.InputEvent = v2
					case NRPVH_SRC_MIDICC, NRPVH_SRC_MIDICC_NEG
						Me.Source = v1
						Me.CC = v2
					case else 
						Me.Source = v1
				end select					
			case NRPNL_CHAN
				Me.ChanType = v1
				Me.ChanNumber = v2
			case NRPNL_VOLTS
				Me.Volts = v2
			case NRPNL_PITCHSCHEME
				Me.PitchScheme = v2
			case NRPNL_TRANSPOSE
				Me.Transpose = v2
		end select
	End Sub
End Class

' =================================================================
' CV CALIBRATION
' =================================================================
Class CVCalibration
	Public Key 
	Public CVCal
	Public CVScale
	Public CVOfs
	'--------------------------
	Public Sub Class_Initialize
		Key = ""
		CVScale = 64
		CVOfs = 64
		CVCal = False
	End Sub	
	'--------------------------
	Public Sub ReadForm
		ReadCVCal Me		
		ReadCVScale Me		
		ReadCVOfs Me		
	End Sub	
	'--------------------------		
	Public Sub WriteSysex(id)
		If Me.CVCal Then
			WriteParam id, NRPNL_CAL_SCALE, 0, Me.CVScale
			WriteParam id, NRPNL_CAL_OFS, 0, Me.CVOfs			
		End If
	End Sub
	'--------------------------
	Public Sub ReadSysex(p1, p2, v1, v2)
		select case p2
			case NRPNL_CAL_SCALE
				Me.CVScale = v2
				Me.CVCal = True
			case NRPNL_CAL_OFS
				Me.CVOfs = v2
				Me.CVCal = True
		end select
	End Sub
End Class

' =================================================================
' GATE OUTPUT
' =================================================================
Class GateOutput
	Public Key 
	Public Source
	Public InputEvent
	Public ChanType
	Public ChanNumber
	Public MinNote
	Public MaxNote
	Public MinVel
	Public CC
	Public Threshold
	Public Divider
	Public Offset
	Public DurationType
	Public Duration
	'--------------------------
	Public Sub Class_Initialize
		Key  = ""
		Source = 0
		InputEvent = 1
		ChanType = NRPVH_CHAN_GLOBAL
		ChanNumber = 0	
		MinNote = 0
		MaxNote = 127
		MinVel = 0
		CC = 0		
		Threshold = 64
		Divider = 6
		Offset = 0
		DurationType = NRPVH_DUR_GLOBAL
		Duration = 0
	End Sub
	'--------------------------
	Public Sub ReadForm
		ReadSource Me
		ReadInputEvent Me
		ReadChan Me
		ReadMinNote Me
		ReadMaxNote Me
		ReadMinVel Me
		ReadCC Me
		ReadThreshold Me
		ReadDivider Me
		ReadOffset Me
		ReadDuration Me
	End Sub	
	'--------------------------		
	Public Sub WriteSysex(id)
		Select Case Me.Source
			Case NRPVH_SRC_STACK1, NRPVH_SRC_STACK2, NRPVH_SRC_STACK3, NRPVH_SRC_STACK4
				WriteParam id, NRPNL_SRC, Me.Source, Me.InputEvent
			Case NRPVH_SRC_MIDINOTE
				WriteParam id, NRPNL_SRC, NRPVH_SRC_MIDINOTE, Me.MinNote
				WriteParam id, NRPNL_CHAN, Me.ChanType, Me.ChanNumber
				WriteParam id, NRPNL_NOTE_MAX, 0, Me.MaxNote
				WriteParam id, NRPNL_VEL_MIN, 0, Me.MinVel
			Case NRPVH_SRC_MIDICC, NRPVH_SRC_MIDICC_NEG
				WriteParam id, NRPNL_SRC, Me.Source, Me.CC
				WriteParam id, NRPNL_CHAN, Me.ChanType, Me.ChanNumber
				WriteParam id, NRPNL_THRESHOLD, 0, Me.Threshold
			Case NRPVH_SRC_MIDITICK, NRPVH_SRC_MIDITICKRUN
				WriteParam id, NRPNL_SRC, Me.Source, Me.Divider
				WriteParam id, NRPNL_TICK_OFS, 0, Me.Offset			
			case else
				WriteParam id, NRPNL_SRC, Me.Source, 0
		End select
		WriteParam id, NRPNL_GATE_DUR, Me.DurationType, Me.Duration
	End Sub
	'--------------------------
	Public Sub ReadSysex(p1, p2, v1, v2)
		select case p2
			case NRPNL_SRC
				select case v1
					Case NRPVH_SRC_STACK1, NRPVH_SRC_STACK2, NRPVH_SRC_STACK3, NRPVH_SRC_STACK4
						Me.Source = v1
						Me.InputEvent = v2
					case NRPVH_SRC_MIDINOTE
						Me.Source = v1
						Me.MinNote = v2
					case NRPVH_SRC_MIDICC, NRPVH_SRC_MIDICC_NEG
						Me.Source = v1
						Me.CC = v2
					Case NRPVH_SRC_MIDITICK, NRPVH_SRC_MIDITICKRUN
						Me.Source = v1
						Me.Divider = v2					
					case else 
						Me.Source = v1
				end select					
			case NRPNL_CHAN
				Me.ChanType = v1
				Me.ChanNumber = v2
			case NRPNL_NOTE_MAX
				Me.MaxNote = v2
			case NRPNL_VEL_MIN
				Me.MinVel = v2
			case NRPNL_GATE_DUR
				Me.DurationType = v1
				Me.Duration = v2
			case NRPNL_TICK_OFS
				Me.Offset = v2
			case NRPNL_THRESHOLD
				Me.Threshold = v2
		end select
	End Sub
End Class

Function InitMappings
	Set dictMappings = Server.CreateObject("Scripting.Dictionary")

	Set o = New Globals
	o.Key = "global"
	dictMappings.Add 1, o
	
	' NOTE INPUTS
	For i = 1 to 4
		Set o = New NoteInput
		o.Key = "in" & i
		dictMappings.Add 10 + i, o
	Next

	' CV OUTPUTS
	For i = 1 to 4
		Set o = New CVOutput
		o.Key = "cv" & i
		dictMappings.Add 20 + i, o
	Next

	' GATE OUTPUTS
	For i = 1 to 12
		Set o = New GateOutput
		o.Key = "gt" & i
		dictMappings.Add 30 + i, o
	Next	
End Function

Function InitMappingsCal
	Set dictMappings = Server.CreateObject("Scripting.Dictionary")
	For i = 1 to 4
		Set o = New CVCalibration
		o.Key = "cv" & i
		dictMappings.Add 20 + i, o
	Next
End Function

Function ReadForm
	For each o in dictMappings.Items
		o.ReadForm
	Next
End Function

Function WriteSysex
	arrKeys = dictMappings.Keys
	arrItems = dictMappings.Items
	response.binarywrite chrb(&HF0) 
	response.binarywrite chrb(MANUF_ID_0) 
	response.binarywrite chrb(MANUF_ID_1) 
	response.binarywrite chrb(MANUF_ID_2) 		
	For i = 0 to UBound(arrKeys)
		Set o = arrItems(i)
		o.WriteSysex(arrKeys(i))
	Next
	response.binarywrite chrb(&HF7) 
End Function

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
		Set o = dictMappings(ascb(midb(data,index,1)))
		o.ReadSysex _
			ascb(midb(data,index,1)), _
			ascb(midb(data,index+1,1)), _
			ascb(midb(data,index+2,1)), _
			ascb(midb(data,index+3,1))
		index = index + 4		
	loop
	ReadSysEx = true
end function
%>