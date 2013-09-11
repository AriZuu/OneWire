Test application to set and stop mission for recording
temperature and humidity or voltage for the DS1923.  The
application will also set the passwords of the device.  It
also will read the current data if a mission is not in
progress.  The program can use temperature or humidity
calibration.  The values just have to be set using the
configLog structure.  The application will also work 
for the DS1922L/DS1922T.

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
humalog.c   - the application program for reading the humidity


Common Module File(s):		'\common'
crcutil.c   - used to calculate the crc of the data
findtype.c  - find the type of part
humutil.c   - functions to help with interfacing with the DS1923
ioutil.c    - I/O utility functions
mbee77.c    - memory bank for the DS1923 and DS1977
mbscrx77.c  - memory bank for the scratchpad of the DS1923 and DS1977
owerr.c     - error untility and list
pw77.c      - password functions for the DS1923 and DS1977
ownet.h     - main header file

