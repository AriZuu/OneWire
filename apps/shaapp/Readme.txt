Test application for using the DS2432 and DS1961S on the 1-Wire
Net.  This application can write first secret and set all the
options of the part such as the following options:
   - Write protect secret
   - Write protect pages 0 - 3
   - Turn page 1 into EPROM mode of write once
   - Write protect page 0 only

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

Application File(s):		'\source\apps'
SHAapp.c   - the interface for accessing the different
             DS2432 and DS1961S functions.


Common Module File(s):		'\source\common'
crcutil.c   - Keeps track of the CRC for 16 and 8 bit
	           operations.
ibsha33.h   - Include file for the sha functions on the
	           DS2432.
ibsha33o.c  - Functions to do operations on the DS2432.
ioutil.c    - Miscellanous functions for I/O.
ownet.h     - main include




