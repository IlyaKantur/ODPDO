// aboutdlg.cpp : implementation of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "records.h"

#include "Cfg.h"
#include "validate.h"
#include "aboutdlg.h"

//////////////////////////////////////////////////////////////////////////
// Routines
//////////////////////////////////////////////////////////////////////////

// Change font 
static BOOL CALLBACK ChangeFontProc(HWND hWnd, LPARAM lPar)
{
	::SendMessage(hWnd, WM_SETFONT, (WPARAM)lPar, FALSE) ;
	return TRUE ;
}

static BOOL ChangeFont(HWND hParent, HFONT hFont)
{
	return ::EnumChildWindows(hParent, &ChangeFontProc, (LPARAM)hFont) ;
}

// For Multiline Edit Controls Only ... :0(
static void EditCenterArea(HWND hEdit)
{
	CEdit Edit(hEdit) ;

	CRect Rt, RtC ;
	Edit.GetClientRect(&RtC) ; Edit.GetRect(&Rt) ;

	int nDleta = (RtC.Height() - Rt.Height()) / 2 ;
	Rt.InflateRect( - nDleta, 0) ;
	Rt.MoveToY(nDleta) ;

	Edit.SetRect(&Rt) ;
	Edit.Detach() ;
}

static BOOL CenterEdits(HWND hParent)
{
	HWND hWnd = NULL ;

	for(hWnd = FindWindowEx(hParent, NULL, _T("Edit"), NULL) ;
		NULL != hWnd ;
		hWnd = FindWindowEx(hParent, hWnd, _T("Edit"), NULL))
	{
		EditCenterArea(hWnd) ;
	}

	return TRUE ;
}

// Messages formats //////////////////////////////////////////////////////

void MsgFormatDataFixed(const CValidatorB::VData vData, CString &sMsg)
{
	CString sFormat ;
	sFormat.Format(vData.fixedData.bSgn ? IDS_FIXED : IDS_FIXEDU, 
					vData.fixedData.nFrac, vData.fixedData.nFrac) ;

	if(vData.fixedData.bSgn)
	{
		CFixedPoint<LONGLONG> fpMin(vData.fixedData.nMin, vData.fixedData.nFrac) ;
		CFixedPoint<LONGLONG> fpMax(vData.fixedData.nMax, vData.fixedData.nFrac) ;

		sMsg.Format((LPCTSTR)sFormat, 
			fpMin.WholePart(), fpMin.FracPart(),
			fpMax.WholePart(), fpMax.FracPart()) ;
	}
	else
	{
		CFixedPoint<ULONGLONG> fpMin(vData.fixedData.nMin, vData.fixedData.nFrac) ;
		CFixedPoint<ULONGLONG> fpMax(vData.fixedData.nMax, vData.fixedData.nFrac) ;
	
		sMsg.Format((LPCTSTR)sFormat, 
			fpMin.WholePart(), fpMin.FracPart(),
			fpMax.WholePart(), fpMax.FracPart()) ;
	}
}

//////////////////////////////////////////////////////////////////////////

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	SetFont(g_Cfg.m_GUI.m_FntGUI, FALSE) ;
	ChangeFont(m_hWnd, g_Cfg.m_GUI.m_FntGUI) ;

	HICON hIcon = ::LoadIcon(NULL, IDI_QUESTION) ;
	if(hIcon) SetIcon(hIcon, FALSE) ;

	CenterWindow(GetParent());
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}

//////////////////////////////////////////////////////////////////////////
// Controls
//////////////////////////////////////////////////////////////////////////

class CFixedEdit : public CWindowImpl<CFixedEdit, CEdit>,
					public CFixedPointValidator<CFixedEdit, LONG>
{
public:

	CFixedEdit() : CFixedPointValidator<CFixedEdit, LONG>() {} ;
	CFixedEdit(UINT uFrac, LONG nMin, LONG nMax) :
		CFixedPointValidator<CFixedEdit, LONG>(uFrac, nMin, nMax) {} ;

	typedef CFixedPointValidator<CFixedEdit, LONG> CFPLongValidator_ ;

	BEGIN_MSG_MAP(CFixedEdit)
		CHAIN_MSG_MAP(CFPLongValidator_) ;
	END_MSG_MAP()
} ;

class CUFixedEdit : public CWindowImpl<CUFixedEdit, CEdit>,
					public CFixedPointValidator<CUFixedEdit, ULONG>
{
public:

	CUFixedEdit() : CFixedPointValidator<CUFixedEdit, ULONG>() {} ;
	CUFixedEdit(UINT uFrac, ULONG uMin, ULONG uMax) :
		CFixedPointValidator<CUFixedEdit, ULONG>(uFrac, uMin, uMax) {} ;

	typedef CFixedPointValidator<CUFixedEdit, ULONG> CFPULongValidator_ ;

	BEGIN_MSG_MAP(CUFixedEdit)
		CHAIN_MSG_MAP(CFPULongValidator_) ;
	END_MSG_MAP()

} ;

//////////////////////////////////////////////////////////////////////////
// CSerialDlg
//////////////////////////////////////////////////////////////////////////

class CSerialDlg : public CDialogImpl<CSerialDlg>
{
public:
	enum { IDD = IDD_SERIAL } ;

	BEGIN_MSG_MAP(CSerialDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	BOOL InitControls() ;
	BOOL SetData() ;
	BOOL GetData() ;

	CCfgSerial *m_pSerial ;
	CIcon       m_Icon ;
} ;

LRESULT CSerialDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	m_pSerial = (CCfgSerial*)lParam ;
	if(NULL == m_pSerial)
	{
		EndDialog(IDCANCEL) ;
		return TRUE ;
	}

	SetFont(g_Cfg.m_GUI.m_FntGUI, FALSE) ;
	ChangeFont(m_hWnd, g_Cfg.m_GUI.m_FntGUI) ;

	if(NULL != m_Icon.LoadIcon(IDI_SERIAL))
	{
		SetIcon(m_Icon, TRUE) ;
	}

 	CenterWindow(GetParent()) ;

	if(!InitControls())
	{
		EndDialog(IDCANCEL) ;
		return TRUE ;
	}

	SetData() ;

	return TRUE;
}

LRESULT CSerialDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(IDOK == wID) GetData() ;
	EndDialog(wID) ;
	return 0;
}

BOOL CSerialDlg::InitControls()
{
	UINT i ;
	CString Name ;
	CComboBox Box ; 

	Box.Attach(GetDlgItem(IDC_COMBOPORT)) ;
	for(i = 0 ; i < m_pSerial-> Count() ; ++ i)
	{
		m_pSerial -> GetNameOf(i, Name) ;
		Box.AddString(Name) ;
	}
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOBAUD)) ;
	for(i = 0 ; i < m_pSerial -> nBaud ; ++ i)
	{
		m_pSerial -> NameOfBaud(i, Name) ;
		Box.AddString(Name) ;
	}
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOPARITY)) ;
	for(i = 0 ; i < m_pSerial -> nParity ; ++ i)
	{
		m_pSerial -> NameOfParity(i, Name) ;
		Box.AddString(Name) ;
	}
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOSTOPS)) ;
	for(i = 0 ; i < m_pSerial -> nStops ; ++ i)
	{
		m_pSerial -> NameOfStops(i, Name) ;
		Box.AddString(Name) ;
	}
	Box.Detach() ;

	return TRUE ;
}

BOOL CSerialDlg::SetData()
{
	CComboBox Box ; 

	Box.Attach(GetDlgItem(IDC_COMBOPORT)) ;
	Box.SetCurSel(m_pSerial -> m_uSelected) ;
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOBAUD)) ;
	Box.SetCurSel(m_pSerial -> m_uBaudIdx) ;
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOPARITY)) ;
	Box.SetCurSel(m_pSerial -> m_uParityIdx) ;
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOSTOPS)) ;
	Box.SetCurSel(m_pSerial -> m_uStopsIdx) ;
	Box.Detach() ;

	return TRUE ;
}

BOOL CSerialDlg::GetData()
{
	CComboBox Box ; 

	Box.Attach(GetDlgItem(IDC_COMBOPORT)) ;
	m_pSerial -> m_uSelected = Box.GetCurSel() ;
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOBAUD)) ;
	m_pSerial -> m_uBaudIdx = Box.GetCurSel() ;
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOPARITY)) ;
	m_pSerial -> m_uParityIdx = Box.GetCurSel() ;
	Box.Detach() ;

	Box.Attach(GetDlgItem(IDC_COMBOSTOPS)) ;
	m_pSerial -> m_uStopsIdx = Box.GetCurSel() ;
	Box.Detach() ;

	return TRUE ;
}

//////////////////////////////////////////////////////////////////////////
// Interface function:

int ShowSerialDlg(CCfgSerial *pSerialCfg)
{
	CSerialDlg dlg ;
	return dlg.DoModal(::GetActiveWindow(), (LPARAM)pSerialCfg) ;
}

//////////////////////////////////////////////////////////////////////////
// CMiscDlg
//////////////////////////////////////////////////////////////////////////

class CMiscDlg : public CDialogImpl<CMiscDlg>,
				public CFltWinDataExchange<CMiscDlg>
{
public:
	enum { IDD = IDD_MISC } ;

	CMiscDlg() : m_pMisc(NULL), m_dwRefresh(NULL), m_wndRefresh(3, 10, 10000), m_wndDelay(0, 20, 200)
	{
		INIT_RESETFOCUS_MAP() ;		
	}

	BEGIN_MSG_MAP(CMiscDlg)
		POSTINVALID_HANDLER()
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CMiscDlg)
		DDX_CONTROL(IDC_EREFRESH, m_wndRefresh)
		DDX_CONTROL(IDC_EDELAY, m_wndDelay)
		CDDX_FIXED(m_wndRefresh, m_dwRefresh)
		CDDX_FIXED(m_wndDelay, m_dwDelay)
	END_DDX_MAP()

	LRESULT OnPostSetFocus(UINT, WPARAM wParam, LPARAM, BOOL &bHandled) ;
	void OnDataExchangeError(UINT nCtrlID, BOOL /*bSave*/) ;
	void OnDataValidateError(UINT nCtrlID, BOOL bSave, CFltWinDataExchange<CMiscDlg>::_XData& data) ;
	void OnCustomDataValidateError(UINT nCtrlID, BOOL bSaveValidate, const CValidatorB::VData &vData) ;
	
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	BOOL InitControls() ;

protected :

	BEGIN_RESETFOCUS_MAP(CMiscDlg)
		RESETPROCESSFOCUS(IDC_EREFRESH, m_wndRefresh) ;
		RESETPROCESSFOCUS(IDC_EREFRESH, m_wndDelay) ;
	END_RESETFOCUS_MAP()

	CUFixedEdit  m_wndRefresh, m_wndDelay ;
	DWORD        m_dwRefresh, m_dwDelay ;
	CCfgMisc    *m_pMisc ;
	CIcon        m_Icon ;
} ;

LRESULT CMiscDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	m_pMisc = (CCfgMisc*)lParam ;
	if(NULL == m_pMisc)
	{
		EndDialog(IDCANCEL) ;
		return TRUE ;
	}

	SetFont(g_Cfg.m_GUI.m_FntGUI, FALSE) ;

	if(NULL != m_Icon.LoadIcon(IDI_MISC))
	{
		SetIcon(m_Icon, TRUE) ;
	}
	
 	CenterWindow(GetParent()) ;

	if(!InitControls())
	{
		EndDialog(IDCANCEL) ;
		return TRUE ;
	}

	m_dwRefresh = m_pMisc -> m_dwMSecs ;
	m_dwDelay = m_pMisc -> m_dwDelay ;
	DoDataExchange(false);
	
	return TRUE;
}

LRESULT CMiscDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(IDOK == wID)
	{
		if(DoDataExchange(true))
		{
			m_pMisc -> m_dwMSecs = m_dwRefresh ;
			m_pMisc -> m_dwDelay = m_dwDelay ;
		} 
		else
		{
			return 0 ;
		}
	}
	
	EndDialog(wID) ;
	return 0 ;
}

BOOL CMiscDlg::InitControls()
{
	SetFont(g_Cfg.m_GUI.m_FntGUI, FALSE) ;
	if(!ChangeFont(m_hWnd, g_Cfg.m_GUI.m_FntGUI)) return FALSE ;
	if(!CenterEdits(m_hWnd)) return FALSE ;

	return TRUE ;
}

/////

LRESULT CMiscDlg::OnPostSetFocus(UINT, WPARAM wParam, LPARAM, BOOL &bHandled)
{
	::SetFocus(GetDlgItem(wParam)) ;
	return 0 ;
}

void CMiscDlg::OnDataExchangeError(UINT nCtrlID, BOOL /*bSave*/)
{
	AtlMessageBox(::GetActiveWindow(), _T(" "), IDS_CAPERROR, MB_OK | MB_ICONERROR) ;
	::SetFocus(GetDlgItem(nCtrlID));
}

void CMiscDlg::OnDataValidateError(UINT nCtrlID, BOOL bSave, CFltWinDataExchange<CMiscDlg>::_XData& data)
{
	CString sCap ;
	sCap.LoadString(IDS_CAPERROR) ;
	
	AtlMessageBox(::GetActiveWindow(), _T(" "), IDS_CAPERROR, MB_OK | MB_ICONERROR) ;
	::SetFocus(GetDlgItem(nCtrlID));
}

void CMiscDlg::OnCustomDataValidateError(UINT nCtrlID, BOOL bSaveValidate, const CValidatorB::VData &vData)
{
	_XData xData ;

	if(ConvertVtoX(xData, vData)) 
		OnDataValidateError(nCtrlID, bSaveValidate, xData) ;
	else {

		CString sTxt ;

		switch(vData.nDataType) {

			case CValidatorB::ddxDataFixed : {
				MsgFormatDataFixed(vData, sTxt) ;
				break ;
			}

			default :
				::SetFocus(GetDlgItem(nCtrlID)) ; 
				return ;
		}

		AtlMessageBox(::GetActiveWindow(), (LPCTSTR)sTxt, IDS_CAPERROR, MB_OK | MB_ICONERROR) ;
		::SetFocus(GetDlgItem(nCtrlID)) ; 
	}
}	

/////

//////////////////////////////////////////////////////////////////////////
// Interface function:

int ShowMiscDlg(CCfgMisc *pMisc)
{
	CMiscDlg dlg ;
	return dlg.DoModal(::GetActiveWindow(), (LPARAM)pMisc) ;
}


//////////////////////////////////////////////////////////////////////////
// CPacketDlg
//////////////////////////////////////////////////////////////////////////

class CPacketDlg : public CDialogImpl<CPacketDlg>,
				public CFltWinDataExchange<CPacketDlg>
{
public:
	enum { IDD = IDD_PACKET } ;

	CPacketDlg() : m_pDoc(NULL), m_wndExpos(0, 1, 480), m_wndTicks(0, 0, 500)
	{
		INIT_RESETFOCUS_MAP() ;		
	}
	
	BEGIN_MSG_MAP(CPacketDlg)
		POSTINVALID_HANDLER()
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		COMMAND_ID_HANDLER(IDC_BCHANGE, OnChangeFN)
	END_MSG_MAP()

	BEGIN_DDX_MAP(CPacketDlg)
		DDX_CONTROL(IDC_EEXPOS, m_wndExpos)
		DDX_CONTROL(IDC_ETICKS, m_wndTicks)
		DDX_TEXT(IDC_EPACKET, m_FilePacket)
		CDDX_FIXED(m_wndExpos, m_dwExpos)
		CDDX_FIXED(m_wndTicks, m_dwTicks)
	END_DDX_MAP()

	LRESULT OnPostSetFocus(UINT, WPARAM wParam, LPARAM, BOOL &bHandled) ;
	void OnDataExchangeError(UINT nCtrlID, BOOL /*bSave*/) ;
	void OnDataValidateError(UINT nCtrlID, BOOL bSave, CFltWinDataExchange<CPacketDlg>::_XData& data) ;
	void OnCustomDataValidateError(UINT nCtrlID, BOOL bSaveValidate, const CValidatorB::VData &vData) ;
	
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) ;
	LRESULT OnChangeFN(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) ;

	BOOL InitControls() ;

protected:

	BEGIN_RESETFOCUS_MAP(CPacketDlg)
		RESETPROCESSFOCUS(IDC_EREFRESH, m_wndExpos) ;
		RESETPROCESSFOCUS(IDC_EREFRESH, m_wndTicks) ;
	END_RESETFOCUS_MAP()

	CCfgDoc     *m_pDoc ;
	CUFixedEdit  m_wndExpos, m_wndTicks ;
	DWORD        m_dwExpos, m_dwTicks ;
	CString      m_FilePacket ;
	CIcon        m_Icon ;
} ;

LRESULT CPacketDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
	m_pDoc = (CCfgDoc*)lParam ;
	if(NULL == m_pDoc)
	{
		EndDialog(IDCANCEL) ;
		return TRUE ;
	}

	SetFont(g_Cfg.m_GUI.m_FntGUI, FALSE) ;

	if(NULL != m_Icon.LoadIcon(IDI_PACKET))
	{
		SetIcon(m_Icon, TRUE) ;
	}
	
 	CenterWindow(GetParent()) ;

	if(!InitControls())
	{
		EndDialog(IDCANCEL) ;
		return TRUE ;
	}

	m_dwExpos = m_pDoc -> m_dwExpos ;
	m_dwTicks = m_pDoc -> m_dwTicks ;
	m_FilePacket = m_pDoc -> m_FilePacket ;
	DoDataExchange(false);
	
	return TRUE;
}

LRESULT CPacketDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(IDOK == wID)
	{
		if(DoDataExchange(true))
		{
			m_pDoc -> m_dwExpos = m_dwExpos ;
			m_pDoc -> m_dwTicks = m_dwTicks ;
			m_pDoc -> m_FilePacket = m_FilePacket ;
		} 
		else
		{
			return 0 ;
		}
	}
	
	EndDialog(wID) ;
	return 0 ;
}

BOOL CPacketDlg::InitControls()
{
	SetFont(g_Cfg.m_GUI.m_FntGUI, FALSE) ;
	if(!ChangeFont(m_hWnd, g_Cfg.m_GUI.m_FntGUI)) return FALSE ;
	if(!CenterEdits(m_hWnd)) return FALSE ;

	CEdit(GetDlgItem(IDC_EPACKET)).SetLimitText(MAX_PATH) ;

	return TRUE ;
}

LRESULT CPacketDlg::OnChangeFN(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// Select filename and save the record to the file
	CString FileExt, Filters ;
	FileExt.LoadString(IDS_FILEEXT) ;
	Filters.LoadString(IDS_FILTERS) ;
	for(int i = Filters.ReverseFind(_T('#')) ; i != -1 ; i = Filters.ReverseFind(_T('#')))
		Filters.SetAt(i, 0) ;

	DoDataExchange(true, IDC_EPACKET) ;
	CCenteredFileDialog dlg(FALSE, 
		FileExt, 
		m_FilePacket, 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, 
		Filters, 
		m_hWnd) ;

	if(IDOK == dlg.DoModal())
	{
		m_FilePacket = dlg.m_szFileName ;
		DoDataExchange(false, IDC_EPACKET) ;
	}

	return 0 ;
}

/////

LRESULT CPacketDlg::OnPostSetFocus(UINT, WPARAM wParam, LPARAM, BOOL &bHandled)
{
	::SetFocus(GetDlgItem(wParam)) ;
	return 0 ;
}

void CPacketDlg::OnDataExchangeError(UINT nCtrlID, BOOL /*bSave*/)
{
	AtlMessageBox(::GetActiveWindow(), _T(" "), IDS_CAPERROR, MB_OK | MB_ICONERROR) ;
	::SetFocus(GetDlgItem(nCtrlID));
}

void CPacketDlg::OnDataValidateError(UINT nCtrlID, BOOL bSave, CFltWinDataExchange<CPacketDlg>::_XData& data)
{
	CString sCap ;
	sCap.LoadString(IDS_CAPERROR) ;
	
	AtlMessageBox(::GetActiveWindow(), _T(" "), IDS_CAPERROR, MB_OK | MB_ICONERROR) ;
	::SetFocus(GetDlgItem(nCtrlID));
}

void CPacketDlg::OnCustomDataValidateError(UINT nCtrlID, BOOL bSaveValidate, const CValidatorB::VData &vData)
{
	_XData xData ;

	if(ConvertVtoX(xData, vData)) 
		OnDataValidateError(nCtrlID, bSaveValidate, xData) ;
	else {

		CString sTxt ;

		switch(vData.nDataType) {

			case CValidatorB::ddxDataFixed : {
				MsgFormatDataFixed(vData, sTxt) ;
				break ;
			}

			default :
				::SetFocus(GetDlgItem(nCtrlID)) ; 
				return ;
		}

		AtlMessageBox(::GetActiveWindow(), (LPCTSTR)sTxt, IDS_CAPERROR, MB_OK | MB_ICONERROR) ;
		::SetFocus(GetDlgItem(nCtrlID)) ; 
	}
}	

/////

//////////////////////////////////////////////////////////////////////////
// Interface function:

int ShowPacketDlg(CCfgDoc *pDoc)
{
	CPacketDlg dlg ;
	return dlg.DoModal(::GetActiveWindow(), (LPARAM)pDoc) ;
}

