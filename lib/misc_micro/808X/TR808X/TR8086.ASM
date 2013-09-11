;
;   The following 8086 code demonstrates reading and writing 92's, 93's, 94's
; using universal data packets. The string written to the touch memory is
; defined at offset tmessage at the end of this code.
;
; Program TchWrite                                            { 8/29/92 }
;
         extrn     touchreset: near;
         extrn     touchbyte: near;
         extrn     touchbit: near;
;
CR       EQU       13
LF       EQU       10
;
CODE     segment byte public 'CODE'
         assume    cs:CODE
;
         jmp       start
;
CRC16    dw        1        dup (0)         ; Define CRC16 variable.
PNUM     db        1        dup (0)         ; Page number variable.
CB       db        1        dup (0)         ; Command byte variable.
;
start:
         mov       ax,      cs              ; Get cs in ax.
         mov       ds,      ax              ; Get cs in ds.
         mov       dx,      offset promptem ; Get offset of message in dx.
         mov       ah,      9               ; Function code for print.
         int       21H                      ; Print the string.
main:
         mov       ah,      0BH             ; Function code to look for key.
         int       21H                      ; Look for a key press.
         cmp       al,      0FFH            ; Was a key pressed.
         je        term                     ; If so terminate.
         call      writetm                  ; Try to write data packet.
         or        al,      al              ; Was the write successful?
         jz        main                     ; If not try again.
         call      readtm                   ; Try to read contents.
         or        al,      al              ; Was the read successful?
         jnz       success                  ; If so tell tell the user
         jmp       main                     ;    and terminate. 
success:
         mov       dx,      offset stext    ; Get offset of message in dx.
         mov       ah,      9               ; Function code for print.
         int       21H                      ; Print the string.
term:
         mov       ah,      4CH             ; Function code for termination.
         int       21H                      ; Request termination from DOS.
;
writetm  proc      near
         mov       PNUM,    0               ; Starting page in secure memory.
         mov       CB,      0FH             ; Write scratchpad command.
         call      scrcomm                  ; Send 4 byte command to bus.
         or        al,      al              ; Was there a part out there.
         jnz       contin                   ; Continue if there was.
         jmp       exfail                   ; Abort if there wasn't.
contin:
         mov       ah,      0               ; Initialize cumulative CRC16
         mov       al,      PNUM            ;    variable with the page
         mov       CRC16,   ax              ;       number in the ls byte.
         mov       ax,      strlen          ; Get string length in ax.
         call      docrc16                  ; Calculate CRC16 of length byte.
         push      ax                       ; Push length byte on stack.
         push      cs                       ; Prepare for a far call.
         call      touchbyte                ; Send byte to 1-wire bus.
         mov       bx,      offset tmessage ; bx = pointer to 1st data byte.
         mov       cx,      31              ; 31 bytes left after length sent.
nextbyte:
         mov       al,      [bx]            ; Get data byte in al.
         cmp       al,      0               ; Check for null terminator.
         je        sendcrc                  ; Send CRC16 if finished.
         push      cx                       ; Save byte counter.
         push      bx                       ; Save offset of current data byte.
         call      docrc16                  ; Do cummulative CRC16 calc.
         push      ax                       ; Push data byte on stack.
         push      cs                       ; Prepare for far call.
         call      touchbyte                ; Send data byte to 1-wire bus.
         pop       bx                       ; Restore offset of data byte.
         pop       cx                       ; Restore byte counter.
         inc       bx                       ; Point to next data byte.
         loop      nextbyte                 ; Continue if scratchpad not full.
         mov       CB,     55H              ; Copy scratchpad command byte.
         call      scrcomm                  ; Copy the scratchpad to smem.
         inc       PNUM                     ; Indicate next page for copy.
         mov       CB,     0FH              ; Write scratchpad command.
         call      scrcomm                  ; Send write scratchpad command.
         mov       cx,     32               ; Reset byte counter.
         jmp       nextbyte                 ; Fill scratchpad again.
sendcrc:
         mov       ax,     CRC16            ; Get CRC value in ax.
         xor       ax,     0FFFFH           ; Get ones complement in ax.
         push      ax                       ; Save a copy on the stack.
         push      cx                       ; Save byte counter.
         push      ax                       ; Push copy to send to touchbyte.
         push      cs                       ; Prepare for far call.
         call      touchbyte                ; Send low byte of CRC16.
         pop       cx                       ; Restore byte counter.
         dec       cx                       ; Increment byte counter.
         cmp       cx,     0                ; Have we filled up the scratchpad.
         jne       sncrcb                   ; If not, send next CRC16 byte.
         mov       CB,     055H             ; Copy scratchpad command byte.
         call      scrcomm                  ; Copy the scratchpad.
         inc       PNUM                     ; Indicate next page to copy to.
         mov       CB,     0FH              ; Write scratchpad command byte.
         call      scrcomm                  ; Send write scratchpad command.
         mov       cx,     32               ; Set byte counter.
sncrcb:
         pop       ax                       ; Restore copy of CRC16.
         xchg      al,     ah               ; Get high byte in al.
         push      cx                       ; Save byte counter.
         push      ax                       ; Put it on stack for touchbyte.
         push      cs                       ; Prepare for far call.
         call      touchbyte                ; Send high byte of CRC16.
         pop       cx                       ; Restore byte counter.
         dec       cx                       ; Dec for last byte of CRC16.
         cmp       cx,     0                ; Is the scratchpad full yet?
         jz        dofincp                  ; Do the final copy scratch.
         mov       ax,     32               ; Move ascii space into ax.
morefill:
         push      cx                       ; Save loop counter.
         push      ax                       ; Push byte to send on stack.
         push      cs                       ; Prepare for far call.
         call      touchbyte                ; Send byte to 1-wire bus.
         pop       cx                       ; Restore loop counter.
         loop      morefill                 ; Continue until page filled.
dofincp:
         mov       CB,     055H             ; Copy scratchpad command byte.
         call      scrcomm                  ; Copy into secure memory.
         mov       al,     1                ; Indicate success.
exfail:
         ret                                ; Return to caller.
writetm  endp
;
readtm   proc      near
         mov       bx,     offset RSM       ; Get offset of read smem in bx.
         call      sendcomm                 ; Send command bytes.
         or        al,     al               ; Was there a part out there?
         jz        npd                      ; Abort if not.
         mov       CRC16,  0                ; Initialize CRC for page 0.
         mov       al,     0FFH             ; Prepare to read byte from bus.
         push      ax                       ; Send byte on stack.
         push      cs                       ; Prepare for a far call.
         call      touchbyte                ; Read byte of data from 1-wire. 
         call      docrc16                  ; Calculate cumm. CRC of same.
         cmp       al,     0FFH             ; See if this is extended length. 
         jne       doread                   ; Continue if it isn't.
         push      ax                       ; Send 0FFH on stack to read byte.
         push      cs                       ; Prepare for far call.
         call      touchbyte                ; Read byte of data from 1-wire. 
         call      docrc16                  ; Calculate cummulative CRC. 
         mov       cx,     0FFH             ; Get result from 1st length byte.
         xor       ah,     ah               ; Clear high byte of accumulator.
         add       cx,     ax               ; Add second length byte.
         add       cx,     2                ; Add two for CRC16 bytes.
         jmp       gotlngth                 ; Go read cx bytes of data.
doread:
         xor       ah,     ah               ; Clear high byte of accumulator.
         mov       cx,     ax               ; Get length byte in cx.
         add       cx,     2                ; Add two for CRC16 bytes.
gotlngth:
         mov       al,     0FFH             ; Prepare to read a byte of data.
         push      cx                       ; Save byte counter.
         push      ax                       ; Send 0FFH to touchbyte on stack.
         push      cs                       ; Prepare for a far call.
         call      touchbyte                ; Read byte of data from 1-wire.
         call      docrc16                  ; Calculate cummulative CRC16.
         pop       cx                       ; Restore byte counter.
         loop      gotlngth                 ; Continue until all bytes read.
         cmp       CRC16,  0B001H           ; Is the CRC16 good.
         jne       badcrc                   ; Set failure condition if not.
         mov       al,     1                ; Set success condition.
         jmp       npd                      ; Exit procedure.
badcrc:
         xor       al,     al               ; Indicate failure.
npd:
         ret                                ; Return to caller. 
readtm   endp
;
;   This procedure sends a 4 byte command to the 1-wire bus. the offset of
; the beginning of the command is passed in bx.
;  
;              Note: Original values of ax, dx are destroyed. 
;
sendcomm proc      near
         push      bx                        ; Save bx register.
         push      cx                        ; Save cx register.
         push      bx                        ; Save our offset to command. 
         mov       ax,     1                 ; Get port number in ax. 
         push      ax                        ; Send it on stack.
         push      cs                        ; Prepare for far call.
         call      touchreset                ; Send reset pulse to 1-wire bus.
         pop       bx                        ; Restore offset to command.
         or        al,     al                ; Test for presence pulse.
         jz        np1                       ; If none detected, abort.
         mov       cx,     4                 ; Prepare to send 4 bytes to bus.
nextcb:
         mov       ax,     [bx]              ; Get command byte in ax.
         push      bx                        ; Save pointer to command bytes.
         push      cx                        ; Save counter.
         push      ax                        ; Push byte to send on stack.
         push      cs                        ; Prepare for far call.
         call      touchbyte                 ; Send byte to 1-wire bus.
         pop       cx                        ; Restore counter.
         pop       bx                        ; Restore pointer.
         inc       bx                        ; Point to next byte.
         loop      nextcb                    ; Send all 4 command bytes.
         mov       al,     1                 ; Set success condition.
np1:
         pop       cx                        ; Restore cx register.
         pop       bx                        ; Restore bx register.
         ret                                 ; Return to caller.      
sendcomm endp
;
;   This procedure sends the copy scratchpad or write scratchpad command to 
; the 1-wire bus depending on the value passed in CB. For a copy scratchpad
; it assumes that the scratchpad has been filled so that the ending transfer
; address is 31 decimal.
;
scrcomm  proc      near
         push      bx                        ; Save bx register.
         push      cx                        ; Save cx register.
         mov       ax,     1                 ; Get port number in ax. 
         push      ax                        ; Send it on stack.
         push      cs                        ; Prepare for far call.
         call      touchreset                ; Send reset pulse to 1-wire bus.
         or        al,     al                ; Test for presence pulse.
         jz        np2                       ; If none detected, abort. 
         mov       ax,    0CCH               ; Get skip through ROM comm in ax.

         push      ax                        ; Send byte on stack.
         push      cs                        ; Prepare for far call.
         call      touchbyte                 ; Send byte to 1-wire bus.
         mov       al,    CB                 ; Get copy scratchpad comm. in ax.

         push      ax                        ; Send byte on stack.
         push      cs                        ; Prepare for far call.
         call      touchbyte                 ; Send byte to 1-wire bus.
         mov       al,    PNUM               ; Get page number in al.
         mov       cl,    5                  ; Prepare to rotate left by 5.
         shl       al,    cl                 ; Get low byte starting address.
         push      ax                        ;    in ax and send it on stack.
         push      cs                        ; Prepare for far call.
         call      touchbyte                 ; Send byte to 1-wire bus.
         mov       al,    PNUM               ; Get page number in al.
         mov       cl,    3                  ; Setup to rotate al right by 3.
         shr       al,    cl                 ; Get high byte starting address.
         push      ax                        ;    in ax and send it on stack.
         push      cs                        ; Prepare for far call.
         call      touchbyte                 ; Send byte to 1-wire bus.
         cmp       CB,    0FH                ; If this is a write scratchpad
         je        noea                      ;    command don't send end addr.
         mov       al,    31                 ; Get last byte of scratch in
         push      ax                        ;    al and send it on stack.
         push      cs                        ; Prepare for far call.
         call      touchbyte                 ; Send byte to 1-wire bus.
noea:
         mov       al,    1                  ; Indicate success.
np2:
         pop       cx                        ; Restore cx register.
         pop       bx                        ; Restore bx register.
         ret                                 ; Return to caller.
scrcomm  endp
;
docrc16  proc      near
         push      ax                        ; Save byte passed in ax.
         push      bx                        ; Save bx register.
         push      cx                        ; Save cx register.
         xor       bx,     bx
         mov       ah,     bh
         xor       ax,     CRC16
         xchg      bl,     ah
         and       al,     al
         jpe       skpxor
         xor       bx,     0C001H
skpxor:  
         mov       cl,     6
         rol       ax,     cl
         xor       bx,     ax
         rol       ax,     1
         xor       ax,     bx
         mov       CRC16,  ax
         pop       cx                       ; Restore cx register.
         pop       bx                       ; Restore bx register.
         pop       ax                       ; Restore byte passed in ax.
         ret                                ; Return to caller.
docrc16  endp
;
RSM      db 0CCH, 0F0H, 0, 0                ; Read secure memory command bytes.
; 
stext    db 'TOUCH MEMORY WRITE COMPLETE', CR, LF, '$'
tmessage db 'THIS IS A TEST OF THE EMERGENCY BROADCAST SYSTEM', 0, '$'
strlen   equ $ - tmessage - 2
promptem db 'PLEASE TOUCH A TOUCH MEMORY, PRESS ANY KEY TO ABORT', CR, LF, '$'
;
CODE     ends
;
         end

