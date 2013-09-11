These utilities are used to download (thermodl) and 
mission (thermoms) a DS1921G Thermochron iButton.  The
DS1921Z/DS1921H are not supported with this application.

THERMODL:

usage: thermodl 1wire_net_name <output_filename> </Fahrenheit>
  - Thermochron download on the 1-Wire Net port
  - 1-wire_net_port required port name
    example: "COM1" (Win32 DS2480B),"/dev/cua0"
    (Linux DS2480B),"1" (Win32 TMEX)
  - <output_filename> optional output filename
  - </Fahrenheit> optional Fahrenheit mode (default Celsius)
  - version 1.03

THERMOMS:

usage: thermoms 1wire_net_name </Fahrenheit>
  - Thermochron configuration on the 1-Wire Net port
  - 1-wire_net_port required port name
    example: "COM1" (Win32 DS2480B),"/dev/cua0"
    (Linux DS2480B),"1" (Win32 TMEX)
  - </Fahrenheit> optional Fahrenheit mode (default Celsius)
  - version 1.03


The Thermochron has the following features:

   -Temperature range -40°C to 85°C (-40°F to 185°F) 
   -Selectable starting offset (0 minutes to 46 days) 
   -Sample interval (1 to 255 minutes) 
   -Alarms and capture registers triggered when outside specified range 
   -2048 time-stamped temperature readings with optional data wrap 
   -Long-term temperature histogram with 2°C resolution in 56 bins 
   -4096 bits of general-purpose read/write nonvolatile memory 

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

Application File(s):		'\apps\thermo'
thermodl.c   -	 this utility uses to download the results of the
                 current mission of a DS1921 Thermochron iButton
thermoms.c   -   mission/configure the DS1921 Thermochron iButton 
	     	

Common Module File(s):		'\common'
thermo21.c   - 	Thermochron iButton utility functions
thermo21.h   - 	include file for Thermochron demo
ownet.h    -   	include file for 1-Wire Net library
crcutil.c  -    keeps track of the CRC for 8 and 16 
                bit operations 
ioutil.c   - 	I/O utility functions


