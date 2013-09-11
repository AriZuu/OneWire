        page      58,132

CODE    segment   byte public 'CODE'
        assume    cs:CODE

; The following functions are assumed to be externally defined
; to perform 1-Wire communication.  The function TouchReset
; returns with the carry bit set if a Presence pulse was
; detected.  Otherwise the carry bit is cleared.  The function
; TouchByte transmits the byte in AL to the 1-Wire bus,
; concurrently receives a byte from the 1-Wire bus, and
; returns it in AL.  The function TouchBit transmits the
; carry bit to the 1-Wire bus, concurrently receives a bit
; from the 1-Wire bus, and returns it in the carry.

        extrn     touchreset:far,touchbyte:far,touchbit:far

L0      db        1      dup(0)
LD      db        1      dup(0)
LC      db        1      dup(0)
EC      db        1      dup(0)
FB      db        1      dup(0)
romdta  db        8      dup(0)

;
;   The following 8086 code implements the ROM search as shown in the
;   flowchart of figure 5-3. If no DOW parts are found, or the last DOW part
;   was the previous part found, the code searches for a DS1990.
;
;
;   This procedure finds the first DOW part on the 1-wire bus. If no DOW parts
; are found it searches for a DS1990.
;
first   proc      near                  ; Procedure to find first part on bus.
        mov       L0,     65            ; Point Rom Search algorithm to top.
        mov       fb,     0             ; Try to find a DOW part first.
        call      next                  ; Go find first part.
        ret                             ; Return to caller.
first   endp                            ; End of first procedure.
;
;   This procedure finds the next DOW part on the 1-wire bus. If no more DOW
; parts are on the bus is searches for a DS1990.
;
next    proc      near                  ; Procedure to find next part on bus.
        cmp       fb,     1             ; Have we already looked for DS1990.
        je        look90                ; If so force next failure.
        jg        levc                  ; At end of line time to fail.
        call      onext                 ; Find next part on bus.
        cmp       al,     1             ; Did we find a DOW part. 
        je        levs                  ; Leave if successful.
        cmp       al,     2             ; Did we find the last DOW part.
        jne       look90                ; onext returned false.
        dec       al                    ; Return a one.
        mov       fb,     1             ; Set look for DS1990 flag.
        jmp       levs                  ; Leave successful.
look90:
        call      access90              ; Look for a DS1990.
        mov       fb,     2             ; Indicate we looked for a ds1990. 
        jmp       levs                  ; Leave w/ return result of access90.
levc:
        mov       fb,     0             ; Clear look for DS1990 flag.
        xor       al,     al            ; Set failure condition.
levs:
        ret                             ; Return to caller.
next    endp                            ; End of next procedure.
;
onext   proc      near                  ; Rom search procedure.
        push      si                    ; Save si reg.
        push      bx                    ; Save bx register.
        push      cx                    ; Save cx register.
        mov       al,      L0           ; Get rom search pointer in al.
        mov       LD,      al           ; Save last disagreement position
        xor       si,      si           ;    from last search.
        call      touchreset            ; Send a TouchReset command.
        jnc       bjump                 ; Abort if no Presence pulse.
        mov       al,     0F0H          ; Call TouchByte to send rom
        call      touchbyte             ;    search command to 1-wire bus.
        mov       LC,     64            ; 64 bits of Rom Data.
        mov       EC,     8             ; Initialize eight counter.
        jmp       romloop               ; Time to read the bits.
bjump:
        jmp       fail                  ; Make big jump to fail.
romloop:
        mov       al,     0             ; Clear the bit accumulator.
        stc				; Prepare to send read time slot.
        call      touchbit              ; Send read time slot.
        rcl       al,     1             ; Capture result in al.
        stc				; Prepare to send read time slot.
        call      touchbit              ; Send read time slot.
        rcl       al,     1             ; Capture result in al.
        cmp       al,     3             ; Check for error condition.
        je        fail                  ; Fail if error has occured.
        cmp       al,     0             ; Check for disagreement.
        je        dis                   ; Is there a disagreement on bus.
        xor       al,     1             ; No disagreement, write bit in al. 
        jmp       nodis                 ; Go send the bit.
dis:
        mov       cl,     LC            ; Get rom loop counter in cl.
        cmp       cl,     LD            ; Compare current position w/ last.
        jg        slb                   ; Before dis. from last search.
        jl        s0                    ; If there not equal send write 0.
        mov       al,     1             ; If equal send write 1 to bus.
        jmp       nodis                 ; Don't save location of this dis.
s0:
        mov       al,     0             ; Clear al to indicate send write 0.
        jmp       saveloc               ; Go save location of this dis.
slb:                                    
        mov       al,     romdta[si]    ; Get last bit in lsb of accumulator.
        and       al,     1             ; Save lsb to send to DOW bus.
        jnz       nodis                 ; If bit is one don't save location.
saveloc:
        mov       L0,     cl            ; Save position of this disagreement.
nodis:
        rcr       al,     1             ; Move output bit into carry.
        call      touchbit              ; Transmit the time slot.
        rcl       al,     1             ; Put result into al.
        mov       bl,     romdta[si]    ; Get previous byte value in dl.
        mov       bh,     al
        shr       bx,     1
        mov       romdta[si],bl         ; Save new byte value.
        dec       EC                    ; Decrement eight counter.
        jnz       notbyte               ; Not a complete byte yet.
        inc       si                    ; Point to location for next byte.
        mov       EC,      8            ; Reload eight counter.
notbyte:
        dec       LC                    ; Decrement 64 counter.
        jnz       romloop               ; Have we gotten 64 bits yet?
        mov       al,      1            ; Indicate successful completion.
        mov       bl,      LD           ; Get last dis. from prev. src. in bl.
        cmp       bl,      L0           ; See if this was last part on bus.
        jne       success               ; If not leave.
        inc       al                    ; Indicate this was last part.
        jmp       success               ; Leave.
fail:
        xor       al,      al           ; Set failure condition in ax.
success:   
        pop       cx                    ; Restore cx register.
        pop       bx                    ; Restore bx register.
        pop       si                    ; Restore si register.
        ret                             ; Return to caller.
onext   endp                            ; End of Rom Search procedure.       

;
access90 proc     near                  ; Procedure to look for DS1990.
        push      si                    ; Preserve si register.
        push      bx                    ; Preserve bx register.
        call      touchreset            ; Send TouchReset command.
        jnc       No90                  ; Abort if no Presence pulse.
        mov       al,     0FH           ; Send the Read
        call      touchbyte             ;    DS1990 command. 
        xor       si,     si            ; Zero si register. 
        xor       ah,     ah            ; Clear high byte of ah.
        push      ax                    ; Save ax.
loop90:
        mov       al,     0FFH          ; Read a byte
        call      touchbyte             ;    of ROM data.
        mov       romdta[si],al         ; Store byte in temporary buffer.
        pop       bx                    ; Restore crc value.
        mov       bl,     al            ; Get new byte in bl.
        xchg      bx,     ax            ; Get word for dowcrc in ax.
        call      dowcrc                ; Compute cummulative crc.
        push      ax                    ; Save current value of crc.
        inc       si                    ; Point to next storage location.
        cmp       si,     8             ; Have read read all of the Rom Data.
        jne       loop90                ; Read next byte if not finished.
        pop       bx                    ; Clean up stack.
        cmp       ah,     0             ; Is the crc good.
        je        scexit                ; Exit successful.
        cmp       ah,     53            ; Is the crc good.
        je        scexit                ; Exit successful.
        xor       al,     al            ; Return failure condition.
        jmp       No90                  ; Not a good crc, return failure con.
scexit:
        mov       al,     1             ; Set success condition.
No90:
        pop       bx                    ; Restore bx register.
        pop       si                    ; Restore si register.
        ret                             ; Return to caller.
access90 endp                           ; End of access90 procedure.
;
dowcrc  proc     near                   ; Procedure to calculate DOW crc.
        push     cx                     ; Save contents of cx register.
        mov      cx,     8              ; Prepare to calc. crc for 8 bytes.
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
