// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "autores.h"
#include "resource.h"

#include "records.h"
#include "Cfg.h"
#include "aboutdlg.h"
#include "view.h"
#include "tasks.h"
#include "MainFrm.h"

void CMainFrame::DSBState(enum SBStates eState, WPARAM wParam, LPARAM lParam)
{
	m_StatusBar.SetIcon(0, m_Icons[int(eState) - int(SSB_EMPTY)]) ;
	m_StatusBar.SetIcon(-1, m_Icons[int(eState) - int(SSB_EMPTY)]) ;

	CString Str ;
	switch(eState) 
	{
		case SSB_EMPTY :
		{
			UISetText(1, _T("")) ; UISetText(2, _T("")) ;
		}

		case SSB_CONNECTED :
		case SSB_MOVING :
		{
			Str.LoadString((UINT)eState) ;
			break ;
		}

		case SSB_CONERROR :
		{
			if(wParam == ERROR_SUCCESS)
			{
				Str.Format(IDS_FRAME, lParam) ;
			}
			else
			{
				LPVOID lpMsgBuf ;
				DWORD  dwPos = FormatErrorMessage(wParam, lpMsgBuf) ;

				if(dwPos)
					Str.Format((UINT)eState, LPCSTR(lpMsgBuf)) ;

				::LocalFree( lpMsgBuf ) ;
			}
			break ;
		}

		default :
			ATLASSERT(FALSE) ;
	}

	UISetText(0, Str) ;
}

void CMainFrame::DSBSetLast(WORD wLast)
{
	CString Str ;
	Str.Format(IDS_LASTMSG, wLast) ;
	UISetText(2, Str) ;
}

void CMainFrame::DSBSetLost(DWORD dwLost)
{
	CString Str ;
	Str.Format(IDS_LOSTMSG, dwLost) ;
	UISetText(1, Str) ;
}

class CAutoImageList : public CImageList
{
public:
	~CAutoImageList() {	Destroy() ;	}
};

BOOL CMainFrame::CreateDetectStatusBar() 
{
//	HWND Create(HWND hWndParent, UINT nTextID = ATL_IDS_IDLEMESSAGE, DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | SBARS_SIZEGRIP, UINT nID = ATL_IDW_STATUS_BAR)

	int nSMIcon = GetSystemMetrics(SM_CXSMICON) ; 
	nSMIcon = nSMIcon >= nSmallIconSize ? nSmallIconSize : nTinyIconSize ;
	
	CAutoImageList Icons ;
	Icons.CreateFromImage(MAKEINTRESOURCE(nSMIcon >= nSmallIconSize ? IDB_STATES : IDB_STATES12),
							nSMIcon,
							1,
							CLR_DEFAULT,
							IMAGE_BITMAP) ;	
	
	if(NULL == Icons.m_hImageList) return FALSE ;
	
	for(int i = 0 ; i < nStates ; ++ i) 
	{
		m_Icons[i].Attach(Icons.GetIcon(i + 1, ILD_TRANSPARENT)) ;
	}

	m_Basket.Attach(Icons.GetIcon(0, ILD_TRANSPARENT)) ;

	m_hWndStatusBar = m_StatusBar.Create(*this, 
										ATL_IDS_IDLEMESSAGE,
										WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN 
										| WS_CLIPSIBLINGS | SBARS_SIZEGRIP) ;
	if(m_hWndStatusBar == NULL) return FALSE ;

	m_StatusBar.SetFont(g_Cfg.m_GUI.m_FntGUI) ;

	int nPanes[] = { ID_DEFAULT_PANE,  IDS_LOST, IDS_LAST } ;
	if(!m_StatusBar.SetPanes(nPanes, sizeof(nPanes) / sizeof(nPanes[0]), false))
		return FALSE ;

	m_StatusBar.SetIcon(1, m_Basket) ;

	UIAddStatusBar ( m_hWndStatusBar ) ;

	DSBState(SSB_EMPTY, 0, 0) ;
	return TRUE ;
}

BOOL CMainFrame::SaveRecord(CString &FileName)
{
	CAutoHandle File(CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL)) ;
	if(File.IsInvalid()) return FALSE ;

	int nOffset = FileName.ReverseFind(_T('\\')) + 1 ;
	
	CString Str ;
	Str.Format(_T("%s\r\n"), LPCTSTR(FileName) + nOffset) ;

	DWORD   dwBytes ;
	if(!WriteFile(File, LPCTSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	SYSTEMTIME ST ;
	FileTimeToSystemTime(&m_TimeIni, &ST) ;

	// HMS DMY %02hu:%02hu:%02hu %02hu.%02hu.%04hu
	Str.Format(_T("INI TIME : %02hu:%02hu:%02hu %02hu.%02hu.%04hu\r\n"), 
		ST.wHour, ST.wMinute, ST.wSecond,
		ST.wDay, ST.wMonth, ST.wYear) ;
	if(!WriteFile(File, LPCTSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	FileTimeToSystemTime(&m_TimeFin, &ST) ;
	
	// HMS DMY %02hu:%02hu:%02hu %02hu.%02hu.%04hu
	Str.Format(_T("FIN TIME : %02hu:%02hu:%02hu %02hu.%02hu.%04hu\r\n"), 
		ST.wHour, ST.wMinute, ST.wSecond,
		ST.wDay, ST.wMonth, ST.wYear) ;
	if(!WriteFile(File, LPCTSTR(Str), Str.GetLength(), &dwBytes, NULL))
		return FALSE ;

	for(int i = 0 ; i < m_Record.Lengh() ; ++ i)
	{
		Str.Format(_T("%04d %u\r\n"), i, m_Record[i]) ;
		if(!WriteFile(File, LPCTSTR(Str), Str.GetLength(), &dwBytes, NULL))
			break ;
	}

	return i == m_Record.Lengh() ;
}


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
//  	UIUpdateMenuBar(FALSE, TRUE) ;
	UIUpdateToolBar();
	UIUpdateStatusBar() ;
	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(!m_Record.Allocate(nChannels)) return - 1 ;

	m_hThrd = NULL ;
	m_nTimer = 1 ;
	m_nTTL = 0 ;
	m_bWantExit = FALSE ;
	m_Task = TSK_IDLE ;

	m_Cancel = CreateEvent(NULL, TRUE, FALSE, NULL) ;
	if(m_Cancel.IsNull()) return -1 ;

	m_TaskArgs.hCancel = m_Cancel ;
	m_TaskArgs.hMaster = m_hWnd ;

	HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
	if(NULL == hWndToolBar) return -1 ;

	HWND hWndToolTip = (HWND)::SendMessage(hWndToolBar, TB_GETTOOLTIPS, 0, 0) ;
//	if(NULL != hWndToolTip)
//		::SendMessage(hWndToolTip, WM_SETFONT, (WPARAM) g_Cfg.m_GUI.m_FntGUI.m_hFont, FALSE) ;

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndToolBar);

	if(!CreateDetectStatusBar()) return -1 ;

	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
	if(m_hWndClient == NULL) return -1 ;
	m_view.SetFont(g_Cfg.m_GUI.m_FntMono, FALSE) ;

	UIAddToolBar(hWndToolBar);
	
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UIEnable(ID_FILE_SAVE, FALSE) ;
	UIEnable(ID_FILE_CLOSE, FALSE) ;

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	// Set size of the window created:
	
	CRect rcMain ;
	GetWindowRect(&rcMain) ;

	m_ptMinMax.x = 640 + 2 * ::GetSystemMetrics(SM_CXEDGE) ;
	m_ptMinMax.x += 2 * ::GetSystemMetrics(SM_CXSIZEFRAME) ;

	m_ptMinMax.y = 480 + 2 * ::GetSystemMetrics(SM_CYEDGE) ;
	m_ptMinMax.y += 2 * ::GetSystemMetrics(SM_CYSIZEFRAME) ;
	m_ptMinMax.y += ::GetSystemMetrics(SM_CYCAPTION) ;
	m_ptMinMax.y += ::GetSystemMetrics(SM_CYMENU) ;
	
	CRect rc ;

	::GetWindowRect(m_hWndToolBar, &rc) ;
	m_ptMinMax.y += rc.Height() ;
	::GetWindowRect(m_hWndStatusBar, &rc) ;
	m_ptMinMax.y += rc.Height() ;
	
	rcMain.right = rcMain.left + m_ptMinMax.x ;
	rcMain.bottom = rcMain.top + m_ptMinMax.y ;

	MoveWindow(&rcMain, FALSE) ;
	GetClientRect(&rc) ;

	return 0;
}

void CMainFrame::OnNCDestroy()
{
	SetMsgHandled(FALSE) ;

	RECT rc ;
	GetWindowRect(&rc) ;
	g_Cfg.m_GUI.m_ptOrigin.x = rc.left ;
	g_Cfg.m_GUI.m_ptOrigin.y = rc.top ;
}

void CMainFrame::OnClose()
{
	if(TSK_IDLE != m_Task)
	{
		if(IDYES == AtlMessageBox(m_hWnd, IDS_EXITQUEST, IDS_CAPQUESTION, MB_YESNO | MB_ICONQUESTION))
		{
			m_bWantExit = TRUE ;
			::SetEvent(m_Cancel) ;
		}
	}
	else
	{
		SetMsgHandled(FALSE) ;
	}
}

void CMainFrame::OnMinMaxInfo(LPMINMAXINFO pMMI)
{
	if(m_ptMinMax.x == 0)
	{
		SetMsgHandled(FALSE) ; return ;
	}

	pMMI -> ptMaxSize = m_ptMinMax ;
	pMMI -> ptMaxTrackSize = m_ptMinMax ;
	pMMI -> ptMinTrackSize = m_ptMinMax ;
}

void CMainFrame::OnTimer(UINT nTimerID, TIMERPROC)
{
	if(nTimerID != (UINT)m_nTimer)
		SetMsgHandled(FALSE) ;
	else
	{
		-- m_nTTL ;
		if(m_nTTL <= 0)
		{
			KillTimer(m_nTimer) ;
			DSBState(SSB_CONNECTED, 0, 0) ;
		}
	}
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0 ;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: add code to initialize document

	// Select filename and save the record to the file
	CString FileExt, Filters ;
	FileExt.LoadString(IDS_FILEEXT) ;
	Filters.LoadString(IDS_FILTERS) ;
	for(int i = Filters.ReverseFind(_T('#')) ; i != -1 ; i = Filters.ReverseFind(_T('#')))
		Filters.SetAt(i, 0) ;

	CCenteredFileDialog dlg(FALSE, 
		FileExt, 
		g_Cfg.m_Doc.m_FileName, 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, 
		Filters, 
		m_hWnd) ;
	
	if(dlg.DoModal() != IDOK) return 0 ;
	g_Cfg.m_Doc.m_FileName = dlg.m_szFileName ;

	// Prepare data:

	m_bDiscard = FALSE ;
	// Reset Record:
	m_Record.Reset() ;

	m_dwLastTime = ::GetTickCount() ;
	m_dwLost = 0 ;

	// Activate view
	::SendMessage(m_view.m_hWnd, CVM_ACTIVATE, TRUE, (LPARAM)&m_Record) ;

	ResetEvent(m_Cancel) ;
	g_Cfg.m_Serial.GetPortCfg(m_TaskArgs.SerCfg) ;

	GetSystemTimeAsFileTime(&m_TimeIni) ;

	// Create thread
	DWORD dwID ;
	m_hThrd = CreateThread(NULL, STD_STACK, SerialTask, &m_TaskArgs, 0, &dwID) ;
	if(NULL == m_hThrd) 
	{
		ErrorMsgBox(m_hWnd, ::GetLastError(), IDS_CAPERROR) ;

		// Deactivate View
		::SendMessage(m_view.m_hWnd, CVM_ACTIVATE, FALSE, 0) ;

		// Leave activation procedure
		return 0 ;
	}

	m_Task = TSK_SINGLE ;

	// Disable/Enable UIs
	UIEnable(ID_FILE_NEW, FALSE) ;
	UIEnable(ID_FILE_SAVE, TRUE) ;
	UIEnable(ID_FILE_CLOSE, TRUE) ;

	UIEnable(ID_PACKET, FALSE) ;
	UIEnable(ID_SERIAL, FALSE) ;
	UIEnable(ID_MISC, FALSE) ;

	DSBState(SSB_CONNECTED, 0, 0) ;
	DSBSetLast(0) ;
	return 0;
}

LRESULT CMainFrame::OnPacket(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	if(IDOK != ShowPacketDlg(& g_Cfg.m_Doc)) return 0 ;

	ResetEvent(m_Cancel) ;
	
	g_Cfg.m_Serial.GetPortCfg(m_PacketArgs.SerCfg) ;
	m_PacketArgs.hMaster = m_hWnd ;
	m_PacketArgs.hCancel = m_Cancel ;
	m_PacketArgs.dwDelay = g_Cfg.m_Misc.m_dwDelay ;
	m_PacketArgs.dwExpos = g_Cfg.m_Doc.m_dwExpos * 60 ;		// Turn minutes to seconds
	m_PacketArgs.dwTicks = g_Cfg.m_Doc.m_dwTicks ;

	// Start task

	m_bDiscard = FALSE ;
	// Reset Record:
	m_Record.Reset() ;

	m_dwLastTime = ::GetTickCount() ;
	m_dwLost = 0 ;

	// Activate view
	::SendMessage(m_view.m_hWnd, CVM_ACTIVATE, TRUE, (LPARAM)&m_Record) ;

	GetSystemTimeAsFileTime(&m_TimeIni) ;

	// Create thread
	DWORD dwID ;
	m_hThrd = CreateThread(NULL, STD_STACK, PacketTask, &m_PacketArgs, 0, &dwID) ;
	if(NULL == m_hThrd) 
	{
		ErrorMsgBox(m_hWnd, ::GetLastError(), IDS_CAPERROR) ;

		// Deactivate View
		::SendMessage(m_view.m_hWnd, CVM_ACTIVATE, FALSE, 0) ;

		// Leave activation procedure
		return 0 ;
	}

	m_Task = TSK_PACKET ;

	// Disable/Enable UIs
	UIEnable(ID_FILE_NEW, FALSE) ;
	UIEnable(ID_FILE_SAVE, TRUE) ;
	UIEnable(ID_FILE_CLOSE, TRUE) ;

	UIEnable(ID_PACKET, FALSE) ;
	UIEnable(ID_SERIAL, FALSE) ;
	UIEnable(ID_MISC, FALSE) ;

	DSBState(SSB_CONNECTED, 0, 0) ;
	DSBSetLast(0) ;

	return 0 ;
}

LRESULT CMainFrame::OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	::SetEvent(m_Cancel) ;
	return 0;
}

LRESULT CMainFrame::OnFileClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	switch(m_Task) 
	{
		case TSK_SINGLE :
		case TSK_PACKET :
			if(IDYES == AtlMessageBox(NULL, IDS_FILEQUEST, 
								IDS_CAPQUESTION, MB_YESNO | MB_ICONQUESTION))
			{
				m_bDiscard = TRUE ;
				::SetEvent(m_Cancel) ;
			}

			break ;

		default :
			ATLASSERT(FALSE) ;
	}

	return 0 ;
}


LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST);	// toolbar is first 1st band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnSerial(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ShowSerialDlg(& g_Cfg.m_Serial) ;
	return 0 ;
}

LRESULT CMainFrame::OnMisc(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	ShowMiscDlg(& g_Cfg.m_Misc) ;
	return 0 ;
}

#define SMALL_TIMEOUT	20

LRESULT CMainFrame::OnTSMFault(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	ErrorMsgBox(m_hWnd, wParam, IDS_CAPERROR) ;

	WaitForSingleObject(m_hThrd, SMALL_TIMEOUT) ;
	CloseHandle(m_hThrd) ; m_hThrd = NULL ;

	// Deactivate view
	::SendMessage(m_view.m_hWnd, CVM_ACTIVATE, FALSE, 0) ;

	// Disable/Enable UIs
	UIEnable(ID_FILE_NEW, TRUE) ;
	UIEnable(ID_FILE_SAVE, FALSE) ;
	UIEnable(ID_FILE_CLOSE, FALSE) ;

	UIEnable(ID_SERIAL, TRUE) ;
	UIEnable(ID_MISC, TRUE) ;
	
	DSBState(SSB_EMPTY, 0, 0) ;

	if(m_bWantExit)	PostMessage(WM_CLOSE) ;
	return 0 ;
}

LRESULT CMainFrame::OnTSMSave(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(m_bDiscard) return 0 ;

	// Time when observation was ended 
	GetSystemTimeAsFileTime(&m_TimeFin) ;

	WORD    wNumSurvey = LOWORD(wParam) ;
	CString FileName ;

	if(wNumSurvey == 0)
	{
		FileName = g_Cfg.m_Doc.m_FileName ;
	}
	else
	{
		// Check for file extention
		int nPointPos = g_Cfg.m_Doc.m_FilePacket.ReverseFind(_T('.')) ;
		int nSlashPos = g_Cfg.m_Doc.m_FilePacket.ReverseFind(_T('\\')) ;

		if(nSlashPos < nPointPos)
		{
			// Yes, the filename has extension
			FileName = g_Cfg.m_Doc.m_FilePacket.Left(nPointPos) ;

			CString Number ;
			Number.Format(_T("-%05hu"), wNumSurvey) ;
		
			FileName += Number ;
			FileName += g_Cfg.m_Doc.m_FilePacket.Mid(nPointPos) ;
		}
		else
		{
			// No, the filename doesn't have any extension
			FileName.Format("%s-%05hu", g_Cfg.m_Doc.m_FileName, wNumSurvey) ;
		}
	}

	if(!SaveRecord(FileName))
		ErrorMsgBox(NULL, ::GetLastError(), IDS_CAPERROR) ;

	m_TimeIni = m_TimeFin ;
	m_Record.Reset() ;
	::SendMessage(m_view.m_hWnd, CVM_ACTIVATE, TRUE, (LPARAM)&m_Record) ;
	return 0 ;
}

LRESULT CMainFrame::OnTSMError(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) 
{
	// Set error state
	DSBState(SSB_CONERROR, wParam, lParam) ;

	if(m_nTTL == 0)
	{
		// Start up timer
		SetTimer(m_nTimer, nTimeQuantum) ;
		m_nTTL = 2 ;
	}
	else
	{
		m_nTTL = 4 ;
	}

	return 0 ;
}

LRESULT CMainFrame::OnTSMValue(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	// Transfer new value
//	WORD wChannel = LOWORD(lParam) ;	// DDEEBBUUGG
	WORD wChannel = WORD(wParam) ;
	m_Record.SetAt(wChannel, 1 + m_Record[(int)wChannel]) ;

	DWORD dwCurrent = ::GetTickCount() ;
	if(dwCurrent - m_dwLastTime >= g_Cfg.m_Misc.m_dwMSecs)
	{
		m_dwLastTime = dwCurrent ;
		DSBSetLost(m_dwLost) ;
		DSBSetLast(wChannel) ;
		::SendMessage(m_view.m_hWnd, CVM_ACTIVATE, TRUE, (LPARAM)&m_Record) ;
	}

	return 0 ;
}

LRESULT CMainFrame::OnTSMExit(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(wParam != ERROR_SUCCESS)
	{
		ErrorMsgBox(m_hWnd, wParam, IDS_CAPERROR) ;
	}
	
	m_Task = TSK_IDLE ;

	// Deactivate view
	::SendMessage(m_view.m_hWnd, CVM_ACTIVATE, FALSE, 0) ;

	// Disable/Enable UIs
	UIEnable(ID_FILE_NEW, TRUE) ;
	UIEnable(ID_FILE_SAVE, FALSE) ;
	UIEnable(ID_FILE_CLOSE, FALSE) ;

	UIEnable(ID_PACKET, TRUE) ;
	UIEnable(ID_SERIAL, TRUE) ;
	UIEnable(ID_MISC, TRUE) ;

	DSBState(SSB_EMPTY, 0, 0) ;

	// Invalidate thrd hndl
	WaitForSingleObject(m_hThrd, SMALL_TIMEOUT) ;
	CloseHandle(m_hThrd) ; m_hThrd = NULL ;

	if(m_bWantExit)	PostMessage(WM_CLOSE) ;
	return 0 ;
}

LRESULT CMainFrame::OnTSMOver(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	WORD wOver = LOWORD(wParam) ;
	m_dwLost += wOver ? wOver : 256 ;

	return 0 ;
}

LRESULT CMainFrame::OnTSMMotion(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DSBState(wParam ? SSB_MOVING : SSB_CONNECTED, 0, 0) ;

	return 0 ;
}
