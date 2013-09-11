CODE    segment byte public 'CODE'
        assume cs:CODE

romdta  db        8       dup(0)        ; Buffer containing ROM data.
;
;   The following code checks the Dallas Semiconductor 1-Wire CRC of the
;   data in the 8 byte array romdta. The function dowchk initializes the
;   cummulative crc byte (bx register) and computes the crc of all eight
;   bytes. It returns 1 (true) if the crc = 0 and 0 (false) otherwise.
;
dowchk  proc      near
        push      si                    ; Preserve si register.
        push      bx                    ; Preserve bx register.
        xor       si,     si            ; Clear index.
        xor       bx,     bx            ; Initailize reg. used for Dow CRC.
        push      bx                    ; Get CRC value on stack.
loop:
        mov       al,     romdta[si]    ; Store byte from array.
        pop       bx                    ; Restore crc value.
        mov       bl,     al            ; Get new byte in bl.
        xchg      bx,     ax            ; Get word for dowcrc in ax.
        call      dowcrc                ; Compute cummulative crc.
        push      ax                    ; Save current value of crc.
        inc       si                    ; Point to next storage location.
        cmp       si,     8             ; Have read read all of the Rom Data.
        jne       loop                  ; Read next byte if not finished.
        pop       bx                    ; Clean up stack.
        cmp       ah,     0             ; Is the crc good.
        je        scexit                ; Exit successful.
        xor       al,     al            ; Return failure condition.
        jmp       No90                  ; Not a good crc, return failure con.
scexit:
        mov       al,     1             ; Set success condition.
No90:
        pop       bx                    ; Restore bx register.
        pop       si                    ; Restore si register.
        ret                             ; Return to caller.
dowchk  endp
;
dowcrc  proc     near                   ; Procedure to calculate DOW crc.
        push     cx                     ; Save contents of cx register.
        mov      cx,     8              ; Prepare to calc. crc for 8 bits.
crca:
        mov      bl,     ah             ; Get ah in bl.
        xor      bl,     al             ; Xor result with byte given.
        shr      ax,     1              ; Shift result right.
        test     bl,     1              ; Look at bl.
        jz       crcb                   ; Dont xor if 0.
        xor      ah,     8CH            ; Flip bits 7, 3, 2.
crcb:
        loop     crca                   ; Repeat until all eight bits done.
        pop      cx                     ; Restore contents of cx register.
        ret                             ; Return to caller.
dowcrc  endp                            ; End of DOW crc procedure.

CODE    ends

        end
