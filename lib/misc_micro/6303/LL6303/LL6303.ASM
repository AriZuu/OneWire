*
*         Touch Memory I/O Procedures for the Hitachi
*         6301 and 6303 or the Motorola 68CH11
*
*         Version 1.06  SMC  August 28, 1992
*
*         These procedures perform the Reset/Presence function
*         and the data I/O function for a Touch Memory connected
*         to Bit 6 of data I/O port 6. This port pin must have
*         a an external 4.7K pullup resistor to Vcc to permit it
*         to operate correctly as a one-wire port. ESD protection
*         diodes are also desirable, as described in Chapter 8.
*
*         The procedures are designed to be used in a system
*         with a 3.6864 MHz crystal. Code timing adjustments
*         will be required with other crystals to conform to
*         the timing requirements of the one-wire bus.
*
P6DDR     EQU       $16       *       Port six data direction.
P6        EQU       $17       *       Port six I/O address.
*
*         The TOUCHRESET procedure issues the reset signal on the
*         one-wire bus and detects the presence of any one-wire
*         device on the line. On return, X is zero if no
*         presence pulse was detected, one if a presence
*         pulse but no alarm pending pulse was detected,
*         two if an alarm pending pulse was detected, and
*         three if a short circuit was detected (lasting
*         longer than the maximum duration of an alarm
*         pending pulse followed by a presence pulse).
*         The values in accumulators D and X are altered.
*
TOUCHRESET
          CLRA                *       Make sure the
          STAA      P6        *          output is low.
          LDAA      #$40      *       Pull the data
          STAA      P6DDR     *          line low.
          LDAB      #109      *   2.  (480 us / T - 6) / 4 = 109.
TR1
          DECB                * 109.  Wait while holding
          BNE       TR1       * 327.     data line low.
          CLRA                *   1.  Let the data
          STAA      P6DDR     *   3.     line pull high.
          LDX       #0        *   3.  Assume False result.
          LDAB      #36       *   2.  (480 us / T - 5) / 12 = 36.
TR2
          LDAA      P6        * 108.  Read the data input line.
          BITA      #$40      *  72.  Check for high before presence.
          BNE       TR3       * 108.  Branch if high detected.
          DECB                *  36.  Wait for high
          BNE       TR2       * 108.     before presence.
          BRA       ALARM     *   3.  Alarm if no initial high.
TR3
          LDAA      P6        *       Read the data input line.
          BITA      #$40      *       Check for low presence pulse.
          BEQ       TR4       *       Branch if low detected.
          DECB                *       Wait for the
          BNE       TR3       *          presence pulse
          BRA       FALSE     *       False if no presence pulse.
TR4
          LDAA      P6        *       Read the data input line.
          BITA      #$40      *       Check for high after presence.
          BNE       TR5       *       Branch if high detected.
          DECB                *       Wait for high
          BNE       TR4       *          after presence.
          BRA       FALSE     *       False if no final high.
TR5
          ASLB                *       Adjust count for short loop.
TR6
          NOP                 *       Pad loop out
          NOP                 *          to six cycles.
          DECB                *       Wait out rest of
          BNE       TR6       *          reset sequence.
          BRA       TRUE      *       Return.
ALARM
          LDX       #2        *    3. Indicate alarm.
          LDAA      #4        *    2. Set up 3099
          LDAB      #1        *    2.    cycle delay.
TR7
          DECB                *  769. Wait
          BNE       TR7       * 2307.    out
          DECA                *    4.       the
          BNE       TR7       *   12.          delay.
          LDAA      P6        *       Check if data line
          BITA      #$40      *          is still held low.
          BNE       FALSE     *       Alarm and presence good.
TRUE
          INX                 *       Indicate good or short.
FALSE
          RTS                 *       Return.
*
*         The TOUCHBYTE procedure transfers a byte of data between
*         the accumulator D and the one-wire device. To send a
*         byte of data to the one-wire bus, place the byte in
*         accumulator D and call TOUCHBYTE. To receive a byte of
*         data from the one-wire bus into accumulator X, place
*         $FF in accumulator D, call TOUCHBYTE, and the value from
*         the one-wire device is returned in accumulator X. The
*         values in accumulators D and X are altered.
*
TOUCHBYTE
          COMB                *       Complement output.
          RORB                *       Position the
          RORB                *          LSB at bit
          RORB                *             number 6.
          LDAA      #8        *       Set the bit count.
BIT_LOOP
          PSHA                *   4.  Save the bit count.
          LDAA      #$40      *   2.  Setup to start time slot.
          STAA      P6DDR     *   3.  Pull data line low.
          TBA                 *   1.  Isolate output
          ANDA      #$40      *   2.     data bit.
          STAA      P6DDR     *   3.  Output the data bit.
          NOP                 *   1.  Delay to give
          NOP                 *   1.     one-wire bus
          NOP                 *   1.        time to go high.
          LDAA      P6        *   3.  Get the input bit.
          ANDB      #$BF      *   2.  Prepare bit position.
          BITA      #$40      *   2.  Check the input bit.
          BEQ       ZERO      *   3.  Place input bit in
          ORAB      #$40      *   2.     the A register.
ZERO
          RORB                *   1.  Get next bit in position.
          LDAA      #8        *   2.  (60 us / T - 26) / 4 = 8.
BIT_WAIT
          DECA                *   8.  Wait out rest of
          BNE       BIT_WAIT  *  24.     the time slot.
          STAA      P6DDR     *   3.  Let data line go high.
          PULA                *   3.  Restore the bit count.
          DECA                *   1.  Decrement the bit count.
          BNE       BIT_LOOP  *   3.  Go for the next bit.
          ROLB                *       Reposition
          ROLB                *          the bits.
          XGDX                *       Move result to X.
          RTS                 *       Return.


