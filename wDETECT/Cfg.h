#if !defined(AFX_CFG_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_)
#define AFX_CFG_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_

const int nChannels = 4096 ;

//////////////////////////////////////////////////////////////////////////

template <class TConfig>
class ATL_NO_VTABLE CConfigImpl 
{
public:
	// Front ends ////////////////////////////////////////////////////////

	BOOL Init()
	{
		TConfig *pConfig = static_cast<TConfig*>(this) ;
		return pConfig -> _Init() ;
	}

	void SetDefaults()
	{
		TConfig *pConfig = static_cast<TConfig*>(this) ;
		pConfig -> _SetDefaults() ;
	}

	BOOL LoadFromFile(HANDLE hFile)
	{
		TConfig *pConfig = static_cast<TConfig*>(this) ;
		return pConfig -> _LoadFromFile(hFile) ;
	}

	BOOL SaveToFile(HANDLE hFile)
	{
		TConfig *pConfig = static_cast<TConfig*>(this) ;
		return pConfig -> _SaveToFile(hFile) ;
	}

	// Overrides /////////////////////////////////////////////////////////

	BOOL _Init()
	{
		return TRUE ;
	}

	void _SetDefaults()
	{
	}

	BOOL _LoadFromFile(HANDLE hFile)
	{
		return TRUE ;
	}

	BOOL _SaveToFile(HANDLE hFile)
	{
		return TRUE ;
	}
} ;

//////////////////////////////////////////////////////////////////////////

struct PORTCFG
{
	WORD  wSerial ;
	DWORD dwBaud ;
	BYTE  byParity ;
	BYTE  byStops ;
} ;

class ATL_NO_VTABLE CCfgSerial : public CConfigImpl<CCfgSerial>
{
public :

	BOOL _Init()
	{
		return EnumerateSerialPorts() ;
	}

	void _SetDefaults() ;
	BOOL _LoadFromFile(HANDLE hFile) ;
	BOOL _SaveToFile(HANDLE hFile) ;

	enum { nBaud = 8, nParity = 5, nStops = 3 } ;

	CCfgSerial() : m_uSelected(0), m_uBaudIdx(0), m_uParityIdx(0), m_uStopsIdx(0)
	{ }

	BOOL EnumerateSerialPorts() throw() ;

	inline UINT Count() const throw() { return m_Numbers.GetSize() ; }
	inline UINT Selected() const throw() { return m_uSelected ; }
	INT GetNumberOf(UINT uIdx) const throw() ;
	BOOL GetNameOf(UINT uIdx, CString &PortName) const throw() ;

	BOOL NameOfBaud(UINT uIdx, CString &BaudName) const throw() ;
	BOOL NameOfParity(UINT uIdx, CString &ParityName) const throw() ;
	BOOL NameOfStops(UINT uIdx, CString &StopsName) const throw() ;

	void GetPortCfg(PORTCFG &Cfg) ;
	
	UINT  m_uSelected ;
	UINT  m_uBaudIdx ;
	UINT  m_uParityIdx ;
	UINT  m_uStopsIdx ;

private :

	enum { nTokens = 4 } ;
	enum Tokens { tkPort = 0, tkBaud = 1, tkParity = 2, tkStops = 3 } ;
	enum TokenMasks { mskPort = 1 << tkPort, mskBaud = 1 << tkBaud, 
						mskParity = 1 << tkParity, mskStops = 1 << tkStops } ;

	CSimpleArray<UINT> m_Numbers ;
	static DWORD m_dwBaud[nBaud] ;
	static BYTE  m_byParity[nParity] ;
	static BYTE  m_byStops[nStops] ;
	static LPCTSTR m_pcTokens[nTokens] ;
};

//////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CCfgMisc : public CConfigImpl<CCfgMisc>
{
public:
	void _SetDefaults() { m_dwMSecs = 1000 ; m_dwDelay = 100 ; }
	BOOL _LoadFromFile(HANDLE hFile) ;
	BOOL _SaveToFile(HANDLE hFile) ;

	CCfgMisc() : m_dwMSecs(1000) {}

	DWORD m_dwMSecs ;
	DWORD m_dwDelay ;

private:
	enum { nTokens = 2 } ;
	enum Tokens { tkMSecs = 0, tkDelay } ;
	enum TokenMasks { mskMSecs = 1 << tkMSecs,  mskDelay = 1 << tkDelay } ;
	static LPCTSTR m_pcTokens[nTokens] ;
};

//////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CGUI : public CConfigImpl<CGUI>
{
public :

	BOOL _Init()
	{
		return InitFonts() ;
	}

	BOOL _LoadFromFile(HANDLE hFile) ;
	BOOL _SaveToFile(HANDLE hFile) ;

	void _SetDefaults() { m_ptOrigin.x = m_ptOrigin.y = CW_USEDEFAULT ; }
//	BOOL _LoadFromFile(HANDLE hFile) ;
//	BOOL _SaveToFile(HANDLE hFile) ;


	CFont m_FntMono ;
	CFont m_FntGUI ;
	CPoint m_ptOrigin ;

	CGUI() : m_FntMono(NULL), m_FntGUI(NULL), m_ptOrigin(CW_USEDEFAULT, CW_USEDEFAULT) {}

	BOOL InitFonts() ;
	
private:
	enum { nTokens = 1 } ;
	enum Tokens { tkOrigin = 0 } ;
	enum TokenMasks { mskOrigin = 1 << tkOrigin } ;
	static LPCTSTR m_pcTokens[nTokens] ;
};

//////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CCfgDoc : public CConfigImpl<CCfgDoc>
{
public :
	void _SetDefaults() ;	
	BOOL _LoadFromFile(HANDLE hFile) ;
	BOOL _SaveToFile(HANDLE hFile) ;

	CString m_FileName ;
	CString m_FilePacket ;
	DWORD   m_dwExpos ;
	DWORD	m_dwTicks ;
	
	CCfgDoc() : m_FileName(_T("c:\\arco.txt")) {}

private:
	enum { nTokens = 4 } ;
	enum Tokens { tkPath = 0, tkPathPkt, tkExpos, tkTicks } ;
	enum TokenMasks { mskPath = 1 << tkPath, mskPathPkt = 1 << tkPathPkt,
						mskExpos = 1 << tkExpos, mskTicks = 1 << tkTicks} ;

	static LPCTSTR m_pcTokens[nTokens] ;
};

//////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CConfig : public CConfigImpl<CConfig>
{
public :

	BOOL _Init() ;
	void _SetDefaults() ;
	BOOL _LoadFromFile(HANDLE) ;
	BOOL _SaveToFile(HANDLE) ;

	CCfgSerial m_Serial ;
	CCfgMisc   m_Misc ;
	CGUI       m_GUI ;
	CCfgDoc    m_Doc ;

	CString    m_IniPath ;

	BOOL MakeIniPath() ;

	BOOL SaveConfig() ;
	BOOL LoadConfig() ;

private:
	enum { nTokens = 4 } ;
	enum Tokens { tkSerial = 0, tkMisc = 1, tkGUI = 2, tkDoc = 3 } ;
	enum TokenMasks { mskSerial = 1 << tkSerial, mskMisc = 1 << tkMisc, 
						mskGUI = 1 << tkGUI, mskDoc = 1 << tkDoc } ;

	static LPCTSTR m_pcTokens[nTokens] ;
};

//////////////////////////////////////////////////////////////////////////

extern CConfig g_Cfg ;

extern BOOL ConfigInit() ;
extern BOOL CongfigDone() ;

#endif // !defined(AFX_CFG_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_)
