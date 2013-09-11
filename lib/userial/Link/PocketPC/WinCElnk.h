// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

// Windows Header Files:
#include <windows.h>

// Local Type
#define IBAPI short pascal
#define StdFunc pascal
#define IBFAR

// headers your program requires here
int OpenCOM(int, char *);
int SetupCOM(short, ulong);
void FlushCOM(int);
void CloseCOM(int);
int WriteCOM(int, int, uchar *);
int ReadCOM(int, int, uchar *);
void BreakCOM(int);
int PowerCOM(short, short);
int RTSCOM(short, short);
int FreeCOM(short);
void StdFunc MarkTime(void);
int ElapsedTime(long);
int DSRHigh(short);
void msDelay(int len);
void SetBaudCOM(int, uchar);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
