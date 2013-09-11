/* (CCW)
   SACWD32.c
   
   Windows 95, NT, 32s Access System (06/97 Release V4.3)
   
//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES 
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
// OTHER DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of Dallas Semiconductor 
// shall not be used except as stated in the Dallas Semiconductor 
// Branding Policy. 
//---------------------------------------------------------------------------
   
   12/10/96 Added a new CheckOverdrive Function to test overdrive state
   12/13/96 Fixed Sleep problem.  Replaced Sleep() with FastSleep()
    6/09/97 Fixed grounded printer problem by putting EC on port before dropping 14
    6/09/97 Wait for semaphore before opening handle to driver.  Driver is now exclusive access

   Modifications to standard SACWD32 denoted with (OWPD)

*/

#include <dos.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <windows.h>
#include <windef.h>
#include <winbase.h>
#include <winnt.h>
#define  W32SUT_32
#include "w32sut.h"
#include "utdll.h"
#include "sacwd32.h"

#define DO_RESET         0
#define DO_BIT           1 
#define DO_BYTE          2 
#define TOGGLE_OVERDRIVE 3 
#define TOGGLE_PASSTHRU  4 
#define CHECK_BRICK      5 
#define SET_PORT         6

#define SAUTH95_SEM          "SAuth95AccessSem"
#define SA16COMPAT_SEM       "SAuth95Win16BitCompatSem"
#define SAUTH95_PASSTHRU_SEM "SAuth95PassThruSem"
#define SEM_TIMEOUT          10000 //milliseconds
#define PASSTRHU_TIMEOUT     10    //milliseconds
#define ROMSEARCH            0xF0

//nt variables
HANDLE  DOWHandle;          // Handle to file representing our driver 
HANDLE  sHandle = NULL;     // Handle to our semaphore 
DWORD   NumTrans;    
UCHAR   gpn = 1;
UCHAR   Passthru = TRUE;
UCHAR   TimeOut;
UCHAR   mch[256];  // (OWPD) was 3, enlarged for block function

BOOL WinNT  = FALSE;
BOOL Win95  = FALSE;
BOOL Win32s = FALSE;
BOOL loaded = FALSE;

uchar ToggleOverdrive(void); // (OWPD) made not static
static uchar CheckDriver(void);
uchar CheckOverdrive(void);  // (OWPD) mad not static
static void  TogglePassthru(void);
static uchar EnterPassthru (void);
static uchar ExitPassthru  (void);
static uchar CheckBusy     (void);
static uchar DOWBit        (uchar);
static uchar DOWByte       (uchar);
uchar DOWReset      (void);  // (OWPD) made not static
static uchar RomSearch     (sa_struct *gb);
static void  SetPortNumber(void);
static void  FastSleep(WORD Delay);
static void  MyInt64Add(PLARGE_INTEGER pA, PLARGE_INTEGER pB);
static BOOL  MyInt64LessThanOrEqualTo(PLARGE_INTEGER pA, PLARGE_INTEGER pB);

//-----------------
//Win32s typedefs
//-----------------
typedef BOOL (WINAPI * PUTREGISTER) ( HANDLE     hModule,
                  LPCSTR     lpsz16BitDLL,
                  LPCSTR     lpszInitName,
                  LPCSTR     lpszProcName,
                  UT32PROC * ppfn32Thunk,
                  FARPROC    pfnUT32Callback,
                  LPVOID     lpBuff
             );


typedef VOID (WINAPI * PUTUNREGISTER) (HANDLE hModule);

typedef DWORD (APIENTRY * PUT32CBPROC) (LPVOID lpBuff, DWORD dwUserDefined );
//-----------------
//Win32s variables
//-----------------
UT32PROC      pfnUTProc = NULL;
PUTREGISTER   pUTRegister = NULL;
PUTUNREGISTER pUTUnRegister = NULL;
int           cProcessesAttached = 0;
HANDLE        hKernel32 = 0;

//------------------
//Win95/NT variables
//------------------
HANDLE hSemaphore    = NULL,              // Handle of Semaphore
       hSA16CompatSem= NULL;              // Semaphore for Win16 compatibility
HANDLE hDriver       = NULL;              // Handle of Driver
HANDLE hThread       = NULL;              // Handle of Thread
HANDLE hPassThruSem  = NULL;              // Handle of PassThru Semaphore
uchar  RomDta[8]     = {0,0,0,0,0,0,0,0}; // Current ROM Id
uchar  DriverBuff[4];                     // Buffer for exchanging data with driver
void   *lpDriverBuff = DriverBuff;        // Pointer to data buffer
BOOL   AuthFlag      = FALSE;             // Thread has driver control?
BOOL   SemCreated    = FALSE;             // Semaphore created yet?

//////////////////////////////////////////////////////////
/////////////// software auth vars ///////////////////////
static uchar  Compatl0   = 0;  // for SACWD300.dll compatability
static ushort pa_vals[3] = { 0x378, 0x278, 0x3BC };
ushort bpa; // (OWPD) made non-static

////////////////////////////////////////////////////////////////////////////////
//==============================================================================
// DLLInit  
//==============================================================================
BOOL WINAPI DllMain(HANDLE hInst, DWORD fdwReason, LPVOID lpReserved)
{                 
   DWORD dwVersion;
   DWORD dwMajorVersion, dwMinorVersion;
   BOOL retVal;
   
   if ( fdwReason == DLL_PROCESS_ATTACH )
   {
                                                  
    
     // Registration of UT need to be done only once for first
     // attaching process.  At that time set the Win32s flag
     // to indicate if the DLL is executing under Win32s or not.
     
      if( cProcessesAttached++ )
      {
      
         return(TRUE);         // Not the first initialization.
      }

      // Find out if we're running on Win32s
      
      dwVersion = GetVersion();
      dwMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
      dwMinorVersion =  (DWORD)(HIBYTE(LOWORD(dwVersion)));
      
      if(dwVersion < 0x80000000)// NT
      {
         WinNT = TRUE;
         Win32s = Win95 = FALSE;
         return TRUE;
      }
      else if (dwMajorVersion < 4)        // Win32s
      {
         Win32s = TRUE;
         Win95 = WinNT = FALSE;
      }
      else                             // Windows 95 -- No build numbers provided
      {
         Win95 = TRUE;
         WinNT = Win32s = FALSE;
         return TRUE;
      }
      hKernel32 = LoadLibrary( "Kernel32.Dll" ); // Get Handle to Kernel32.Dll
      
      pUTRegister = (PUTREGISTER) GetProcAddress( hKernel32, "UTRegister" );
      
      if( !pUTRegister )
         return(FALSE);        // Error - On Win32s, but can't find UTRegister
      
      pUTUnRegister = (PUTUNREGISTER) GetProcAddress( hKernel32, "UTUnRegister" );
      
      if( !pUTUnRegister )
         return(FALSE);        // Error - On Win32s, but can't find UTUnRegister
      retVal = (*pUTRegister)( hInst,           // UTSamp32.DLL module handle
              "SWA16UT.DLL",  // name of 16-bit thunk dll
              NULL,        // name of init routine
              "UTProc",        // name of 16-bit dispatch routine (in utsamp16)
              &pfnUTProc,      // Global variable to receive thunk address
              NULL,  // callback function
              NULL );          // no shared memroy
   }
   if( (fdwReason == DLL_PROCESS_DETACH) && (0 == --cProcessesAttached) && Win32s )
   {
      (*pUTUnRegister)( hInst );
      FreeLibrary( hKernel32 );
   }
   return 1;      
}

////////////////////////////////////////////////////////////////////////////////
static uchar CallDriver(DWORD FuncNum, uchar Param)
{
  uchar  *pByte = DriverBuff;   // first byte is data
  ushort *pBPA;                 // next two are the port address
  DWORD BuffSize = 0;           // driver returns number of data bytes to us

  (uchar *)pBPA = &DriverBuff[1];           
  memset(DriverBuff,0,sizeof(DriverBuff));  // clear buffer,
  *pByte = Param;                           // set parameter value
  *pBPA  = bpa;                             // and port address

  DeviceIoControl(hDriver, // communicate with driver
                  FuncNum,
                  lpDriverBuff,sizeof(DriverBuff),
                  lpDriverBuff,sizeof(DriverBuff),(LPDWORD)(&BuffSize),
                  NULL);

  return (*pByte);
}
////////////////////////////////////////////////////////////////////////////////
void CleanUp(void)
{
  if(hSemaphore)
    CloseHandle(hSemaphore);// release semaphore handle to system
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY garbagebag(uchar *ld)// Added for Win16 compatibility 
{                          // returns lastone for SACWD300.dll
  *ld = Compatl0;
  return Compatl0;
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY OverdriveOn(void)
{
   if(!Win32s)
   {
      // (OWPD) if (!DOWReset())
      // (OWPD)    return FALSE;
      
      // (OWPD) // Put all overdrive parts into overdrive mode.
      // (OWPD) DOWByte(0x3C);
      
      return ToggleOverdrive() ? TRUE : ToggleOverdrive();
   }
   return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
void APIENTRY OverdriveOff(void)
{
   if(!Win32s)
   {
      // Turn overdrive off
      if(ToggleOverdrive())
         ToggleOverdrive();
   }
   return;
}
////////////////////////////////////////////////////////////////////////////////
static uchar EnterPassthru(void)
{
   if(Win95 || WinNT)
   {
      TogglePassthru();
      Passthru = TRUE;
      return TRUE; 
   } 
   return FALSE;
}
////////////////////////////////////////////////////////////////////////////////
static uchar ExitPassthru(void)
{
   uchar i = 0;

   if(!Win32s)
   {
      Passthru = TRUE;
      
      while(Passthru && (i++ < 3))
      {
        TogglePassthru();
        Passthru = !CheckBusy();// if busy lines don't drop we're in passthru mode
      }
      
      return !Passthru;
   }
   return FALSE;
   
}
////////////////////////////////////////////////////////////////////////////////
BOOL LoadDriver(void)
{
   SC_HANDLE hSCManager, hDS1410D;
   char Service[] = "DS1410D";
   DWORD LastError;

   /* Get a handle to the Service control manager database */
   hSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);
   if(hSCManager)
   {
      /* get a handle to our driver */
      hDS1410D = OpenService(hSCManager,
                    Service,
                    SERVICE_ALL_ACCESS);
      if(hDS1410D)
      {
         /* attempt to start DS1410D.SYS */
         if(StartService(hDS1410D, 0, NULL))
            return TRUE;
         else
         {
            LastError = GetLastError();
          
            return FALSE;
         }
      }
      else
      {
         hDS1410D= CreateService(
            hSCManager, // handle to service control manager database  
            Service, // pointer to name of service to start 
            Service, // pointer to display name 
            SERVICE_ALL_ACCESS,  // type of access to service 
            SERVICE_KERNEL_DRIVER,  // type of service 
            SERVICE_AUTO_START,  // when to start service 
            SERVICE_ERROR_NORMAL,   // severity if service fails to start 
            "SYSTEM32\\drivers\\DS1410D.SYS",   // pointer to name of binary file 
            "Extended Base",  // pointer to name of load ordering group 
            NULL, // pointer to variable to get tag identifier 
            NULL, // pointer to array of dependency names 
            NULL, // pointer to account name of service 
            NULL  // pointer to password for service account 
         );
         if(hDS1410D)
         {
            /* attempt to start DS1410D.SYS */
            if(StartService(hDS1410D, 0, NULL))
                 return TRUE;
             else
                 return FALSE;
         }
      }
   }
   else
      return FALSE;

  return FALSE;  // (OWPD) get all control paths
}


// This function is called on NT to determine if the driver is loaded on the System
// CheckDriver should only be called by dowcheck()
static uchar CheckDriver(void)
{

   SC_HANDLE hSCManager, hDS1410D;
   char Service[] = "DS1410D";
   /* Get a handle to the Service control manager database */
   hSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_CONNECT);//(3.11a fix)
   if(hSCManager)
   {
      /* get a handle to our driver */
      hDS1410D = OpenService(hSCManager,
                    Service,
                    SERVICE_QUERY_STATUS);//(3.11a fix)
      if(hDS1410D)
      {
         CloseServiceHandle(hDS1410D);
         CloseServiceHandle(hSCManager);
         return TRUE;
      }
      else
      {
         CloseServiceHandle(hSCManager);
         return FALSE;
      }
   }
   return FALSE;
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY dowcheck(void) // Must be used in All Apps!!!!!
{
   DWORD dwVersion;
   DWORD dwMajorVersion, dwMinorVersion; 
   dwVersion = GetVersion();
   dwMajorVersion =  (DWORD)(LOBYTE(LOWORD(dwVersion)));
   dwMinorVersion =  (DWORD)(HIBYTE(LOWORD(dwVersion)));

   if(dwVersion < 0x80000000)          // NT
   {
      WinNT = TRUE;
      Win32s = Win95 = FALSE;
   }
   else if (dwMajorVersion < 4)        // Win32s
   {
      Win32s = TRUE;
      WinNT = Win95 = FALSE;
   }
   else                             // Windows 95 -- No build numbers provided
   {
      Win32s = WinNT = FALSE;
      Win95 = TRUE;
   }

   if(Win95)
   {
      if((!hDriver) || (hDriver == INVALID_HANDLE_VALUE))
         hDriver=CreateFile("\\\\.\\VSAUTHD.VXD",0,0,NULL,OPEN_ALWAYS,
                        FILE_FLAG_DELETE_ON_CLOSE,NULL);
      
       if(hDriver==INVALID_HANDLE_VALUE)
         return FALSE;         // could not get handle to our driver
      
      hThread=GetCurrentThread();

      if(!hPassThruSem)
      {
         if(!(hPassThruSem = OpenSemaphore(SYNCHRONIZE|SEMAPHORE_MODIFY_STATE,
                                     FALSE,
                                     SAUTH95_PASSTHRU_SEM)))
         {
            hPassThruSem = CreateSemaphore(NULL,1,1,SAUTH95_PASSTHRU_SEM);
         }
      }
      
      return TRUE;
   }
   else if(WinNT)
   {
       if(!CheckDriver())
          return ((uchar)LoadDriver());
       else
          return TRUE;
   }
   else
      return FALSE;
   
}
////////////////////////////////////////////////////////////////////////////////
uchar nowaitkeyopen(void)// Added for Win16 compatibility.
{                        // This function should never be called 
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY keyopen(void) // Must be used in Win32 Apps!!!!!
{
   PSECURITY_DESCRIPTOR pSD;//(3.11Beta2)
   SECURITY_ATTRIBUTES sa;//(3.11Beta2)

   DWORD WaitResult=0;
   if(Win32s)
   {
      return (uchar)((*pfnUTProc)(NULL, MYKEYOPEN, NULL));
   }
   else if(Win95)
   {
      if(!AuthFlag)
      {
         AuthFlag = ((hDriver==INVALID_HANDLE_VALUE)?FALSE:TRUE);
      
         if(AuthFlag) // have valid driver handle
         {
            if(!SemCreated)
            {
               hSemaphore = CreateSemaphore(NULL,1,1,SAUTH95_SEM);
               SemCreated = TRUE;
            }
      
            if(!hSemaphore) // no semaphore handle yet
            {
               hSemaphore = OpenSemaphore(SYNCHRONIZE|SEMAPHORE_MODIFY_STATE,
                                          FALSE,
                                          SAUTH95_SEM);
            }
      
            if(hSemaphore)
            {
               // try to spread access to all applications evenly
               SetThreadPriority(hThread,THREAD_PRIORITY_ABOVE_NORMAL);// 1 point higher
               WaitResult=WaitForSingleObject(hSemaphore,SEM_TIMEOUT);
      
               if(WaitResult==WAIT_FAILED)
               {
                  SetThreadPriority(hThread,THREAD_PRIORITY_NORMAL);
                  ReleaseSemaphore(hSemaphore,1,NULL);
                  AuthFlag = FALSE;
                  Sleep(0);
                  return FALSE;
               }  
      
               // Added for Win16 compatibility
               hSA16CompatSem=CreateSemaphore(NULL,1,1,SA16COMPAT_SEM);
            }
            else
               return FALSE;
         }// if(AuthFlag)
      
      }// if(!AuthFlag)
      
      if(AuthFlag)
      {
         ExitPassthru();
         if(CheckOverdrive())
            OverdriveOff();      // call overdrive off if mixed stacking is not enabled
      }
      
      return AuthFlag;
   }
   else if(WinNT)
   {  
      if (!AuthFlag)                
      {  
         // If it existed we still don't have a handle 
        if (sHandle == NULL || !SemCreated) 
        {
           sHandle = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, 
                                FALSE, 
                                "DS1410XAccessSem");

           // Make sure we got a handle.
           if (sHandle != NULL)
              SemCreated = TRUE;
        }
        // Create semaphore if it doesn't exist 
        if (!SemCreated)           
        {
		  // Begin 3.11Beta1 Changes 
		  pSD = (PSECURITY_DESCRIPTOR) LocalAlloc( LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
      
		  if (pSD == NULL)
		  {
			 return FALSE;
		  }
		  if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
		  {
			 return FALSE;
		  }
      
		  // Add a NULL DACL to the security descriptor to share objects with service (3.11Beta)
      
		  if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE))
		  {
			 return FALSE;
		  }
      
		  sa.nLength = sizeof(sa);
		  sa.lpSecurityDescriptor = pSD;
		  sa.bInheritHandle = TRUE;
		  sHandle = CreateSemaphore(&sa, 1, 1, "DS1410XAccessSem");
		  if (sHandle != NULL)
              SemCreated = TRUE;
		  
			/* begin old sect (3.11alpha and prior)
           sHandle = CreateSemaphore(NULL, 1, 1, "DS1410XAccessSem");
           if (sHandle != NULL)
              SemCreated = TRUE;
		   */
        }

        if (SemCreated == TRUE)
        {
			
            // Wait until semaphore value = 0, or 5 second timeout 
            WaitForSingleObject(sHandle, 5000); 
            
            // Once we've got the semaphore we should be able to get the handle
            // (unless semaphore wait timed out, then we fail CreateFile)
            // Try to get handle to our driver
            AuthFlag = !((DOWHandle = CreateFile("LPT9",     
                                           GENERIC_READ | GENERIC_WRITE,
                                           0,       
                                           NULL,   
                                           OPEN_EXISTING,
                                           FILE_ATTRIBUTE_NORMAL,
                                          NULL)) == INVALID_HANDLE_VALUE);
            if(AuthFlag)
            {
                // Set port number in the driver
                SetPortNumber();
            
                // Enter EPP passthru mode
                ExitPassthru();
                if (CheckOverdrive())
                    OverdriveOff();
            }
            else
            {
               ReleaseSemaphore(sHandle, 1, NULL);      
               return FALSE;
            }
        }
        else
            return AuthFlag;
      }
      return AuthFlag;
   }
   return FALSE;
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY keyclose(void) // Must be Used in All Apps!!!!!
{
   if(Win32s)
   {
      (*pfnUTProc)(NULL, MYKEYCLOSE, NULL);
      return TRUE;
   }
   else if(Win95)
   {
      if(AuthFlag)
      {
         EnterPassthru();
         
         // Added for Win16 compatibility
         if(hSA16CompatSem)
         {
            CloseHandle(hSA16CompatSem);// release semaphore handle to system
         }
         
         ReleaseSemaphore(hSemaphore,1,NULL);
         AuthFlag = FALSE;
         // allow fair access to other applications
         SetThreadPriority(hThread,THREAD_PRIORITY_BELOW_NORMAL);// 1 point lower
         Sleep(0); //release time slice
         SetThreadPriority(hThread,THREAD_PRIORITY_NORMAL);
         return TRUE;
      }
   }
   else if(WinNT)
   {  
      // If we had control, release control 
      if (AuthFlag)                    
      {  
         // Enter EPP passthru mode
         EnterPassthru();
         // Close handle to DS1410E.SYS 
         CloseHandle(DOWHandle);               
         // Set semaphore value back to 0 
         ReleaseSemaphore(sHandle, 1, NULL);  
         AuthFlag = FALSE;     
      }
      return TRUE;
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY first(sa_struct *gb)
{
   DWORD Args[1];
   if(Win95 || WinNT)
   {
      // Don't force a failure here
      gb->lastone = FALSE;
      // Point Rom Search algorithm to the top
      gb->ld = 0;
      
      // Go look for the first DOW part on the bus
      return next(gb);
   }
   else
   {
      Args[0] = (DWORD) gb; //only passing one argument, windows handles translation
      return (uchar)((* pfnUTProc)(gb, MYFIRST, NULL));
   }
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY next(sa_struct *gb)
{
   uchar tr;
   DWORD Args[1];
   if(Win32s)
   {
      Args[0] = (DWORD) gb;//only passing one argument, windows handles translation
      return(uchar)( (*pfnUTProc)(gb, MYNEXT, NULL));
   }
   else if(Win95)
   {
      if (gb->setup_ok)
      {
         // See if last search found last button
         if (gb->lastone)
         {
            gb->lastone = FALSE;
      
            // Reset next function
            gb->ld = 0;
         }
         else
         while ((tr = RomSearch(gb)) != 0) // Do that ROM search thang
         {
            // See if we should force failure
            if (tr == 2)
               gb->lastone = 1;
      
            // Detect short circuit
      
            if (!RomDta[0])
              return FALSE;
      
            return TRUE;         
         }//else
      
      }//if (gb->setup_ok)
      return FALSE;
   }
   else if(WinNT)
   {
      if (gb->setup_ok)
      {
         // See if last search found last button
         if (gb->lastone)                    
         {
            gb->lastone = FALSE; 
      
            // Reset next function 
            gb->ld = 0;                                        
         } 
         else while ((tr = RomSearch(gb)) != 0) 
         {
            // See if we should force failure
            if (tr == 2) 
               gb->lastone = 1;                
           
            // Detect short circuit
            if (!RomDta[0])       
               return FALSE;
      
            gb->accflg = TRUE;
      
            return TRUE;
         }
      }
      return FALSE;
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY access(sa_struct *gb)
{
   uchar i, j;
   DWORD Args[1];

   // update 'local' rom number from state buffer
   memcpy(&(RomDta[0]), &(gb->romdta[0]), 8);

   if(Win32s)
   {
      Args[0] = (DWORD) gb;//only passing one argument, windows handles translation
      return (uchar)( (*pfnUTProc)(gb, MYACCESS, NULL));
   }
   else if(Win95)
   {
      // Assume failure
      gb->accflg = FALSE;
      
      // Send reset pulse
      if (DOWReset())
      {
         // ROM search command byte
         DOWByte(ROMSEARCH);
      
         // Byte loop
         for (i = 0; i < 8; i++)
         {
            // Bit loop
            for (j = 0; j < 8; j++)
            {
               if (((DOWBit(TRUE) << 1) | DOWBit(TRUE)) == 3)
                  return FALSE;     // Part not there
      
               // Send write time slot
               DOWBit((uchar) ((RomDta[i] >> j) & 1));
            }
         }
      
         // Success if we made it through all the bits
         gb->accflg = TRUE;
      }
      
      return gb->accflg;
   }
   else if (WinNT)
   {
       // Assume failure 
      gb->accflg = FALSE;                                       
      
      // Send reset pulse
      if (DOWReset())                                         
      {
         // ROM search command byte
         DOWByte(0xF0);                                
      
         // Byte loop
         for (i = 0; i < 8; i++)                                     
         {
            // Bit loop
            for (j = 0; j < 8; j++)                                   
            {
               if (((DOWBit(TRUE) << 1) | DOWBit(TRUE)) == 3)                
                  return FALSE;     
      
               // Send write time slot
               DOWBit((UCHAR) ((RomDta[i] >> j) & 1));    
            }      
         }
      
         // Success if we made it through all the bits
         gb->accflg = TRUE;         
      }
      
      return gb->accflg;
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY gndtest(sa_struct *gb)
{  

   DWORD Args[1];
   if(Win32s)
   {
      Args[0] = (DWORD) gb;  //passing one argument, no translation needed
      return (uchar)( (*pfnUTProc)(gb, MYGNDTEST, NULL));
   }
   else if(Win95)
   {
      if (gb->setup_ok)
      {
        DOWReset();
        return DOWBit(TRUE);
      }
      
      return FALSE;
   }
   else if(WinNT)
      return (uchar)(gb->setup_ok);

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
uchar * APIENTRY romdata(sa_struct *gb)
{
   uchar far *romptr;
   DWORD Args[1];
   if(Win32s)
   {
       Args[0] = (DWORD) gb;//passing one argument, no translation needed
      (*pfnUTProc)(gb, MYROMDATA, NULL);
      return gb->romdta;
   }
   else 
   {
      romptr = &(gb->romdta[0]); // point to the first byte of romdta array   
      return(romptr);            // send far pointer back to caller 
   }
//   return (uchar *)RomDta;
}
////////////////////////////////////////////////////////////////////////////////
uchar APIENTRY databyte(uchar data, sa_struct *gb)
{
   DWORD Args[2]; //function has two arguments;
   PVOID TransList[2]; //One pointer needs translation: gb

   if(!Win32s)
      return (gb->setup_ok && gb->accflg) ? DOWByte(data) : data;
   else
   {
      Args[0] = (DWORD) data; //passing two arguments
      Args[1] = (DWORD) gb;
      TransList[0] = &Args[1]; //translate one pointer
      TransList[1] = NULL;    //End translation with NULL
      return (uchar)( (* pfnUTProc)(Args, MYDATABYTE, TransList));
   }
}
///////////////////////////////////////////////////////////////////////////////
uchar APIENTRY databit(uchar tbit, sa_struct *gb)
{
   DWORD Args[2];
   PVOID TransList[2];

   if(!Win32s)
      return (gb->setup_ok && gb->accflg) ? DOWBit(tbit) : tbit;
   else
   {
      Args[0] = (DWORD) tbit;
      Args[1] = (DWORD) gb;
      TransList[0] = &Args[1];
      TransList[1] = NULL;
      return (uchar)( (* pfnUTProc)(Args, MYDATABIT, TransList));
   }
}
///////////////////////////////////////////////////////////////////////////////
uchar APIENTRY setup(uchar pn, sa_struct *gb)
{  
   DWORD Args[2]; //function has one argument;
   PVOID TransList[2]; //One pointer needs translation: gb
               
   if(Win32s)
   {
      Args[0] = (DWORD) pn;     //Passing two parameters
      Args[1] = (DWORD) gb;
      
      TransList[0] = &Args[1];        //One pointer needs to be translated 
      TransList[1] = NULL;            //End translation with NULL  
      return (uchar)( (* pfnUTProc)(Args, MYSETUP, TransList));
   }
   else if(Win95)
   {  
      memset(gb, 0, sizeof(sa_struct));
      
      // Reset RomSearch (first, next) algorithm
      gb->lastone = FALSE;
      gb->ld = 0;
      
      if (pn > 0 && pn < 4)                     // Make sure port number is valid
      {
         // This allows all other functions to execute
         gb->setup_ok = TRUE;
         gb->accflg = TRUE; //(OWPD) 
         // Set base port address
         bpa = pa_vals[pn - 1];
      }
      else
         bpa = pa_vals[0];  // Set to default in case caller ignores FALSE return
      
      return  (uchar)(gb->setup_ok);                          // Return result of setup function
   }
   else if(WinNT)
   {
      // Initialize global flags
      memset(gb, 0, sizeof(sa_struct));
      // Reset RomSearch (first, next) algorithm 
      gb->lastone = FALSE;                             
      gb->ld = 0;                            
      
      // Make sure port number is valid
      if (pn > 0 && pn < 4)           
      {
         // This allows all other functions to execute
         gb->setup_ok = TRUE;            
         gb->accflg = TRUE; //(OWPD) 
         // Assign port number
         gpn = pn;
      }
      else
         gpn = 1; 
      
      // Return result of setup function
      return (uchar)(gb->setup_ok);
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
static 
uchar DOWBit(uchar bit_val)
{
   BOOLEAN BitResult = TRUE;
   if(Win95)
      return CallDriver(DOWBIT,bit_val);
   else if(WinNT)
   {
      if (AuthFlag)
      {
         mch[0] = DO_BIT;  // Tell driver to do a bit 
         mch[1] = bit_val; // Specify bit value 
      
         if (WriteFile(DOWHandle, (LPSTR) mch, 2, &NumTrans, NULL) &&
             ReadFile(DOWHandle, (LPSTR) mch, 1, &NumTrans, NULL))
         {
            BitResult = (BOOLEAN) *mch; 
         }
      } 
      return BitResult;
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
static 
uchar DOWByte(uchar byte_val)
{
   UCHAR ByteResult = byte_val;
   if(Win95)
      return CallDriver(DOWBYTE,byte_val);
   else if(WinNT)
   {
      if (AuthFlag)
      {
         mch[0] = DO_BYTE;  // Tell driver to do a bit 
         mch[1] = byte_val; // Specify bit value 
      
         if (WriteFile(DOWHandle, (LPSTR) mch, 2, &NumTrans, NULL) &&
             ReadFile(DOWHandle, (LPSTR) mch, 1, &NumTrans, NULL))
         {
            ByteResult = *mch;                  
         }
      } 
      
      return ByteResult;
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
//(OWPD) static 
uchar DOWReset()
{
   BOOLEAN ResetResult = FALSE;
   if(Win95)
      return CallDriver(DOWRESET,0);
   else if (WinNT)
   {
      if (AuthFlag)
      {
         mch[0] = DO_RESET; // Tell driver to do a reset 
         mch[1] = gpn;      // Specify a port 
      
         if (WriteFile(DOWHandle, (LPSTR) mch, 2, &NumTrans, NULL) && 
             ReadFile(DOWHandle, (LPSTR) mch, 1, &NumTrans, NULL))
         {
            // Assign result 
            ResetResult = (BOOLEAN) *mch;                     
         }
      } 

      return ResetResult;
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
uchar ToggleOverdrive(void)  // (OWPD)
{
   UCHAR ByteResult = 0;
   if(Win95)
      return CallDriver(DOWTOGGLEOD,0);
   else if(WinNT)
   {
      if (AuthFlag)
      {
         mch[0] = TOGGLE_OVERDRIVE; 
      
         if (WriteFile(DOWHandle, (LPSTR) mch, 2, &NumTrans, NULL) &&
             ReadFile(DOWHandle, (LPSTR) mch, 1, &NumTrans, NULL))
         {
            ByteResult = *mch;                  
         }
      } 
      return ByteResult;
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
void APIENTRY copyromdata(ushort *ip, sa_struct *gb)
{
   short i;
   for (i = 0; i < 8; ++i)
      ip[i] = (int) gb->romdta[i];
}
////////////////////////////////////////////////////////////////////////////////
void APIENTRY copy2romdata(ushort *ip, sa_struct *gb)
{
   short i;
   for (i = 0; i < 8; i++)
      gb->romdta[i] = 0xff & ip[i];
}

////////////////////////////////////////////////////////////////////////////////
uchar CheckOverdrive() // (OWPD) make not static 
{
   uchar ByteResult = 0;

   if(Win95)
   {
      return CallDriver(CHECK_OVERDRIVE,0);//Busy Lines Dropped
   }
   else if(WinNT)
   {
      if (AuthFlag)
      {
         mch[0] = CHECK_OVERDRIVE; 
      
         if (WriteFile(DOWHandle, (LPSTR) mch, 2, &NumTrans, NULL) &&
             ReadFile(DOWHandle, (LPSTR) mch, 1, &NumTrans, NULL))
         {
            ByteResult = mch[0];                  
         }
      } 

      return ByteResult;
   }

   return FALSE;  // (OWPD) get all control paths
}

////////////////////////////////////////////////////////////////////////////////
static 
uchar CheckBusy()
{
   UCHAR ByteResult = 0;
   if(Win95)
      return CallDriver(DOWCHECKBSY,0);//Busy Lines Dropped
   else if(WinNT)
   {
      if (AuthFlag)
      {
         mch[0] = CHECK_BRICK; 
      
         if (WriteFile(DOWHandle, (LPSTR) mch, 2, &NumTrans, NULL) &&
             ReadFile(DOWHandle, (LPSTR) mch, 1, &NumTrans, NULL))
         {
            ByteResult = *mch;                  
         }
      } 
      
      return ByteResult;
   }

   return FALSE;  // (OWPD) get all control paths
}
////////////////////////////////////////////////////////////////////////////////
static
void TogglePassthru(void)
{
   UCHAR i;
   if(Win95)
      CallDriver(DOWTOGGLEPASS,0);
   else if(WinNT)
      for (i = 0; i < 4; i++)
         ToggleOverdrive();
   FastSleep(10);
}
////////////////////////////////////////////////////////////////////////////////
static 
uchar RomSearch(sa_struct *gb)
{
   uchar
         i = 0,
         x = 0,
         ld = gb->ld;
   uchar RomBit, 
         Mask;
   uchar j = 0;
   uchar k = 1;
   uchar lo = 0, nxt = 0;
   if(!Win32s)
   {
      // Reset DOW bus
      if (DOWReset())
         DOWByte(ROMSEARCH);  // Send search command
      else
      {
         gb->ld = 0;
         return FALSE;   // No DOW parts were found on bus
      }
      // While not done and not bus error
      while ((i++ < 64) && (x < 3))
      {
         Mask = 1 << ((i - 1) % 8);
      
         // Get last pass bit
         RomBit = RomDta[(i - 1) >> 3] & Mask ? TRUE : FALSE;
      
         // Send read time slots
         x = DOWBit(TRUE) << 1;
         x |= DOWBit(TRUE);
      
         // Is there a disagreement in this bit position
         if (!x)
         {
            // Stay on old path or pick a new one ?
            if (i >= ld)
               RomBit = (i == ld);         // Send write 1 if at position of ld 
              
            // Save this value as temp last disagreement
            if (!RomBit)
               gb->ld = i;
         }
         else
            RomBit = (x & 1) ^ 1;          // Get lsb of x and flip it
      
         if (RomBit)
            RomDta[(i - 1) >> 3] |= Mask;          // Set bit in Rom Data byte
         else
            RomDta[(i - 1) >> 3] &= (Mask ^ 0xFF); // Clear bit in Rom Data byte
      
         // Send write time slot
         DOWBit(RomBit);
      }
      
      memcpy(&(gb->romdta[0]), &(RomDta[0]), 8);
      Compatl0=gb->ld;
      if (x == 3)
         gb->ld = 0;
      return (x == 3) ? 0 : 1 + (ld == gb->ld);
   }

   return FALSE;  // (OWPD) get all control paths
}

void SetPortNumber()
{
   UCHAR ByteResult = 0;

   if (AuthFlag)
   {
      mch[0] = SET_PORT; 
      mch[1] = gpn; 

      WriteFile(DOWHandle, (LPSTR) mch, 2, &NumTrans, NULL);
      ReadFile(DOWHandle, (LPSTR) mch, 1, &NumTrans, NULL);
   } 
}

static void MyInt64Add(PLARGE_INTEGER pA, PLARGE_INTEGER pB)
{
   BYTE c = 0;

   if ((pA->LowPart != 0L) && (pB->LowPart != 0L))
      c = ((pA->LowPart + pB->LowPart) < pA->LowPart) ? 1 : 0;

   pA->LowPart  += pB->LowPart;
   pA->HighPart += pB->HighPart + c;
}

//////////////////////////////////////////////////////////////////////////////
//
//   Returns TRUE is A <= B.
//
static BOOL MyInt64LessThanOrEqualTo(PLARGE_INTEGER pA, PLARGE_INTEGER pB)
{
   BOOL CompRes = FALSE;

   if (pA->HighPart < pB->HighPart)
      CompRes = TRUE;
   else  if ((pA->HighPart == pB->HighPart) && (pA->LowPart <= pB->LowPart))
      CompRes = TRUE;

   return CompRes;
}
static void FastSleep(WORD Delay)
{
   LARGE_INTEGER CountsPerSec, CurrentCount, FinalCount, DelayCounts;
   BOOL          UseOldSleep = TRUE;
    
   if (QueryPerformanceFrequency(&CountsPerSec))
   {
      if (CountsPerSec.HighPart == 0L)
      {
         DelayCounts.HighPart = 0;
         DelayCounts.LowPart  = (CountsPerSec.LowPart / 1000L) * Delay;

         if (DelayCounts.LowPart)
         {
            // Get current count value
            QueryPerformanceCounter(&FinalCount);
            // FinalCount += DelayCounts
            MyInt64Add(&FinalCount, &DelayCounts);

            do
            {
               SleepEx(1, FALSE);
               QueryPerformanceCounter(&CurrentCount);   
            }
            while (MyInt64LessThanOrEqualTo(&CurrentCount, &FinalCount));

            UseOldSleep = FALSE;
         }
      }
   }
 
   if (UseOldSleep)
   {
      // Use regular sleep if hardware doesn't support a performance counter
      SleepEx(Delay, FALSE);
   }
}

////////////////////////////////////////////////////////////////////////////////
// sacwd32.c
