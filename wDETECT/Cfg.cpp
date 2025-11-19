#include "stdafx.h"
#include "resource.h"
#include "autores.h"

#include "Cfg.h"

//////////////////////////////////////////////////////////////////////////
// CCfgSerial
//////////////////////////////////////////////////////////////////////////

DWORD CCfgSerial::m_dwBaud[nBaud] = 
{
	CBR_4800, CBR_9600, CBR_14400, CBR_19200, CBR_38400, CBR_56000, CBR_57600, CBR_115200
} ;

BYTE CCfgSerial::m_byParity[nParity] =
{
	NOPARITY, ODDPARITY, EVENPARITY, MARKPARITY, SPACEPARITY
} ;

BYTE CCfgSerial::m_byStops[nStops] =
{
	ONESTOPBIT, ONE5STOPBITS, TWOSTOPBITS 
} ;

LPCTSTR CCfgSerial::m_pcTokens[nTokens] =
{
	_T("Port"), _T("BaudRate"), _T("Parity"), _T("StopBits")
} ;

INT CCfgSerial::GetNumberOf(UINT uIdx) const throw() 
{
	ATLASSERT(uIdx < Count()) ;
	if(uIdx < Count()) 
		return (INT) m_Numbers[uIdx] ;
	else
		return -1 ;
}

BOOL CCfgSerial::GetNameOf(UINT uIdx, CString &PortName) const throw()
{
	ATLASSERT(uIdx < Count()) ;
	if(uIdx < Count()) 
		return PortName.Format(_T(" COM%u"), m_Numbers[uIdx]) ;
	else
		return FALSE ;
}

void CCfgSerial::_SetDefaults()
{
	m_uSelected = 0 ;
	m_uBaudIdx = nBaud - 1 ;
	m_uParityIdx = 0 ;
	m_uStopsIdx = 0 ;
}

static BOOL LoadSingleLine(HANDLE hFile, LPTSTR pBuf, DWORD dwBufSz)
{
	BOOL bRslt ;
	DWORD dwBytes ;
	DWORD i = 0 ;

	ATLASSERT(dwBufSz && pBuf) ;

	do {
		bRslt = ReadFile(hFile, pBuf + i, sizeof(*pBuf), &dwBytes, NULL) ;
		if(dwBytes != sizeof(*pBuf)) break ;
	} while(! isprint(pBuf[i])) ;
	
	if(!bRslt) return bRslt ;
	if(dwBytes != sizeof(*pBuf)) // EoF ?
	{
		pBuf[i] = 0 ;
		return TRUE ;
	}

	for(i += isprint(pBuf[i]) != 0 ; i < dwBufSz ; ++ i)
	{
		bRslt = ReadFile(hFile, pBuf + i, sizeof(*pBuf), &dwBytes, NULL) ;
		if(dwBytes != sizeof(*pBuf)) break ;

		if(pBuf[i] == _T('\r')) break ;
	}

	if(dwBytes == sizeof(*pBuf))
	{
		if(i == dwBufSz)
		{
			::SetLastError(ERROR_NOT_ENOUGH_MEMORY) ;
			bRslt = FALSE ;
		}
		else
		{
			pBuf[i] = 0 ; // End Of Line
		}
	}
	else
	{
		if(bRslt) pBuf[i] = 0 ; // EoF, add EoL
	}

	return TRUE ;
}

#define STD_BUFSIZE		(2 * MAX_PATH + 1)

BOOL CCfgSerial::_LoadFromFile(HANDLE hFile)
{
	BOOL    bRslt = TRUE ;
	LPTSTR  pBuf = (LPTSTR)_alloca(sizeof(TCHAR) * STD_BUFSIZE) ;
	DWORD   dwMask = 0 ;
	CString Token, Val, Name ;

	for(int i = 0 ; i < nTokens && bRslt ; ++ i)
	{
		bRslt = LoadSingleLine(hFile, pBuf, STD_BUFSIZE) ;
		if(!bRslt) break ;

		Token = pBuf ;
		int nPos = Token.Find(_T('=')) ;
		if(nPos == -1)
		{
			::SetLastError(ERROR_INVALID_DATA) ; break ;
		}

		Token.Delete(nPos, STD_BUFSIZE) ;
		Val = pBuf + nPos + 1 ;
		Val.TrimLeft() ; Val.TrimRight() ;

		if(Token == m_pcTokens[tkPort])
		{
			if(dwMask & mskPort) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			if(Val.Find(_T("COM")) != 0)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			int nPort = atoi(LPCTSTR(Val) + 3) ;
			if(nPort < 1)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_uSelected = (UINT)(nPort) ;
			nPort = m_Numbers.Find(m_uSelected) ;
			if(nPort < 0) nPort = 0 ;
			m_uSelected = (UINT)nPort ;

			dwMask |= mskPort ;
			continue ;
		}

		if(Token == m_pcTokens[tkBaud])
		{
			if(dwMask & mskBaud) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			for(m_uBaudIdx = 0 ; m_uBaudIdx < nBaud ; ++ m_uBaudIdx)
			{
				NameOfBaud(m_uBaudIdx, Name) ; Name.TrimLeft() ; Name.TrimRight() ;
				if(Val == Name) break ;
			}

			if(m_uBaudIdx == nBaud)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			dwMask |= mskBaud ;
			continue ;
		}

		if(Token == m_pcTokens[tkParity])
		{
			if(dwMask & mskParity) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			for(m_uParityIdx = 0 ; m_uParityIdx < nParity ; ++ m_uParityIdx)
			{
				NameOfParity(m_uParityIdx, Name) ; Name.TrimLeft() ; Name.TrimRight() ;
				if(Val == Name) break ;
			}

			if(m_uParityIdx == nParity)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			dwMask |= mskParity	;
			continue ;
		}

		if(Token == m_pcTokens[tkStops])
		{
			if(dwMask & mskStops) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			for(m_uStopsIdx = 0 ; m_uStopsIdx < nParity ; ++ m_uStopsIdx)
			{
				NameOfStops(m_uStopsIdx, Name) ; Name.TrimLeft() ; Name.TrimRight() ;
				if(Val == Name) break ;
			}

			if(m_uStopsIdx == nStops)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			dwMask |= mskStops	;
			continue ;
		}

		bRslt = FALSE ;
		::SetLastError(ERROR_INVALID_DATA) ;
	}

	return bRslt ;
}

BOOL CCfgSerial::_SaveToFile(HANDLE hFile)
{
	CString Str ;
	DWORD dwBytes ;

	CString Name ;

	GetNameOf(m_uSelected, Name) ; Name.TrimLeft() ; Name.TrimRight() ;
	Str.Format(_T("%s=%s\r\n"), m_pcTokens[tkPort], LPCTSTR(Name)) ;
	if(!WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	NameOfBaud(m_uBaudIdx, Name) ; Name.TrimLeft() ; Name.TrimRight() ;
	Str.Format(_T("%s=%s\r\n"), m_pcTokens[tkBaud], LPCTSTR(Name)) ;
	if(!WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	NameOfParity(m_uParityIdx, Name) ; Name.TrimLeft() ; Name.TrimRight() ;
	Str.Format(_T("%s=%s\r\n"), m_pcTokens[tkParity], LPCTSTR(Name)) ;
	if(!WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	NameOfStops(m_uStopsIdx, Name) ; Name.TrimLeft() ; Name.TrimRight() ;
	Str.Format(_T("%s=%s\r\n"), m_pcTokens[tkStops], LPCTSTR(Name)) ;
	if(!WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	return TRUE ;
}

BOOL CCfgSerial::EnumerateSerialPorts() throw() 
{
	BOOL bRslt ;
	DWORD dwNeeded, dwReturned ;

	// Ask for space needed
	bRslt = EnumPorts(NULL, 1, NULL, 0, &dwNeeded, &dwReturned) ;
	if(!bRslt && GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
		return bRslt ;

	PORT_INFO_1 *pInfo = (PORT_INFO_1*)_alloca(dwNeeded) ;

	// Retrieve info
	DWORD dwTmp ;
	bRslt = EnumPorts(NULL, 1, (LPBYTE)pInfo, dwNeeded, &dwTmp, &dwReturned) ;
	if(!bRslt) return bRslt ;

	CString PortName ;
	// Enumerate all serial ports
	for (DWORD i = 0 ; i < dwReturned ; ++ i)
	{
		PortName = pInfo[i].pName ;
		if(-1 == PortName.Find(_T("COM"),0)) continue ;

		PortName.SetAt(PortName.GetLength() - 1, 0) ;		// Remove ':'

		COMMCONFIG CC = { 0 } ;
		CC.dwSize = dwNeeded = sizeof(COMMCONFIG) ;

		bRslt = GetDefaultCommConfig(PortName, &CC, &dwNeeded) ;
		if(bRslt)
		{
			UINT uTmp = atoi((LPCTSTR)PortName + 3) ;
			bRslt = m_Numbers.Add(uTmp) ;
			if(!bRslt) break ;
		}
	}

	bRslt = dwReturned == i ;

	if(!bRslt)
		ErrorMsgBox(NULL, ::GetLastError(), IDS_CAPERROR) ;
	return bRslt ;
}

BOOL CCfgSerial::NameOfBaud(UINT uIdx, CString &BaudName) const throw()
{
	ATLASSERT(uIdx < nBaud) ;
	if(uIdx >= nBaud) return FALSE ;

	BaudName.Format(_T(" %u"), m_dwBaud[uIdx]) ;
	return TRUE ;
}

BOOL CCfgSerial::NameOfParity(UINT uIdx, CString &ParityName) const throw()
{
	ATLASSERT(uIdx < nBaud) ;
	if(uIdx >= nBaud) return FALSE ;

	return 0 != ParityName.LoadString(IDS_PARITYNO + uIdx) ;
}

BOOL CCfgSerial::NameOfStops(UINT uIdx, CString &StopsName) const throw()
{
	ATLASSERT(uIdx < nBaud) ;
	if(uIdx >= nBaud) return FALSE ;

	return 0 != StopsName.LoadString(IDS_STOPONE + uIdx) ;
}

void CCfgSerial::GetPortCfg(PORTCFG &Cfg)
{
	ATLASSERT(m_uSelected < (UINT) m_Numbers.GetSize() && m_uBaudIdx < nBaud 
				&& m_uParityIdx < nParity && m_uStopsIdx < nStops) ;

	Cfg.wSerial = (WORD) m_Numbers[m_uSelected] ;
	Cfg.dwBaud = m_dwBaud[m_uBaudIdx] ;
	Cfg.byParity = m_byParity[m_uParityIdx] ;
	Cfg.byStops = m_byStops[m_uStopsIdx] ;
}

//////////////////////////////////////////////////////////////////////////
// Miscellaneous
//////////////////////////////////////////////////////////////////////////

LPCTSTR CCfgMisc::m_pcTokens[nTokens] =
{
	_T("RefrRate"), _T("MotorDelay")
} ;

BOOL CCfgMisc::_LoadFromFile(HANDLE hFile)
{
	BOOL    bRslt = TRUE ;
	LPTSTR  pBuf = (LPTSTR)_alloca(sizeof(TCHAR) * STD_BUFSIZE) ;
	DWORD   dwMask = 0 ;
	CString Token, Val ;

	for(int i = 0 ; i < nTokens && bRslt ; ++ i)
	{
		bRslt = LoadSingleLine(hFile, pBuf, STD_BUFSIZE) ;
		if(!bRslt) break ;

		Token = pBuf ;
		int nPos = Token.Find(_T('=')) ;
		if(nPos == -1)
		{
			::SetLastError(ERROR_INVALID_DATA) ; break ;
		}

		Token.Delete(nPos, STD_BUFSIZE) ;
		Val = pBuf + nPos + 1 ;
		Val.TrimLeft() ; Val.TrimRight() ;

		if(Token == m_pcTokens[tkMSecs])
		{
			if(dwMask & mskMSecs) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_dwMSecs = atoi(Val) ;
			if(m_dwMSecs == 0)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			dwMask |= mskMSecs ;
			continue ;
		}		

		if(Token == m_pcTokens[tkDelay])
		{
			if(dwMask & mskDelay) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_dwDelay = atoi(Val) ;
			if(m_dwDelay == 0)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			dwMask |= mskDelay ;
			continue ;
		}		

		bRslt = FALSE ;
		::SetLastError(ERROR_INVALID_DATA) ;
	}

	return bRslt ;
}

BOOL CCfgMisc::_SaveToFile(HANDLE hFile)
{
	CString Str ;
	DWORD   dwBytes ;
	BOOL    bRslt ;

	Str.Format(_T("%s=%u\r\n"), m_pcTokens[tkMSecs], m_dwMSecs) ;
	bRslt = WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL) ;

	if(bRslt)
	{
		Str.Format(_T("%s=%u\r\n"), m_pcTokens[tkDelay], m_dwDelay) ;
		bRslt = WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL) ;
	}

	return bRslt ;
}

//////////////////////////////////////////////////////////////////////////
// CCfgDoc
//////////////////////////////////////////////////////////////////////////

LPCTSTR CCfgDoc::m_pcTokens[nTokens] =
{
	_T("DocPath"), _T("PacketPath"), _T("Exposition"), _T("TicksToGo")
} ;

void CCfgDoc::_SetDefaults()
{ 
	m_FileName = _T("c:\\arco.txt") ; 
	m_FilePacket = _T("c:\\packet.txt") ;
	m_dwExpos = 300 ;
	m_dwTicks = 200 ;
}

BOOL CCfgDoc::_LoadFromFile(HANDLE hFile)
{
	BOOL    bRslt = TRUE ;
	LPTSTR  pBuf = (LPTSTR)_alloca(sizeof(TCHAR) * STD_BUFSIZE) ;
	DWORD   dwMask = 0 ;
	CString Token, Val ;

	for(int i = 0 ; i < nTokens && bRslt ; ++ i)
	{
		bRslt = LoadSingleLine(hFile, pBuf, STD_BUFSIZE) ;
		if(!bRslt) break ;

		Token = pBuf ;
		int nPos = Token.Find(_T('=')) ;
		if(nPos == -1)
		{
			::SetLastError(ERROR_INVALID_DATA) ; break ;
		}

		Token.Delete(nPos, STD_BUFSIZE) ;
		Val = pBuf + nPos + 1 ;
		Val.TrimLeft() ; Val.TrimRight() ;

		if(Token == m_pcTokens[tkPath])
		{
			if(dwMask & mskPath) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_FileName = Val ;

			dwMask |= mskPath ;
			continue ;
		}		

		if(Token == m_pcTokens[tkPathPkt])
		{
			if(dwMask & mskPathPkt) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_FilePacket = Val ;

			dwMask |= mskPathPkt ;
			continue ;
		}		

		if(Token == m_pcTokens[tkExpos])
		{
			if(dwMask & mskExpos) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_dwExpos = atoi(Val) ;
			if(m_dwExpos == 0)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			dwMask |= mskExpos ;
			continue ;
		}		

		if(Token == m_pcTokens[tkTicks])
		{
			if(dwMask & mskTicks) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_dwTicks = atoi(Val) ;
			if(m_dwTicks == 0)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			dwMask |= mskTicks ;
			continue ;
		}		

		bRslt = FALSE ;
		::SetLastError(ERROR_INVALID_DATA) ;
	}

	return bRslt ;
}

BOOL CCfgDoc::_SaveToFile(HANDLE hFile)
{
	CString Str ;
	DWORD   dwBytes ;
	BOOL    bRslt ;

	Str.Format(_T("%s=%s\r\n"), m_pcTokens[tkPath], m_FileName) ;
	if(!WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	Str.Format(_T("%s=%s\r\n"), m_pcTokens[tkPathPkt], m_FilePacket) ;
	if(!WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	Str.Format(_T("%s=%u\r\n"), m_pcTokens[tkExpos], m_dwExpos) ;
	if(!WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	Str.Format(_T("%s=%u\r\n"), m_pcTokens[tkTicks], m_dwTicks) ;
	bRslt = WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL) ;

	return bRslt ;
}

//////////////////////////////////////////////////////////////////////////
// CGUI
//////////////////////////////////////////////////////////////////////////

LPCTSTR CGUI::m_pcTokens[nTokens] =
{
	_T("WndOrigin")
} ;

BOOL CGUI::_LoadFromFile(HANDLE hFile)
{
	BOOL    bRslt = TRUE ;
	LPTSTR  pBuf = (LPTSTR)_alloca(sizeof(TCHAR) * STD_BUFSIZE) ;
	DWORD   dwMask = 0 ;
	CString Token, Val ;

	for(int i = 0 ; i < nTokens && bRslt ; ++ i)
	{
		bRslt = LoadSingleLine(hFile, pBuf, STD_BUFSIZE) ;
		if(!bRslt) break ;

		Token = pBuf ;
		int nPos = Token.Find(_T('=')) ;
		if(nPos == -1)
		{
			::SetLastError(ERROR_INVALID_DATA) ; break ;
		}

		Token.Delete(nPos, STD_BUFSIZE) ;
		Val = pBuf + nPos + 1 ;
		Val.TrimLeft() ; Val.TrimRight() ;

		if(Token == m_pcTokens[tkOrigin])
		{
			if(dwMask & mskOrigin) 
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_ptOrigin.x = atoi(Val) ;

			nPos = Val.Find(_T(',')) ;
			if(nPos == -1)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			m_ptOrigin.y = atoi(LPCTSTR(Val) + nPos + 1) ;

			dwMask |= mskOrigin ;
			continue ;
		}		

		bRslt = FALSE ;
		::SetLastError(ERROR_INVALID_DATA) ;
	}

	return bRslt ;
}

BOOL CGUI::_SaveToFile(HANDLE hFile)
{
	CString Str ;
	DWORD   dwBytes ;
	BOOL    bRslt ;

	Str.Format(_T("%s=%d,%d\r\n"), m_pcTokens[tkOrigin], m_ptOrigin.x, m_ptOrigin.y) ;
	bRslt = WriteFile(hFile, LPCSTR(Str), Str.GetLength(), &dwBytes, NULL) ;

	return bRslt ;
}

static int CALLBACK FindFont(const LOGFONT *pLF, const TEXTMETRIC *pTM, DWORD dwFType, LPARAM lParam)
{
	LPCTSTR pFontName = (LPCTSTR)lParam ;
	if(lstrcmp(pFontName, pLF -> lfFaceName) == 0) return FALSE ;
	return TRUE ;
}

BOOL CGUI::InitFonts()
{
	CLogFont LogFnt((HFONT)::GetStockObject(DEFAULT_GUI_FONT)) ;
	LOGFONT  LF = { 0 } ;
	LF.lfCharSet = LogFnt.lfCharSet ;

	CString FaceName(_T("Courier New")) ; 
	CWindowDC dc(NULL) ;
	if(TRUE == EnumFontFamiliesEx(dc, &LF, &FindFont, (LPARAM)(LPCTSTR)FaceName, 0))
		m_FntMono = (HFONT)::GetStockObject(SYSTEM_FIXED_FONT) ;
	else
	{
		LogFnt.lfWeight = FW_BOLD ;
		LogFnt.lfPitchAndFamily = FF_MODERN | FIXED_PITCH ;
		lstrcpy(LogFnt.lfFaceName, (LPCTSTR)FaceName) ;
		
		m_FntMono = LogFnt.CreateFontIndirect() ;
		if(NULL == m_FntMono.m_hFont)
			m_FntMono = (HFONT)::GetStockObject(SYSTEM_FIXED_FONT) ;
	}

	m_FntGUI = (HFONT)::GetStockObject(DEFAULT_GUI_FONT) ;
	LogFnt = m_FntGUI ;

	BOOL bRslt ;

	FaceName = _T("Microsoft Sans Serif") ;
	bRslt = EnumFontFamiliesEx(dc, &LF, &FindFont, (LPARAM)(LPCTSTR)FaceName, 0) ;
//	if(bRslt)
//	{
//		FaceName = _T("MS Sans Serif") ;
//		bRslt = EnumFontFamiliesEx(dc, &LF, &FindFont, (LPARAM)(LPCTSTR)FaceName, 0) ;
//	}

	if(!bRslt)
	{
		lstrcpy(LogFnt.lfFaceName, (LPCTSTR)FaceName) ;
	}

	LogFnt.lfWeight = FW_BOLD ;
	HFONT hFntGUI = LogFnt.CreateFontIndirect() ;
	if(NULL != hFntGUI)	m_FntGUI = hFntGUI ;
/*
	if(!bRslt) 
	{
		LogFnt.lfWeight = FW_BOLD ;
		lstrcpy(LogFnt.lfFaceName, (LPCTSTR)FaceName) ;
		HFONT hFntGUI = LogFnt.CreateFontIndirect() ;
		if(NULL != hFntGUI)	m_FntGUI = hFntGUI ;
	}
*/

	return TRUE ;
}

//////////////////////////////////////////////////////////////////////////
// CCfg
//////////////////////////////////////////////////////////////////////////

LPCTSTR CConfig::m_pcTokens[nTokens] =
{
	_T("[SERIAL]"), _T("[MISCELLANEOUS]"), _T("[GUI]"), _T("[DOCUMENTS]") 
} ;

BOOL CConfig::_Init()
{
	BOOL bRslt ;

	// Make ini path 
	bRslt = MakeIniPath() ;

	bRslt = m_Doc.Init() && m_GUI.Init() && m_Misc.Init() && m_Serial.Init() ;

	return bRslt ;
}

void CConfig::_SetDefaults()
{
	m_Doc.SetDefaults() ;
	m_GUI.SetDefaults() ;
	m_Misc.SetDefaults() ;
	m_Serial.SetDefaults() ;
}

BOOL CConfig::_LoadFromFile(HANDLE hFile)
{
	BOOL    bRslt = TRUE ;
	LPTSTR  pBuf = (LPTSTR)_alloca(sizeof(TCHAR) * STD_BUFSIZE) ;
	DWORD   dwMask = 0 ;
	CString Token ;
	
	for(int i = 0 ; i < nTokens && bRslt ; ++ i)
	{
		bRslt = LoadSingleLine(hFile, pBuf, STD_BUFSIZE) ;
		if(!bRslt) break ;

		Token = pBuf ;

		if(Token == m_pcTokens[tkSerial])
		{
			if(dwMask & mskSerial)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			bRslt = m_Serial.LoadFromFile(hFile) ;
			dwMask |= mskSerial ;
			continue ;
		}

		if(Token == m_pcTokens[tkMisc])
		{
			if(dwMask & mskMisc)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			bRslt = m_Misc.LoadFromFile(hFile) ;
			dwMask |= mskMisc ;
			continue ;
		}

		if(Token == m_pcTokens[tkDoc])
		{
			if(dwMask & mskDoc)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			bRslt = m_Doc.LoadFromFile(hFile) ;
			dwMask |= mskDoc ;
			continue ;
		}

		if(Token == m_pcTokens[tkGUI])
		{
			if(dwMask & mskGUI)
			{
				bRslt = FALSE ;
				::SetLastError(ERROR_INVALID_DATA) ; break ;
			}

			bRslt = m_GUI.LoadFromFile(hFile) ;
			dwMask |= mskDoc ;
			continue ;
		}

		bRslt = FALSE ;
		::SetLastError(ERROR_INVALID_DATA) ;
	}

	return bRslt ;
}

BOOL CConfig::_SaveToFile(HANDLE hFile)
{
	BOOL bRslt ;
	CString Section ;
	DWORD dwBytes ;

	Section.Format(_T("%s\r\n"), m_pcTokens[tkSerial]) ;
	bRslt = WriteFile(hFile, LPCSTR(Section), Section.GetLength(), &dwBytes, NULL) ;
	if(bRslt) bRslt = m_Serial.SaveToFile(hFile) ;
	if(!bRslt) return bRslt ;

	Section.Format(_T("%s\r\n"), m_pcTokens[tkMisc]) ;
	bRslt = WriteFile(hFile, LPCSTR(Section), Section.GetLength(), &dwBytes, NULL) ;
	if(bRslt) bRslt = m_Misc.SaveToFile(hFile) ;
	if(!bRslt) return bRslt ;

	Section.Format(_T("%s\r\n"), m_pcTokens[tkDoc]) ;
	bRslt = WriteFile(hFile, LPCSTR(Section), Section.GetLength(), &dwBytes, NULL) ;
	if(bRslt) bRslt = m_Doc.SaveToFile(hFile) ;
	if(!bRslt) return bRslt ;

	Section.Format(_T("%s\r\n"), m_pcTokens[tkGUI]) ;
	bRslt = WriteFile(hFile, LPCSTR(Section), Section.GetLength(), &dwBytes, NULL) ;
	if(bRslt) bRslt = m_GUI.SaveToFile(hFile) ;

	return bRslt ;
}

BOOL CConfig::MakeIniPath()
{
	m_IniPath = GetCommandLine() ;
	int nLast = m_IniPath.Find(_T('"'), 1) ;

	if(-1 == nLast) 
	{
		::SetLastError(ERROR_INVALID_DATA) ;
		return FALSE ;
	}

	m_IniPath.Delete(nLast, MAX_PATH) ;
	m_IniPath.Delete(0) ;

	nLast = m_IniPath.ReverseFind('.') ;

	if(-1 == nLast) 
	{
		::SetLastError(ERROR_INVALID_DATA) ;
		return FALSE ;
	}

	m_IniPath.Delete(++ nLast, MAX_PATH) ;
	m_IniPath += _T("ini") ;

	return TRUE ;
}

BOOL CConfig::LoadConfig()
{
	CAutoHandle File(
		CreateFile(m_IniPath,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL)) ;

	BOOL bRslt = File.IsValid() ;

	if(bRslt)
		bRslt = LoadFromFile(File) ;

	if(!bRslt)
	{
		bRslt = ERROR_FILE_NOT_FOUND == ::GetLastError() ;
		SetDefaults() ;
	}

	return bRslt ;
}

BOOL CConfig::SaveConfig() 
{
	CAutoHandle File(
		CreateFile(m_IniPath,
					GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL,
					NULL)) ;

	BOOL bRslt = File.IsValid() ;
	
	if(bRslt)
		bRslt = SaveToFile(File) ;

	return bRslt ;
}


//////////////////////////////////////////////////////////////////////////

CConfig g_Cfg ;

BOOL ConfigInit()
{
	if(g_Cfg.Init())
	{
		return g_Cfg.LoadConfig() ;
	}
	
	return FALSE ;
}

BOOL CongfigDone()
{
	return g_Cfg.SaveConfig() ;
}

//////////////////////////////////////////////////////////////////////////

