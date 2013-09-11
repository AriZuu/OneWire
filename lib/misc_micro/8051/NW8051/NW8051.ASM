          EXTRN     CODE(TOUCHRESET, TOUCHBYTE, BIT_IO)
          EXTRN     DATA(ROMDTA, TEMP, L0)
          EXTRN     BIT(SETOK, L90, FAIL)
;
;   The following 8051 code implements the ROM search as shown in the
;   flowchart of figure 5-3. If no DOW parts are found, or the last DOW part
;   was the previous part found, the code searches for a DS1990. 
;
ACCESS90:
          PUSH      B                     ; Save the B register.
          CLR       C                     ; Assume failure.
          JNB       SETOK,    N90         ; Leave if SETUP failed.
          CALL      TOUCHRESET            ; Issue reset pulse.
          JNC       N90                   ; Leave if no parts on bus.
          MOV       A,        #0FH        ; Get read rom command in ACC.
          CALL      TOUCHBYTE             ; Send command byte.
          MOV       B,        #8          ; Prepare to read 8 bytes.
          MOV       TEMP,     #0          ; Initialize CRC variable.
READMORE:
          MOV       A,        #0FFH       ; Prepare to read a byte.
          CALL      TOUCHBYTE             ; Read byte.
          PUSH      ACC                   ; Save byte on stack.
          CALL      DOW_CRC               ; Calculate cummulative CRC.
          DJNZ      B,        READMORE    ; Repeat until finished.
          MOV       B,        #8          ; Prepare to pop 8 bytes off stack.
          MOV       A,        TEMP        ; Get CRC value in ACC.
          JZ        SUCCESS               ; Jump if successful.
          CJNE A,   #53,      FAILURE     ; Jump if failed.
SUCCESS:
          MOV       R0,       #ROMDTA+7   ; R0 points to last byte of ROM.
MOREPOP:
          POP       ACC                   ; Pop off byte.
          MOV       @R0,      A           ; Put byte in ROM data buffer.
          DEC       R0                    ; Point next least sig. byte of ROM.
          DJNZ      B,        MOREPOP     ; Get next byte.
          SETB      C                     ; Indicate success.
          SJMP      N90                   ; Leave.
FAILURE:
          POP       ACC                   ; Clean up stack.
          DJNZ      B,        FAILURE     ; Keep on going.
          CLR       C                     ; Indicate failure.
N90:
          POP       B                     ; Restore the B register.
          RET                             ; Return to caller.
FIRST:
          MOV       L0,       #65         ; Point rom search alg. to top.
NEXT:
          CLR       C                     ; Assume failure.
          JNB       SETOK,    LEV         ; Leave if SETUP failed.
          JB        L90,      LEVC        ; Already tried DS1990.
          CALL      ONEXT                 ; See if another DOW part on line.
          JC        LEV                   ; Leave if DOW part was found.
          CALL      ACCESS90              ; Look for a DS1990.
          SETB      L90                   ; Indicate we looked for a DS1990.
          SJMP      LEV                   ; Leave.
LEVC:
          CLR       L90                   ; Indicate next fail we can look.
          CLR       C                     ; Indicate failure.
LEV:
          RET                             ; Return to caller.
ONEXT:
          PUSH      B                     ; Save B register.
          CLR       C                     ; Assume failure.
          JB        FAIL,     ABORT       ; Continue if not at end.
          CALL      TOUCHRESET            ; Look for presence.
          CLR       A                     ; Zero the accumulator in case
          JNC       ABORT                 ;   there is no presence.
          MOV       R0,       #ROMDTA     ; Point to first byte of Rom Data.
          MOV       R1,       L0          ; Make copy of last unresolved dis.
          MOV       R2,       #0          ; Initialize 8 counter.
          MOV       B,        #64         ; Loop through all 64 bits.
          MOV       A,        #0F0H       ; Get search rom command in ACC.
          CALL      TOUCHBYTE             ; Output command byte.
GETNBIT:
          MOV       A,        #81H        ; Send first read time slot.
          CALL      BIT_IO                ; Read bit.
          RL        A                     ; Shift first bit left one.
          CALL      BIT_IO                ; Send second read time slot.
          JZ        DIS                   ; Analyze disagreement.
          CJNE A,   #3,       $+3         ; Look for error condition.
          JNC       ABORT                 ; Abort if error has occurred.
          CPL       ACC.0                 ; Get correct bit to output in
          SJMP      NOSAVE                ;   ACC.0 and skip dis code.
DIS:
          MOV       A,        B           ; Get current location in ACC.
          MOV       TEMP,     R1          ; Can't compare ACC and Rn.
          CJNE A,   TEMP,     L10         ; Compare current loc. w/ last dis.
          SETB      ACC.0                 ; Been here and sent 0, now send 1.
          SJMP      NOSAVE                ; Send out the bit.
L10:
          JNC       GL                    ; Remain on previous path.
          CLR       ACC.0                 ; Past last dis. fall to the right.
          SJMP      NZ                    ; Save this position in L0.
GL:
          MOV       A,        @R0         ; Get byte of Rom Data in ACC.
NZ:
          JB        ACC.0,    NOSAVE      ; Don't fool with L0 if old bit 1.
          MOV       L0,       B           ; Save new value of L0.
NOSAVE:
          CALL      BIT_IO                ; Send bit.
          MOV       C,        ACC.0       ; Get bit back in carry.
          MOV       A,        @R0         ; Get current byte in ACC.
          RRC       A                     ; Shift in new bit.
          MOV       @R0,      A           ; Save new value.
          INC       R2                    ; Point to location for next bit.
          CJNE R2,  #8,       DONTINC     ; Byte not ready yet.
          INC       R0                    ; R0 points to next storage loc.
          MOV       R2,       #0          ; Reload 8 counter.
DONTINC:
          DJNZ      B,        GETNBIT     ; Repeat until all bits known.
          MOV       A,        R1          ; Copy previous last dis. into ACC.
          CJNE A,   L0,       $+5         ; Is this the last part ?
          SETB      FAIL                  ; Leave.
          SETB      C                     ; Indicate success.
ABORT:
          POP       B                     ; Restore B register.
          RET                             ; Return to caller.
;
;         Procedure DOW_CRC
;
;         The assembly language procedure DOW_CRC given below
;         calculates the cumulative CRC of all the bytes passed
;         to it in the accumulator. Before it is used to calculate
;         the CRC of a data stream, it should be initialized by
;         setting the variable TEMP to zero. Each byte of the data
;         is then placed in the accumulator and DOW_CRC is called
;         to update the CRC. After all the data has been passed
;         to DOW_CRC, the variable TEMP will contain the result.
;
DOW_CRC:
          PUSH      ACC                   ; Save the Accumulator.
          PUSH      B                     ; Save the B register.
          PUSH      ACC                   ; Save bits to be shifted.
          MOV       B,        #8          ; Set to shift eight bits.
CRC_LOOP:
          XRL       A,        TEMP        ; Calculate DQIN xor CRCT0.
          RRC       A                     ; Move it to the carry.
          MOV       A,        TEMP        ; Get the last CRC value.
          JNC       ZERO                  ; Skip if DQIN xor CRCT0 = 0.
          XRL       A,        #18H        ; Update the CRC value.
ZERO:
          RRC       A                     ; Position the new CRC.
          MOV       TEMP,     A           ; Store the new CRC.
          POP       ACC                   ; Get the remaining bits.
          RR        A                     ; Position next bit in LSB.
          PUSH      ACC                   ; Save the remaining bits.
          DJNZ      B,        CRC_LOOP    ; Repeat for eight bits.
          POP       ACC                   ; Clean up the stack.
          POP       B                     ; Restore the B register.
          POP       ACC                   ; Restore the Accumulator.
          RET                             ; Return.
          END                             ; End of code.