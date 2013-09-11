Test application to read the DS2409 
1-Wire Net Coupler 

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

Application File(s):		'\apps'
coupler.c    -	application that tests the DS2409 commands 
		and searches for the DS2409 and the devices
		on the branch of the DS2409

Common Module File(s):		'\common'
swt1f.c     - 	module(s) to set and display switch status 
		and find the branches on the DS2409 device 
findtype.c  - 	finds all of the devices of the same type 
		(family code) on a 1-Wire Net
ownet.h    -   	include file for 1-Wire Net library
crcutil.c  -    keeps track of the CRC for 8 and 16 
                bit operations 


