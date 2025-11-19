// wDETECTView.h : interface of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WDETECTVIEW_H__A395AAA8_BDAB_44B2_8FEC_8FEA8730D84E__INCLUDED_)
#define AFX_WDETECTVIEW_H__A395AAA8_BDAB_44B2_8FEC_8FEA8730D84E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define CVM_ACTIVATE	WM_APP + 9		// wPar -- TRUE activate / FALSE deactivate, lPar -- pRecord

class CView : public CWindowImpl<CView>
{
public:
	DECLARE_WND_CLASS(NULL)

	CView() : m_bActive(FALSE), 
				m_rcGraph(64, 48, 64 + 512, 48 + 384),
				m_rcZero(0, 0, 0, 0),
				m_rcLength(0, 0, 0, 0),
				m_rcMax(0, 0, 0, 0),
				m_hFont(NULL)
	{}

	BOOL PreTranslateMessage(MSG* pMsg);

	BEGIN_MSG_MAP(CView)
		MSG_WM_CREATE(OnCreate)
		MSG_WM_ERASEBKGND(OnEraseBkgnd)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
		MSG_WM_SETFONT(OnSetFont)
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
	void OnSetFont(HFONT hFont, BOOL bRedraw) ;

	LRESULT OnCVMActivate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) ;

private:

	void DrawChannel(HDC hDC, int nIdx) ;
	void DrawGraph(HDC hDC, RECT &rc) ;
	void DrawLegends(HDC hDC, RECT &rc) ;
	int  ScreenToChannel(int nScrX) ;
	int  ChannelToScreen(int nIdx) ;
	void CalcLegendRects() ;
	
	BOOL            m_bActive ;
	CRecord<DWORD>  m_Record ;
	CBrush          m_brBack ;
	CBrush          m_brBackCh ;
	CPen            m_penChannel ;
	CPen            m_penActive ;
	CRect           m_rcGraph ;
	CRect           m_rcZero, m_rcLength, m_rcMax ;
	HFONT           m_hFont ;
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WDETECTVIEW_H__A395AAA8_BDAB_44B2_8FEC_8FEA8730D84E__INCLUDED_)
