          EXTRN     DATA(TEMP, ROMDTA)
;
;   The following code checks the Dallas Semiconductor 1-Wire CRC of the
;   data in the 8 byte array romdta. The function dowchk initializes the
;   cummulative crc byte (TEMP) and computes the crc of all eight bytes.
;   It returns with the carry set if TEMP = 0. Otherwise it returns with
;   the carry clear.
;
DOWCHK:
          MOV       A,        R0          ; Get contents of R0 in accumulator.
          PUSH      ACC                   ; Save contents of R0.
          PUSH      B                     ; Save the B register.
          MOV       B,        #8          ; Prepare for 8 bytes.
          MOV       TEMP,     #0          ; Initialize CRC variable.
          MOV       R0,       #ROMDTA     ; Point to first byte of ROM data.
CALCMORE:
          MOV       A,       @R0          ; Get byte of ROM data in acc.
          CALL      DOW_CRC               ; Calculate cummulative CRC.
          INC       R0                    ; Point to next byte of ROM data.
          DJNZ      B,        CALCMORE    ; Repeat until finished.
          MOV       A,        TEMP        ; Get CRC value in ACC.
          JZ        SUCCESS               ; Jump if successful.
          SJMP      FAILURE               ; Exit with failure.
SUCCESS:
          SETB      C                     ; Indicate success.
          SJMP      GOODCRC               ; Leave.
FAILURE:
          CLR       C                     ; Indicate failure.
GOODCRC:
          POP       B                     ; Restore the B register.
          POP       ACC                   ; Get old R0 in accumulator.
          MOV       R0,       A           ; Restore R0.
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