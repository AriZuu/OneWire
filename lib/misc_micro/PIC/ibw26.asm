; File: IBW26.ASM

; iButton reader with 26 bit Wiegand output -

; This code reads an arriving iButton and then presents the compromised
; iButton ID out as Wiegand formatted pulses. Output pulses are low-going,
; 100us wide with 2ms minimum between pulses.

; The clock/crystal frequency is assumed to be nominally 4 MHz +/- 20%.


#Define SiteCode 123		; We use a fioxed Wiegand site code

;	processor 12C508A
	processor 16C54A
	radix dec

;	include 12C508.asm
	include 16C54.asm
	include picmacs.asm

#Define gpio RB				; Cross-define for PIC 16C54 testing

	CBLOCK	8

		LoopCounter			; General Purpose Loop Counter
		RcvByte				; Working I/O byte
		crcl				; CRC-8 Working Accumulator
		flags				; Varios boolean flag bits
		Delay				; A counter for time delays
		
		RomData0			; ROM ID as read from arriving iButton device
		RomData1			; and also the Wiegand message workspace
		RomData2
		RomData3

	ENDC


; We re-use the RomData bytes to save RAM -


#Define Wiegand0 RomData3		; Different names when they become Wiegand buffer
#Define Wiegand1 RomData2
#Define Wiegand2 RomData1
#Define Wiegand3 RomData0

#Define Data0Pin gpio,0			; Data 0 Line
#Define Data1Pin gpio,1			; Data 1 Line
#Define GreenLED gpio,3			; Line that drives GREEN LED (input)
#Define OptDevice gpio,4		; Auxilliary 1-Wire Device Data Line
#Define iBPin gpio,5			; iButton Probe Data line

#Define AllHiZ  11111111B		; GPIO TRIS write to make all Hi-Z
#Define Data0   11111110B		; TRIS value to gen pulse on DATA0 line
#Define Data1   11111101B		; TRIS value to gen pulse on DATA1 line
#Define iButton 11011111B		; TRIS value to gen low on iButton data line

#Define EP flags,1				; Define an Even Parity bit
#Define OP flags,2				; Define an Odd Parity bit
#Define Presence flags,3		; Define a presence pulse flag bit
#Define Short flags,3			; Define a shoted bus flag bit



;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;
;
; iButton Serial Number as read into RomData0-7:
;
;	Byte 0		Byte 1		Byte 2		Byte 3		Byte 4		Byte 5		Byte 6		Byte 7
;	01234567	01234567	01234567	01234567	01234567	01234567	01234567	01234567	
;	<- FC ->	<-L------------------------ Serial Number -----------------------M->	<-CRC8->
;
;
; 26-bit Wiegand Format:
;
;	Byte 0		Byte 1		Byte 2		Byte 3
;	01234567	01234567	01234567	01XXXXXX	
;	P0123456	70123456	70123456	7PXXXXXX
;   E<----------><---------------------->O
;	  Site Code       Key ID Number
;
;	E=Even parity of adjacent 12 bits
;	O=Odd parity of adjacent 12 bits
;



	org 0
	goto Start


;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

; Function: TReset

; Desc:		Generate a Touch Reset pulse on the probe and see if there is
;			a device present. These timings can be off by +/- 20% and
;			remain valid. Crystal/clock frequency can be between 3.33 Mhz and
;			4.8 Mhz and timings remain valid.

; Entry:	None

; Exit:		Presence flag set if a presence pulse was observed
;			Short flag set if line was shorted on entry

; Uses:		Delay

TReset:
	bsf Short
	btfss iBPin					; If we see a presence pulse, then
	retlw 0
	bcf Short
	movlw iButton
	tris gpio					; Take 1-Wire bus LOW
	movlw 192
	movwf Delay
DlyLp5:
	nop
	decfsz Delay,f				; Delay 580us (480us + 20%)
	goto DlyLp5
	movlw AllHiZ
	tris gpio					; Release the bus
	nop
	nop
	nop
	nop							; Allow for rise time
	nop
	nop
	bcf Presence				; Assume no presence pulse will be seen
	movlw 145
	movwf Delay
PWaitLp:
	btfss iBPin					; If we see a presence pulse, then
	bsf Presence				; Indicate that it was observed
	decfsz Delay,f				; Watch for 580us (480us + 20%)
	goto PWaitLp
	retlw	0					; Return
	

;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

; Function:	TBit, TByte, RByte

; Desc:		Perform a Standard Speed 1-Wire Touch Bit, Touch Byte,
;			or Read Byte function. These timings can be off by +/- 20% and
;			remain valid. Crystal/clock frequency can be between 3.33 Mhz and
;			4.8 Mhz and timings remain valid.

; Entry:	None for RByte
;			RcvByte = Byte to send for TByte
;			RcvByte.0 = Bit to send for TBit

; Exit:		WREG.7, RcvBit.7 = Received bit for TBit
;			WREG, RcvByte = Byte Read for RByte
;			WREG, RcvByte = Echo of byte sent for TByte

; Uses:		LoopCounter, Delay, RcvByte, WREG

; Timing:							Nominal:		-20%:		+20%:
;
; 			Write-One/Read Low Time	7 us			5.833 us 	8.4 us
;			Write-Zero Low Time		72 us			60 us		86.4 us
;			Sample time				15 us			12.5 us		18 us
;			Recovery time			6 us			5 us		7.2 us			
;			Overall Time Slot		78 us			65 us		93.6 us
;			Date rate 				12,820 BPS		15384 BPS	10,683 BPS

; Note: These timings should be extended for long line 1-Wire operations.

TBit:
	movlw 1						; Ask for a loop of only one
	goto TBEnt					; Do it like the rest

RByte:
	movlw 255					; For receives, always send 0xFF
	movwf RcvByte				;    from the RcvByte register

TByte:
	movlw 8						; Normal process 8 bits per byte

TBEnt:
	movwf LoopCounter			; Start a processing loop once per bit

TBLp:
		nop						;		75		75
		nop						;		76		76
		movlw iButton			;		77		77
		tris gpio				; 		00/78	00/78	-> Start Time Slot
		btfss RcvByte,0			;		01		01
		goto BIsZero1			;		02		02
		nop						;		03		-
		nop						;		04		-
		nop						;		05		-
		movlw AllHiZ			;		06
		tris gpio				;		07		-		-> End a read or write one bit
		movlw 1					;		08		-
		goto FinBit1			;		09		-
BIsZero1:						;		-		-
		movlw 3					;		-		03
		nop						;		-		04
FinBit1:						;		-		-
		movwf Delay				;		10		05
BDLoop11:						;		-		-
		decfsz Delay,f			;		11		06
		goto BDLoop11			; 		12		12	
		bcf C					;		13		13
		nop						;		14		14
		btfsc iBPin				;		15		15		-> Sample the line
		bsf C					;		16		16
		rrf	RcvByte,1			;		17		17
		movlw 25				;		18		18
		movwf Delay				;		19		19
BDLoop21:						;		-		-
		decfsz Delay,f			;		20		20
		goto BDLoop21			; 		21		21	
		movlw AllHiZ			;		71		71
		tris gpio				;		72		72		-> End a write zero bit			
	decfsz LoopCounter,f		;		73
	goto TBLp					;		74
	retlw 0


;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

; Function: crc8, crc8first

; Desc:		Perform a the 8 bit CRC on the value in WREG on entry.

; Entry:	WREG = New byte

; Exit:		crcl holds the resulting crc8

; Uses:		RcvByte as a working register

; Call crc8first to process the first byte, and crc8 for subsequent bytes.

; Note: This code is a direct translation from 8051 code and can likely be
; optimized.

crc8first:
	clrf crcl
crc8:
	movwf RcvByte
	movf crcl,w
	xorwf RcvByte,f
	movf RcvByte,w
	movwf crcl
	movlw 0F0H
	andwf crcl,f	
	swapf RcvByte,f
	movf RcvByte,w
	xorwf crcl,f
	rrf RcvByte,w
	rrf RcvByte,f
	movf RcvByte,w
	xorwf crcl,f
	rrf RcvByte,w
	rrf RcvByte,f
	swapf RcvByte,f
	movlw 0C7H
	andwf RcvByte,f
	movlw 00001000B
	btfsc RcvByte,2
	xorwf RcvByte,f
	movf RcvByte,w
	xorwf crcl,f
	swapf RcvByte,f
	movlw 0CH
	andwf RcvByte,f
	movf RcvByte,w
	xorwf crcl,f
	rrf RcvByte,w
	rrf RcvByte,f
	movf RcvByte,w
	xorwf crcl,f
	retlw 0


	
;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Start:

	movlw 00001111B
	option

	clrf gpio				; Prepare to make zeros on all GPIO pins
	movlw AllHiZ			; Leave Test Pin on Output mode
	tris gpio				; Take all I/O pins to Hi-Z


;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

; The reader loops emitting presence pulses and waiting for an iButton to
; arrive. The iButton is then read for ID number.

Main:

	clrwdt
	call TReset				; Issue 480 us Touch Reset pulse
	btfss Short
	goto Main				; Loop in wait if the bus is shorted
	btfss Presence
	goto Main				; Loop in wait for a presence to be seen

	movlw 33H
	movwf RcvByte
	call TByte				; Perform the Read ROM command byte

	call RByte				; Read 8 bytes of iButton serial number
	movf RcvByte,w

	btfsc Z
	goto Main				; Bad read if Family Code = 0

	movwf RomData0
	call crc8first
	
	call RByte
	movf RcvByte,w
	movwf RomData1
	call crc8
	
	call RByte
	movf RcvByte,w
	movwf RomData2
	call crc8
	
	call RByte
	movf RcvByte,w
	call crc8
	
	call RByte
	movf RcvByte,w
	call crc8
	
	call RByte
	movf RcvByte,w
	call crc8
	
	call RByte
	movf RcvByte,w
	call crc8
	
	call RByte
	movf RcvByte,w
	call crc8
	
	clrwdt

; Check the CRC8 of the ROMID. If it's BAD then goto MAIN and try again -

	movf crcl,w
	btfss Z
	goto Main						; Loop back if CRC8 is not good


GotKeyID:

; Map the Rom Data bits to Wiegand bits -

MapBits26:

	movlw SiteCode					; Start with the constant site code
	movwf Wiegand0
	
; Compute the Wiegand Parity Bits

; We rotate the entire 24 bit Wiegand register around and count bits -

ComputeParity:

; Count the 12 EP-related bits -

	clrf RcvByte					; Use RcvByte as a bit counter
	bcf EP							; Assume EP bit is zero
	movlw 12						; We will count 12 bits
	movwf LoopCounter
BCLp5:
	rlf Wiegand0,w					; Get the LS bit into the carry flag
	rlf Wiegand2,f
	rlf Wiegand1,f					; Rotate the entire 24 bit Wiegand value right
	rlf Wiegand0,f					;   so carry is the LS bit
	btfsc C							; If the bit is a "1" then
	incf RcvByte,f					; Increment the "1" bit counter
	decfsz LoopCounter,f				
	goto BCLp5						; Loop until 12 bits are counted	

	btfsc RcvByte,0					; The Parity is the LS bit of the bit count
	bsf EP							; We will need an EP bit if the count was odd


; Count the 12 OP-related bits -

	clrf RcvByte					; Use RcvByte as a bit counter
	bcf OP							; Assume OP bit is zero
	movlw 12						; We will count 12 bits
	movwf LoopCounter
BCLp2:
	rlf Wiegand0,w					; Get the LS bit into the carry flag
	rlf Wiegand2,f
	rlf Wiegand1,f					; Rotate the entire 24 bit Wiegand value right
	rlf Wiegand0,f					;   so carry is the LS bit
	btfsc C
	incf RcvByte,f
	decfsz LoopCounter,f
	goto BCLp2						; Loop until 12 bits are counted	

	btfss RcvByte,0					; The Parity is the LS bit of the bit count
	bsf OP							; We will need an OP bit if the count was even


; Insert the parity bits into the Wiegand data -

	clrf Wiegand3					; Assume all upper-bits to be zero

	bcf C
	btfsc OP						; Set CY to equal OP flag
	bsf C
	rrf Wiegand3,f					; Insert the Odd Parity bit first

	bcf C
	btfsc EP
	bsf C							; Make carry equal the EP flag

	rrf Wiegand0,f
	rrf Wiegand1,f					; Add EP bit, rotate all left once
	rrf Wiegand2,f					
	rrf Wiegand3,f					; Now Wiegand 0-3 equal 26 bit message

	clrwdt


;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

; Function:	WiegandOut

; Desc:		Given a site code and a key ID, output the Wiegand bit stream

WiegandOut:

	movlw 26					; Prepare to generate 26 Wiegand pulses
	movwf LoopCounter

WBSLoop:

	clrwdt
	rlf Wiegand3,f
	rlf Wiegand2,f
	rlf Wiegand1,f
	rlf Wiegand0,f
	btfsc C
	goto GenDataOne

GenDataZero:

	movlw Data0
	tris gpio					; Take the Data0 line LOW
	goto InterBit

GenDataOne:

	movlw Data1
	tris gpio					; Take the Data1 line LOW

InterBit:

	movlw 55					; 55 Loops = 110 cycles = 110us
	movwf Delay
DlyLp1:
	decfsz Delay,f
	goto DlyLp1

	movlw AllHiZ
	tris gpio					; Release DATA0 and DATA1 to Hi-Z

	movlw 0						; 256 Loops, 256*8 = 2048 cycles = 2.048 ms
	movwf Delay
DlyLp2:
	nop
	nop
	nop
	nop
	nop
	nop
	decfsz Delay,f
	goto DlyLp2

	decfsz LoopCounter,f
	goto WBSLoop


;++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

; Wait for the iButton to be gone -

WaitGone:

	clrwdt
	movlw 50					; We will expect 50 Resets without any presence
	movwf LoopCounter
WtLp1:
	call TReset					; Issue Touch Resets
	btfsc Presence				; If presence, start the wait over again
	goto WaitGone
	decfsz LoopCounter,f		; Keep checking to make sure it is gone
	goto WtLp1

	goto Main




	END
