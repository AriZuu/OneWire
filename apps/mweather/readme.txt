Test application to read the 1-Wire Weather Station.
The weather station consists of the following devices
on the trunk of the 1-Wire Net:
   DS1820 - temperature measurement
   DS2423 - counter for reading wind speed (page 15)
   
   DS2406 - switch to isolate 8 DS2401's for wind 
            direction on channel B.
     or 
   DS2450 - wind direction value for channels 
	    A, B, C, D on the 1-Wire Net DS2450 
	    Quad A/D Converter  

The 'ini.txt' file is used for initialization of the
application.  If the file is not available AND the
1-Wire Weather station is the 'DS2450 type' then 
an 'ini.txt' file will automatically be generated.

The ini.txt file consists of the 1-Wire net numbers 
of all the devices used in the Weather Station.  It is 
one column starting with all the DS2401s and DS2406 
or the DS2450, the DS1820 and the DS2423.  If the 
DS2406/DS2401 type wind direction sensor is used then
the DS2401 will be listed in order. Also the 'North'
disignator will be at the bottom fo the file.  By
default the first DS2401 is north.

You can find information article about the weather station here:
http://www.ibutton.com/weather/
and order them from here:
http://www.aag.com.mx/indexaag.html

Required on the command line is the 1-Wire port name:

example:  "COM1"                (Win32 DS2480B)
          "/dev/cua0"           (Linux DS2480B)
          "1"                   (Win32 TMEX)
          "\\.\DS2490-1"        (Win32 USB DS2490)
          "{1,5}"               (Win32 DS2480B multi build)
          "{1,6}"               (Win32 USB DS2490 multi build)
          "{1,2}"               (Win32 DS1410E multi build)

This application uses the 1-Wire Public Domain API. 
Implementations of this API can be found in the '\lib' folder.
The libraries are divided into three categories: 'general', 
'userial' and 'other'. Under each category are sample platform
link files. The 'general' and 'userial' libraries have 'todo' 
templates for creating new platform specific implementations.  

This application also uses utility and header files found
in the '\common' folder. 

Application File(s):			'\apps'
mweather.c  - 	reads a 1-Wire Weather Station with DS2406 
		and/or DS2450

Common Module File(s):			'\common'
atod20.c   -    module to do scale set up, conversion, 
                read, and write for channels A,B,C,D 
                of the DS2450 1-Wire Quad A/D Converter 
cnt1d.c    -    module to read the DS2423 1-kbit/4-kbit 
                1-Wire RAM with Counter associated 
                to the memory page
crcutil.c  -    keeps track of the CRC for 8 and 16 
                bit operations 
findtype.c - 	 finds all of the devices of the same type 
               (family code) on a 1-Wire Net
owerr.c    -    Error handling routines.  Provides exception
                stack and printError functions.
ownet.h    -    include file for 1-Wire Net library
ioutil.c   - 	I/O utility functions
swt12.c    -    performs operations on a DS2406 Dual 
                Addressable Switch and tests the different 
                channels
swt12.h    -    include this file to test the DS2406 Dual 
		          Addressable Switch
temp10.c   -    module to read the temperature measurement 
		          for the DS1920/DS1820/DS18S20 1-Wire 
		          thermometer
weather.c  -    to find the type of weather station, modules to 
                find the direction of the weather station, 
                and read/write output files
weather.h   -   include this file for the Weather Station struct
