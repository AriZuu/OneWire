Test application to read the 1-Wire humidity
sensor, DS2438 plus Honeywell sensor type.  An
application note on the DS2438 can be found here:
http://www.maxim-ic.com/appnotes.cfm/appnote_number/403/ln/en

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
getHumd.c   - the application program for reading the humidity 


Common Module File(s):		'\common'
ad26.c      - the function interface to get humidity data
ad26.h      - header file
crcutil.c   - used to calculate the crc of the data
findtype.c  - find the type of part
ioutil.c    -	 I/O utility functions
ownet.h     - main header file
screenio.c  - for different platform screen input and output

