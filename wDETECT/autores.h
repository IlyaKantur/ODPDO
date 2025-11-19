#ifndef _AUTORES_H_
#define _AUTORES_H_

#pragma once

//////////////////////////////////////////////////////////////////////////
// Exceptions
//////////////////////////////////////////////////////////////////////////

#ifdef _WTL_USE_CSTRING

class CWtlExceptionBase
{
protected:
	int m_nError;
	int m_nLine;
	LPTSTR m_pszError;
	const char* m_pszFile;
public:
	CWtlExceptionBase(int nLine, const char* pFile) throw()
		: m_nError(GetLastError())
		, m_nLine(nLine)
		, m_pszFile(pFile)
		, m_pszError(NULL)
	{
		FormatMessage();
	}

	CWtlExceptionBase(int nError, int nLine, const char* pFile)
	throw()
		: m_nError(nError)
		, m_nLine(nLine)
		, m_pszFile(pFile)
		, m_pszError(NULL)
	{
		FormatMessage();
	}

	~CWtlExceptionBase(void) throw()
	{
		if (m_pszError)
			::LocalFree(m_pszError);
	}

	inline int GetError(void) const throw() { return m_nError; }
	inline LPTSTR GetErrorString(void) const throw() { return m_pszError; }
	inline int GetLine(void) const throw() { return m_nLine; }
	inline const char* GetFile(void) const throw() { return m_pszFile; }
	virtual inline LPCTSTR what(void) const throw() = 0;

	int ReportError(HWND hWnd = NULL, LPCTSTR lpszCaption =
		NULL, UINT nFlags = MB_OK|MB_ICONEXCLAMATION) throw()
	{
		// Format a suitable error message
		CString str;
#ifdef _UNICODE
		str.Format(_T("Error: 0x%x\nDescription: %s\nFile: %S\nLine: %d"),
				m_nError, m_pszError, m_pszFile, m_nLine);
#else
		str.Format(_T("Error: 0x%x\nDescription: %s\nFile: %s\nLine: %d"),
				m_nError, m_pszError, m_pszFile, m_nLine);
#endif
		// Display
		return ::MessageBox(hWnd, str, lpszCaption == NULL ?
					what() : lpszCaption, nFlags);
	}

	int ReportUserError(HWND hWnd = NULL, LPCTSTR lpszCaption =
		NULL, UINT nFlags = MB_OK|MB_ICONEXCLAMATION) throw()
	{
		// Get the error string
		CString str = m_pszError;
		// Do we have an error message?
		if (str.IsEmpty())
		str.Format(_T("Error: 0x%x (%d)"), m_nError, m_nError);
		// Display
		return ::MessageBox(hWnd, str, lpszCaption == NULL ?
				what() : lpszCaption, nFlags);
	}

	void Trace(void) const throw()
	{
#ifdef _UNICODE
		ATLTRACE(_T("%s in file %S, line %d: 0x%x, %s"), what(), 
			m_pszFile, m_nLine, m_nError, m_pszError);
#else
		ATLTRACE(_T("%s in file %s, line %d: 0x%x, %s"), what(),
			 m_pszFile, m_nLine, m_nError, m_pszError);
#endif
}

	void FormatErrorMessage(CString& str) const throw()
	{
#ifdef _UNICODE
		str.Format(_T("%s in file %S, line %d: 0x%x, %s"),
				what(), m_pszFile, m_nLine, m_nError, m_pszError);
#else
		str.Format(_T("%s in file %s, line %d: 0x%x, %s"),
				what(), m_pszFile, m_nLine, m_nError, m_pszError);
#endif
		str.TrimRight();
	}

	CString GetErrorMessage(void) const throw()
	{
		CString str;
		FormatErrorMessage(str);
		return str;
	}

protected:

	void FormatMessage(void)
	{
		HMODULE hModule = NULL;
		DWORD dwFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER|
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS;

#ifndef _WTL_NO_WININET_EXCEPTIONS
		if (m_nError > INTERNET_ERROR_BASE && m_nError <= INTERNET_ERROR_LAST)
		{
			// WinInet error
			hModule = LoadLibraryEx(_T("wininet.dll"),
						NULL, LOAD_LIBRARY_AS_DATAFILE);
			if (hModule != NULL)
				dwFlags |= FORMAT_MESSAGE_FROM_HMODULE;
		}
#endif

		::FormatMessage(dwFlags,
				hModule,
				m_nError,
				MAKELANGID(LANG_NEUTRAL,
				SUBLANG_DEFAULT),
				(LPTSTR)&m_pszError,
				0,
				NULL);
		
	}
} ;

class CWtlException : public CWtlExceptionBase
{
public:
	CWtlException(int nLine, const char* pFile) throw()
		: CWtlExceptionBase(nLine, pFile)
	{
	}

	CWtlException(int nError, int nLine, const char* pFile) throw()
		: CWtlExceptionBase(nError, nLine, pFile)
	{
	}

	inline LPCTSTR what(void) const throw() { return _T("WtlException"); }
};

class CWtlCriticalException : public CWtlExceptionBase
{
public:
	CWtlCriticalException(int nLine, const char* pFile) throw()
		: CWtlExceptionBase(GetLastError(), nLine, pFile)
	{
		// Format the error message
		FormatMessage();
	}

	CWtlCriticalException(int n, int nLine, const char* pFile) throw()
		: CWtlExceptionBase(n, nLine, pFile)
	{
		// Format the error message
		FormatMessage();
	}

	inline LPCTSTR what(void) const throw() { return _T("WtlCriticalException"); }
};

#define WtlThrowException throw CWtlException(__LINE__, __FILE__)
#define WtlThrowExceptionCode(n) throw CWtlException((n), __LINE__, __FILE__)
#define WtlThrowCriticalException throw CWtlCriticalException(__LINE__, __FILE__)
#define WtlThrowCriticalExceptionCode(n) throw CWtlCriticalException((n), __LINE__, __FILE__)

#endif // _WTL_USE_CSTRING

//////////////////////////////////////////////////////////////////////////
// Error boxes
//////////////////////////////////////////////////////////////////////////

extern DWORD FormatErrorMessage(DWORD dwCode, LPVOID &lpMsgBuf) ;
extern void ErrorMsgBox(HWND hWnd, DWORD dwCode, DWORD dwCapID) ;

//////////////////////////////////////////////////////////////////////////
// Resource allocation and deallocation
//////////////////////////////////////////////////////////////////////////

template <class T> 
class CAutoPtr
{
public :
	explicit CAutoPtr(T *pT = NULL) throw() : m_pT(pT), m_bOwns(pT != NULL) {}
	CAutoPtr(const CAutoPtr<T> &Ptr) throw() : m_bOwns(Ptr.m_bOwns), m_pT(Ptr.Release()) {}

	~CAutoPtr()
	{
		if(m_bOwns) delete m_pT ;
	}
	
	CAutoPtr<T> &operator=(const CAutoPtr<T> &Ptr) throw()
	{ 
		if(&Ptr != this) {
			if (Ptr.Get() != m_pT) {
				if(m_bOwns) delete m_pT ;
				m_bOwns = Ptr.m_bOwns ;
			} else if(Ptr.m_bOwns) {
				m_bOwns = true ;
			}

			m_pT = Ptr.Release() ;
		}

		return *this ;
	}

	T &operator*() const throw() { return *Get() ; }
	T *operator->() const throw() { return Get() ; }
	T &operator[](int n) const { return m_pT[n] ; }

	T *Get() const throw() { return m_pT ; } 
	T *Release() const throw() { ((CAutoPtr<T>*) this) -> m_bOwns = false ; return m_pT ; }
	operator const T*() const throw() {	return Get() ; }
	inline bool IsNull() const throw() { return m_pT == NULL ; }

protected :
	T *m_pT ;
	bool m_bOwns ;
};

template <class T>
class CBoundArray
{
public:
	CBoundArray() : m_pT(NULL), m_nLength() {}
	CBoundArray(int nLength, T *pT) : m_pT(pT), m_nLength(nLength)
	{ 
		ATLASSERT(pT && nLength > 0) ;
	}

	BOOL Attach(int nLength, T *pT)
	{
		ATLASSERT(pT && nLength > 0) ;
		m_pT = pT ; m_nLength = nLength ;
		return pT && nLength ;
	}

	inline T* Get() const { return m_pT ; }
	inline operator T*() const { return Get() ; }
	inline T& operator[](int nIdx) const { return GetAt(nIdx) ; }
	inline int Length() const { return m_nLength ; }

	inline T& GetAt(int nIdx) const
	{
		ATLASSERT(0 <= nIdx && nIdx < m_nLength) ;
		return m_pT[nIdx] ;
	}

	inline void SetAt(int nIdx, T t)
	{
		ATLASSERT(0 <= nIdx && nIdx < m_nLength) ;
		m_pT[nIdx] = t ;
	}

private:
	T *m_pT ;
	int  m_nLength ;
};

class CAutoHandle
{
public :
	explicit CAutoHandle(HANDLE hndl = NULL) throw() : m_Handle(hndl), m_bOwns(hndl != NULL && hndl != INVALID_HANDLE_VALUE) {}
	CAutoHandle(const CAutoHandle &Handle) throw() : m_bOwns(Handle.m_bOwns), m_Handle(Handle.Release()) {}

	~CAutoHandle()
	{
		if(m_bOwns) CloseHandle(m_Handle) ;
	}

	CAutoHandle &operator=(const CAutoHandle &Handle) throw()
	{ 
		if(&Handle != this) {
			if (Handle.Get() != m_Handle) {
				if(m_bOwns) CloseHandle(m_Handle) ;
				m_bOwns = Handle.m_bOwns ;
			} else if(Handle.m_bOwns) {
				m_bOwns = true ;
			}

			m_Handle = Handle.Release() ;
		}

		return *this ;
	}

	CAutoHandle &operator=(const HANDLE hHandle) throw()
	{
		if(m_Handle != hHandle) {
			if(m_bOwns) CloseHandle(m_Handle) ;
			m_Handle = hHandle ;
			m_bOwns = hHandle != NULL && hHandle != INVALID_HANDLE_VALUE ;
		}

		return *this ;
	}

	inline operator HANDLE() const throw() { return Get() ; }
	inline HANDLE Get() const throw() { return m_Handle ; } 
	HANDLE Release() const throw() { ((CAutoHandle*) this) -> m_bOwns = false ; return m_Handle ; }

	inline bool IsInvalid() const throw() { return INVALID_HANDLE_VALUE == m_Handle ; }
	inline bool IsNull() const throw() { return NULL == m_Handle ; }
	inline bool IsValid() const throw() { return !IsInvalid() && !IsNull() ; }

protected :
	HANDLE m_Handle ;
	bool m_bOwns ;
};

class CMutex
{
public:
//	CMutex() throw() { InitializeCriticalSection(&m_CS) ; }
//	~CMutex() throw() { DeleteCriticalSection(&m_CS) ; }

	CMutex() throw() { CRITICAL_SECTION CS = { 0 } ; m_CS = CS ; }

	inline void Acquire() throw() {	InitializeCriticalSection(&m_CS) ; }
	inline void Release() throw() { DeleteCriticalSection(&m_CS) ; }
	inline void Lock() throw() { EnterCriticalSection(&m_CS) ; }
	inline void Unlock() throw() { LeaveCriticalSection(&m_CS) ; }

private:
	CRITICAL_SECTION m_CS ;
} ;
	
template <typename T>
class CLockingPtr {
public:
	// Constructors/destructors
	CLockingPtr(volatile T& obj, CMutex& mtx) : pObj_(const_cast<T*>(&obj)), pMtx_(&mtx)
	{ mtx.Lock(); }

	~CLockingPtr() { pMtx_ -> Unlock() ; }

	// Pointer behavior
	T& operator*() { return *pObj_ ; }
	T* operator->() { return pObj_ ; }

private:

	T* pObj_;
	CMutex* pMtx_;

	CLockingPtr(const CLockingPtr&) ;
	CLockingPtr& operator=(const CLockingPtr&) ;
};


#endif