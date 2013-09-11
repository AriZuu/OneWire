Public Domain 1-Wire Net Functions
Version 3.00
1/1/04
Copyright (C) 2004 Dallas Semiconductor

The 1-Wire Net functions provided are written in 'C' and are intended
to be used on platforms not supported by the TMEX 1-Wire Net
drivers. The '1-Wire Net' is a one wire and ground network with one
master and one or more slave devices.

This source code creates a 1-Wire Net master that can be used to identify
and communicate with slave devices. It provides all of the 1-Wire Net,
and some of the transport and file level services to communicate with all
of Dallas Semiconductor's 1-Wire devices including iButtons.

This source code is designed to be portable.  There are provided 'TODO'
templates to be completed for a specific platform. Several platform example
implementations have been provided. There are also several example
applications that use these platform implementations.

There are two sets of portable source files. The first set is general purpose
and is intended for platforms that already have the primitive linklevel
1-Wire Net communication functions. This is the lowest level that is hardware
dependent.

The other set of portable source files assumes that the user has a serial port
(RS232) and wishes to utilize our 'Universal Serial 1-Wire Line Driver Master
chip' called the DS2480B. This chip receives commands over the serial port,
performs 1-Wire Net operations and then sends the results back to the serial
port. The source code converts the intended 1-Wire operations into serial
communications packets to the DS2480B. The only thing that needs to be
provided for a platform are the serial port read/write primitives.

These two sets of portable source code files implement the same 1-Wire Net
functions and are interchangeable.

The files in the sub-directory '\lib\MICRO\' are written for example programs
using assembly language.

It is best to not mix library files from one platform build with another.
This could create an inefficient implementation.


CONTENTS:

*    lib (1-Wire Net library code sets)
   *    userial - Code set based on Universial Serial chip (DS2480B)
   *    general - Code set based on 1-Wire link-level functions
   *    micro - Formerly in the TMEX kit, written for example programs
                using assembly language

*    apps (application examples)
   *    atodtst - finds and display the value for channels A,B,C,D on the
                  DS2450 Quad A/D Converter
   *    counter - reads the DS2423 counter value of the associated memory page
   *    coupler - tests the DS2409 commands and searches for the DS2409 and the
                  devices on the branch of the DS2409
   *    fish - Read, write, format and display the file system on a 1-Wire
               memory device.
   *    gethumd - Measures the humidity using the DS2438.
   *    humalog - Sets up the DS1923 for mission or reads the current mission
                  log.  Also can set the passwords.
   *    jib - Loads and tests the java iButton.
   *    mweather - 1-Wire Weather Station example, supports versions of the
                   Weather Station
   *    memutil - Minimal 1-Wire application to list the devices present on
                  the default 1-Wire Adapter/Port.
   *    sa_ps - Uses a DS1991 for software authorization.
   *    sa_sha - Uses the DS1961S or the DS1963S for software authorization.
   *    sa_time - Uses the DS1994 for software authorization timeouts.
   *    shaapp - 1-Wire application for using DS1961 functions.
   *    shadebit - DS1963/DS1961 SHA iButton monetary demo.  Initializes the
                   the dedicated co-processor DS1963 and the user performs
                   authenticated secure money debits using DS1963/DS1961.
   *    swtloop - performs various operations on the Dual Addressable Switch
   *    swtsngl - turns the DS2405 switch on & off and reads the on/off status
   *    temp - finds and displays the temperature measurement for the
               DS1920/DS1820
   *    thermo - DS1921 Thermochron download and mission example application
   *    tstfind - loop to find all 1-Wire devices on Net

*    examples (example implementations on platforms)
   *    linux - Linux implementation using 'userial' code set
   *    win32 - Windows 32-Bit implementation using 'userial' code set
   *    tmexwrap - Windows 32-bit implementation using 'general' code set
                   TMEX 32-bit as the 1-Wire link-level functions.
   *    DS550 - Dallas Micro-controller code using the 'userial' and
                'general' code set.
   *    wince - Windows CE implementation using 'userial' code set.
   *    multiwvc - Windows 32-bit implementation using TMEX drivers to
                   communicate to multiple ports.  (COM, USB, Parallel)

*    Documentation
   *    ReadMe.txt - this file (Version 3.00, application descriptions,
                     platforms used, information, kit history)
   *    license.txt - 'public domain' source code license (based
                      Consortium's/MIT license
                      http://www.opensource.org/mit-license.html
   *    OWPD300.txt - Notes to the user, device listing, application
                      interface code
   *    \apps\(application-name)\ReadMe.txt - documentation throughout the
                           One Wire Domain kit with code explanation and
                           location of other files that may be required
                           for usage

INFORMATION:

The best source of information about 1-Wire
devices
including iButtons is Dallas Semiconductor's
iButton web site at:
http://www.ibutton.com/

This file has the latest download listings:
ftp://ftp.dalsemi.com/pub/auto_id/softdev/softdev.html

Dallas Semiconductor/Maxim's main web site:
http://www.maxim-ic.com/

Data sheets: http://dbserv.maxim-ic.com/pl_list.cfm?filter=1w

KIT HISTORY:
1.01 -> 1.02 Support multiple DS1820 temperature devices in example.
   *    Change MLANU(userial) section to implement
        opening and closing of communication port with
        generic functions.
   *    Fix incorrect declarations of global variables
        in MLANU(userial).
   *    Fix reading incorrect counter page in
        MWeather example.
   *    Extended DSO of MLANU(userial) adapter.

1.02 -> 1.03 Reorganization of files/directories
   *    Make applications not library dependent
        (userial/general)
   *    Add Linux support

1.03 -> 2.00 Reorganization of files/directories
   *    Addition of the Common directory
   *    Additional support added to the Lib file
        for multiple ports
   *    New applications and examples
   *    New Weather Station example to both
        versions of the Weather Station
   *    Change from MicroLAN (MLAN) to 1-Wire Net (OW)

2.00 -> 3.00 Reorganization of files/directories
   *    Added error handling code
   *    Added code for running applications on micros
   *    Added modules to support memory IO for all current
        1-Wire memory devices.
   *    Expanded the 1-Wire file I/O to implement all
        functions similar to the TMEX API.
   *    Updated the 1-Wire Link Layer functions to match
        up with App Note 187 and App Note 192.
   *    Added Software Authorization application examples
        along with Java iButton applications.
   *    Added code to handle the DS1923, DS1922L/T, DS2422 
        and DS1977 in memory bank applications
   *    Created a DS1923, DS1922L/T and DS2422 application 
        for starting missions and accessing the logged data.
   *    Added a multiple adapter build that allows access
        to COM, USB or Parallel ports.
   *    Added an owAcquireEx and OpenComEx for returning 
        the port number instead of passing it to owAquire
        and OpenCOM.  These new functions have replaced
        the old ones for this build.