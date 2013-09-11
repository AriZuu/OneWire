There are two sets of portable source files.
The first set is general purpose and is intended
for platforms that already have the primitive
link-level 1-Wire Net communication functions.
This is the lowest level that is hardware dependent.

The other set of portable source files assumes that
the user has a serial port (RS232) and wishes to
utilize our 'Universal Serial 1-Wire Line Driver
Master chip' called the DS2480B.  This chip receives
commands over the serial port, performs 1-Wire Net
operations and then sends the results back to the
serial port.  The source code converts the intended
1-Wire operations into serial communications packets to
the DS2480B.  The only thing that needs to be provided
for a platform are the serial port read/write primitives.

These two sets of portable source code files implement
the same 1-Wire Net functions and are interchangeable.
* It is best to not mix library files from one platform
  build with another.  This could create an inefficient
  implementation.

The files in the sub-directory 'miscMICRO\' are written for
example programs using assembly language.
It includes Functions of the Link Layer: Reset/Presence Detect,
Byte I/O, Bit I/O. This software runs on Hitachi 6301, 6303, 808X
as well as on Motorola 68HC11 and Intel 8051.

In the other directory is the link files for the userial, USB
and Parallel ports.  This directory is for the multi build,
which can access multiple ports.


CONTENTS:

source
   lib (1-Wire Net library code sets)*

      userial  - Code set based on Universial Serial chip (DS2480B)

	\Link

           \DOS (DOS link file)
            udoslnk

           \DS80C390 (DS80C390 link file)
            ds390lnk

           \DS80C400 (DS80C400 link files)
            ds400lnk
            ser400

           \DS87C520 (DS87C520 link files)
            ds520lnk

           \DS87C550 (DS87C550 link files)
            ds550lnk
            ser550

           \Linux (Linux link file)
            linuxlnk

           \Mac (Mac link file)
            maclnk

           \Palm (Palm link file)
            palmlnk

           \PocketPC (WinCE port link files)
            WinCElnk

           \Win16 (Win16 port link file)
            uwin16lk

           \Win32 (Win32 port link file
            win32lnk

      general  - Code set based on 1-Wire link-level functions

	\Link
           \DS80C400 (DS80C400 link files)
            owm
            owmlnk
            owmses

           \DS87C520 (DS87C520 link files)
            ds520lnk
            ds520ses

           \DS87C550 (DS87C550 link files)
            ds550lnk
            ds550ses

           \LPT_Win32 (Parallel port link files)
            llpwin32
            pseswin32
            sacwd32
            utdll
            ds1410d.zip
            vsauthd.zip

           \TMEX_Win32 (TMEX link files)
            tmexlnk
            tmexses

           \Visor (Visor link files)
            visowll
            visowses


      MiscMICRO (assembly language)

      	\6303
        	\LL6303    - Reset/Presence Detect, Byte I/O, Bit I/O
       	\8051
       		\LL8051    - Reset/Presence Detect, Byte I/O, Bit I/O
      	        \NW8051    - Multi-drop iButtons
	        \TR8051    - Multi-drop iButtons
         	\CRC8051   - Multi-drop iButtons

      	\808x
               \LL808x     - Reset/Presence Detect, Byte I/O, Bit I/O
	       \TR808x     - Multi-drop iButtons
	       \NW808x     - Multi-drop iButtons
               \CRC808x    - Multi-drop iButtons

        \ATMEL
               onewire.asm - sample code for search the net.
        \PIC
               ibw26.asm   - sample assembly code for PIC

      Other (USB, TMEX and Multiple adapter library files)

        \Multi_Win32 (Multiple port link files)
         mownet
         multilnk
         multinet
         multises
         multitran

        \TMEXWrap_Win32 (TMEX port link files)
         iffs32.lib
         tmexlnk
         tmexnet
         tmexses
         tmextran

        \USB_Win32 (USB port link files)
         ds2490
         ds2490ut
         usbwlnk
         usbwnet
         usbwses
         usbwtran
         
