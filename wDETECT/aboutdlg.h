// aboutdlg.h : interface of the CAboutDlg class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ABOUTDLG_H__04D7E791_447F_43F2_B9D7_36DB376CC29D__INCLUDED_)
#define AFX_ABOUTDLG_H__04D7E791_447F_43F2_B9D7_36DB376CC29D__INCLUDED_

class CCenteredFileDialog : public CFileDialogImpl<CCenteredFileDialog> 
{
public:
	CCenteredFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		HWND hWndParent = NULL)
		: CFileDialogImpl<CCenteredFileDialog>(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent)
	{ }

	// override base class map and references to handlers
//	BEGIN_MSG_MAP(CCenteredFileDialog)
//		NOTIFY_CODE_HANDLER(CDN_INITDONE, _OnInitDone)
//	END_MSG_MAP()
	DECLARE_EMPTY_MSG_MAP()

	void OnInitDone(LPOFNOTIFY /*lpon*/)
	{
		ATLASSERT(FALSE) ;
		CWindow Wnd = GetFileDialogWindow() ;
		Wnd.CenterWindow() ;
		Wnd.Detach() ;
	}
	
} ;

class CAboutDlg : public CDialogImpl<CAboutDlg>
{
public:
	enum { IDD = IDD_ABOUTBOX };

	BEGIN_MSG_MAP(CAboutDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
};

//////////////////////////////////////////////////////////////////////////

extern int ShowSerialDlg(CCfgSerial*) ;
extern int ShowMiscDlg(CCfgMisc*) ;
extern int ShowPacketDlg(CCfgDoc*) ;

#endif // !defined(AFX_ABOUTDLG_H__04D7E791_447F_43F2_B9D7_36DB376CC29D__INCLUDED_)
