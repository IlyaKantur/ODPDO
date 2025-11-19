// wDETECTView.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WDETECTVIEW_H__A395AAA8_BDAB_44B2_8FEC_8FEA8730D84E__INCLUDED_)
#define AFX_WDETECTVIEW_H__A395AAA8_BDAB_44B2_8FEC_8FEA8730D84E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define CVM_VALUE		WM_APP + 9		// wPar -- wValue
#define CVM_ACTIVATE	WM_APP + 10		// wPar -- TRUE activate / FALSE deactivate, lPar -- pRecord

class CView : public CWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

	CView() : m_bActive(FALSE), 
				m_pRecord(NULL),
				m_wLast(nChannels),
				m_dwMax(1) 
	{
		m_rcChan[0].SetRect(64, 25, 64 + 512, 25 + 200) ;
		m_rcChan[1].SetRect(64, 25 + 200 + 25, 64 + 512, 25 + 200 + 25 + 200) ;
	}

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CView)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MESSAGE_HANDLER(CVM_VALUE, OnCVMValue)
		MESSAGE_HANDLER(CVM_ACTIVATE, OnCVMActivate)
// 		MESSAGE_HANDLER(WM_)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	LRESULT OnCreate(LPCREATESTRUCT) ;
	inline LRESULT OnEraseBkgnd(HDC hDC) const { return 0 ; } ;
	LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	LRESULT OnCVMValue(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;
	LRESULT OnCVMActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;

private:

	enum { nParts = 2, nPixPerBar = 4, nPtsPerBar = 16 } ;
	void DrawChannel(HDC hDC, int nIdx) ;
	void DrawGroup(int nIdx, HDC hDC, RECT &rc) ;
	int  ScreenToChannel(int nIdx, int nScrX) ;
	void GetChannelRect(int nIdx, RECT *rc) ;
	

	BOOL            m_bActive ;
	CRecord<DWORD> *m_pRecord ;
	WORD            m_wLast ;
	DWORD           m_dwMax ;
	CBrush          m_brBack ;
	CBrush          m_brBackCh ;
	CBrush          m_brChannel ;
	CBrush          m_brActive ;
	CRect           m_rcChan[nParts] ;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WDETECTVIEW_H__A395AAA8_BDAB_44B2_8FEC_8FEA8730D84E__INCLUDED_)
