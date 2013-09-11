;
;         Procedures for communicating with Touch Memory using the
;         DS1206 phantom serial interface. This code is designed
;         to run in an Intel 8088 microprocessor with a 4.77 MHz
;         system clock.
;
code      segment   para
          assume    cs:code
          public    TouchReset, TouchByte
;
X         equ       +6[bp]              ; Argument of function.
;
Hi        equ       es:[6]
Lo        equ       es:[14]
Rst       equ       es:[0]
;
SegAddr   equ       cs:0[si]
;
;         The value of Count should be the nearest number of
;         LOOP instruction executions required to produce a
;         delay of 60 us. In an 8088 with a 4.77 MHz clock,
;         this value is 18. [A loop instruction with branch
;         takes 17 cycles and without the branch takes 5
;         cycles, so the total time to count down from 18 is
;         (17 * 17 cycles + 5 cycles) / 4.77 MHz = 61.6 us.]
;         This value can be adjusted as needed to produce the
;         correct timing with other processors or clock rates.
;
Count     equ       18                  ; Selected for 4.77 MHz 8088.
;
;         Function TouchReset(X: Byte): Boolean;
;
;         This function delivers a reset signal through the
;         DS1206 connected to the bus, with X as the most
;         significant byte of the segment address. During
;         the high part of the reset signal, the program watches
;         for a presence detect pulse. The function returns a
;         boolean True value (01) in AL if a presence pulse was
;         detected, and a boolean False value (00) otherwise.
;         The loop count required to produce a 60 microsecond
;         delay is stored in the variable Count for subsequent
;         use by the function ByteIO. The value of the segment
;         address is also stored for later use in the variable
;         SegAddr.
;
TouchReset proc     far
;
          push      bp                  ; Save the BP register.
          mov       bp,       sp        ; Save stack pointer in BP.
          call      locate              ; Find the local typed consts.
                                        ; si now points to local consts.
          mov       cx,       Count     ; Get the time calibration.
;
          mov       dh,       X         ; Get the MS segment byte.
          xor       dl,       dl        ; Zero the LS byte.
          mov       es,       dx        ; ES is segment addr of DS1250.
          shl       cx,       1         ; Scale time
          shl       cx,       1         ;    slot up
          mov       bx,       cx        ;       to 8T.
          shl       cx,       1         ;
          pushf                         ; Save the state of interrupts.
          cli                           ; Inhibit all interrupts.
          call      DS1250_Protocol     ; Send the protocol of the DS1206.
          mov       al,Lo               ; Clear IO line
LA10:
          loop      LA10                ; Wait for time interval.
          mov       cx,       bx        ; Get 8T time slot count.
          shl       cx,       1         ; Scale up to 64T
          shl       cx,       1         ;    to skip over any
          shl       cx,       1         ;       alarm interrupt.
          mov       al,Hi               ; Set IO line
LA2:
          mov       al,Hi               ; Set IO line to read
          test      al,       01H       ; Check D0.
          jnz       LA13                ; Branch if it goes high.
          loop      LA2                 ; Wait while it's still low.
          jmp       short     LA7       ; Abort if it never goes high.
LA13:
          mov       cx,       bx        ; Get 8T time slot count.
LA3:
          mov       al,Hi               ; Set IO line to read
          test      al,       01H       ; Check D0.
          jz        LA4                 ; Branch if it goes low.
          loop      LA3                 ; Wait while it's still high.
          jmp       short     LA7       ; Abort if it never goes low.
LA4:
          mov       al,Hi               ; Set IO line to read
          test      al,       01H       ; Check D0.
          jnz       LA5                 ; Branch if it goes high.
          loop      LA4                 ; Wait while it's still low.
          jmp       short     LA7       ; Abort if it never goes high.
LA5:
          loop      LA5                 ; Wait until reset complete.
          mov       bl,       1         ; Set True result.
          jmp       short     LA14      ; Prepare to return.
LA7:
          xor       bl,       bl        ; Set False result.
LA14:
          mov       al,Rst
          pop       cx                  ; Get previous interrupt state.
          test      ch,       2         ; Check if interrupts enabled.
          jz        LA12                ; Branch if they weren't.
          sti                           ; Enable if they were.
LA12:
          mov       al,       bl        ; Get result into accumulator.
          xor       ah,       ah        ; Zero the MS byte.
LA8:
          mov       SegAddr,  es        ; Store the segment address.
          mov       sp,       bp        ; Restore the stack pointer.
          pop       bp                  ; Restore the BP register.
          ret       2                   ; Pop one parameter and return.
;
TouchReset     endp
;
Locate    proc      near
;
;         This procedure returns with the code segment offset of
;         the local typed constant SegAddr in the SI register.
;
          call      LX                  ; Push SegAddr offset on Stack.
          dw        0                   ; Local typed constant SegAddr.
LX:
          pop       si                  ; Get CS offset of SegAddr in SI.
          ret                           ; Return.
;
Locate    endp
;
;         Function TouchByte(X: Byte): Byte;
;
;         This function transmits the byte X to the DS1494
;         through the DS1206, and returns the byte received in AL.
;         The timing of the transmission is determined by Count.
;
TouchByte proc      far
;
          push      bp                  ; Save the BP register.
          mov       bp,       sp        ; Save stack pointer in BP.
          sub       sp,       2         ; Make space for local variable.
          call      locate              ; Find the local typed consts.
          mov       cx,       Count     ; Get the time calibration.
          mov       es,       SegAddr   ; Get the segment address.
          mov       al,       X         ; Fail return value.
          mov       bl,       al        ; Put byte to send in BL.
          mov       ah,       8         ; Set the bit counter.
LB1:
          pushf                         ; Save the state of interrupts.
          cli                           ; Inhibit all interrupts.
          call      DS1250_Protocol     ; Send protocol to the DS1250
          mov       cx,       Count     ; Compute and
          shr       cx,       1         ;    set the data
          shr       cx,       1         ;       stabilization
          shr       cx,       1         ;          period.
          inc       cx                  ; Round up to nearest integer.
          shr       bl,       1         ; Get output bit in carry.
          jnc       LD4                 ; Branch if bit is a zero.
          mov       al,Lo               ; Set IO line
          mov       al,Hi               ; Set IO line
LD2:
          loop      LD2                 ; Wait for data to stabilize.
          mov       al,Hi               ; Set IO line
          test      al,       01H       ; Check D0.
          jz        LD6                 ; Branch if it's zero.
          or        bl,       80H       ; Store a one bit.
          jmp       short     LD6       ; Go finish up.
LD4:
          mov       al,Lo               ; Set IO line
          add       cx,       Count     ; Set up time period.
LD5:
          loop      LD5                 ; Wait out time slot.
          mov       al,Hi               ; Set IO line
LD6:
          mov       al,Rst
          pop       cx                  ; Get previous interrupt state.
          test      ch,       2         ; Check if interrupts enabled.
          jz        LD8                 ; Branch if they weren't.
          sti                           ; Enable if they were.
LD8:
          sub       ah,       1         ; Decrement bit counter.
          jnz       LB1                 ; Repeat until all bits sent.
          mov       al,       bl        ; Get input byte in accumulator.
LB2:
          xor       ah,       ah        ; Zero MS byte.
          mov       sp,       bp        ; Restore the stack pointer.
          pop       bp                  ; Restore the BP register.
          ret       2                   ; Pop one parameter and return.
;
TouchByte endp
;
DS1250_Protocol     proc      near
          push      ds                 ; Save segment register.
          push      es
          push      si
          push      ax
          push      bx
          push      cx
          Mov       al,Rst
          mov       si,      offset Protocol_Pat
          mov       bx,      0         ; Setup outer loop.
Next_Pattern_Byte:
          mov       al,      cs:[si+bx]; Load with proto. pat. byte.
          mov       cx,      8
Next_Protocol:
          xor       dh,dh
          mov       dl,al
          and       dl,1
          or        dl,2
          mov       di,dx
          mov       ah,[es:di]
          or        dl,4
          mov       di,dx
          mov       ah,[es:di]
          shr       al,1
          loop      Next_Protocol
          inc       bx
          cmp       bx,      3
          jne       Next_Pattern_Byte   ; Read all three protocol bytes.
          pop       cx
          pop       bx
          pop       ax
          pop       si
          pop       es
          pop       ds                 ; Restore segment register.
          ret                          ; Return.
;
Protocol_Pat        db   062h,001h,0E0h
;
DS1250_Protocol     endp
;
code      ends
;
          end


