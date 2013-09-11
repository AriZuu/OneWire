The DS1991 iButtons allow you to program your own 
passwords.  Independent passwords can be set for each
of the three RAM pages.  Circuitry inside the iButton
will match future passwords with this register.  Only
you then hold the key to opening the iButton, for
only you know the password.  

False passwords written to the DS1991 will 
automatically invoke a random number generator
(contained in the iButton) that replies with false
responses.  This eliminates attempts to break security
by patter association.  Conventional protection devices
do not support this feature.

The DS1991 Multi-key iButton contains three separate,
independently password protected memory storage areas.
Each password is 8 bytes.  For advanced security 
applications, the three memory areas can all be used
to protect one application. If information critical to
operating the application is stored in page three (and
password protected), its password could be stored in 
page two. In like fashion, storage area two's password
could be stored in storage area one and locked using its
passwords. The net effect is a "nesting" of passwords 
which provides three levels of protection. 

The application 'ps_init' supplied here takes the given
password and ID along with constant data to supply 64 
bytes of data to algorithm to compute a MAC, which the 
first 8 bytes are used for the secret, the next 8 bytes
are used for the ID and the last 4 bytes are 'xored'
with the data that is to be stored.  The 'ps_check'
application undoes all of the above to print out 
the data that was given.  The retrieving of the data
involves waiting 30 seconds to do the actual 
transmission and while it is waiting random data
is being read from the device to confuse anyone
that might be monitoring the 1-Wire network
communications.

Test application for software authentication using the
DS1991 iButton.  45 bytes of additional memory
can be stored along with the software authorization.

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
ps_check.c - main check application for software 
             authentication.
ps_init.c  - initializes a software authentication ibutton.

Common Module File(s):			'\common'
crcutil.c   - used to calculate the crc of the data
findtype.c  - Find all devices of one type.
ioutil.c    - I/O utility functions
owerr.c     - error codes, description and functions
ownet.h     - main header file
ps02.c      - The file for communication to the DS1991.
ps02.h      - the header file for ps02.c
