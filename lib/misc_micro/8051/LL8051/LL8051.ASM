; The following 8051 code uses a bi-directional port pin (specified by
; DATA_BIT) for 1-wire I/O. This code was written for an 11.0592 MHz
; crystal.
;
;         Procedure TouchReset
;
;         This procedure transmits the Reset signal to the Touch
;         Memory and watches for a presence pulse. On return,
;         the Carry bit is set if a presence pulse was detected,
;         otherwise the Carry is cleared. The code is timed for
;         an 11.0592 MHz crystal.
;
PUBLIC   TOUCHRESET,DATA_IN,TOUCHBYTE,TOUCHBIT,BIT_IO
DATA_BIT  BIT       P0.0
;
TOUCHRESET:
          PUSH      B                     ;       Save the B register.
          PUSH      ACC                   ;       Save the accumulator.
          MOV       A,        #4          ;       Load outer loop variable.
          CLR       DATA_BIT              ;       Start the reset pulse.
          MOV       B,        #221        ;   2.  Set time interval.
          DJNZ      B,        $           ; 442.  Wait with Data low.
          SETB      DATA_BIT              ;   1.  Release Data line.
          MOV       B,        #6          ;   2.  Set time interval.
          CLR       C                     ;   1.  Clear presence flag.
WAITLOW:
          JB        DATA_BIT, WH          ;       Exit loop if line high.
          DJNZ      B,        WAITLOW     ;       Hang around for 3360
          DJNZ      ACC,      WAITLOW     ;          us if line is low.
          SJMP      SHORT                 ;       Line could not go high.
WH:
          MOV       B,        #111        ;       Delay for presence detect.
HL:
          ORL       C,        /DATA_BIT   ; 222.  Catch presence pulse.
          DJNZ      B,        HL          ; 222.  Wait with Data high.
SHORT:
          POP       ACC                   ;       Restore accumulator.
          POP       B                     ;       Restore B register.
          RET                             ;       Return.
;
;         Procedures Data_In and TouchByte
;
;         The procedure TouchByte sends the byte in the accumulator
;         to the Touch Memory, and the procedure Data_In returns a
;         byte from the Touch Memory in the accumulator. Note that
;         the NOPs in the following code are intended to give the
;         optimum performance when using a 11.0592 MHz crystal.
;         Their purpose is to make the pulses as long as
;         possible consistent with the Touch Memory timing
;         constraints. When using other crystal frequencies,
;         the delays in this code should be adjusted to conform
;         to the timing requirements of the Touch Memory.
;
BIT_IO:
          RRC       A                     ; Get bit to send in carry.
          CALL      TOUCHBIT              ; Send bit.
          RLC       A                     ; Collect returned bit in ACC.
          RET                             ; Return to caller.
DATA_IN:
          MOV       A,        #0FFH       ;       Initialize for input.
TOUCHBYTE:
          PUSH      B                     ;       Save the B register.
          MOV       B,        #8          ;       Setup for 8 bits.
BIT_LOOP:
          RRC       A                     ;   1.  Get bit in carry.
          CALL      TOUCHBIT              ;   2.  Send bit.
          DJNZ      B,        BIT_LOOP    ;   2.  Get next bit.
          RRC       A                     ;       Get final bit in ACC.
          POP       B                     ;       Restore B register.
          RET                             ;       Return to caller.
TOUCHBIT:
          CLR       DATA_BIT              ;   1.  Start the time slot.
          NOP                             ;   1.  Delay to make sure
          NOP                             ;   1.    that the Touch Memory
          NOP                             ;   1.      sees a low for at
          NOP                             ;   1.        least 1 microsecond.
          MOV       DATA_BIT, C           ;   2.  Send out the data bit.
          NOP                             ;   1.  Delay to give the
          NOP                             ;   1.    data returned from
          NOP                             ;   1.      the Touch Memory
          NOP                             ;   1.        time to settle
          NOP                             ;   1.          before reading
          NOP                             ;   1.            the bit.
          MOV       C,        DATA_BIT    ;   1.  Sample input data bit.
          PUSH      B                     ;   2.  Save B register.
          MOV       B,        #12H        ;   2.  Delay until the end
          DJNZ      B,        $           ;   36.   of the time slot.
          POP       B                     ;       Restore B register.
          SETB      DATA_BIT              ;       Terminate time slot.
          RET                             ;       Return to caller.
;
          END                             ; End of module.