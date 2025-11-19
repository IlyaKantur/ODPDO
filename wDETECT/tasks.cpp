#include "stdafx.h"
#include "autores.h"

#include "Cfg.h"
#include "tasks.h"
#include "log.h"

#define CHR_START		'T'
#define CHR_STOP		'P'
#define CHR_STEP		'S'

enum RestType { RST_EMPTY, RST_LO, RST_HI, RST_LO2 } ;

#define MASK_LOW		0x00
#define MASK_HIGH		0x40
#define MASK_HIGH2		0x80
#define MASK_OVER		0xC0
#define MASK_ORDER		0xC0
#define MASK_VALUE		0x3F

#define RANGE_SPAN		0xFFF

#define IO_BUFF			1024

static void ParseStream(HWND hMaster, WORD &wRest, enum RestType &eType, int nSize, BYTE *pBytes)
{
	CBoundArray<BYTE> Bytes(nSize, pBytes) ;

	LOG_STRING(_T("ParseStream entered, ok")) ;
	LOG_ERROR(_T("Rest"), wRest) ;
	LOG_ERROR(_T("Rest type"), eType) ;
	LOG_BYTES(Bytes, Bytes.Length()) ;

	for(int nPos = 0 ; nPos < nSize ; ++ nPos)
	{
		BOOL bRslt = TRUE ;
		BYTE byType = Bytes[nPos] & MASK_ORDER ;

		if(eType == RST_LO)
		{
			if(byType == MASK_HIGH || byType == MASK_OVER)
			{
				wRest |= (Bytes[nPos] & MASK_VALUE) << 6 ;
				eType = RST_EMPTY ;

				::PostMessage(hMaster, 
					byType == MASK_HIGH ? TSM_VALUE : TSM_OVER, 
					wRest, 
					0) ;

				if(byType == MASK_HIGH)
				{
					LOG_ERROR(_T("*** Value"), wRest) ;
				}
				else
				{
					LOG_ERROR(_T("*** Overflow"), wRest) ;
				}
			}
			else
			{
				// Make shift of values
				wRest = (Bytes[nPos] & MASK_VALUE) ;
				bRslt = FALSE ;
				LOG_ERROR(_T(" -- Position, MASK_HIGH expected"), (DWORD)nPos) ;
			}

		}
		else
		{
			if((Bytes[nPos] & MASK_ORDER) == MASK_LOW)
			{
				wRest = Bytes[nPos] & MASK_VALUE ;
				eType = RST_LO ;
			}
			else
			{
				bRslt = FALSE ;
				LOG_ERROR(_T(" -- Position, MASK_LOW expected"), (DWORD)nPos) ;
			}
		}

		if(!bRslt) 
			::PostMessage(hMaster, TSM_ERROR, ERROR_INVALID_DATA, 0) ;
	}

	LOG_STRING(_T("ParseStream leaving, ok")) ;
}

struct CRest
{
	enum RestType eType ;
	WORD wFirst ;
	WORD wSecond ;
} ;

static void ParseStream_(HWND hMaster, struct CRest &Rest, int nSize, BYTE *pBytes)
{
	CBoundArray<BYTE> Bytes(nSize, pBytes) ;

	LOG_STRING(_T("ParseStream_ entered, ok")) ;
	LOG_ERROR(_T("Rest"), wRest) ;
	LOG_ERROR(_T("Rest type"), eType) ;
	LOG_BYTES(Bytes, Bytes.Length()) ;

	for(int nPos = 0 ; nPos < nSize ; ++ nPos)
	{
		BYTE byType = Bytes[nPos] & MASK_ORDER ;
		BOOL bRslt = TRUE ;

		switch(Rest.eType)
		{
			case RST_EMPTY :

				if(byType == MASK_LOW) 
				{
					Rest.wFirst = Bytes[nPos] & MASK_VALUE ;
					Rest.eType = RST_LO ;
				}
				else
					bRslt = FALSE ;

				break ;

			case RST_LO :

				if(byType == MASK_HIGH)
				{
					Rest.wFirst |= (Bytes[nPos] & MASK_VALUE) << 6 ;
					Rest.eType = RST_HI ;
				}
				else
					bRslt = FALSE ;

				break ;

			case RST_HI :

				if(byType == MASK_LOW) 
				{
					Rest.wSecond = Bytes[nPos] & MASK_VALUE ;
					Rest.eType = RST_LO2 ;
				}
				else
					bRslt = FALSE ;

				break ;

			case RST_LO2 :

				if(byType == MASK_HIGH2)
				{
					Rest.wSecond |= (Bytes[nPos] & MASK_VALUE) << 6 ;
					Rest.eType = RST_EMPTY ;

					if(Rest.wFirst != 0 && Rest.wFirst != RANGE_SPAN &&
						Rest.wSecond != 0 && Rest.wSecond != RANGE_SPAN)
					{
						// Здесь поменяем формулу с A / (A + B)
						// на (A - B) / (A + B) + (A + B) / 2
						// Ещё раз заменили на
						// (A - B) / (A + B) + 1
						double dResult = 0.5 * ((1.0 * Rest.wFirst - Rest.wSecond) / (Rest.wFirst + Rest.wSecond) +
												1.0 ) * RANGE_SPAN ;
						WORD wValue = (WORD)(dResult + 0.5) ;

						::PostMessage(hMaster, TSM_VALUE, wValue, MAKELPARAM(Rest.wFirst, Rest.wSecond)) ;
					}
				}
				else
					bRslt = FALSE ;

				break ;

			default :
				ATLASSERT(0) ;
		}

		if(!bRslt)
		{
			LOG_ERROR(_T(" -- ERROR Position"), (DWORD)nPos) ;
			LOG_ERROR(_T(" -- ERROR Type expected"), Rest.eType) ;
			LOG_ERROR(_T(" -- ERROR Byte arrived"), Bytes[nPos]) ;

			::PostMessage(hMaster, TSM_ERROR, ERROR_INVALID_DATA, 0) ;
		}
	}

	LOG_STRING(_T("ParseStream_ leaving, ok")) ;
}

static HANDLE OpenPort(PORTCFG &PortCfg)
{
	CString PortName ;
	PortName.Format(_T("COM%hu"), PortCfg.wSerial) ;

	CAutoHandle Port(
		CreateFile(PortName, 
					GENERIC_READ | GENERIC_WRITE,
					0,
					NULL,
					OPEN_EXISTING,
					FILE_FLAG_OVERLAPPED,
					NULL)
	) ;

	if(!Port.IsValid()) return INVALID_HANDLE_VALUE ;

	DCB dcb ;
	
	if(!GetCommState(Port, &dcb)) return INVALID_HANDLE_VALUE ;

	dcb.BaudRate = PortCfg.dwBaud ;
	dcb.Parity = PortCfg.byParity ;
	dcb.StopBits = PortCfg.byStops ;

	dcb.ByteSize = 8 ;

	dcb.fBinary = TRUE ;
	dcb.fParity = TRUE ;
	dcb.fOutxCtsFlow = FALSE ;
	dcb.fOutxDsrFlow = FALSE ;
	dcb.fDtrControl = DTR_CONTROL_ENABLE ;
	dcb.fDsrSensitivity = FALSE ;
	dcb.fTXContinueOnXoff = FALSE ;
	dcb.fOutX = dcb.fInX = FALSE ;
	dcb.fErrorChar = TRUE ;
	dcb.ErrorChar = -1 ;
	dcb.fNull = FALSE ;
	dcb.fRtsControl = RTS_CONTROL_TOGGLE ;
	dcb.fAbortOnError = FALSE ;

	if(!SetCommState(Port, &dcb)) return INVALID_HANDLE_VALUE ;
	if(!SetupComm(Port, IO_BUFF, IO_BUFF)) return INVALID_HANDLE_VALUE ;

	COMMTIMEOUTS CTO = { 0 } ;

	if(!SetCommTimeouts(Port, &CTO)) return INVALID_HANDLE_VALUE ;
	if(!SetCommMask(Port, EV_RXCHAR | EV_ERR)) return INVALID_HANDLE_VALUE ;

	return Port.Release() ;
}

struct CHANNEL
{
	HANDLE hPort ;
	OVERLAPPED Ov ;
	HANDLE hCancel ;
};

#define LOCAL_TOUT			1000
#define SLEEP_ON_ERROR		200

static BOOL Survey(CHANNEL Chan, HWND hwndMaster, DWORD dwTimeout)
{
	BOOL   bRslt ;
	DWORD  dwResult, dwBytes ;

	// try to allocate buffer
	CAutoPtr<BYTE> Bytes(new BYTE[IO_BUFF]) ;
	bRslt = ! Bytes.IsNull() ;

	// Clean up Comm buffer
	if(bRslt)
	{
		bRslt = PurgeComm(Chan.hPort, PURGE_RXCLEAR | PURGE_RXABORT) ;
	} 
	else
	{
		::SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
	}

//	CAutoTimer Timer(dwTimeout * 1000) ;
//	bRslt = Timer.IsValid() ;

	// Send 'T' character to start survey
	if(bRslt)
	{
		char chSS = CHR_START ;

		bRslt = WriteFile(Chan.hPort, &chSS, sizeof(chSS), &dwBytes, & Chan.Ov) ;
		if(!bRslt) 
		{
			bRslt = ERROR_IO_PENDING == ::GetLastError() ;
			if(bRslt)
				bRslt = GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, TRUE) ;
		}
	}

	if(!bRslt) return FALSE ;

	// operate until time is over
	COMSTAT CS ;
	WORD wRest = 0 ;
	enum RestType eRest = RST_EMPTY ;
	HANDLE Events[] = { Chan.Ov.hEvent, Chan.hCancel } ;

	dwTimeout *= 1000 ;				// to msecs
	DWORD dwTimeToStop = GetTickCount() + dwTimeout ;

	for(DWORD dwCurrentTime = GetTickCount() ; dwCurrentTime < dwTimeToStop ; dwCurrentTime = GetTickCount())
	{
		// Wait for chars arrival
		DWORD dwMask ;

		ResetEvent(Chan.Ov.hEvent) ;
		if(!WaitCommEvent(Chan.hPort, &dwMask, &Chan.Ov) && ::GetLastError() != ERROR_IO_PENDING) 
		{
			::PostMessage(hwndMaster, TSM_ERROR, ::GetLastError(), 0) ;
			Sleep(SLEEP_ON_ERROR) ;
			continue ;
		}

		dwResult =  WaitForMultipleObjects(sizeof(Events) / sizeof(Events[0]), 
													Events, FALSE, 
													dwTimeToStop - dwCurrentTime) ;

		if(WAIT_OBJECT_0 != dwResult)
		{
			// if time is over or cancel event is set up escape from the loop
			if(WAIT_OBJECT_0 + 1 == dwResult || WAIT_TIMEOUT == dwResult)
			{
				CancelIo(Chan.hPort) ; break ;
			}

			// another kind of error, sleep for some time and continue from the begining
			Sleep(SLEEP_ON_ERROR) ;
			continue ;
		}
		
		DWORD dwErrors ;
		
		bRslt = GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, FALSE) ;
		if(bRslt)
			bRslt = ClearCommError(Chan.hPort, &dwErrors, &CS) ;

		if(!bRslt)
		{
			::PostMessage(hwndMaster, TSM_ERROR, ::GetLastError(), 0) ;
			Sleep(SLEEP_ON_ERROR) ;
			continue ;
		}

		if(dwErrors)
		{
			LOG_ERROR(_T("ClearCommError, queue purged, error mask"), dwErrors) ;
			::PostMessage(hwndMaster, TSM_ERROR, 0, dwErrors) ;
			PurgeComm(Chan.hPort, PURGE_RXCLEAR) ;
			wRest = 0 ; eRest = RST_EMPTY ;
			continue ;
		}

		if(CS.cbInQue == 0) continue ;

		// Read out all bytes arrived
		
		ResetEvent(Chan.Ov.hEvent) ;
		bRslt = ReadFile(Chan.hPort, Bytes.Get(), CS.cbInQue, &dwBytes, &Chan.Ov) ;

		if(!bRslt)
		{
			bRslt = ::GetLastError() == ERROR_IO_PENDING ;

			if(bRslt)
			{
				dwResult =  WaitForMultipleObjects(sizeof(Events) / sizeof(Events[0]), 
													Events, FALSE, INFINITE) ;
				if(WAIT_OBJECT_0 + 1 == dwResult)
				{
					CancelIo(Chan.hPort) ; break ;
				}

				bRslt = WAIT_OBJECT_0 == dwResult && 
					GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, FALSE) ;
			}
		}
		
		if(!bRslt) 
		{
			::PostMessage(hwndMaster, TSM_ERROR, ::GetLastError(), 0) ;
			PurgeComm(Chan.hPort, PURGE_RXCLEAR) ;
			wRest = 0 ; eRest = RST_EMPTY ;
		}
		else
		{
			// Parse it
			ParseStream(hwndMaster, wRest, eRest, dwBytes, Bytes.Get()) ;
		}
	}
	
	// Send 'P' character to stop survey
	char chSS = CHR_STOP ;

	bRslt = WriteFile(Chan.hPort, &chSS, sizeof(chSS), &dwBytes, &Chan.Ov) ;
	if(!bRslt) 
	{
		bRslt = ERROR_IO_PENDING == ::GetLastError() ;
		if(bRslt)
			GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, TRUE) ;
	}

	return TRUE ;
}

static BOOL Survey_(CHANNEL Chan, HWND hwndMaster, DWORD dwTimeout)
{
	BOOL   bRslt ;
	DWORD  dwResult, dwBytes ;

	// try to allocate buffer
	CAutoPtr<BYTE> Bytes(new BYTE[IO_BUFF]) ;
	bRslt = ! Bytes.IsNull() ;

	// Clean up Comm buffer
	if(bRslt)
	{
		bRslt = PurgeComm(Chan.hPort, PURGE_RXCLEAR | PURGE_RXABORT) ;
	} 
	else
	{
		::SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
	}

//	CAutoTimer Timer(dwTimeout * 1000) ;
//	bRslt = Timer.IsValid() ;

	// Send 'T' character to start survey
	if(bRslt)
	{
		char chSS = CHR_START ;

		bRslt = WriteFile(Chan.hPort, &chSS, sizeof(chSS), &dwBytes, & Chan.Ov) ;
		bRslt = ERROR_SUCCESS ;
		if(!bRslt) 
		{
			bRslt = ERROR_IO_PENDING == ::GetLastError() ;
			if(bRslt)
				bRslt = GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, TRUE) ;
		}
	}

	if(!bRslt) return FALSE ;

	// operate until time is over
	COMSTAT CS ;
	CRest Rest = { RST_EMPTY, 0, 0 } ;

	HANDLE Events[] = { Chan.Ov.hEvent, Chan.hCancel } ;

	dwTimeout *= 1000 ;				// to msecs
	DWORD dwTimeToStop = GetTickCount() + dwTimeout ;

	for(DWORD dwCurrentTime = GetTickCount() ; dwCurrentTime < dwTimeToStop ; dwCurrentTime = GetTickCount())
	{
		// Wait for chars arrival
		DWORD dwMask ;

		ResetEvent(Chan.Ov.hEvent) ;
		if(!WaitCommEvent(Chan.hPort, &dwMask, &Chan.Ov) && ::GetLastError() != ERROR_IO_PENDING) 
		{
			::PostMessage(hwndMaster, TSM_ERROR, ::GetLastError(), 0) ;
			Sleep(SLEEP_ON_ERROR) ;
			continue ;
		}

		dwResult =  WaitForMultipleObjects(sizeof(Events) / sizeof(Events[0]), 
													Events, FALSE, 
													dwTimeToStop - dwCurrentTime) ;

		if(WAIT_OBJECT_0 != dwResult)
		{
			// if time is over or cancel event is set up escape from the loop
			if(WAIT_OBJECT_0 + 1 == dwResult || WAIT_TIMEOUT == dwResult)
			{
				CancelIo(Chan.hPort) ; break ;
			}

			// another kind of error, sleep for some time and continue from the begining
			Sleep(SLEEP_ON_ERROR) ;
			continue ;
		}
		
		DWORD dwErrors ;
		
		bRslt = GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, FALSE) ;
		if(bRslt)
			bRslt = ClearCommError(Chan.hPort, &dwErrors, &CS) ;

		if(!bRslt)
		{
			::PostMessage(hwndMaster, TSM_ERROR, ::GetLastError(), 0) ;
			Sleep(SLEEP_ON_ERROR) ;
			continue ;
		}

		if(dwErrors)
		{
			LOG_ERROR(_T("ClearCommError, queue purged, error mask"), dwErrors) ;
			::PostMessage(hwndMaster, TSM_ERROR, 0, dwErrors) ;
			PurgeComm(Chan.hPort, PURGE_RXCLEAR) ;
			Rest.eType = RST_EMPTY ;
			continue ;
		}

		if(CS.cbInQue == 0) continue ;

		// Read out all bytes arrived
		
		ResetEvent(Chan.Ov.hEvent) ;
		bRslt = ReadFile(Chan.hPort, Bytes.Get(), CS.cbInQue, &dwBytes, &Chan.Ov) ;

		if(!bRslt)
		{
			bRslt = ::GetLastError() == ERROR_IO_PENDING ;

			if(bRslt)
			{
				dwResult =  WaitForMultipleObjects(sizeof(Events) / sizeof(Events[0]), 
													Events, FALSE, INFINITE) ;
				if(WAIT_OBJECT_0 + 1 == dwResult)
				{
					CancelIo(Chan.hPort) ; break ;
				}

				bRslt = WAIT_OBJECT_0 == dwResult && 
					GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, FALSE) ;
			}
		}
		
		if(!bRslt) 
		{
			::PostMessage(hwndMaster, TSM_ERROR, ::GetLastError(), 0) ;
			PurgeComm(Chan.hPort, PURGE_RXCLEAR) ;
			Rest.eType = RST_EMPTY ;
		}
		else
		{
			// Parse it
			ParseStream_(hwndMaster, Rest, dwBytes, Bytes.Get()) ;
		}
	}
	
	// Send 'P' character to stop survey
	char chSS = CHR_STOP ;

	bRslt = WriteFile(Chan.hPort, &chSS, sizeof(chSS), &dwBytes, &Chan.Ov) ;
	if(!bRslt) 
	{
		bRslt = ERROR_IO_PENDING == ::GetLastError() ;
		if(bRslt)
			GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, TRUE) ;
	}

	return TRUE ;
}

static BOOL MotionPerform(CHANNEL Chan, DWORD dwDelay, DWORD dwTicks)
{
	BOOL   bRslt = TRUE ;
	HANDLE Events[] = { Chan.Ov.hEvent, Chan.hCancel } ;
	DWORD  dwBytes ;
	char   chSS = CHR_STEP ;

	for(DWORD i = 0 ; i < dwTicks ; ++ i)
	{
		bRslt = WriteFile(Chan.hPort, &chSS, sizeof(chSS), &dwBytes, &Chan.Ov) ;
		if(!bRslt) 
		{
			bRslt = ERROR_IO_PENDING == ::GetLastError() ;
			if(bRslt)
				bRslt = GetOverlappedResult(Chan.hPort, &Chan.Ov, &dwBytes, TRUE) ;
		}

		if(!bRslt || WAIT_OBJECT_0 == WaitForSingleObject(Chan.hCancel, dwDelay))
			break ;
	}
	
	return bRslt ;
}

#define TWENTYFOUR_HOURS			(26 * 60 * 60)

DWORD WINAPI SerialTask(LPVOID pArgs)
{
	TASKARGS *pTskArgs = (TASKARGS*)pArgs ;
	BOOL bRslt ;

	CAutoHandle OvEvent ;
	CAutoHandle Port(OpenPort(pTskArgs -> SerCfg)) ;
	bRslt = Port.IsValid() ;

	if(bRslt)
	{
		OvEvent = CreateEvent(NULL, TRUE, FALSE, NULL) ;
		bRslt = OvEvent.IsValid() ;
	}

	if(!bRslt) 
	{
		::PostMessage(pTskArgs -> hMaster, TSM_EXIT, ::GetLastError(), 0) ;
		return 0 ;
	}

	CHANNEL Channel = { 0 } ; 

	Channel.Ov.hEvent = OvEvent ;
	Channel.hPort = Port ;
	Channel.hCancel = pTskArgs -> hCancel ;

	LOG_OPEN(_T("wDETECT.log")) ;
	LOG_STRING(_T("SerialTask is invoked, Log opened")) ;
	
	bRslt = Survey_(Channel, pTskArgs -> hMaster, TWENTYFOUR_HOURS) ;

	if(bRslt)
		// Save the histo 
		::PostMessage(pTskArgs -> hMaster, TSM_SAVE, 0, 0) ;

	ATLASSERT(::IsWindow(pTskArgs -> hMaster)) ;
	::PostMessage(pTskArgs -> hMaster,
		TSM_EXIT,
		bRslt ? ERROR_SUCCESS : ::GetLastError(),
		0) ;

	LOG_CLOSE() ;

	return 0 ;
}

DWORD WINAPI PacketTask(LPVOID pArgs)
{
	PACKETARGS *pTskArgs = (PACKETARGS*)pArgs ;
	BOOL bRslt, bCancel = FALSE ;

	CAutoHandle OvEvent ;
	CAutoHandle Port(OpenPort(pTskArgs -> SerCfg)) ;
	bRslt = Port.IsValid() ;

	if(bRslt)
	{
		OvEvent = CreateEvent(NULL, TRUE, FALSE, NULL) ;
		bRslt = OvEvent.IsValid() ;
	}

	if(!bRslt) 
	{
		::PostMessage(pTskArgs -> hMaster, TSM_EXIT, ::GetLastError(), 0) ;
		return 0 ;
	}

	CHANNEL Channel = { 0 } ; 

	Channel.Ov.hEvent = OvEvent ;
	Channel.hPort = Port ;
	Channel.hCancel = pTskArgs -> hCancel ;

	LOG_OPEN(_T("wDETECT.log")) ;
	LOG_STRING(_T("PacketTask is invoked, Log opened")) ;

	DWORD dwCount = 0 ;

	do 
	{
		bRslt = Survey_(Channel, pTskArgs -> hMaster, pTskArgs -> dwExpos) ;
		if(bRslt)
		{
			// Save the histo 
			::PostMessage(pTskArgs -> hMaster, TSM_SAVE, ++ dwCount, 0) ;

			bCancel = WAIT_OBJECT_0 == WaitForSingleObject(pTskArgs -> hCancel, 0) ;
			if(!bCancel)
			{
				::PostMessage(pTskArgs -> hMaster, TSM_MOTION, TRUE, 0) ;
				bRslt = MotionPerform(Channel, pTskArgs -> dwDelay, pTskArgs -> dwTicks) ;
				bCancel = WAIT_OBJECT_0 == WaitForSingleObject(pTskArgs -> hCancel, 0) ;
				::PostMessage(pTskArgs -> hMaster, TSM_MOTION, FALSE, 0) ;
			}
		}
	} while(bRslt && !bCancel) ;

	::PostMessage(pTskArgs -> hMaster,
		TSM_EXIT,
		bRslt ? ERROR_SUCCESS : ::GetLastError(),
		0) ;

	LOG_CLOSE() ;

	return 0 ;
}

