// wDETECTView.cpp : implementation of the CWDETECTView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "records.h"
#include "Cfg.h"

#include "view.h"

BOOL CView::PreTranslateMessage(MSG* pMsg)
{
	pMsg;
	return FALSE;
}

void CView::DrawChannel(HDC hDC, int nIdx)
{
	ATLASSERT(0 <= nIdx && nIdx < m_Record.Lengh()) ;

	int nTop = m_rcGraph.bottom 
		- (int)(1.0 * m_rcGraph.Height() * m_Record[nIdx] / m_Record.GetMax() + 0.5) ;
	int nScrX = ChannelToScreen(nIdx) ;

	CDCHandle DC(hDC) ;
	DC.MoveTo(nScrX, nTop) ; DC.LineTo(nScrX, m_rcGraph.bottom) ;
}

void CView::DrawGraph(HDC hDC, RECT &rc)
{
	CRect rcBody = m_rcGraph ;
	rcBody.InflateRect(1, 1) ;

	CRect rcInter ;
	if(!rcInter.IntersectRect(&rc, &rcBody)) return ;

	CDCHandle DC(hDC) ;

	// Draw frame
	CPen Pen = (HPEN)::GetStockObject(BLACK_PEN) ;
	HPEN hOldPen = DC.SelectPen(Pen) ;
	HBRUSH hOldBrush = DC.SelectBrush(m_brBackCh) ;

	DC.Rectangle(rcBody) ;

	rcInter.IntersectRect(&rc, &m_rcGraph) ;

	int nLeft = ScreenToChannel(rcInter.left) ;
	int nRight = ScreenToChannel(rcInter.right - 1) ;

	while(rcInter.left <= ChannelToScreen(-- nLeft) && 0 <= nLeft) ;
	++ nLeft ;

	while(ChannelToScreen(++ nRight) < rcInter.right && nRight < m_Record.Lengh()) ;
	-- nRight ;

	DC.SelectPen(m_penChannel) ;

	for(int i = nLeft ; i <= nRight ; ++ i)
		DrawChannel(DC, i) ;

	if(nLeft <= m_Record.Last() && m_Record.Last() <= nRight)
	{
		DC.SelectPen(m_penActive) ;
		DrawChannel(DC, m_Record.Last()) ;
	}

	DC.SelectBrush(hOldBrush) ;
	DC.SelectPen(hOldPen) ;
}

void CView::DrawLegends(HDC hDC, RECT &rc)
{
	CDCHandle DC(hDC) ;
	HFONT hOldFont ;

	if(m_hFont != NULL) hOldFont = DC.SelectFont(m_hFont) ;

	CString Str ;

	CRect rcInter ;

	if(rcInter.IntersectRect(&rc, &m_rcMax))
	{
		Str.Format("%u", m_Record.GetMax()) ;
		DC.DrawText(Str, -1, &m_rcMax, DT_LEFT | DT_VCENTER) ;
	}

	if(rcInter.IntersectRect(&rc, &m_rcZero))
	{
		Str.Format("%d", 0) ;
		DC.DrawText(Str, -1, &m_rcZero, DT_LEFT | DT_VCENTER) ;
	}

	if(rcInter.IntersectRect(&rc, &m_rcLength))
	{
		Str.Format("%d", m_Record.Lengh() - 1) ;
		DC.DrawText(Str, -1, &m_rcLength, DT_RIGHT | DT_VCENTER) ;
	}

	if(m_hFont != NULL) DC.SelectFont(hOldFont) ;
}

int CView::ScreenToChannel(int nScrX)
{
	ATLASSERT(m_Record.Lengh()) ;

	int nIdx = nScrX - m_rcGraph.left ;
	nIdx = int(1.0 * nIdx / (m_rcGraph.Width() - 1) * (m_Record.Lengh() - 1) + 0.5) ;

	if(nIdx < 0 || m_Record.Lengh() <= nIdx) nIdx = -1 ;
	return nIdx ;
}

int CView::ChannelToScreen(int nIdx)
{
	ATLASSERT(m_Record.Lengh()) ;

	int nScrX = m_rcGraph.left ;
	nScrX += int(1.0 * nIdx / (m_Record.Lengh() - 1) * (m_rcGraph.Width() - 1)) ;

	return nScrX ;
}

void CView::CalcLegendRects()
{
	CClientDC DC(m_hWnd) ;

	HFONT hOldFont ;
	if(m_hFont != NULL)	hOldFont = DC.SelectFont(m_hFont) ;

	CString Str ;
	Str.Format("%u", MAXDWORD) ;

	DC.DrawText(Str, -1, &m_rcZero, DT_CALCRECT) ;
	m_rcLength = m_rcZero ;
	m_rcZero.MoveToXY(m_rcGraph.left, m_rcGraph.bottom + 1) ;
	m_rcLength.MoveToXY(m_rcGraph.right - m_rcLength.Width(), m_rcGraph.bottom + 1) ;

	DC.DrawText(Str, -1, &m_rcMax, DT_CALCRECT) ;
	m_rcMax.MoveToXY(m_rcGraph.left, m_rcGraph.top - m_rcMax.Height() - 1) ;

	if(m_hFont != NULL) DC.SelectFont(hOldFont) ;
}

LRESULT CView::OnCreate(LPCREATESTRUCT)
{
	// Create related GDI objects
	m_brBack = (HBRUSH)::GetStockObject(WHITE_BRUSH) ;
	m_brBackCh = (HBRUSH)::GetStockObject(WHITE_BRUSH) ;

	m_penActive.CreatePen(PS_SOLID, 0, RGB(255, 0, 0)) ;
	if(m_penActive.m_hPen == NULL) return -1 ;

	m_penChannel.CreatePen(PS_SOLID, 0, RGB(0,0, 255)) ;
	if(m_penChannel.m_hPen == NULL) return -1 ;

	// Legend layout
	CalcLegendRects() ;

	return 0 ;
}

/*
640 x 480
	25
	64	512 x 200	64
	25
		512 x 200
		25
	4 channels per Pixel == 2048 channels

*/

LRESULT CView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);

	CMemoryDC mdc(dc, dc.m_ps.rcPaint) ;

	mdc.FillRect(& dc.m_ps.rcPaint, m_bActive ? m_brBack : (HBRUSH)::GetStockObject(GRAY_BRUSH)) ;
	if(m_bActive)
	{
		DrawGraph(mdc, dc.m_ps.rcPaint) ;
		DrawLegends(mdc, dc.m_ps.rcPaint) ;
	}

	return 0 ;
}

void CView::OnSetFont(HFONT hFont, BOOL bRedraw)
{
	m_hFont = hFont ;
	CalcLegendRects() ;
	if(bRedraw) InvalidateRect(NULL) ;
}

LRESULT CView::OnCVMActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	m_bActive = (BOOL)wParam ;

	if(m_bActive)
	{
		m_Record = *(CRecord<DWORD>*)lParam ;
		ATLASSERT(lParam != NULL) ;
	}
	
	InvalidateRect(NULL) ;

	return 0 ;
}
