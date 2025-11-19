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
	ATLASSERT(0 <= nIdx && nIdx < nChannels) ;
	if(nIdx % nPtsPerBar) return ;

	CDCHandle DC(hDC) ;
	CRect rc ;
	GetChannelRect(nIdx, &rc) ;

	DWORD dwMax = 0, dwValue ;
	for(int i = 0 ; i < nPtsPerBar ; ++ i)
	{
		dwValue = m_pRecord -> GetAt(i + nIdx) ;
		if(dwMax < dwValue) dwMax = dwValue ;
	}

//	RECT rcWhite = rc ;
//	rcWhite.bottom -= (int)(1.0 * rc.Height() / m_dwMax + 0.5) ;
//	rc.top = rcWhite.bottom ;
	rc.top = rc.bottom - (int)(1.0 * rc.Height() * dwMax / m_dwMax + 0.5) ;

	DC.FillRect(&rc, m_wLast / nPtsPerBar == nIdx / nPtsPerBar ? m_brActive : m_brChannel) ;
}

void CView::DrawGroup(int nIdx, HDC hDC, RECT &rc)
{
	ATLASSERT(0 <= nIdx && nIdx < nParts) ;

	CRect rcBody = m_rcChan[nIdx] ;
	rcBody.InflateRect(1, 1) ;

	CRect rcInter ;
	if(!rcInter.IntersectRect(&rc, &rcBody)) return ;
	rcBody.bottom ; rcBody.right ;

	WORD wRef = nChannels / 2 * nIdx ;
	CDCHandle DC(hDC) ;

	// Draw frame
	CPen Pen = (HPEN)::GetStockObject(BLACK_PEN) ;
	DC.SaveDC() ;
	DC.SelectPen(Pen) ;
	DC.SelectBrush(m_brBackCh) ;
	DC.Rectangle(rcBody) ;
	DC.RestoreDC(-1) ;

	int nLeft = ScreenToChannel(nIdx, rcInter.left) ;
	int nRight = ScreenToChannel(nIdx, rcInter.right) ;

	for(int i = nLeft ; i <= nRight ; ++ i)
		DrawChannel(DC, i) ;

}

int CView::ScreenToChannel(int nIdx, int nScrX)
{
	ATLASSERT(0 <= nIdx && nIdx < nParts) ;

	const int nChanPerPart = nChannels / nParts ;
	const int nMaxWidth = nChanPerPart / nPtsPerBar * nPixPerBar ;

	int nRslt = (nScrX - m_rcChan[nIdx].left) ;
	if(nRslt >= nMaxWidth) nRslt = nMaxWidth - 1 ;
	nRslt = nRslt / nPixPerBar * nPtsPerBar + nIdx * nChannels / nParts ;

	return nRslt ;
}

void CView::GetChannelRect(int nIdx, RECT *rc)
{
	ATLASSERT(0 <= nIdx && nIdx < nChannels) ;

	const int nChanPerPart = nChannels / nParts ;
	*rc = m_rcChan[nIdx / nChanPerPart] ;

	rc -> left += nIdx % nChanPerPart / nPtsPerBar * nPixPerBar ;
	rc -> right = rc -> left + nPixPerBar ;
}

LRESULT CView::OnCreate(LPCREATESTRUCT)
{
	m_brBack = (HBRUSH)::GetStockObject(WHITE_BRUSH) ;
	m_brBackCh = (HBRUSH)::GetStockObject(WHITE_BRUSH) ;

	m_brActive.CreateSolidBrush(RGB(255, 0, 0)) ;
	if(m_brActive.m_hBrush == NULL) return -1 ;

	m_brChannel.CreateSolidBrush(RGB(0,0, 255)) ;
	if(m_brChannel.m_hBrush == NULL) return -1 ;

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

	mdc.FillRect(& dc.m_ps.rcPaint, m_brBack) ;

	if(m_bActive)
		for(int i = 0 ; i < nParts ; ++ i)
			DrawGroup(i, mdc, dc.m_ps.rcPaint) ;

	return 0 ;
}

LRESULT CView::OnCVMValue(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	if(!m_bActive) return 0 ;

	WORD wChan = LOWORD(wParam) ;
	ATLASSERT(wChan < nChannels) ;

	DWORD dwNew = m_pRecord -> GetAt(wChan) + 1 ;
	m_pRecord -> SetAt(wChan, dwNew) ;
	
	if(m_dwMax < dwNew)
	{
		m_dwMax = dwNew ;
		m_wLast = wChan ;
		InvalidateRect(NULL) ;
	}
	else
	{
		RECT rc ;

		if(m_wLast >= nChannels) m_wLast = wChan ;

		if(m_wLast != wChan)
		{
			GetChannelRect(m_wLast, &rc) ;
			InvalidateRect(&rc, FALSE) ;
		}

		m_wLast = wChan ;
		GetChannelRect(m_wLast, &rc) ;
		InvalidateRect(&rc, FALSE) ;
	}
	
	return 0 ;
}

LRESULT CView::OnCVMActivate(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	if(m_bActive == (BOOL)wParam) return 0 ;

	m_bActive = (BOOL)wParam ;

	if(m_bActive)
	{
		m_pRecord = (CRecord<DWORD>*)lParam ;
		ATLASSERT(lParam != NULL) ;
	}
	else
	{
		m_wLast = nChannels ;
		m_pRecord = NULL ;
	}
	
	InvalidateRect(NULL) ;

	return 0 ;
}
