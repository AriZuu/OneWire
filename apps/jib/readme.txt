Test applications for the Java-powered iButton (DS1955/DS1957).
The Java-powered iButton runs small Java applications.
See the subdirectory 'iButton' for development tools to create
these Java applications, examples, and documentation.  More
information on the Java-powered iButton and Java host 
development tools (iB-IDE) available on:
   http://www.ibutton.com/jibkit/index.html
   http://www.ibutton.com/ibuttons/java.html

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
jibtest.c   - example application to read and test the Jib and reader.
jibload.c   - example application to load a JiB application on a Jib.
jibmodpow.c - example application to exercise TestModExp JiB 
              application on Jib.

Common Module File(s):		'\source\common'
jib96.c    - main API module for the Jib
jib96o.c   - low level communication with Jib
jib96.h    - header file with API declarations and types
findtype.c - finds all of the devices of the same type 
             (family code) on a 1-Wire Net
ownet.h    - include file for 1-Wire Net library
crcutil.c  - keeps track of the CRC for 8 and 16 
             bit operations 
owerr.c    - error reporting functions
ioutil.c   - I/O utility functions

