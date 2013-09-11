;
;   The following 8051 code demonstrates reading and writing 92's, 93's, 94's
; using universal data packets. It transmits the results to the serial port
; at 9600 bps.
;
          EXTRN     CODE(TOUCHRESET, DATA_IN, TOUCHBYTE)
;
DATA_BIT  BIT       P1.3                ; Touch Memory data.
GND_BIT   BIT       P1.4                ; Touch Memory ground.
;
PCON      EQU       87H                 ; Define special purpose
MCON      EQU       0C6H                ;    registers specific
TA        EQU       0C7H                ;       to the DS5000.
;
BUFF      EQU       900H                ; Buffer in external memory.
;
CODE_SEG  SEGMENT   CODE                ; Code is in this segment.
;
          DSEG AT   20H
LO:       DS        1                   ; Low byte of CRC.
; 
          DSEG AT   30H
HI:       DS        1                   ; High byte of CRC.
STACK:    DS        1                   ; Stack starts after data.
;
          CSEG      AT        RESET     ; Power on reset vector.
          LJMP      START               ; Program begins here.
;
          RSEG      CODE_SEG            ; Code segment begins here.
START:
          MOV       TA,       #0AAH     ; Timed
          MOV       TA,       #55H      ;    access.
          MOV       PCON,     #0        ; Disable watchdog timer.
          MOV       TA,       #0AAH     ; Timed
          MOV       TA,       #55H      ;    access.
          ORL       MCON,     #2        ; Enable partition access.
          MOV       MCON,     #12H      ; Set partition at 800H.
          MOV       TA,       #0AAH     ; Timed
          MOV       TA,       #55H      ;    access.
          ANL       MCON,     #0FDH     ; Disable partition access.
;
          MOV       SP,       #STACK-1  ; Reset the stack pointer.
          CLR       GND_BIT             ; Touch Memory ground level.
;
          CLR       TR1                 ; Stop the serial timer.
          MOV       TH1,      #0FAH     ; Set for 9600 bps when using
          MOV       TL1,      #0FAH     ;    a 11.0592 MHz crystal.
          ANL       TMOD,     #0FH      ; Set timer for the eight
          ORL       TMOD,     #20H      ;    bit auto-reload mode.
          ORL       PCON,     #80H      ; Set double bit rate.
          MOV       SCON,     #52H      ; Setup the serial port.
          SETB      TR1                 ; Start the serial timer.
MAIN_LOOP:
          JB        DATA_BIT, $         ; Wait for presence start.
          JNB       DATA_BIT, $         ; Wait for presence end.
          CALL      KEYMACRO            ; *** Try to read Touch Memory.
          JNC       MAIN_LOOP           ; No valid packet, try again. 
          LJMP      SPILLIT             ; Send valid data packet to CPU.
          SJMP      MAIN_LOOP           ; Look for touch memory.
SPILLIT:
          MOV       DPTR,     #BUFF     ; Initialize the data pointer.
          MOV       B,        #1        ; Assume short string.
          MOVX      A,        @DPTR     ; Get string length.
          INC       DPTR                ; Increment data pointer.
          CJNE A,   #0FFH,    LA        ; Branch if short string.
          INC       B                   ; Set for long string.
          MOVX      A,        @DPTR     ; Get string length - 255.
          INC       DPTR                ; Increment data pointer.
LA:
          PUSH      B                   ; Set long or short string.
          MOV       B,        A         ; Set character count.
          SJMP      LC                  ; Start getting characters.
LB:
          PUSH      B                   ; Set long or short string.
          MOV       B,        #0FFH     ; Do 255 more characters.
LC:
          MOVX      A,        @DPTR     ; Get a character.
          INC       DPTR                ; Increment data pointer.
          JNB       TI,       $         ; Wait until transmitter not busy.
          MOV       SBUF,     A         ; Send out character in accumulator.
          CLR       TI                  ; Indicate transmitter bust.
          DJNZ      B,        LC        ; Go transmit next character.
          POP       B                   ; Get extended length.
          DJNZ      B,        LB        ; Go get next group of 255.
          CALL      GO_AWAY             ; *** Wait for part to leave.
          LJMP      MAIN_LOOP           ; Wait for next touch.
;
;********************************************************************
;
;         Procedure KeyMacro
;
;         This procedure executes a debounced call to GetData
;         and returns with the Carry cleared or set according
;         to the result of GetData.
;
KEYMACRO:
          MOV       B,        #255      ; Number of trys allowed.
KEYLA:
          CALL      GETDATA             ; Try to read the data.
          JC        KEYLB               ; Continue if successful.
          DJNZ      B,        KEYLA     ; Otherwise try again.
KEYLB:
          RET                           ; Return.
;
;         Procedure Go_Away
;
;         This procedure insures that the Touch Memory is gone
;         before returning so that it will not be read more than
;         once.
;
GO_AWAY:
          MOV       B,        #128      ; Number of trys allowed.
GO:
          CALL      TOUCHRESET          ; See if there's a part
          JC        GO_AWAY             ; Wait while there is.
          DJNZ      B,        GO        ; Count while there's not.
          RET                           ; Return.
;
;         Procedure GetData
;
;         This procedure should be called to attempt to read the
;         data from a Touch Memory after a presence pulse is
;         received on the one-wire bus. It attempts to read a
;         string of characters from the Touch Memory into a RAM
;         buffer and check the CRC16 which follows the string.
;         If the CRC is correct, the procedure returns with the
;         Carry bit set, indicating that the string in the RAM
;         buffer is ready for output. The first byte in the RAM
;         buffer is the number of subsequent characters to be
;         output. If the Carry is cleared, then the attempt to
;         read the Touch Memory was unsuccessful.
;
GETDATA:
          PUSH      B                   ; Save the try count.
          CALL      TOUCHRESET          ; Send out reset signal.
          JNC       SA                  ; Abort if no presence.
          SJMP      PD                  ; Skip jump.
SA:
          LJMP      ABORT               ; Take the hike to abort.
PD:
          MOV       A,        #33H      ; Get read rom command in accumulator.
          CALL      TOUCHBYTE           ; Command to read the ROM.
          CALL      DATA_IN             ; Read the family code.
          CALL      TOUCHRESET          ; Restart at beginning.
          JNC       ABORT               ; Abort if no presence.
          CJNE      A,  #8,   NOT_1992  ; Branch if not a DS1992.
          MOV       B,        #126      ; Max string length is 125.
          SJMP      CONT
NOT_1992:
          MOV       B,        #0        ; Max string length is 508.
          CJNE      A,  #6,   NOT_1993  ; Branch if not a DS1993.
          SJMP      CONT
NOT_1993:
          CJNE      A,  #4,   ABORT     ; Abort if not a DS1994.
CONT:
          MOV       A,        #0CCH
          CALL      TOUCHBYTE           ; Skip over ROM section.
          MOV       A,        #0F0H
          CALL      TOUCHBYTE           ; Command to read the RAM.
          CLR       A
          CALL      TOUCHBYTE           ; First address byte zero.
          CLR       A
          CALL      TOUCHBYTE           ; Second address byte zero.
FINISH:
          MOV       LO,       #0        ; Initialize the CRC accumulator
          MOV       HI,       #0        ;   with the starting page number.
          MOV       DPTR,     #BUFF     ; Point to start of buffer.
          CALL      GET_BYTE            ; Read the string length.
          INC       B                   ; Prepare to test B for zero.
          DJNZ      B,        BNZ       ; Branch if B not zero.
          CJNE A,   #0FFH,    ANZ     
          MOVX      @DPTR,    A         ; Store zero for extended length.
          INC       DPTR                ; Increment buffer pointer.
          CALL      GET_BYTE            ; Get the length - 256.
          JZ        ANZ                 ; Branch if length = 256.
          CJNE      A, #254,  $+3       ; Check the string length.
          JNC       ABORT               ; Branch if length > 508.
          MOV       B,        #2        ; Setup to read long string.
          SJMP      SIZE                ; Set string size and go.
ABORT:
          CLR       C                   ; Indicate failure.
          POP       B                   ; Retrieve the try count.
          RET                           ; Return.
BNZ:
          JZ        ABORT               ; Abort if string too long.
          CJNE      A,   B,   $+3       ; Check size of string.
          JNC       ABORT               ; Abort if string too long.
ANZ:
          MOV       B,        #1        ; Setup to read short string.
SIZE:
          PUSH      B                   ; Store string size byte.
          MOV       B,        A         ; Set string length.
          MOVX      @DPTR,    A         ; First byte in buffer is length.
          INC       DPTR                ; Increment buffer pointer.
          SJMP      IN_LOOP             ; Go input the string.
BIG_LOOP:
          PUSH      B                   ; Save loop count.
          MOV       B,        #0FFH     ; Set for next group.
IN_LOOP:
          CALL      GET_BYTE            ; Get a character.
          MOVX      @DPTR,    A         ; Store it in the buffer.
          INC       DPTR                ; Increment buffer pointer.
          DJNZ      B,        IN_LOOP   ; Go for next character.
          POP       B                   ; Check high byte of count.
          DJNZ      B,        BIG_LOOP  ; Go for next character.
          CALL      GET_BYTE            ; Include the CRC in
          CALL      GET_BYTE            ;    the CRC calculation.
          MOV       A,        HI        ; Check high byte of CRC.
          CJNE      A, #0B0H, ABORT     ; Abort if it is wrong.
          MOV       A,        LO        ; Check low byte of CRC.
          CJNE      A, #001H, ABORT     ; Abort if it is wrong.
SUCCESS:
          SETB      C                   ; Indicate successful read.
          POP       B                   ; Retrieve the try count.
          RET                           ; Return.
;
;   This procedure takes the byte received from DATA_IN and calculates the
; cummulative CRC16.
;
GET_BYTE:
          CALL      DATA_IN
CRC16:
          PUSH     ACC                   ; Save the character.
          XRL      A,      LO            ; Xor accumulator with val. in LO.
          MOV      LO,     HI            ; Put high byte of crc in its dest.
          MOV      HI,     A             ; Save data xor low(crc) for later
          MOV      C,      P             ; Get parity bit in carry.
          JNC      CRC0                  ; Don't XRL if carry clear.
          XRL      LO,     #01h          ; Add the parity to crc bit 0
CRC0:
          RRC      A                     ; Get the low bit in c
          JNC      CRC1                  ; Don't XRL if carry clear.
          XRL      LO,     #40h          ; Need to fix bit 6 of the result
CRC1:
          MOV      C,      ACC.7         ; Get msb of accumulator in carry.
          XRL      A,      HI            ; Compute the results for bits P...U
          RRC      A                     ; Shift them into place
          MOV      HI,     A             ; And save them
          JNC      CRC2                  ; Don't flip msb LO if carry clear.
          CPL      LO.7                  ; Now clean up bit 7
CRC2:
          POP      ACC                   ; Restore the character.
          RET                            ; Return to caller.
;
          END                            ; End of Program.