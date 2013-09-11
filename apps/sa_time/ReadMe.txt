Test applications to read and write to
a DS1994 (DS1427, DS2404) as it relates to
a "timed trial" for software authorization
purposes.


Introduction:
=============
The DS1994 provides many features that make it
a powerful choice for providing software
authorization and licensing.  Software
authorization in this setting includes software
security and piracy prevention, and software
licensing in this sense includes the idea of a
timed trial.  Many software vendors like to
provide a demo of their software application
that runs for a pre-determined amount of
time.

The DS1994 is a good choice for software
authorization because it has a real-time clock
that can be write-protected (to prevent
tampering).  It also has alarms, and an
expiration mechanism which can limit
the part if necessary, such as
making the part read-only, or even making the
part unusable except for reading the 1-Wire
net address.  Plus, the DS1994 has 16 pages
of non-volatile RAM (32 bytes per page) that
can hold a license-authentication token in the
form of a 1-Wire file.

Two applications have been written to
demonstrate how a DS1994 can be used for
software authorization purposes.  The
first application, tm_init, sets up a
DS1994 to be used as a software authorization
device, and the second application, tm_check,
continuously checks in a loop the validity
of the DS1994's timed trial and the
license-authentication token.


Application:  tm_init
=====================

The application, tm_init, sets up a DS1994 to
be used as a software authorization device.

Two command line parameters are possible when
running the tm_init program.  The first
parameter is the port where the 1-Wire Network
can be found.  This parameter is platform
specific and is required for the program to run.
For 32-bit Windows, it is "COM1", "COM2",
"COM3", etc., and for Linux, it is
"/dev/cua0", "/dev/cua1", etc.

The second command line parameter is the
write-protect switch -wp.  When used, this
switch will cause the program to prompt for
a confirmation message to write-protect the
DS1994's clock, and its clock alarm.

**WARNING**
After write-protecting the clock and clock alarms,
if the clock alarm gets triggered, the part
will become read-only (including the non-volatile
RAM), and cannot be written to again (at least
in the iButton form factor of the DS1994).

The following steps show the flow of the program:

1)  First, the application, tm_init, prompts
for expiration time limits.  The first is a "hard"
limit that will be used to set the real-time clock alarm
on the device.  The hard limit is the time in
seconds from the current time on the PC's
clock to the desired alarm time on the DS1994.
The default time is 180 seconds (3 minutes) in
the future.

2)  The second expiration time limit prompt
is for a "soft" expiration time limit.  This will provide
a license period that does not expire the device and
will be a SHA validated value residing in a
memory file in the DS1994.  The "soft" limit is the
time in seconds from the current PC time to
a desired alarm time on the DS1994. The default
time for this time limit is 120 seconds (2
minutes) in the future.

3)  The program then prompts for a secret.  It
can be given in either hexadecimal bytes or in
an ASCII string.  This is used to help secure the
1-Wire file and make it tamper resistent.  It is
NOT written to the file (the license-authentication
token) on the DS1994.  It is used to calculate
a SHA1 generated MAC that is put into the file.

4)  The program prompts for User Text.  This is
placed "as is" into the 1-Wire file, and can be
used to store program configuration information,
tracking dates, numbers, etc.

The size limit of the user text is:
51 - secretsize.

5)  The program then does the following:
a.  Reads the PC's clock.
b.  Sets the DS1994's clock to the PC's clock.
c.  Turns on the oscillator of the DS1994 to
    make it "tick".
d.  Sets the clock alarm on the DS1994 to the
    "hard" limit.
e.  Write the license-authentication token to a file
    residing in the DS1994's memory.  This is done
    first by creating a MAC with the following method:
    ROM + UserText + SOFTTIME + Secret =>(SHA)=>  MAC.
    The MAC is a 20-byte result of a SHA1 function.
    Then, the UserText + SOFTTIME + MAC is written to
    a file on the DS1994 named EXP.000.

6)  Optionally, the program will prompt to write-
protect the DS1994 if the correct command line parameter
was given when initially running the program.  See the
warning above.

Security of the License-Authentication Token
============================================

The license-authentication token has been
designed above to do the following:

1)  Prevent the end user from copying the
token file to another DS1994.  If that
were to happen, then the tm_check would
fail to validate the license because the
ROM_ID (1-Wire Net Address) is used in
calculating the 20-byte MAC.

2)  Prevent the end user from extending his
software trial by editing the file and
extending his time period.  If this were
to happen, the tm_check program would
fail to validate because the "soft" time
limit is used in calculating the 20-byte MAC.

3)  Also, if the end user knew how to
create a SHA1 MAC from the above components,
and edit the file to give himself more time,
then the tm_check program would still fail
to validate because he would have to know
the secret used in calculating the 20-byte
MAC.


Application:  tm_check
======================

The application, tm_check, prompts for the
secret given in the tm_init program and
then checks the "hard" and "soft"
expiration time limits and the validity of
the license-authentication file.

This program also needs the correct
command line parameter to run properly.  It
is the the port where the 1-Wire Network
can be found.  As mentioned above, this
parameter is platform specific.  For example,
on 32-bit Windows, it is "COM1", "COM2",
"COM3", etc., and on Linux, it is
"/dev/cua0", "/dev/cua1", etc.

The first thing that tm_check does is
prompt for the secret used in the tm_init
program above in either hexadecimal or
ASCII text format.  Using the secret, it
then checks for the validity of the license-
authentication token and the real-time clock
alarm.  It does this continuously in a loop
and prints out the results.

In the main loop, the program first reads
the real-time clock and real-time clock
alarm, and then reads and parses the
license-authentication token file, retrieving
the "soft" time limit, the user text, and
the MAC.  Next, the program generates a new
MAC from the secret previously prompted for,
along with the ROM_ID of the iButton, the
"soft" time limit, and the user text.  It
compares the generated MAC to the MAC coming
from the EXP.000.  If they are equal, the
token validates.  Then, the "hard" and "soft"
limits are compared to see if the hardware
time limit has expired or the software time
limit has expired.


The following is the flow of the main tm_check
loop in pseudocode form:

loop
   {
      if (DS1994 PRESENT)
      {
         print ROM

         Read Real-Time Clock (RTC) and Alarm (RTCA)
         Read EXP.000 and extract: UserText, SOFTTIME and MAC

         if (failed to read EXP.000)
         {
            print 'Token NOT VALID, no license file'
            continue
         }

         Construct a new MAC with
         ROM + UserText + SOFTTIME + Secret =>(SHA)=>  MAC'

         if (MAC != MAC')
            print 'Token NOT VALID, secret must be different'
         else
         {
            print 'Token VALID'

            if (RTC < SOFTTIME)
               print 'License VALID' + UserText
            else
               print 'License EXPIRED'

            if (RTC >= RTCA)
               print 'Device EXPIRED'
         }
      }
      else
         print 'NO DEVICE PRESENT'
   }

So the possible output of tm_check may be:

         NO DEVICE PRESENT
         5400000011227704, Token NOT VALID, no license file
         1100000055266704, Token NOT VALID, secret must be different
         D7000000068C7A04, Token VALID, License VALID, MyTextGoesHere
         D7000000068C7A04, Token VALID, License EXPIRED
         D7000000068C7A04, Token VALID, License EXPIRED, Device EXPIRED


File Information
================

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

Application File(s):			'\source\apps'
tm_init.c   - initializes a software authentication iButton.
tm_check.c  - main check application for software
              authentication timed trial.

Common Module File(s):			'\source\common'
crcutil.c   - used to calculate the crc of the data
findtype.c  - Find all devices of one type.
ioutil.c    - I/O utility functions.
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
mbscrx77.h  - header file
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
owfile.c    - file I/O functions
owfile.h    - header file
ownet.h     - main header file
owpgrw.c    - page read/write functions for file I/O
owprgm.c    - the program job functions for file I/O using EPROM
              parts.
rawmem.c    - the utility that sorts memory bank functions
              depending on part and bank
rawmem.h    - header file
time04.c    - The file for communication to the DS1994.
time04.h    - the header file for time04.c

