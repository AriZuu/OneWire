The onewire.asm program runs through a loop looking for 1-Wire
devices through Port E.  The first byte of the serial number
will be displayed on the LEDs and by heading the switches
it will display the rest of the serial number bytes.  When
it is done with one 1-Wire device it will search for it and
display the serial number using the LEDs.  Once the search
is done the LEDs with flash from 0xAA to 0x55 and then start
the search over.  The serial number is also sent through the
serial port.