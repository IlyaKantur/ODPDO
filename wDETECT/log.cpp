#include "stdafx.h"
#include <stdio.h>
#include <windows.h>

#ifdef LOGFACILITY

static FILE *log ;
static CRITICAL_SECTION log_cs ;

void LogOpen(LPCTSTR lpName)
{	
	log = fopen(lpName, "a+") ;
	if(NULL != log)	InitializeCriticalSection(&log_cs) ;
}

void LogTimeStamp()
{
	if(NULL == log) return ;

	SYSTEMTIME ST ;
	GetSystemTime(&ST) ;

	fprintf(log, "%02hu%02hu%02hu.%02hu%02hu%02hu.%03hu ",
			ST.wDay, ST.wMonth, ST.wYear % 100,
			ST.wHour, ST.wMinute, ST.wSecond,
			ST.wMilliseconds) ;
}

void LogString(LPCSTR lpcStr)
{
	if(NULL == log) return ;

	EnterCriticalSection(&log_cs) ;

	LogTimeStamp() ;
	fprintf(log, "%s\n", lpcStr) ;
	fflush(log) ;

	LeaveCriticalSection(&log_cs) ;
}

void LogError(LPCSTR lpcPlace, DWORD dwErrCode)
{
	if(NULL == log) return ;

	EnterCriticalSection(&log_cs) ;

	LogTimeStamp() ;
	fprintf(log, "CODE %05u %s\n", dwErrCode, lpcPlace) ;
	fflush(log) ;

	LeaveCriticalSection(&log_cs) ;
}

DWORD LogErrorCode(LPCSTR lpcPlace)
{
	DWORD dwCode = GetLastError() ;
	LogError(lpcPlace, dwCode) ;
	return dwCode ;
}

void LogBytes(BYTE *lpBytes, int nLen)
{
	if(NULL == log) return ;

	EnterCriticalSection(&log_cs) ;

	for(int i = 0 ; i < nLen ; i ++) {

		if(0 == (i % 16)) {
			if(i) fprintf(log, "\n") ;	
			LogTimeStamp() ;
		}

		fprintf(log, " %02X", (DWORD)(lpBytes[i])) ;
	}

	fprintf(log, "\n") ;
	fflush(log) ;

	LeaveCriticalSection(&log_cs) ;
}

void LogClose()
{
	if(NULL == log) return ;

	EnterCriticalSection(&log_cs) ;
	LeaveCriticalSection(&log_cs) ;

	DeleteCriticalSection(&log_cs) ;
	fclose(log) ; log = NULL ;
}

#endif // LOGFACILITY

//////////////////////////////////////////////////////////////////////////