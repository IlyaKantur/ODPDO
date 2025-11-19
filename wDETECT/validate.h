#ifndef _VALIDATE_H_
#define _VALIDATE_H_

// Version V0.04

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#pragma warning( push )
#pragma warning( disable : 4146 )

template<class T>
T vAbs(T tA)
{
	return tA < 0 ? - tA : tA ;
}

#pragma warning( pop )

template<class T>
T vMax(T tA, T tB)
{
	return tA < tB ? tB : tA ;
}

class ATL_NO_VTABLE CValidatorB {
public:

	enum StatusFlg { vsOK = 0, vsSyntax = 1 } ;
	enum { voFill = 0x0001, voTransfer = 0x0002, voOnAppend = 0x0003,
					voReserved = 0xFFFFFFF8 } ;
	enum TransferFlg { vtDataSize, vtSetData, vtGetData } ;

	enum VDataType
	{
		ddxDataNull = 0,
		ddxDataText = 1,
		ddxDataInt = 2,
		ddxDataFloat = 3,
		ddxDataDouble = 4,
		ddxDataFilter = 5,
		ddxDataRange = 6,
		ddxDataPicture = 7,
		ddxDataFixed = 8
	};

	struct VTextData
	{
		int nLength;
		int nMaxLength;
	};

	struct VIntData
	{
		long nVal;
		long nMin;
		long nMax;
	};

	struct VFloatData
	{
		double nVal;
		double nMin;
		double nMax;
	};

	struct VFilterData
	{
		LPCTSTR pFlt ;
	};

	struct VRangeData
	{
		LONGLONG nVal;
		LONGLONG nMin;
		LONGLONG nMax;
	};

	struct VPictureData
	{
		LPCTSTR pPicture ;
	};

	struct VFixedData
	{
		UINT     nFrac ;
		LONGLONG nMin ;
		LONGLONG nMax ;
		bool     bSgn ;
	};

	struct VData
	{
		enum VDataType nDataType;
		union
		{
			VTextData textData;
			VIntData intData;
			VFloatData floatData;
			VFilterData filterData ;
			VRangeData rangeData ;
			VPictureData pictureData ;
			VFixedData fixedData ;
		};
	};

	CValidatorB(enum VDataType nDataType) : m_dwOptions(0), m_dwStatus(0) {
		VData vData = { nDataType } ;
		m_vData = vData ;
	}

	const VData &GetVData() const { return m_vData ; }

	inline DWORD GetOptions() const { return m_dwOptions ; } 

	inline DWORD SetOptions(DWORD dwNewOpts)
	{
		DWORD dwTmp = m_dwOptions ;

		m_dwOptions = dwNewOpts ;
		return dwTmp ;
	} 

	inline DWORD GetStatus() const { return m_dwStatus ; }

protected:

	int Pos(LPCTSTR pcStr, TCHAR Ch, int nStart = 0) const
	{
		int nLen = lstrlen(pcStr) ;

		if(nStart >= nLen) return -1 ;

		for(int i = nStart ; i < nLen ; ++ i)
			if(pcStr[i] == Ch) break ;

		return i < nLen ? i : -1 ;
	}

	DWORD m_dwOptions ;
	DWORD m_dwStatus ;

	VData m_vData ;
} ;

#define WM_POSTINVALID			(WM_APP + 3)	// wParam -- VData struct, lParam -- m_hWnd

#define VALIDATE_REASONABLE_LEN			256

template <class TEdit,  class T>
class ATL_NO_VTABLE CValidator_ : public CMessageMap,
								public CValidatorB
{
public:

	bool bProcessFocus ;

	// Declarations //////////////////////////////////////////////////////////
	
	typedef class CValidator_<TEdit, T> CMaster_ ;

	BEGIN_MSG_MAP(CMaster_)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_PASTE, OnPaste)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

	// Interface /////////////////////////////////////////////////////////////
	
	CValidator_(enum VDataType nDataType) : CValidatorB(nDataType) {
		bProcessFocus = true ;
		m_dwStatus = vsSyntax ;
	}

	bool GetText(LPTSTR &pStr, int &nLen) 
	{
		TEdit *pTEdit = static_cast<TEdit*>(this) ;

		nLen = pTEdit -> GetWindowTextLength() ;

		if(NULL != pStr) delete pStr ;
		pStr = new TCHAR[++ nLen] ;

		bool bRslt = FALSE ;

		if(NULL != pStr)
			bRslt = pTEdit -> GetWindowText(pStr, nLen) + 1 == nLen ;

		return bRslt ;
	}

	bool GetText(int nMax, LPTSTR pStr)
	{
		TEdit *pTEdit = static_cast<TEdit*>(this) ;

		int nLen = pTEdit -> GetWindowTextLength() + 1 ;
		if(nLen > nMax) return false ;

		return pTEdit -> GetWindowText(pStr, nMax) ;
	}

//	bool GetText(BSTR &bStr)
//	{
//		TEdit *pTEdit = static_cast<TEdit*>(this) ;
//
//		return pTEdit -> GetWindowText(&bStr) ;
//	}

	bool IsValidText(bool bReportError = FALSE)
	{
		bool bRslt = true ;
		TEdit *pTEdit = static_cast<TEdit*>(this) ;

		if(pTEdit -> GetLineCount() <= 1) {		
			
			int nLen ;
			LPTSTR pStr = NULL ;

			bRslt = !GetText(pStr, nLen) ;
			if(!bRslt) {

				if(bReportError) 				
					bRslt = Valid(pStr) ;
				else
					bRslt = IsValid(pStr) ;
			} 

			delete pStr ;
		}
		
		return bRslt ;
	}

	// thunk s for quasi virtual s ///////////////////////////////////////////

	bool Valid(LPCTSTR pcStr)
	{ 
		T *pT = static_cast<T*>(this) ;
		bool bRslt = vsOK == m_dwStatus ? pT -> IsValid(pcStr) : false ;

		if(!bRslt) pT -> Error() ;
		return bRslt ;
	} ;

	inline void Error()
	{
		if(vsOK == m_dwStatus) {
			T *pT = static_cast<T*>(this) ;
			bProcessFocus = false ;
			pT -> Error_() ;
		}
	} ;

	inline bool IsValid(LPCTSTR pcStr)
	{ 
		T *pT = static_cast<T*>(this) ;
		return vsOK == m_dwStatus ? pT -> IsValid_(pcStr) : false ;
	} ;

	inline bool IsValidInput(LPTSTR &pStr, int &nLen, bool SuppressFill)
	{
		T *pT = static_cast<T*>(this) ;
		return vsOK == m_dwStatus ? pT -> IsValidInput_(pStr, nLen, SuppressFill) : false ;
	}

	inline int Transfer(LPVOID pBuf, int nBufSz, TransferFlg eFlg) 
	{
		T *pT = static_cast<T*>(this) ;
		return vsOK == m_dwStatus ? pT -> Transfer_(pBuf, nBufSz, eFlg) : 0 ;
	}

	// Common message handlers ///////////////////////////////////////////////

	LRESULT OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		TEdit *pTEdit = static_cast<TEdit*>(this) ;

		if(bProcessFocus && IsOur((HWND)wParam)) {
			UINT   nNextCtrl = ::GetDlgCtrlID((HWND)wParam) ;
			
			if(nNextCtrl != IDCANCEL && !IsValidText()) {

				pTEdit -> DefWindowProc(uMsg, wParam, lParam) ;
				bProcessFocus = false ;
				PostMessage(pTEdit -> GetParent(), WM_POSTINVALID, (WPARAM)&m_vData, (LPARAM) pTEdit -> m_hWnd) ;
			} 
		}

		bHandled = false ;
		return 0 ;
	}
	
	//  Responds to the GetDlgCode query according to the
	//  current state of the control.  If the edit control
	//  contains valid input, then TABs are allowed for
	//  changing focus.  Otherwise, requests that TABs be
	//  sent to Self, where we will generate the Invalid
	//  message (See WMKeyDown below). 

	LRESULT OnGetDlgCode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		TEdit *pTEdit = static_cast<TEdit*>(this) ;

		LRESULT lRslt = pTEdit -> DefWindowProc(uMsg, wParam, lParam) ;
		if(::GetFocus() == pTEdit -> m_hWnd)
			if(!IsValidText()) lRslt |= DLGC_WANTTAB ;
		
		return lRslt ;
	}
	
	// Validates Self whenever a character is entered.  Allows
	// the character entry to be processed normally, then validates
	// the result and restores Self's text to its original state
	// if there is an incorrect entry.
	//
	// By default, the SupressFill parameter of the IsValidInput
	// method call to the Validator is set to False, so that it
	// is free to modify the string, if it is so configured.

	LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		TEdit *pTEdit = static_cast<TEdit*>(this) ;
		TCHAR ChCode = (TCHAR)wParam ;

		if(VK_BACK == ChCode || pTEdit -> GetLineCount() > 1) {

			bHandled = FALSE ;
		} else {

			LPTSTR pOldStr = NULL, pStr = NULL ;
			int nOldLen, nLen ;

			if(!GetText(pOldStr, nOldLen)) {
				bHandled = FALSE ;
				delete pOldStr ;
				return 0 ;
			}

			bool bWasAppend ;
			int nStartPos, nEndPos ;
			int nOldStart, nOldEnd ;

			pTEdit -> GetSel(nOldStart, nOldEnd) ;
			bWasAppend = (nOldLen - 1) == nOldEnd ;

			pTEdit -> DefWindowProc(uMsg, wParam, lParam) ;
			
			if(GetText(pStr, nLen)) {
			
				pTEdit -> GetSel(nStartPos, nEndPos) ;

				if((m_dwOptions & voOnAppend) == 0 ||
						(bWasAppend && (nLen - 1 == nEndPos))) {

					if(!IsValidInput(pStr, nLen, false)) {

						pTEdit -> SetWindowText(pOldStr) ;
						nStartPos = nOldStart ; nEndPos = nOldEnd ;
					} else {

						pTEdit -> SetWindowText(pStr) ;


						if(nLen > nOldLen) {
							
							if(nStartPos > nOldLen) nStartPos = nLen ;
							if(nEndPos > nOldLen) nEndPos = nLen ;
						}
					}

					pTEdit -> SetSel(nStartPos, nEndPos) ;
				} else {

					if((nLen - 1) == nEndPos)
						if(!IsValidInput(pStr, nLen, false)) 
							Error() ;
				}
			}

			delete pStr ; delete pOldStr ;
		}

		return 0 ;	
	}

	//  If the TAB key is sent to the Edit Control, check
	//  the validity before allowing the focus to change.
	//  The control will only get a TAB if WMGetDlgCode (above)
	//  allows it, which is done when the control contains
	//  invalid input (we re-validate here just for completeness,
	//  in case descendants redefine any of this behavior).
	//
	//  We need to validate on TAB focus-changes because there
	//  is a case not handled by WMKillFocus: when focus is
	//  lost to an OK or CANCEL button by tabbing. 

	LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		TCHAR ChCode = (TCHAR)wParam ;

		if(VK_TAB == ChCode) {
			if(!IsValidText(TRUE)) return 0 ;
		}

		TEdit *pTEdit = static_cast<TEdit*>(this) ;

		if((voOnAppend & m_dwOptions) != 0 && pTEdit -> GetLineCount() <= 1) {

			int nStartSel, nEndSel ;
			pTEdit -> GetSel(nStartSel, nEndSel) ;

			LPTSTR pStr = NULL ;
			int nLen ;

			if(!GetText(pStr, nLen)) {
				bHandled = FALSE ;
				delete pStr ;
				return 0 ;
			}

			bool bWasAppend = nLen - 1 == nEndSel ;
			
			pTEdit -> DefWindowProc(uMsg, wParam, lParam) ;

			if(!bWasAppend) {
			
				pTEdit -> GetSel(nStartSel, nEndSel) ;
				if(GetText(pStr, nLen)) {
				
					if(nLen - 1 == nEndSel && !IsValidInput(pStr, nLen, false))
						Error() ;		
				}
			}

			delete pStr ;
			
		} else 

			bHandled = FALSE ;

		return 0 ;	
	}

	LRESULT OnPaste(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		TEdit *pTEdit = static_cast<TEdit*>(this) ;

		LPTSTR pOldStr = NULL, pStr = NULL ;
		int nOldLen, nLen ;

		if(GetText(pOldStr, nOldLen)) {

			int nStartPos, nEndPos ;
			pTEdit -> GetSel(nStartPos, nEndPos) ;

			pTEdit -> DefWindowProc(uMsg, wParam, lParam) ;

			if(GetText(pStr, nLen)) {

				if(!IsValidInput(pStr, nLen, false)) {

					pTEdit -> SetWindowText(pOldStr) ;
					pTEdit -> SetSel(nStartPos, nEndPos) ;
				}
			}

		} else {

			bHandled = FALSE ;
		}

		delete pStr ; delete pOldStr ;

		return 0 ;
	}

	// Quasi virtual methods /////////////////////////////////////////////////

	void Error_() {
		TEdit *pTEdit = static_cast<TEdit*>(this) ;	

		::SendMessage(pTEdit -> GetParent(),
						WM_POSTINVALID,
						(WPARAM)&m_vData,
						(LPARAM) pTEdit -> m_hWnd) ;
	}   

	bool IsValid_(LPCTSTR pcStr) const { return true ; }

	bool IsValidInput_(LPTSTR &pStr, int &nLen, bool SuppressFill) const  { return true ; }

	int Transfer_(LPVOID pBuf, int nBufSz, TransferFlg eFlg)
	{
		TEdit *pTEdit = static_cast<pTEdit*>(this) ;

		int nRslt = 0 ;

		switch(eFlg) {

			case vtDataSize :
				nRslt =  pTEdit -> GetWindowTextLength() + 1 ; break;

			case vtGetData : {
				nRslt = pTEdit -> GetWindowText(pBuf, nBufSz) ;
				if(!IsValid(pBuf)) nRslt = 0 ;
				break ;
			}

			case vtSetData : {
				nRslt = pTEdit -> SetWindowText(pBuf, nBufSz) ;
				break ;
			}	
		}

		return nRslt ;
	}

protected :

	bool IsOur(HWND hWnd)
	{
		DWORD dwThrdID, dwProcID ;
		dwThrdID = ::GetWindowThreadProcessId(hWnd, &dwProcID) ;

		return ::GetCurrentProcessId() == dwProcID &&
				::GetCurrentThreadId() == dwThrdID ;
	}

} ;

//////////////////////////////////////////////////////////////////////////
// CFilterValidator -- The first simple non-abstract descendant of 
//                     the CValidate class
//////////////////////////////////////////////////////////////////////////

template <class TEdit, class T>
class ATL_NO_VTABLE CFilterValidator_ : public CValidator_<TEdit, T>
{
public:
	CFilterValidator_(enum VDataType nDataType = ddxDataFilter) : CValidator_<TEdit, T>(nDataType) {
		m_pFilter = NULL ;
	}

	CFilterValidator_(LPCTSTR pFlt, enum VDataType nDataType = ddxDataFilter) : CValidator_<TEdit, T>(nDataType) {
		m_pFilter = NULL ;
		SetFilter(pFlt) ;
	}

	~CFilterValidator_() {
		delete m_pFilter ;
	}

	LPCTSTR GetFilter() const {
		return m_pFilter ;
	}

	bool SetFilter(LPCTSTR pFlt) {

		delete m_pFilter ;

		m_pFilter = new TCHAR[lstrlen(pFlt) + 1] ;
		bool bRslt = NULL != m_pFilter ;

		if(bRslt) lstrcpy(m_pFilter, pFlt) ;
		m_vData.filterData.pFlt = m_pFilter ;

		m_dwStatus = bRslt ? vsOK : vsSyntax ;

		return bRslt ;
	}

	//////////////////////////////////////////////////////////////////////////
	
	bool IsValid_(LPCTSTR pcStr) const { 
		return CheckValidity(pcStr) ;
	}

	bool IsValidInput_(LPTSTR &pStr, int &nLen, bool SuppressFill) const {
		return CheckValidity(pStr) ;
	}

protected :

	bool CheckValidity(LPCTSTR pcStr) const {

		int nFltLen = lstrlen(m_pFilter) ;
		int nLen = lstrlen(pcStr) ;

		for(int i = 0 ; i < nLen ; ++ i) {

			if(-1 == Pos(m_pFilter, pcStr[i], 0)) break ;

//			for (int ii = 0 ; ii < nFltLen ; ++ ii) 
//				if(pcStr[i] == m_pFilter[ii]) break ;
//			
//			if(ii == nFltLen) break ;
		}

		return i == nLen ;
	}

	LPTSTR m_pFilter ;
} ;

//////////////////////////////////////////////////////////////////////////
// CFilterValidator -- Inheritance closure
//////////////////////////////////////////////////////////////////////////

template <class TEdit>
class ATL_NO_VTABLE CFilterValidator : public CFilterValidator_< TEdit, CFilterValidator<TEdit> >
{ 
public:
	CFilterValidator() : CFilterValidator_< TEdit, CFilterValidator<TEdit> >() {} 
	CFilterValidator(LPCSTR pFilter) : CFilterValidator_< TEdit, CFilterValidator<TEdit> >(pFilter) {} 
} ;

//////////////////////////////////////////////////////////////////////////
// CRangeValidator
//////////////////////////////////////////////////////////////////////////

#define VALIDATE_FILTER_RANGE		_T("+-0123456789")
#define VALIDATE_RANGE_MAXCHARS		23

template <class TEdit, class TDigits, class T>
class ATL_NO_VTABLE CRangeValidator_ : public CFilterValidator_<TEdit, T>
{
public:
	CRangeValidator_(enum VDataType nDataType = ddxDataRange) : 
	  CFilterValidator_< TEdit, T > (VALIDATE_FILTER_RANGE, nDataType), m_tMin(0), m_tMax(0) {} 

	CRangeValidator_(TDigits tMn, TDigits tMx, enum VDataType nDataType = ddxDataRange) : 
	  CFilterValidator_< TEdit, T > (VALIDATE_FILTER_RANGE, nDataType)
	{
		SetRange(tMn, tMx) ;
	}

	inline void SetRange(TDigits tMin, TDigits tMax)
	{
		m_tMin = tMin ; m_tMax = tMax ;
		m_vData.rangeData.nMin = (LONGLONG)tMin ;
		m_vData.rangeData.nMax = (LONGLONG)tMax ;
	}

	inline void GetRange(TDigits &tMin, TDigits &tMax)
	{
		tMin = m_tMin ; tMax = m_tMax ;
	}

	//////////////////////////////////////////////////////////////////////////

	bool IsValid_(LPCTSTR pcStr) const
	{ 
		TDigits tRslt ;
		bool bRslt = StrToInt64(pcStr, tRslt, m_pFilter) ;

		bRslt &= m_tMin < tRslt && tRslt < m_tMax ;
		return bRslt ;
	} 

	int Transfer_(LPVOID pBuf, int nBufSz, enum TransferFlg eFlg)
	{ 
		TEdit *pTEdit = static_cast<TEdit*>(this) ;

		int nRslt = 0 ;

		switch(eFlg) {

			case vtDataSize :
				nRslt =  sizeof(TDigits) ; break;

			case vtGetData : {
				if(sizeof(TDigits) <= nBufSz) {
					LPTSTR pStr = NULL ;
					int nLen ;

					if(GetText(pStr, nLen)) {

						if(!StrToInt64(pStr, *(TDigits*)pBuf, m_pFilter)) 
							nRslt = 0 ;
						else
							nRslt = sizeof(TDigits) ;

					} else {
						nRslt = 0 ;
					}
					
					delete pStr ;
				}
				break ;
			}

			case vtSetData : {

				if(sizeof(TDigits) <= nBufSz) {

					LPTSTR pStr = (LPTSTR)_alloca(sizeof(TCHAR) * (VALIDATE_RANGE_MAXCHARS + 1)) ;
					
					if(Int64ToStr(pStr, VALIDATE_RANGE_MAXCHARS + 1, *(TDigits*)pBuf))
						nRslt = pTEdit -> SetWindowText(pStr) ? sizeof(TDigits) : 0 ;

				} else {
					nRslt = 0 ;
				}
				break ;
			}
		}

		return nRslt ;
	}

	//////////////////////////////////////////////////////////////////////////
	
protected :

	bool StrToInt64(LPCTSTR pcStr, TDigits &tRslt, LPCTSTR pcDigits, UINT uBase = 10) const
	{
		int nSgn = 1, nLen = lstrlen(pcStr) ;
		bool bRslt = nLen && nLen <= VALIDATE_RANGE_MAXCHARS ;

		if(bRslt) {

			int i = 0 ;
			if(Pos(pcDigits, pcStr[i]) == 1) {
				nSgn = -1 ; ++ i ;
			} else if(Pos(pcDigits, pcStr[i]) == 0)
				++ i ;

			TDigits tRslt = 0 ;

			for( ; i < nLen ; ++ i) {
				int nPos = Pos(m_pFilter, pcStr[i], 2) - 2 ;
				if(nPos < 0) break ;
				tRslt *= uBase ;
				tRslt += nPos ;
			}

			bRslt = nLen == i ;
			if(bRslt) tRslt = nSgn * tRslt ;
		}
		
		return bRslt ;
	}

	bool Int64ToStr(LPTSTR pStr, UINT nLen, TDigits tDigits, UINT uBase = 10) const
	{
		int nIdx = 0 ;

		if(tDigits < 0) {
			if(nLen < 1) return false ;
			pStr[nIdx ++] = _T('-') ;
		}

		for(TDigits tOrder = tDigits ; tDigits ; tDigits /= uBase, ++ nIdx) ;
		if(nIdx + 1 > nLen) return false ;

		pStr[nIdx --] = 0 ;		// eol
		do {
			pStr[nIdx --] = m_pFilter[2 + tDigits % uBase] ;
		} while(tDigits /= uBase) ;

		return true ;
	}

	inline bool IsSigned() const
	{
		TDigits tSgn = 0 ;
		return ~tSgn < 0 ;
	}

	LONGLONG m_tMin, m_tMax ;
} ;

//////////////////////////////////////////////////////////////////////////
// CRangeValidator -- inheritance closure
//////////////////////////////////////////////////////////////////////////

template <class TEdit, class TDigits>
class ATL_NO_VTABLE CRangeValidator : public CRangeValidator_< TEdit, TDigits, CRangeValidator<TEdit, TDigits> >
{
public:
	CRangeValidator() : CRangeValidator_< TEdit, TDigits, CRangeValidator<TEdit, TDigits> >() {} ;
	CRangeValidator(TDigits tMin, TDigits tMax) : CRangeValidator_< TEdit, TDigits, CRangeValidator<TEdit, TDigits> >(tMin, tMax) {} ;
};

//////////////////////////////////////////////////////////////////////////
// CPXPictureValidator -- the top of the scribble
//////////////////////////////////////////////////////////////////////////

#define VALIDATE_CHAR_DIGITS		_T("0123456789")
#define VALIDATE_CHAR_DIGITS_HEX	_T("0123456789ABCDEF")
#define VALIDATE_CHAR_SPECIAL		_T("#?&!@*{}[],")

class ATL_NO_VTABLE CPXPicture
{
public:
	enum PicResult { prComplete, prIncomplete, prEmpty, prError, prSyntax,
						prAmbiguous, prIncompNoFill } ;

	CPXPicture(LPCTSTR pDigits = VALIDATE_CHAR_DIGITS) :
		m_pPicture(NULL), m_pDigits(NULL), m_pPicDigits(NULL), m_pSpecial(NULL)
	{
		SetDigits(pDigits) ;
		SetPicDigits(VALIDATE_CHAR_DIGITS) ;
		SetSpecial(VALIDATE_CHAR_SPECIAL) ;
		
	}
	
	CPXPicture(LPCTSTR pPicture, LPCTSTR pDigits = VALIDATE_CHAR_DIGITS) :
		m_pPicture(NULL), m_pDigits(NULL), m_pPicDigits(NULL), m_pSpecial(NULL)
	{
		SetDigits(pDigits) ;
		SetPicDigits(VALIDATE_CHAR_DIGITS) ;
		SetPicture(pPicture) ;
		SetSpecial(VALIDATE_CHAR_SPECIAL) ;
	}

	~CPXPicture()
	{
		delete m_pPicture ;
		delete m_pSpecial ;
		delete m_pPicDigits ;
		delete m_pDigits ;
	}

	bool SetPicture(LPCTSTR pcPicture)
	{ 
		if(NULL != m_pPicture) delete m_pPicture ;

		if(!SyntaxCheck__(pcPicture)) {
			m_pPicture = NULL ;
			return false ;
		}

		int nLen = lstrlen((pcPicture)) + 1 ;
		m_pPicture = new TCHAR[nLen] ;
		if(NULL != m_pPicture) {
			lstrcpyn(m_pPicture, pcPicture, nLen) ;
			return true ;
		}

		return false ;
	}

	inline LPCTSTR GetPicture() const { return m_pPicture ; } 

	inline bool SetDigits(LPCTSTR pcDigits)
	{
		return SetField(pcDigits, m_pDigits) ;
	}

	inline LPCTSTR GetDigits() const { return m_pDigits ; } 

	inline bool SetPicDigits(LPCTSTR pcDigits)
	{ 
		return SetField(pcDigits, m_pPicDigits) ;
	}

	inline LPCTSTR GetPicDigits() const { return m_pPicDigits ; } 

	inline bool SetSpecial(LPCTSTR pcSpecial)
	{ 
		return SetField(pcSpecial, m_pSpecial) ;
	}

	inline LPCTSTR GetSpecial() const { return m_pSpecial ; } 

	inline bool IsEmpty() const
	{
		return NULL == m_pPicture || !m_pPicture[0] ;
	}

protected :

	bool SetField(LPCTSTR pcStr, LPTSTR &sField)
	{
		if(sField) delete sField ; sField = NULL ;

		if(pcStr) {
			sField = new TCHAR[lstrlen(pcStr) + 1] ;
			
			if(!sField) 
				return false ;
			else 
				lstrcpy(sField, pcStr) ;
		}

		return true ;
	}
	

	inline bool IsEmpty(LPCTSTR pcStr) const
	{
		return NULL == pcStr || !pcStr[0] ;
	}

	inline bool IsCharInSet__(TCHAR Ch, LPCTSTR pcStr) {
		return -1 != Pos(pcStr, Ch, 0) ;
	}

	inline bool IsDigit__(TCHAR Ch) {
		return IsCharInSet__(Ch, m_pDigits) ;
	}

	inline bool IsLetter__(TCHAR Ch) {
		return TRUE == IsCharAlpha(Ch) ;
	}

	inline bool IsSpecial__(TCHAR Ch) {
		return IsCharInSet__(Ch, m_pSpecial) ;
	}
	
	inline bool IsComplete__(enum PicResult eRslt) {
		return prComplete == eRslt || prAmbiguous == eRslt ;
	}

	inline bool IsIncomplete__(enum PicResult eRslt) {
		return prIncomplete == eRslt || prIncompNoFill == eRslt ;
	}

	typedef struct tagCONTEXT_ {
		int             i, j ;
//		enum PicResult  Rslt ;
		bool            Reprocess ;
		LPTSTR          pInputStr ;
	} CONTEXT_ ;

	typedef struct tagPROCCTX {
		CONTEXT_       *pCtx ;
//		enum PicResult  Rslt ;
		bool            Incomp ;
		int             OldI, OldJ, IncompI, IncompJ ;
		int            *pChTerm ;
	} PROCCTX_ ;

	// Consume Input
	void Consume___(PROCCTX_ &pcc, TCHAR Ch)
	{
		pcc.pCtx -> pInputStr[pcc.pCtx -> j - 1] = Ch ;
		++ pcc.pCtx -> i ; ++ pcc.pCtx -> j ;
	}

	// Skip a character or a picture group
	void ToGroupEnd___(PROCCTX_ &pcc, int &i) // *
	{
		int nBrkLevel = 0, nBrcLevel = 0 ;

		do {
			if(* pcc.pChTerm == i) break ;

			switch(m_pPicture[i - 1]) {
				case _T('[') :
					++ nBrkLevel ; break ;

				case _T(']') :
					-- nBrkLevel ; break ;

				case _T('{') : 
					++ nBrcLevel ; break ;

				case _T('}') : 
					-- nBrcLevel ; break ;

				case _T(';') :
					++ i ; break ;

				case _T('*') : {
					++ i ;
					while(IsDigit__(m_pPicture[(i ++) - 1])) ;
					ToGroupEnd___(pcc, i) ;
					continue ;
				}
			}

			++ i ;
			
		} while((nBrkLevel != 0) || (nBrcLevel != 0)) ;
		
	}

	// Find the a comma separator
	bool SkipToComma___(PROCCTX_ &pcc) // *
	{
		do {
			ToGroupEnd___(pcc, pcc.pCtx -> i) ;
		} while(*pcc.pChTerm != pcc.pCtx -> i && m_pPicture[pcc.pCtx -> i - 1] != _T(',')) ;

		pcc.pCtx -> i += m_pPicture[pcc.pCtx -> i - 1] == _T(',') ;  ;
		return pcc.pCtx -> i < *pcc.pChTerm ;
	}

	// Calculate the end of a group

	int CalcTerm___(PROCCTX_ &pcc) // *
	{
		int k = pcc.pCtx -> i ;

		ToGroupEnd___(pcc, k);
		return k ;
	}

	// The next group is repeated X times
	PicResult Iteration___(PROCCTX_ &pcc) // *
	{
		PicResult oRslt, eRslt = prError ;
		int itr = 0, k, l ;
		int NewTermCh ;

		++ pcc.pCtx -> i ; // Skip '*' 

		// Retrieve number 
		while(IsDigit__(m_pPicture[pcc.pCtx -> i - 1])) {
			itr = itr * 10 + m_pPicture[pcc.pCtx -> i - 1] - _T('0') ; // Change It for an universal digit type !!!
			++ pcc.pCtx -> i ;
		}

		if(pcc.pCtx -> i > *pcc.pChTerm) {
			eRslt = prSyntax ; return eRslt ;
		}
		
		k = pcc.pCtx -> i ;
		NewTermCh = CalcTerm___(pcc) ;

		// If Itr is 0 allow any number, otherwise enforce the number
		if(itr) {

			for(l = 1 ; l <= itr ; ++ l) {
				pcc.pCtx -> i = k ;
				oRslt = Process__(*pcc.pCtx, NewTermCh) ;

				if(!IsComplete__(oRslt)) {
					// Empty means incomplete since all are required
					if(prEmpty == oRslt) oRslt = prIncomplete ;
					eRslt = oRslt ; return eRslt ;
				}
			}
			
		} else {

			do {
				pcc.pCtx -> i = k ;
				oRslt = Process__(*pcc.pCtx, NewTermCh) ;
			} while(IsComplete__(oRslt));

			if (prEmpty == oRslt || prError == oRslt) {
				++ pcc.pCtx -> i ;
				oRslt = prAmbiguous ;
			}

		}

		pcc.pCtx -> i = NewTermCh ;
		return oRslt ;
	}

	// Process a picture group

	PicResult Group___(PROCCTX_ &pcc) // *
	{
		PicResult eRslt ;
		int TermCh = CalcTerm___(pcc) ;

		++ pcc.pCtx -> i ;
		eRslt = Process__(*pcc.pCtx, TermCh - 1) ;
		if(! IsIncomplete__(eRslt)) pcc.pCtx -> i = TermCh ;

		return eRslt ;
	}

	PicResult CheckComplete___(PROCCTX_ &pcc, enum PicResult oRslt) // *
	{
		int j = pcc.pCtx -> i ;
		
		if(IsIncomplete__(oRslt)) {

			// Skip optional pieces
			bool bFlg = true ;
			while(bFlg) {

				switch(m_pPicture[j - 1]) {

					case _T('[') : 
						ToGroupEnd___(pcc, j) ; break ;

					case _T('*') : {
						bFlg = !IsDigit__(m_pPicture[j]) ;
						if(bFlg) 
							ToGroupEnd___(pcc, ++ j) ;
						else
							bFlg = false ;
						break ;
					}

					default :
						bFlg = false ;
					}
			}


			if(j == *pcc.pChTerm) oRslt = prAmbiguous ;
		}

		return oRslt ;
	}
	
	PicResult Scan___(PROCCTX_ &pcc) // *
	{
		TCHAR Ch ;
		PicResult eRslt = prError, oRslt = prEmpty ;

		while (pcc.pCtx -> i != *pcc.pChTerm && m_pPicture[pcc.pCtx -> i - 1] != _T(',')) {

			if(pcc.pCtx -> j > lstrlen(pcc.pCtx -> pInputStr)) { 
				eRslt = CheckComplete___(pcc, oRslt);
				return eRslt ;
			}

			Ch = pcc.pCtx -> pInputStr[pcc.pCtx -> j - 1] ;

			switch(m_pPicture[pcc.pCtx -> i - 1]) {

				case _T('#') : 
					if(!IsDigit__(Ch)) 
						return eRslt ;
					else
						Consume___(pcc, Ch) ;
					break ;

				case _T('?') :
					if(!IsLetter__(Ch))
						return eRslt ;
					else
						Consume___(pcc, Ch) ;
					break ;

				case _T('&') :
					if(!IsLetter__(Ch)) 
						return eRslt ;
					else
						Consume___(pcc, (TCHAR)CharUpper((LPTSTR)Ch)) ;
					break ;

				case _T('!') :
					Consume___(pcc, (TCHAR)CharUpper((LPTSTR)Ch)) ; break ;

				case _T('@') :
					Consume___(pcc, Ch) ; break ;

				case _T('*') : {
					oRslt = Iteration___(pcc) ;
					if(!IsComplete__(oRslt)) return oRslt ;
					if(prError == oRslt) oRslt = prAmbiguous ;
					break ;
				}

				case _T('{') : {
					oRslt = Group___(pcc) ;
					if(!IsComplete__(oRslt)) 
						return oRslt ;
					break ;
				}

				case _T('[') : {
					oRslt = Group___(pcc) ;
					if(IsIncomplete__(oRslt)) return oRslt ;
					if(prError == oRslt) oRslt = prAmbiguous;
					break ;
				}

				default : { 
					if(m_pPicture[pcc.pCtx -> i - 1] == _T(';')) ++ pcc.pCtx -> i ;
					if(CharUpper((LPTSTR) m_pPicture[pcc.pCtx -> i - 1]) != CharUpper((LPTSTR)Ch))
						if(Ch == _T(' '))
							Ch = m_pPicture[pcc.pCtx -> i - 1] ;
						else 
							return eRslt ;

					Consume___(pcc, m_pPicture[pcc.pCtx -> i - 1]) ;
				}
			}

			if(prAmbiguous == oRslt)
				oRslt = prIncompNoFill ;
			else
				oRslt = prIncomplete ;
		}

		if(prIncompNoFill == oRslt)
			eRslt = prAmbiguous ;
		else
			eRslt = prComplete ;

		return eRslt ;
	}

	enum PicResult Process__(CONTEXT_ &ctx, int ChTerm)
	{
		PROCCTX_ pcc = { 0 } ;
		pcc.pCtx = &ctx ;
		pcc.Incomp = false ;
		pcc.OldJ = ctx.j ;
		pcc.OldI = ctx.i ;
		pcc.pChTerm = &ChTerm ;

		PicResult eRslt, oRslt ;
		
		do {
			oRslt = Scan___(pcc) ;

			// Only accept completes if they make it farther in the input
			// stream from the last incomplete 
			if((prAmbiguous == oRslt || prComplete == oRslt) &&
					pcc.Incomp && ctx.j < pcc.IncompJ ) {

				oRslt = prIncomplete ;
				ctx.j = pcc.IncompJ ;
			}

			if(prError == oRslt || prIncomplete == oRslt) {

				eRslt = oRslt ;
				if(!pcc.Incomp && prIncomplete == oRslt) {
					pcc.Incomp = true ;
					pcc.IncompI = ctx.i ;
					pcc.IncompJ = ctx.j ;					
				}

				ctx.i = pcc.OldI ;
				ctx.j = pcc.OldJ ;

				if (!SkipToComma___(pcc)) {
					if(pcc.Incomp) {
						eRslt = prIncomplete ;
						ctx.i = pcc.IncompI ;
						ctx.j = pcc.IncompJ ;
					}
					return eRslt ;
				}
				pcc.OldI = ctx.i ;
			}
			
		} while(prError == oRslt || prIncomplete == oRslt) ;

		if (oRslt == prComplete && pcc.Incomp)
			eRslt = prAmbiguous ;
		else
			eRslt = oRslt ;

		return eRslt ;
	}

	bool SyntaxCheck__(LPCTSTR pcPicture) // *
	{
		bool bRslt = false ;
		
		if(IsEmpty(pcPicture)) return bRslt ;

		int nLast = lstrlen(pcPicture) - 1 ; // idx was +1

		if(_T(';') == pcPicture[nLast]) return bRslt ;
		if(_T('*') == pcPicture[nLast] && _T(';') != pcPicture[nLast - 1])
			return bRslt ;

		int nBrkLevel = 0 , nBrcLevel = 0 ;

		for(int i = 0 ; i <= nLast ; ++ i) { // i was + 1

			switch(pcPicture[i]) {

				case _T('[') : 
					++ nBrkLevel ; break ;

				case _T(']') : 
					-- nBrkLevel ; break ;

				case _T('{') : 
					++ nBrcLevel ; break ;

				case _T('}') : 
					-- nBrcLevel ; break ;

				case _T(';') : 
					++ i ; break ;

			}	
		}

		bRslt = !(nBrkLevel || nBrcLevel) ;
		return bRslt ;
	}

	static int Pos(LPCTSTR pcStr, TCHAR Ch, int nStart = 0) 
	{
		int nLen = lstrlen(pcStr) ;

		if(nStart >= nLen) return -1 ;

		for(int i = nStart ; i < nLen ; ++ i)
			if(pcStr[i] == Ch) break ;

		return i < nLen ? i : -1 ;
	}

public:
	
	PicResult Picture(LPTSTR &pInputStr, bool bAutoFill) // *
	{
		CONTEXT_ ctx = { 0 } ;
		PicResult eRslt = prSyntax  ;

// 		if(!SyntaxCheck__(m_pPicture)) return eRslt ;

		eRslt = prEmpty ;
		if(0 == pInputStr[0]) return eRslt ;

		ctx.i = ctx.j = 1 ;

		ctx.pInputStr = pInputStr ;
		
		int nLen = lstrlen(m_pPicture) ;
		eRslt = Process__(ctx, nLen + 1) ; // PAS : nLen + 1
		if(prError != eRslt && prSyntax != eRslt && ctx.j <= lstrlen(pInputStr))
			eRslt = prError ;

		if(prIncomplete == eRslt && bAutoFill) {

			ctx.Reprocess = false ;

			while (ctx.i <= nLen && 
					!IsSpecial__(m_pPicture[ctx.i - 1])) {

				if(m_pPicture[ctx.i - 1] == _T(';')) ++ ctx.i ;
				pInputStr[lstrlen(pInputStr)] = m_pPicture[ctx.i - 1] ;
				++ ctx.i ;
				ctx.Reprocess = true ;
			}

			ctx.j = ctx.i = 1 ; //

			if(ctx.Reprocess) 
				eRslt = Process__(ctx, nLen + 1) ; // ?
		}

		if(prAmbiguous == eRslt) eRslt = prComplete ;
		else if(prIncompNoFill == eRslt) eRslt = prIncomplete ;

		return eRslt ;
	}

protected :
	LPTSTR m_pPicture ;
	LPTSTR m_pDigits ;
	LPTSTR m_pPicDigits ;
	LPTSTR m_pSpecial ;
} ;

// CPXPictureValidator //////////////////////////////////////////////////////////////////////

template <class TEdit, class T>
class ATL_NO_VTABLE CPXPictureValidator_ : public CValidator_<TEdit, T>
{
public :

	CPXPictureValidator_(enum VDataType nDataType = ddxDataPicture) : CValidator_<TEdit, T>(nDataType), m_Picture()
	{
		m_dwOptions |= voOnAppend ;
	} 

	CPXPictureValidator_(LPCTSTR pcPic, bool bAutoFill, enum VDataType nDataType = ddxDataPicture) : CValidator_<TEdit, T>(nDataType), m_Picture() 
	{
		m_dwOptions |= voOnAppend ;
		SetPicture(pcPic, bAutoFill) ;
	}

	bool SetPicture(LPCTSTR pcPic, bool bAutoFill) {

		if(m_Picture.SetPicture(pcPic)) {

			if(bAutoFill) m_dwOptions |= voFill ;
			m_vData.pictureData.pPicture = GetPicture() ;
			m_dwStatus = NULL != m_vData.pictureData.pPicture ? vsOK : vsSyntax ;
			return true ;
		}

		m_vData.pictureData.pPicture = NULL ;
		return false ;
	}

	inline LPCTSTR GetPicture() const {
		return m_Picture.GetPicture() ;
	}

//////////////////////////////////////////////////////////////////////////
	
	bool IsValid_(LPCTSTR pcStr) { 

		if(m_Picture.IsEmpty()) return true ;

		LPTSTR pTmp = AllocCopyStr(pcStr) ;

		if(NULL != pTmp) {
			CPXPicture::PicResult eRslt = m_Picture.Picture(pTmp, false) ;
			delete pTmp ;
			return CPXPicture::prEmpty == eRslt || CPXPicture::prComplete == eRslt ;
		} else {
			return false ;
		}
	} ;

	bool IsValidInput_(LPTSTR &pStr, int &nLen, bool SuppressFill)   {
		
		if(m_Picture.IsEmpty()) return true ;

		LPTSTR pTmp = AllocCopyStr(pStr, VALIDATE_REASONABLE_LEN) ;

		bool bRslt = NULL != pTmp ;
		if(bRslt) {
			bRslt = m_Picture.Picture(pTmp, m_dwOptions & voFill && !SuppressFill)
							!= CPXPicture::prError ;

			if(bRslt) {
				delete pStr ; pStr = pTmp ;
			} else {
				delete pTmp ;
			}
		}

		return bRslt ;
	}

//////////////////////////////////////////////////////////////////////////

protected :

	LPTSTR AllocCopyStr(LPCTSTR pcStr, int nLen = -1) 
	{
		if(-1 == nLen) nLen = lstrlen(pcStr) + 1 ;
		LPTSTR pRslt = new TCHAR[nLen] ; // throw ?
		if(NULL != pRslt) {
			memset(pRslt, 0, nLen * sizeof(TCHAR)) ;
			lstrcpyn(pRslt, pcStr, nLen) ;
		}
		return pRslt ;
	}

	CPXPicture m_Picture ;
} ;

//////////////////////////////////////////////////////////////////////////
// CPXPictureValidator -- inheritance closure
//////////////////////////////////////////////////////////////////////////

template <class TEdit>
class ATL_NO_VTABLE CPXPictureValidator : public CPXPictureValidator_<TEdit, CPXPictureValidator<TEdit> > 
{
	CPXPictureValidator() : CPXPictureValidator_<TEdit, CPXPictureValidator<TEdit> >() {} ;
	CPXPictureValidator(LPCTSTR pcPicture, bool bAutoFill) : CPXPictureValidator_<TEdit, CPXPictureValidator<TEdit> >(pcPicture, bAutoFill) {} ;
} ;

//////////////////////////////////////////////////////////////////////////
//  CFixedPointValidator_ -- mi-class for fixed point range validating
//////////////////////////////////////////////////////////////////////////

#define VALIDATE_FIXED_MAXCHARS			23			// sign, 20 digits, frac sign, zero

template <class TEdit, class TDigits, class T>
class ATL_NO_VTABLE CFixedPointValidator_ : public CPXPictureValidator_<TEdit, T>
{
public :

//	CFixedPointValidator_(enum VDataType nDataType = ddxDataFixed) : CPXPictureValidator_<TEdit, T>(ddxDataFixed), m_uFracPart(0), m_uWholePart(0), m_tMin(0), m_tMax(0) 
//	{
//		m_dwOptions &= ~voOnAppend ;
//	}

	CFixedPointValidator_(enum VDataType nDataType = ddxDataFixed) :
	   CPXPictureValidator_<TEdit, T>(nDataType), m_uFracPart(0), m_uWholePart(0), m_tMin(0), m_tMax(0)
	{
		m_dwOptions &= ~voOnAppend ;
		m_vData.fixedData.bSgn = IsSigned() ;
	}

	CFixedPointValidator_(UINT uFrac, TDigits tMin, TDigits tMax, enum VDataType nDataType = ddxDataFixed) :
		CPXPictureValidator_<TEdit, T>(nDataType)
	{
		m_dwOptions &= ~voOnAppend ;
		m_vData.fixedData.bSgn = IsSigned() ;

		SetLayout(uFrac, tMin, tMax) ;
	}

	bool SetRange(TDigits tMin, TDigits tMax)
	{
		if(tMax < tMin) {
			m_dwStatus = vsSyntax ;
			return false ;
		}

		TDigits tModNew = vMax(vAbs(tMin), vAbs(tMax)) ;
		TDigits tModOld = vMax(vAbs(m_tMin), vAbs(m_tMax)) ;

		while(tModNew && tModOld) {
			tModNew /= 10 ;
			tModOld /= 10 ;
		}

		if(!tModOld && !tModNew) {
			m_tMin = tMin ;  m_tMax = tMax ;
			return true ;
		}
		
		return SetLayout(m_uFracPart, tMin, tMax) ;	
	}

	bool SetLayout(UINT uFrac, TDigits tMin, TDigits tMax) 
	{
		bool bRslt = false ;

		do {
			m_dwStatus = vsSyntax ;

			if(tMax < tMin) break ;

			TDigits tDigits = ~(1 << (sizeof(TDigits) * 8 - 1)) ;
			for(UINT nOrder = 1 ; tDigits ; tDigits /= 10, ++ nOrder) ;
			if(nOrder < uFrac) break ;

			LPTSTR pNewPict = (LPTSTR)_alloca(sizeof(TCHAR) * VALIDATE_FIXED_MAXCHARS) ;

			TDigits tMod = vMax(vAbs(tMin), vAbs(tMax)) ;

			// Calculate i -- number of digits in the whole part
			for(UINT i = 0 ; i < uFrac ;  ++ i, tMod /= 10) ;
			for(i = 0 ; tMod != 0 ; tMod /= 10, ++ i) ;
			i = i ? i : 1 ;

			// Make pattern for whole part
			LPTSTR pPtr = pNewPict ;
			if(IsSigned()) {
				*pPtr = _T('[') ; ++ pPtr ;
				*pPtr = _T('-') ; ++ pPtr ;
				*pPtr = _T(']') ; ++ pPtr ;
			}
			pPtr = MakePattern(pPtr, i) ;

			if(uFrac) {
				*pPtr = _T('[') ; ++ pPtr ;
				*pPtr = _T('.') ; ++ pPtr ;
				pPtr = MakePattern(pPtr, uFrac) ;
				*pPtr = _T(']') ; ++ pPtr ;
			}
			
			*pPtr = 0 ;

			if(!SetPicture(pNewPict, false)) break ;

			m_uFracPart = uFrac ;
			m_tMin = tMin ; m_tMax = tMax ;

			m_vData.fixedData.nFrac = uFrac ;
			m_vData.fixedData.nMin = (LONGLONG)tMin ;
			m_vData.fixedData.nMax = (LONGLONG)tMax ;

			bRslt = true ;
		} while(false) ;

		return bRslt ;
	}

	void GetLayout(UINT &uFrac, TDigits &tMin, TDigits &tMax) const
	{
		uFrac = m_uFracPart ;
		tMin = m_tMin ; tMax = m_tMax ;
	}

	bool IsValid_(LPCTSTR pcStr) { 
		bool bRslt ;

		bRslt = CPXPictureValidator_<TEdit, T>::IsValid_(pcStr) ;
		if(bRslt && 0 != pcStr[0]) {
			TDigits tDigits ;
			bRslt = StrToDigits(pcStr, tDigits) ;

			if(bRslt)
				bRslt = m_tMin <= tDigits && tDigits <= m_tMax ;
		}
		
		return bRslt ;
	}

	int Transfer_(LPVOID pBuf, int nBufSz, enum TransferFlg eFlg)
	{ 

		int nRslt = 0 ;

		switch(eFlg) {

			case vtDataSize :
				nRslt = sizeof(TDigits) ; break ;

			case vtGetData : {
				if(nBufSz < sizeof(TDigits)) break ;

				TDigits *pData = (TDigits*)pBuf ;
				LPTSTR pStr = NULL ;
				int nLen ;

				if(GetText(pStr, nLen)) {
					if(StrToDigits(pStr, *pData))
						nRslt = sizeof(*pData) ;
				}

				delete pStr ;
				break ;
			}

			case vtSetData : {
				if(nBufSz < sizeof(TDigits)) break ;

				TDigits *pData = (TDigits*)pBuf ;
				LPTSTR pStr = (LPTSTR)_alloca(sizeof(TCHAR) * VALIDATE_FIXED_MAXCHARS) ;

				if(DigitsToStr(pStr, VALIDATE_FIXED_MAXCHARS, *pData)) {
					TEdit *pTEdit = static_cast<TEdit*>(this) ;
					pTEdit -> SetLimitText(VALIDATE_FIXED_MAXCHARS - 1) ;
					if(pTEdit -> SetWindowText((LPTSTR)pStr))
						nRslt = sizeof(*pData) ;
				}

				break ;
			}	
		}

		return nRslt ;
	}

protected :

	inline bool IsSigned() const
	{
		TDigits tQuery = 0 ;
		return ~tQuery < 0 ;
	}

	LPTSTR MakePattern(LPTSTR pStr, UINT nDigits) const
	{
		*pStr = _T('#') ; ++ pStr ;
		for(UINT i = 1 ; i < nDigits ; ++ i, ++ pStr) {
			*pStr = _T('[') ;
			++ pStr ;
			*pStr = _T('#') ;
		}
		
		for( ; i > 1 ; -- i, ++ pStr) {
			*pStr = _T(']') ;
		}

		return pStr ;
	}

	bool StrToDigits(LPCTSTR pcStr, TDigits &tDigits) const
	{
		int nSgn = 1 ;

		if(_T('-') == *pcStr) {

			if(IsSigned()) {
				nSgn = -1 ; ++ pcStr ;
			} else
				return false ;

		}

		tDigits = 0 ;
		while(_T('0') <= *pcStr && *pcStr <= _T('9')) {
			tDigits *= 10 ;
			tDigits += *pcStr - _T('0') ;
			++ pcStr ;
		}	

		UINT nFracOrder = m_uFracPart ;

		if(0 != *pcStr) {
			if(_T('.') != *pcStr) return false ;
			++ pcStr ;

			while(_T('0') <= *pcStr && *pcStr <= _T('9') && nFracOrder --) {
				tDigits *= 10 ;
				tDigits += *pcStr - _T('0') ;
				++ pcStr ;
			}

			if(*pcStr) return false ;
		}

		while(nFracOrder --) tDigits *= 10 ;

		tDigits *= nSgn ;
		return true ;
	}

	bool DigitsToStr(LPTSTR pStr, int nLen, TDigits tDigits) const
	{
		if(!nLen) return false ;

		int nIdx = 0 ;
		int nSgn = 1 ;

		if(IsSigned() && tDigits < 0) {
			if(1 == nLen) return false ;

			pStr[nIdx ++] = _T('-') ;
			nSgn = -1 ;
			tDigits *= -1 ;
		}

		// Determ number of digits in the whole part of the number
		TDigits tOrder = tDigits ;
		for(UINT i = 0 ; i < m_uFracPart ; ++ i, tOrder /= 10) ;
		for(i = 0 ; tOrder ; ++ i, tOrder /= 10) ;
		i = i ? i : 1 ;

		// The last char has the index:
		nIdx += i + m_uFracPart + (m_uFracPart != 0) ; // + 1 ; // + wholepart, frac, point, zero
		if(nIdx + 1 > nLen) return false ;

		pStr[nIdx --] = 0 ; // eol
		
		// Fraction part
		for(i = 0 ; i < m_uFracPart ; ++ i, -- nIdx, tDigits /= 10)
			pStr[nIdx] = _T('0') + (TCHAR)(tDigits % 10) ;

		// Whole part
		if(m_uFracPart)
			pStr[nIdx --] = _T('.') ;

		do {
			pStr[nIdx --] = _T('0') + (TCHAR)(tDigits % 10) ;
		} while(tDigits /= 10) ;

		return true ;
	}

	UINT    m_uFracPart, m_uWholePart ;
	TDigits m_tMax, m_tMin ;
} ;

//////////////////////////////////////////////////////////////////////////
// CFixedPointValidator -- inheritance closure
//////////////////////////////////////////////////////////////////////////

template <class TEdit, class TDigits>
class ATL_NO_VTABLE CFixedPointValidator : public CFixedPointValidator_< TEdit, TDigits, CFixedPointValidator<TEdit, TDigits> >
{
public:
	CFixedPointValidator() : CFixedPointValidator_< TEdit, TDigits, CFixedPointValidator<TEdit, TDigits> >() {} ;
	CFixedPointValidator(UINT uFrac, TDigits tMin, TDigits tMax) : 
		CFixedPointValidator_< TEdit, TDigits, CFixedPointValidator<TEdit, TDigits> >(uFrac, tMin, tMax) {} 
} ;

//////////////////////////////////////////////////////////////////////////
// CFltWinDataExchange
//////////////////////////////////////////////////////////////////////////

#define ERRORBOX_HANDLER() \
	if(uMsg == WM_ERRORBOX) \
	{ \
		bHandled = TRUE; \
		lResult = 0 ; \
		if(wParam) \
		OnCustomDataValidateError(::GetDlgCtrlID((HWND)lParam), TRUE, *(CValidatorB::VData*)wParam) ; \
		return TRUE; \
	}


#define POSTINVALID_HANDLER() \
	if(uMsg == WM_POSTINVALID) \
	{ \
		bHandled = TRUE; \
		lResult = OnPostInvalid(uMsg, wParam, lParam, bHandled); \
		if(bHandled) \
			return TRUE; \
	}

#define BEGIN_RESETFOCUS_MAP(thisClass) \
bool m_bFree ; \
LRESULT OnPostInvalid(UINT, WPARAM wParam, LPARAM lParam, BOOL&) \
{ \
	if(m_bFree) { \
		m_bFree = false ; \
		CValidatorB::VData *pvData = (CValidatorB::VData*)wParam ; \
		UINT nCtrlID = ::GetDlgCtrlID((HWND)lParam) ; \
		if(NULL != pvData) { \
			OnCustomDataValidateError(nCtrlID, TRUE, *pvData) ; \
		} else \
			::SetFocus((HWND)lParam) ; \


#define RESETPROCESSFOCUS(nID, Edit)	\
		if(Edit.GetDlgCtrlID() == nID) Edit.bProcessFocus = true ;

#define END_RESETFOCUS_MAP() \
		m_bFree = true ; } \
	return 0 ; }

#define INIT_RESETFOCUS_MAP() \
	m_bFree = true ;

#define UNLOCK_PAGE() \
	m_bFree = true ;

#define LOCK_PAGE() \
	m_bFree = false ;

#define IS_LOCKED_PAGE() \
	m_bFree

//	BOOL DoDataExchange(BOOL bSaveAndValidate = FALSE, UINT nCtlID = (UINT)-1)
#define CDDX_INT64(wnd, var, nMn, nMx) \
	if((UINT)wnd.GetDlgCtrlID() == nCtlID || nCtlID == (UINT)(-1)) { \
		if(bSaveAndValidate && ! wnd.IsValidText()) { \
			OnCustomDataValidateError(wnd.GetDlgCtrlID(), bSaveAndValidate, wnd.GetVData()) ; \
			return FALSE ; \
		} \
		BOOL bSuccess = \
			sizeof(var) == wnd.Transfer(&var, sizeof(var), bSaveAndValidate ? wnd.vtGetData : wnd.vtSetData) ; \
		if(!bSuccess) { \
				OnDataExchangeError(wnd.GetDlgCtrlID(), bSaveAndValidate); \
				return FALSE ; } \
		if(!bSaveAndValidate) wnd.SetRange(nMn, nMx) ; \
	}

#define CDDX_PICTURE(wnd, var, picture, bAutoFill) \
	if((UINT)wnd.GetDlgCtrlID() == nCtlID || nCtlID == (UINT)(-1)) { \
		if(bSaveAndValidate && ! wnd.IsValidText()) { \
			OnCustomDataValidateError(wnd.GetDlgCtrlID(), bSaveAndValidate, wnd.GetVData()) ; \
			return FALSE ; \
		} \
		if(!DDX_Text((UINT)wnd.GetDlgCtrlID(), var, sizeof(var), bSaveAndValidate)) { \
			return FALSE ; \
		} \
		if(!bSaveAndValidate) wnd.SetPicture(picture, bAutoFill) ; \
	}

#define CDDX_FIXED_RANGE(wnd, var, frac, min, max) \
	if((UINT)wnd.GetDlgCtrlID() == nCtlID || nCtlID == (UINT)(-1)) { \
		if(bSaveAndValidate && ! wnd.IsValidText()) { \
			OnCustomDataValidateError(wnd.GetDlgCtrlID(), bSaveAndValidate, wnd.GetVData()) ; \
			return FALSE ; \
		} \
		if(!bSaveAndValidate) wnd.SetLayout(frac, min, max) ; \
		if(sizeof(var) != wnd.Transfer(&var, sizeof(var), bSaveAndValidate ? wnd.vtGetData : wnd.vtSetData )) \
			return FALSE ; \
	}

#define CDDX_FIXED(wnd, var) \
	if((UINT)wnd.GetDlgCtrlID() == nCtlID || nCtlID == (UINT)(-1)) { \
		if(bSaveAndValidate && ! wnd.IsValidText()) { \
			OnCustomDataValidateError(wnd.GetDlgCtrlID(), bSaveAndValidate, wnd.GetVData()) ; \
			return FALSE ; \
		} \
		if(sizeof(var) != wnd.Transfer(&var, sizeof(var), bSaveAndValidate ? wnd.vtGetData : wnd.vtSetData )) \
			return FALSE ; \
	}

#define DDX_COMBO_INDEX(nID, var) \
	if(nCtlID == (UINT)-1 || nCtlID == nID) \
	{ \
	if(!DDX_Combo_Index(nID, var, TRUE, bSaveAndValidate)) \
		return FALSE; \
	}

//////////////////////////////////////////////////////////////////////////

template<class T>
class ATL_NO_VTABLE CFltWinDataExchange : public CWinDataExchange<T>
{
public :

	void OnCustomDataValidateError(UINT nCtrlID, BOOL bSaveValidate, const CValidatorB::VData &vData) {
		ATLASSERT(FALSE) ;
	}

	bool ConvertVtoX(_XData &xData, const CValidatorB::VData &vData) {
		bool bRslt = true ;

		switch(vData.nDataType) {
			case CValidatorB::ddxDataNull :
			case CValidatorB::ddxDataText :
			case CValidatorB::ddxDataInt :
			case CValidatorB::ddxDataFloat :
			case CValidatorB::ddxDataDouble :
				memcpy(&xData, &vData, sizeof(xData)) ;
				break ;

			default:
				bRslt = false ;
		}

		return bRslt ;
	}

	template <class Type>
	BOOL DDX_Combo_Index(UINT nID, 
						 Type& nVal, 
						 BOOL bSigned, 
						 BOOL bSave, 
						 BOOL bValidate = FALSE, 
						 Type nMin = 0, 
						 Type nMax = 0)
	{
		T* pT = static_cast<T*>(this);
		BOOL bSuccess = TRUE;

		if(bSave)
		{
			nVal = ::SendMessage((HWND) (Type)pT->GetDlgItem(nID),
									CB_GETCURSEL,
									(WPARAM) 0,
									(LPARAM) 0);
									bSuccess = (nVal == CB_ERR ? false : true);	
		}
		else
		{
			ATLASSERT(!bValidate || nVal >= nMin && nVal <= nMax);
			int iRet = 	::SendMessage((HWND) (Type)pT->GetDlgItem(nID),
										CB_SETCURSEL,
										(WPARAM) nVal,
										(LPARAM) 0);

			bSuccess = (iRet == CB_ERR ? false : true);
		}
		
		if(!bSuccess)
		{
			pT->OnDataExchangeError(nID, bSave);
		}
		else if(bSave && bValidate)	// validation
		{
			ATLASSERT(nMin != nMax);
			if(nVal < nMin || nVal > nMax)
			{
				_XData data;
				data.nDataType = ddxDataInt;
				data.intData.nVal = (long)nVal;
				data.intData.nMin = (long)nMin;
				data.intData.nMax = (long)nMax;
				pT->OnDataValidateError(nID, bSave, data);
				bSuccess = FALSE;
			}
		}

		return bSuccess;
	}    


};


#endif // _VALIDATE_H_

//////////////////////////////////////////////////////////////////////////

/*
	int Transfer_(LPVOID pBuf, enum TransferFlg eFlg) const
	{ 

		int nRslt = 0 ;

		switch(eFlg) {

			case vtDataSize :
				nRslt = sizeof(nMax) ;	break;

			case vtGetData : {
				LONGLONG *pData = (LONGLONG*)pBuf ;
				LPTSTR pStr ;
				int nLen ;

				if(GetText(pStr, nLen)) {
					StrToInt64(pStr, *pData) ;
					nRslt = sizeof(*pData) ;
				}

				delete pStr ;
				break ;
			}

			case vtSetData : {
				LONGLONG *pData = (LONGLONG*)pBuf ;
				LPTSTR *pStr = (LPTSTR)_alloca(VALIDATE_RANGE_MAXCHARS) ;

				wsprintf(pStr, "%I64d", *pData) ;

				TEdit *pTEdit = static_cast<TEdit*>(this) ;
				pTEdit -> SetWindowText((LPTSTR)pStr) ;

				nRslt = sizeof(*pData) ;
				break ;
			}	
		}

		return nRslt ;
	}
*/