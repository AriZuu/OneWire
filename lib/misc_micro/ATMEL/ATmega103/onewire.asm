
; ** ATmega103(L) Assembly Language - AVRASM Syntax

		.INCLUDE "M103def.inc"	; Add required path to IAR Directory




		.def  bit_test         = r17
		.def  search_direction = r18
		.def  bit_number       = r19
		.def  last_zero        = r21
		.def  serial_byte_num  = r22
		.def  serial_byte_mask = r23

.DSEG
	var0:.BYTE 1
	var1:.BYTE 1
	var2:.BYTE 1
	var3:.BYTE 1
	var4:.BYTE 1
	var5:.BYTE 1
	var6:.BYTE 1
	var7:.BYTE 1

	crc1:.BYTE 1

	LastDiscrepancy:.BYTE 1
	LastDevice:.BYTE 1

.CSEG
.ORG	0
		; ************* Stack Pointer Setup Code
		ldi r16, $0F		; Stack Pointer Setup
		out SPH,r16		; Stack Pointer High Byte
		ldi r16, $FF		; Stack Pointer Setup
		out SPL,r16		; Stack Pointer Low Byte

		; ******* RAMPZ Setup Code ****
		ldi  r16, $00		; 1 = EPLM acts on upper 64K
		out RAMPZ, r16		; 0 = EPLM acts on lower 64K

		; ******* Comparator Setup Code ****
		ldi r16,$80		; Comparator Disabled, Input Capture Disabled
		out ACSR, r16		; Comparator Settings

		; ******* Port B Setup Code ****
		ldi r16, $FF		;
		out DDRB , r16		; Port B Direction Register
		ldi r16, $00		; Init value
		out PORTB, r16		; Port B value

		; ******* Port E Setup Code ****
		ldi r16, $80		; I/O:
		out DDRE, r16		; Port E Direction Register
		ldi r16, $40		; Init value
		out PORTE, r16		; Port E value

		; ******* Port D Setup Code ****
		ldi r16, $00		; I/O:
		out DDRD, r16		; Port D Direction Register
		ldi r16, $00		; Init value
		out PORTD, r16		; Port D value

MAIN:

; ********************************************************
; INI_UART:  Initialize the UART for 9600 bits per second
;            8 data bits, 1 stop bit, no parity
; ********************************************************
init_uart:
	ldi r16, $19    ; set baud rate
	out UBRR, r16
	ldi r16, $18    ; init UART control register
	out UCR, r16

MAIN_loop:

	call ow_first
	brts present
	jmp  MAIN_loop

 search:
	call ow_search
	brts present

	; delay for last device
	; found
	ldi  r16, $AA
	out  DDRB, r16,
	ldi  r16, $FF
	call msDelay
	ldi  r16, $FF
	call msDelay
	ldi  r16, $55
	out  DDRB, r16,
	ldi  r16, $FF
	call msDelay
	ldi  r16, $FF
	call msDelay
	ldi  r16, $AA
	out  DDRB, r16,
	ldi  r16, $FF
	call msDelay
	ldi  r16, $FF
	call msDelay
	ldi  r16, $55
	out  DDRB, r16,
	ldi  r16, $FF
	call msDelay
	ldi  r16, $FF
	call msDelay

	jmp MAIN_loop

 present: ; out puts the serial number to the
          ; LEDs
	lds  r20, var0
	out  DDRB, r20
	call get_switch

	lds  r20, var1
	out  DDRB, r20
	call get_switch

	lds  r20, var2
	out  DDRB, r20
	call get_switch

	lds  r20, var3
	out  DDRB, r20
	call get_switch

	lds  r20, var4
	out  DDRB, r20
	call get_switch

	lds  r20, var5
	out  DDRB, r20
	call get_switch

	lds  r20, var6
	out  DDRB, r20
	call get_switch

	lds  r20, var7
	out  DDRB, r20
	call get_switch

	jmp  search

; **********************************************
; get_switch:  Gets a switch activity.  The
;              routine will wait for a button
;              push and then delay for debounce.
; **********************************************
get_switch:
	push r16
	push r18
	push r19
 inside_get_switch:
	in r18, PIND
	andi r18, $FF
	cpi r18, $FF

	breq inside_get_switch

	cpi r18, $FE
	brne not_0
	jmp found

not_0:
	cpi r18, $FD
	brne not_1
	jmp found

not_1:
	cpi r18, $FB
	brne not_2
	jmp found

not_2:
	cpi r18, $F7
	brne not_3
	jmp found

not_3:
	cpi r18, $EF
	brne not_4
	jmp found

not_4:
	cpi r18, $DF
	brne not_5
	jmp found

not_5:
	cpi r18, $BF
	brne not_6
	jmp found

not_6:
	cpi r18, $7F

found:
	ldi  r16, 20
	call msDelay
release:
	in r19, PIND
	andi r19, $FF
	cpi r19, $FF
	breq release

	ldi  r16, $FF
	call msDelay

	pop  r19
	pop  r18
	pop  r16
	ret

; **********************************************
; ow_first:  Reinitializes the search values
;            and calls ow_search to get the
;            first device found on the 1-Wire Net.
; **********************************************
ow_first:
	push r16

	ldi  r16, 0
	sts  LastDiscrepancy, r16
	sts  LastDevice, r16

	pop  r16

	call ow_search
	ret

; **********************************************
; ow_search:  Searches the 1-Wire Net for iButtons.
;             The T flag is set for a success and
;             cleared if an error, no devices on
;             the 1-Wire Net or the last device
;             was found.
; **********************************************
ow_search:
	push r16
	push r17
	push r18
	push r19
	push r20
	push r21
	push r22
	push r23

  next:
	ldi  bit_number, 1
	ldi  last_zero, 0
	ldi  serial_byte_num, 0
	ldi  serial_byte_mask, 1

	lds  r16, LastDevice
	sbrc r16, 0
	jmp  no_devices

	call ow_reset
	brts presence_pulse
	jmp  no_devices

  call00:
    call set_bit00
	jmp  return00

  call01:
    call set_bit01
	jmp  return01

  no_devices:
  	clt
  	jmp  exit_search

  presence_pulse:
  	ldi  r16, $F0
	call ow_write_byte
	call get_serial_num

   serial_num:
    ldi  r17, $00

	; read first bit of response
    call ow_read_bit
	brts call01
   return01:

	; read second compliment response
	call ow_read_bit
	brts call00
   return00:
	; choosing the bit values
	cpi  r17, 3
	breq no_devices
	cpi  r17, 0
	breq conflict

   bit_set:
   ldi  search_direction, 0
	sbrs r17, 1
	jmp  end_bit_set
	ldi  search_direction, 1
   end_bit_set:
   call setting_bit
   jmp  writebit

   conflict:
   lds  r20, LastDiscrepancy
   cp   bit_number, r20
	brlt same

	ldi  search_direction, 0
	cpse bit_number, r20
	jmp  skip
	ldi  search_direction, 1
	jmp  skip

   same:
    ldi  search_direction, 0
	sbrs serial_byte_mask, 0
	brne bitone
	sbrc r16, 0
	ldi  search_direction, 1
	sbr  r16, 1
	sbrs search_direction, 0
	cbr  r16, 1
	jmp  skip

   bitone:
   sbrs serial_byte_mask, 1
	brne bittwo
	sbrc r16, 1
	ldi  search_direction, 1
	sbr  r16, 2
	sbrs search_direction, 0
	cbr  r16, 2
	jmp  skip

   bittwo:
	sbrs serial_byte_mask, 2
	brne bitthr
	sbrc r16, 2
	ldi  search_direction, 1
	sbr  r16, 4
	sbrs search_direction, 0
	cbr  r16, 4
	jmp  skip

   bitthr:
	sbrs serial_byte_mask, 3
	brne bitfour
	sbrc r16, 3
	ldi  search_direction, 1
	sbr  r16, 8
	sbrs search_direction, 0
	cbr  r16, 8
	jmp  skip

   bitfour:
	sbrs serial_byte_mask, 4
	brne bitfive
	sbrc r16, 4
	ldi  search_direction, 1
	sbr  r16, 16
	sbrs search_direction, 0
	cbr  r16, 16
	jmp  skip

   bitfive:
	sbrs serial_byte_mask, 5
	brne bitsix
	sbrc r16, 5
	ldi  search_direction, 1
	sbr  r16, 32
	sbrs search_direction, 0
	cbr  r16, 32
	jmp  skip

   bitsix:
	sbrs serial_byte_mask, 6
	brne bitsev
	sbrc r16, 6
	ldi  search_direction, 1
	sbr  r16, 64
	sbrs search_direction, 0
	cbr  r16, 64
	jmp  skip

   bitsev:
	sbrs serial_byte_mask, 7
	brne skip
	sbrc r16, 7
	ldi  search_direction, 1
	sbr  r16, 128
	sbrs search_direction, 0
	cbr  r16, 128
	jmp  skip

   skip:
   call setting_bit
   sbrs search_direction, 0
	mov  last_zero, bit_number

   writebit:
	; serial number search direction write bit
   sbrs search_direction, 0
	clt
	sbrc search_direction, 0
	set
	call ow_write_bit

	; increment the byte counter bit_number
	; and shift the mask serial_byte_mask
	inc  bit_number
	clc
	lsl  serial_byte_mask

	brcc loop
	call load_serial_num
	inc  serial_byte_num
	ldi  serial_byte_mask, 1
	call get_serial_num

   loop:
    ldi  r25, 8
    cp   serial_byte_num, r25
	brlt looping

	call newcrc8
	lds  r24, var0
	call crc8
	lds  r24, var1
	call crc8
	lds  r24, var2
	call crc8
	lds  r24, var3
	call crc8
	lds  r24, var4
	call crc8
	lds  r24, var5
	call crc8
	lds  r24, var6
	call crc8
	lds  r24, var7
	call crc8

	ldi  r16, 0
	lds  r24, crc1
	cp   r16, r24
	breq good_crc

   bad_crc:
   	clt
	jmp  exit_search

   good_crc:
    sts  LastDiscrepancy, last_zero
	cp   r16, last_zero
	breq set_lastDevice
	set

  exit_search:
	pop  r23
	pop  r22
	pop  r21
	pop  r20
	pop  r19
	pop  r18
	pop  r17
	pop  r16

	ret

   looping:
    jmp  serial_num

   set_lastDevice:
    ldi  r16, 1
	sts  LastDevice, r16
	set
	jmp  exit_search

; **********************************************
; set_bit01:  Sets the 2nd bit for the compliment
;             bit in reading for the search.
; **********************************************
set_bit01:
	ori  r17, $02
	ret
; **********************************************
; set_bit00:  Sets the 1st bit for the value of
;             the serial number in doing the search.
; **********************************************
set_bit00:
	ori  r17, $01
	ret

; **********************************************
; setting_bit:  Sets of clears the proper bit
;               for the serial number byte in
;               r16.
; **********************************************
setting_bit:
	sbrs serial_byte_mask, 0
	jmp  bit1
	sbr  r16, 1
	sbrs search_direction, 0
	cbr  r16, 1
	jmp  end_setting

   bit1:
    sbrs serial_byte_mask, 1
	jmp  bit2
	sbr  r16, 2
	sbrs search_direction, 0
	cbr  r16, 2
	jmp  end_setting

   bit2:
    sbrs serial_byte_mask, 2
	jmp  bit3
	sbr  r16, 4
	sbrs search_direction, 0
	cbr  r16, 4
	jmp  end_setting

   bit3:
    sbrs serial_byte_mask, 3
	jmp  bit4
	sbr  r16, 8
	sbrs search_direction, 0
	cbr  r16, 8
	jmp  end_setting

   bit4:
    sbrs serial_byte_mask, 4
	jmp  bit5
	sbr  r16, 16
	sbrs search_direction, 0
	cbr  r16, 16
	jmp  end_setting

   bit5:
    sbrs serial_byte_mask, 5
	jmp  bit6
	sbr  r16, 32
	sbrs search_direction, 0
	cbr  r16, 32
	jmp  end_setting

   bit6:
    sbrs serial_byte_mask, 6
	jmp  bit7
	sbr  r16, 64
	sbrs search_direction, 0
	cbr  r16, 64
	jmp  end_setting

   bit7:
	sbr  r16, 128
	sbrs search_direction, 0
	cbr  r16, 128

   end_setting:
    ret

; **********************************************
; get_serial_num:  Gets the current byte of the
;                  serial number into r16.  r24
;                  is used for the counter and
;                  gets number from
;                  serial_byte_number location.
; **********************************************
get_serial_num:
	ldi  r16, 0
	mov  r24, serial_byte_num
	cp   r24, r16
	breq zero_serial_num2
	lds  r16, var1
	dec  r24
	breq return2
	lds  r16, var2
	dec  r24
	breq return2
	lds  r16, var3
	dec  r24
	breq return2
	lds  r16, var4
	dec  r24
	breq return2
	lds  r16, var5
	dec  r24
	breq return2
	lds  r16, var6
	dec  r24
	breq return2
	lds  r16, var7
	dec  r24
	breq return2

   zero_serial_num2:
    lds  r16, var0

   return2:
    ret

; **********************************************
; load_serial_num:  Loads the current byte of the
;                   serial number from r16.  r24
;                   is used for a counter and
;                   gets number from
;                   serial_byte_number location.
; **********************************************
load_serial_num:
	push r18

	ldi  r18, 0
	mov  r24, serial_byte_num
	cp   r24, r18
	brne byte1
	jmp  zero_serial_num1

  byte1:
	dec  r24
	brne byte2
	sts  var1, r16
	jmp  return1

  byte2:
	dec  r24
	brne byte3
	sts  var2, r16
	jmp  return1

  byte3:
	dec  r24
	brne byte4
	sts  var3, r16
	jmp  return1

  byte4:
	dec  r24
	brne byte5
	sts  var4, r16
	jmp  return1

  byte5:
	dec  r24
	brne byte6
	sts  var5, r16
	jmp  return1

  byte6:
	dec  r24
	brne byte7
	sts  var6, r16
	jmp  return1

  byte7:
	sts  var7, r16
	jmp  return1

 zero_serial_num1:
    sts  var0, r16

 return1:
   	pop  r18
    ret

; **********************************************
; ow_read_bit:  Reads a bit of data from the 1-Wire
;               The T flag will be set on what is
;               read
; **********************************************
ow_read_bit:
	set
; **********************************************
; ow_write_bit:  Writes a bit of data to the 1-Wire
;                Set the T flag for writing a 1,
;                or clear for a 0.
; **********************************************

ow_write_bit:
	push r16
	push r18

	brts bitset

	ldi  r16, $00
	jmp done_bit_set

  bitset:
  	ldi r16, $FF

  done_bit_set:
  	ldi  r18, $05
  	sbi  PORTE, 7

  usdelay1:  ; delay of 5 us
  	nop
	dec  r18
	brne usdelay1
	ldi  r18, $09

	; put bit into carry
	ror  r16
	brcc zero
	cbi  PORTE, 7

  zero:      ; delay of 10 us
  	nop
	dec  r18
	brne zero

	; total time before read 16 us
	clt
	sbic PINE, 6
	set

	ldi  r18, $2D  ; delay 45 us for write 0

   usdelay2:  ; delay
	nop
	dec  r18
	brne usdelay2

	; leave I/O pin high for recovery
	ldi  r18, $0A
	cbi  PORTE, 7
   usdelay3:  ; delay for 10 us to recover
	nop
	dec  r18
	brne usdelay3

	pop  r18
	pop  r16
	ret

; **********************************************
; ow_read_byte:  Reads a byte of data from the 1-Wire
;                r16 will be set to the read in
;                byte.
; **********************************************
ow_read_byte:
	ldi r16, $FF
; **********************************************
; ow_write_byte:  Writes a byte of data to the 1-Wire
;                 Set r16 to the value you want
;                 to write.
; **********************************************
ow_write_byte:
	push r17
	push r18
	push r19
	ldi  r17, 8

rwloop:
  	ldi  r18, $04
  	sbi  PORTE, 7

  us4delay1:
  	nop
	dec  r18
	brne us4delay1
	ldi  r18, $0A

	; put bit into carry
	ror  r16
	brcc zero_bit
	cbi  PORTE, 7

  zero_bit:
  	nop
	dec  r18
	brne zero_bit

	clc
	sbic PINE, 6
	sec
	ror  r19

	ldi  r18, $3C
   us50delay:
	nop
	dec  r18
	brne us50delay

	; leave I/O pin high for recovery
	ldi  r18, 5
	cbi  PORTE, 7
   us4delay2:
	nop
	dec  r18
	brne us4delay2

	dec  r17
	brne rwloop

	mov  r16, r19

	pop  r19
	pop  r18
	pop  r17
	ret

; **********************************************
; ow_reset:  A reset of the 1-Wire Net
; **********************************************
ow_reset:
	push r20
	push r21
	push r22
	ldi  r21, 120
	ldi  r20, $40
	sbi  PORTE, 7    ; start of a reset pulse

	; delay 480us
  loop1:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	dec r21
	brne loop1

	cli              ; disable all interrupts
	cbi	PORTE, 7     ; end the reset pulse

	; wait for 16us
	ldi r21, 16
  loop2:
	nop
	dec r21
	brne loop2

	; read sensor pin for 368us
	ldi r21, $FF
  loop3:
  	nop
	nop
	nop
	nop
	nop
	nop
	nop
	clt
	sbis PINE, 6
	jmp done_reset
	dec r21
	brne loop3
	pop  r22
	pop  r21
	pop  r20
	ret

  done_reset:
  	ldi r21, $FF
	set

  presence_loop:
	sbis PINE, 6
	jmp  presence_loop

	; wait 336us after presence pulse
  done_loop:
  	nop
	nop
	nop
	nop
	nop
	nop
	dec r21
	brne done_loop

	pop r22
	pop r21
	pop r20
	ret


; **********************************************
; msDelay:  A micro second delay up to 256.
;           r16 should be set to the number of
;           ms to delay.
; **********************************************
msDelay:
    push r21
  msDelay_loop_0:
	ldi r21, 250
  msDelay_loop_1:
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	dec r21
	brne msDelay_loop_1
	dec r16
	brne msDelay_loop_0
	pop r21
	ret

; **********************************************
; NEWCRC8:  Initializing the crc to perform CRC
;           check on the Serial Num.
; **********************************************

newcrc8:
	push r16

	ldi  r16, 0
	sts  crc1, r16

	pop  r16
	ret

; **********************************************
; CRC8:  After initializing the crc call this
;        to perform CRC check on the Serial Num.
;        r24 should be set to the next crc byte.
; **********************************************
crc8:
	push r17

   ; printing out to the serial port
	; what the serial number received
	; from the search
	mov  r17, r24
	call txcomp

	lds  r17, crc1

	eor  r24, r17
	mov  r17, r24
	andi r17, $F0
	swap r24
	eor  r17, r24
	clc
	sbrc r24, 0
	sec
	ror  r24
	eor  r17, r24
	clc
	sbrc r24, 0
	sec
	ror  r24
	swap r24
	andi r24, $C7
	sbrc r24, 2
	call complimentbit3

	eor  r17, r24
	swap r24
	andi r24, $0C
	eor  r17, r24
	clc
	sbrc r24, 0
	sec
	ror  r24
	eor  r17, r24
	sts  crc1, r17

	pop  r17
	ret

complimentbit3:
	sbrs r24, 3
	jmp  set_bit_3
	jmp  clear_bit_3

   return:
	ret

  set_bit_3:
  	ori  r24, $08
	jmp  return

  clear_bit_3:
  	andi r24, $F7
	jmp  return

; **********************************************
; TXCOMP:  Transmit a byte from the serial port
;          polls to see if the UDRE flag is '1'.
;          If '1' then a byte is written to the
;          UDR to be transmitted.
; **********************************************
txcomp:
	push r18
	push r19
	push r20
	push r21

	ldi  r18, $F0
	ldi  r19, $0A

	and  r18, r17
	swap r18
	cp   r18, r19
	brlt num_print1

	ldi  r20, 'A' - 10
	add  r20, r18

  	out  UDR, r20
  loop_print1:
  	sbis USR, UDRE    ; check end of transmission
	jmp  loop_print1


	jmp  next4bits

  num_print1:
  	ldi  r20, '0'
	add  r20, r18

  	out  UDR, r20
  loop_print2:
  	sbis USR, UDRE    ; check end of transmission
	jmp  loop_print2


  next4bits:
	ldi  r18, $0F
	ldi  r19, $0A

	and  r18, r17
	cp   r18, r19
	brlt num_print2

	ldi  r20, 'A' - 10
	add  r20, r18

  	out  UDR, r20
  loop_print3:
  	sbis USR, UDRE    ; check end of transmission
	jmp  loop_print3


	ldi  r20, $20
	call print
	jmp  exit_txcomp

  num_print2:
  	ldi  r20, '0'
	add  r20, r18

  	out  UDR, r20
  loop_print4:
  	sbis USR, UDRE    ; check end of transmission
	jmp  loop_print4


	ldi  r20, $20
  	out  UDR, r20
  loop_print5:
  	sbis USR, UDRE    ; check end of transmission
	jmp  loop_print5

 exit_txcomp:
	pop  r21
	pop  r20
	pop  r19
	pop  r18
	ret

  print:
  	out  UDR, r20
  loop_print:
  	sbis USR, UDRE    ; check end of transmission
	jmp  loop_print

	ret

.EXIT




