Test application for software authentication using the
DS1963S/DS1961S iButton,  7 bytes of additional memory
can be stored along with the software authorization.


Introduction:
=============
This application uses code that was developed for the
debit application, which debits money from the 
DS1963S/DS1961S user by using the DS1963S coprocessor.
For more information on how this works please refer to:
Application Note 156
Application Note 151

The benefit to using this for software authorization is
that the secret is not passed over the 1-Wire Network.
The secret is used to generate a MAC, which is used to
check the data that is on the iButton.

sha_init:
========
This application initializes the DS1963S/DS1961S user and
sets up the coprocessor using software.  Here is the
code that sets the coprocessor data structure for use 
with the user:

   // MANUALLY SETTING DATA FOR COPROCESSOR
   memcpy(copr.serviceFilename, "DLSM", 4);
   copr.serviceFilename[4] = (uchar) 102;
   copr.signPageNumber     = (uchar) 8;
   copr.authPageNumber     = (uchar) 7;
   copr.wspcPageNumber     = (uchar) 9;
   copr.versionNumber      = (uchar) 1;
   memcpy(copr.bindCode, "bindcde", 7);
   for(i=0;i<8;i++)
   {
      memcpy(&copr.bindData[i*4], "bind", 4);
   }
   copr.encCode           = 0x01;
   copr.ds1961Scompatible = 0x01;

The bind code and bind data are optional.  The
copr variable is then used to setup the user.

This is done by installing the master 
authentication secret (which the user provides)
and binding it to the iButton to produce the
unique secret for the token then write the 
certificate file to the iButton, which 
contains the 7 bytes of provided data.

sha_chck:
=========
This application checks the reading of the data
on the iButton and compares the MAC that was generated
by doing a VerifyUser function call after installing
the authentication secret and signing secret.  The 
data that was put into the button using sha_init is 
printed out when the iButton passes the check.  The same 
coprocessor information as above is used again as the
software coprocessor.  


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
sha_chk.c - main check application for software 
            authentication.
sha_init.c  - initializes a software authentication ibutton.

Common Module File(s):			'\common'
shaib.h     - common SHA header file
sha18.c     - I/O routines for DS1963S
sha33.c     - I/O routines for DS1961S
shaib.c     - Protocol-level routines for sha iButtons
shadbtvm.c  - Debit routines with virtual coprocessor
ioutil.c    - I/O utility functions
crcutil.c   - used to calculate the crc of the data
mbappreg.c  - memory bank functions for the application register
              iButtons
mbappreg.h  - header file
mbee.c      - memory bank functions for the electically erasable
              programmable read only memory
mbee.h      - header file
mbee77.c    - memory bank for the DS1923 and DS1977
mbee77.h    - header file
mbeprom.c   - memory bank functions for the electrically
              programmable read only memory
mbeprom.h   - header file
mbnv.c      - memory bank functions for the read/write nonvolatile
              memory
mbnv.h      - header file
mbnvcrc.c   - memory bank functions for the read/write nonvolatile
              memory with crc
mbnvcrc.h   - header file
mbscr.c     - memory bank functions for the scratchpad
mbscr.h     - header file
mbscrcrc.c  - memory bank functions for the scratchpad with crc
mbscrcrc.h  - header file
mbscree.c   - memory bank functions for the scratchpad ee
mbscree.h   - header file
mbscrx77.c  - memory bank for the scratchpad of the DS1923 and DS1977
mbscrx77.h  - header file
mbscrex.c   - memory bank functions for the scratchpad ex
mbscrex.h   - header file
mbsha.c     - memory bank functions for the sha parts
mbsha.h     - header file
mbshaee.c   - memory bank functions for the shaee parts
mbshaee.h   - header file
owcache.c   - cache functions for file I/O
pw77.c      - password functions for the DS1923 and DS1977
pw77.h      - header file
owerr.c     - error codes, description and functions
ownet.h     - main header file
owfile.c    - file I/O functions
owfile.h    - header file
owpgrw.c    - page read/write functions for file I/O
owprgm.c    - the program job functions for file I/O using EPROM
              parts.
rawmem.c    - the utility that sorts memory bank functions
              depending on part and bank
rawmem.h    - header file
