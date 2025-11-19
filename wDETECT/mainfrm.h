// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__6BB12401_088F_4457_84B1_3A86F6BD1202__INCLUDED_)
#define AFX_MAINFRM_H__6BB12401_088F_4457_84B1_3A86F6BD1202__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMainFrame : public CFrameWindowImpl<CMainFrame>, public CUpdateUI<CMainFrame>,
		public CMessageFilter, public CIdleHandler
{
public:
	DECLARE_FRAME_WND_CLASS(NULL, IDR_MAINFRAME)

	CMultiPaneStatusBarCtrl m_StatusBar ;
	CView          m_view ;
	HANDLE         m_hThrd ;
	CAutoHandle    m_Cancel ;
	CRecord<DWORD> m_Record ;
	TASKARGS       m_TaskArgs ;
	PACKETARGS     m_PacketArgs ;
	DWORD          m_dwLastTime ;
	FILETIME       m_TimeIni ;
	FILETIME       m_TimeFin ;
	BOOL           m_bWantExit ;
	BOOL           m_bDiscard ;
	DWORD          m_dwLost ;

	CMainFrame()  
	{
 		m_ptMinMax.x = m_ptMinMax.y = 0 ;
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnIdle();

	BEGIN_UPDATE_UI_MAP(CMainFrame)
		UPDATE_ELEMENT(ID_VIEW_TOOLBAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_VIEW_STATUS_BAR, UPDUI_MENUPOPUP)
		UPDATE_ELEMENT(ID_FILE_NEW, UPDUI_MENUPOPUP | UPDUI_TOOLBAR) 
		UPDATE_ELEMENT(ID_PACKET, UPDUI_MENUPOPUP | UPDUI_TOOLBAR) 
		UPDATE_ELEMENT(ID_FILE_SAVE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR) 
		UPDATE_ELEMENT(ID_FILE_CLOSE, UPDUI_MENUPOPUP | UPDUI_TOOLBAR) 
		UPDATE_ELEMENT(ID_SERIAL, UPDUI_MENUPOPUP) 
		UPDATE_ELEMENT(ID_MISC, UPDUI_MENUPOPUP) 
		UPDATE_ELEMENT(0, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(1, UPDUI_STATUSBAR)
		UPDATE_ELEMENT(2, UPDUI_STATUSBAR)
	END_UPDATE_UI_MAP()

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MSG_WM_NCDESTROY(OnNCDestroy)
		MSG_WM_GETMINMAXINFO(OnMinMaxInfo)
		MSG_WM_CLOSE(OnClose)
		MSG_WM_TIMER(OnTimer)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
		COMMAND_ID_HANDLER(ID_PACKET, OnPacket)
		COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave) 
		COMMAND_ID_HANDLER(ID_FILE_CLOSE, OnFileClose) 
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_SERIAL, OnSerial)
		COMMAND_ID_HANDLER(ID_MISC, OnMisc)
		MESSAGE_HANDLER(TSM_SAVE, OnTSMSave) 
		MESSAGE_HANDLER(TSM_ERROR, OnTSMError)
		MESSAGE_HANDLER(TSM_VALUE, OnTSMValue)
		MESSAGE_HANDLER(TSM_EXIT, OnTSMExit)
		MESSAGE_HANDLER(TSM_OVER, OnTSMOver)
		MESSAGE_HANDLER(TSM_MOTION, OnTSMMotion)
		CHAIN_MSG_MAP(CUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	void OnNCDestroy() ;
	void OnMinMaxInfo(LPMINMAXINFO) ;
	void OnClose() ;
	void OnTimer(UINT, TIMERPROC) ;
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnPacket(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSerial(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) ;
	LRESULT OnMisc(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) ;

	LRESULT OnTSMFault(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;
	LRESULT OnTSMSave(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;
	LRESULT OnTSMError(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;
	LRESULT OnTSMValue(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;
	LRESULT OnTSMExit(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;
	LRESULT OnTSMOver(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;
	LRESULT OnTSMMotion(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;

private:
	enum { nStates = 4, nSmallIconSize = 16, nTinyIconSize = 12, nTimeQuantum = 1000 } ;
	enum SBStates { SSB_EMPTY = IDS_EMPTY, SSB_CONNECTED = IDS_CONNECTED, SSB_CONERROR = IDS_CONERROR, SSB_MOVING = IDS_MOVING } ;
	enum TASKS { TSK_IDLE, TSK_SINGLE, TSK_PACKET } ;

	int   m_nTimer ;
	int   m_nTTL ;
	CIcon m_Icons[nStates] ;
	CIcon m_Basket ;

	enum TASKS m_Task ;

	BOOL CreateDetectStatusBar() ;
	void DSBState(enum SBStates eState, WPARAM wParam, LPARAM lParam) ;
	void DSBSetLast(WORD wLast) ;
	void DSBSetLost(DWORD dwLost) ;
	BOOL SaveRecord(CString &FileName) ;

	POINT m_ptMinMax ; 

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__6BB12401_088F_4457_84B1_3A86F6BD1202__INCLUDED_)
