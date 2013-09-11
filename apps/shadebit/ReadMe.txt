Test application to debit the DS1963S/DS1961S SHA iButton.
The shainitcopr has to be run first on a DS1963S and then
the shainitrov can be run on a DS1963S or a DS1961S.  The
file name for the coprocessor should be 'copr' because that
is what the shainitrov program is looking for.  The
shadebitdemo debits from the amount that was stored during
the shainitrov program.

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
shadebitdemo.c - main debit application.  Retrieves coprocessor
                 information and debits the user token.
shainitcopr.c  - initializes a coprecessor.
shainitrov.c   - initializes a user token.

Note: Define "COPRVM" to use a 'virtual coprocessor' in the
above applications.

Common Module File(s):			'\common'
shaibutton.h- common SHA header file
sha18.c     - I/O routines for DS1963S
sha33.c     - I/O routines for DS1961S
shaibutton.c- Protocol-level routines for sha iButtons
shadebit.c  - Debit routines for sha transactions
shadebitvm.c- Debit routines with virtual coprocessor
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
mbscrx77.c  - memory bank for the scratchpad of the DS1923 and DS1977
mbscrcrc.c  - memory bank functions for the scratchpad with crc
mbscrcrc.h  - header file
mbscree.c   - memory bank functions for the scratchpad ee
mbscree.h   - header file
mbscrex.c   - memory bank functions for the scratchpad ex
mbscrex.h   - header file
mbsha.c     - memory bank functions for the sha parts
mbsha.h     - header file
mbshaee.c   - memory bank functions for the shaee parts
mbshaee.h   - header file
pw77.c      - password functions for the DS1923 and DS1977
pw77.h      - header file
owcache.c   - cache functions for file I/O
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
