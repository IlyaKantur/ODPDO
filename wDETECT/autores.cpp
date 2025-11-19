#include "stdafx.h"
#include "autores.h"

#define LOCAL_CAP_LENGTH		128

DWORD FormatErrorMessage(DWORD dwCode, LPVOID &lpMsgBuf)
{
	DWORD pos ;

	// Try to get message from the chosen locale set
	pos = ::FormatMessage( 
	    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM | 
	    FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
	    dwCode,
		LANGIDFROMLCID(GetThreadLocale()), // Thread's language
//		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), 
		(LPTSTR) &lpMsgBuf,
	    0,
		NULL 
	) ;

	if(0 == pos) {

		pos = ::FormatMessage( 
			FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			FORMAT_MESSAGE_FROM_SYSTEM | 
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dwCode,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  
			(LPTSTR) &lpMsgBuf,
			0,
			NULL 
		) ;

	}

	return pos ;
}

void ErrorMsgBox(HWND hWnd, DWORD dwCode, DWORD dwCapID)
{
	LPVOID   lpMsgBuf ;
	TCHAR    cap[LOCAL_CAP_LENGTH] ;
	DWORD    pos ;

	pos = FormatErrorMessage(dwCode, lpMsgBuf) ;

	if(pos) {

		// Display the string.
		::LoadString(_Module.GetModuleInstance(), dwCapID, cap, LOCAL_CAP_LENGTH) ;
		::MessageBox(hWnd, (LPCTSTR)lpMsgBuf, cap, MB_OK | MB_ICONERROR ) ;

		// Free the buffer.
		::LocalFree( lpMsgBuf ) ;

	}
}

