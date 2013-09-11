The modules in the 'common' directory 'source\common'
Are device specific and can be used to create new
applications other than the example applications
provided in 'source\apps'.


Contents:

atod26.c    -   functions to read the voltage of the
                DS2438
atod26.h    -   include file for humidity sensor
                (DS2438)
atod20.c    -   module to do scale set up, conversion,
                read, and write for channels A,B,C,D
                of the DS2450 1-Wire Quad A/D Converter
atod20.h    -   header file for the DS2450 functions.
cnt1d.c     -   module to read the DS2423 1-kbit/4-kbit
                1-Wire RAM with Counter associated
                to the memory page
cnt1d.h     -   header file for the DS2423 functions.
crcutil.c   -   keeps track of the CRC for 8 and 16
                bit operations
findtype.c  -   finds all of the devices of the same type
                (family code) on a 1-Wire Net
findtype.h  -   header file.
humutil.c   -   DS1923 humidity logger utility functions
                for setting missions and reading current missions.
humutil.h   -   include file for the DS1923 utility functions.
ibsha33o.c  -   functions to do operations on the DS2432.
ibsha33.h   -   include file for the sha functions on the
                DS2432.
ioutil.c    -	I/O utility functions
jib96.c     -   API for executing methods on the iButton
                for Java and for executing the iButton
                Command Processor.
jib96.h     -   header file.
jib96o.c    -   Low-level Java-Powered iButton Communication.
mbappreg.c  -   memory bank functions for the application register
                iButtons
mbappreg.h  -   header file
mbee.c      -   memory bank functions for the electically erasable
                programmable read only memory
mbee.h      -   header file
mbee77.c    -   memory bank functions for the DS1923 and DS1977.
mbee77.h    -   header file
mbeprom.c   -   memory bank functions for the electrically
                programmable read only memory
mbeprom.h   -   header file
mbnv.c      -   memory bank functions for the read/write nonvolatile
                memory
mbnv.h      -   header file
mbnvcrc.c   -   memory bank functions for the read/write nonvolatile
                memory with crc
mbnvcrc.h   -   header file
mbscr.c     -   memory bank functions for the scratchpad
mbscr.h     -   header file
mbscrcrc.c  -   memory bank functions for the scratchpad with crc
mbscrcrc.h  -   header file
mbscree.c   -   memory bank functions for the scratchpad ee
mbscree.h   -   header file
mbscrex.c   -   memory bank functions for the scratchpad ex
mbscrex.h   -   header file
mbscrx77.c  -   memory bank functions for the scratchpad of the
                DS1923 and DS1977.
mbscrx77.h  -   header file
mbsha.c     -   memory bank functions for the sha parts
mbsha.h     -   header file
mbshaee.c   -   memory bank functions for the shaee parts
mbshaee.h   -   header file
owcache.c   -   cache functions for file I/O
owerr.c     -   Error handling routines.  Provides exception
                stack and printError functions.
owfile.c    -   rudimentary level functions for reading
		and writing TMEX files on NVRAM iButton
		using the packet level functions
owfile.h    -   header file
ownet.h     -   include file for 1-Wire Net library
owpgrw.c    -   page read/write functions for file I/O
owprgm.c    -   the program job functions for the file I/O
                using EPROM
ps02.c      -   Functions for communicating to the DS1991
ps02.h      -   Header file.
pw77.c      -   Password functions for the DS1923 and DS1977.
pw77.h      -   Header file.
rawmem.c    -   the utility that sorts memory bank functions
rawmem.h    -   header file
screenio.c  -   for different platform screen input and output
sha18.c     -   Low-level memory and SHA functions for the
                DS1963S.
sha33.c     -   Low-level memory and SHA functions for the
                DS1961S.
shadbtvm.c  -   Transaction-level functions for SHA Debits
                without a hardware coprocessor.  Also, this
                file contains some Protocol-level functions
                to replace their 'hardware-only' couterparts.
shadebit.c  -   Transaction-level functions for SHA Debits
                with hardware.
shaib.c     -   Protocol-level functions as well as useful
                utility functions for sha applications.
shaib.h     -   Header file.
sprintf.c   -   remake of I/O for Visor and Palm apps.
swt05.c     -   turns the DS2405 Addressable Switch on
                and off and reads the on/off status of
                the device
swt05.h     -   Header file.
swt12.c     -   performs operations on a DS2407 Dual
                Addressable Switch and tests the different
                channels
swt12.h     -   include this file to test the DS2407 Dual
		Addressable Switch
sw1f.c      -   tests the DS2409 commands and searches for
		the DS2409 and the devices on the branch of
		the DS2409, also reports the activity latches
swt1f.h     -   Header file.
temp10.c    -   Module to read the temperature measurement
		for the DS1920/DS1820/DS18S20 1-Wire
		thermometer.
temp10.h    -   Header file.
thermo21.c  -	Thermocron iButton utility functions
thermo21.h  - 	include this file for Thermocron demo
time04.c    -   1-Wire Clock device functions for (DS2404,
                DS1994, DS1427).
time04.h    -   Header file.
weather.c   -	to find the type of weather station, modules to
		find the direction of the weather station,
		and read/write output files
weather.h   - 	Include this file for the Weather Station struct
    