#if !defined(AFX_RECORDS_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_)
#define AFX_RECORDS_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_

template <class T>
bool IsSignedType(T tValue)
{
	T tVal = ~0 ;
	return tVal < 0 ;
}

template <class T>
T GetMaxValue(T tValue)
{
	T tVal = 1 << (8 * sizeof(T) - 1) ;
	if(tVal < 0) 
		return ~tVal ;
	else
		return ~0 ;
}

template <class T>
T GetMinValue(T tValue)
{
	T tVal = 1 << (8 * sizeof(T) - 1) ;
	if(tVal < 0) 
		return tVal ;
	else
		return 0 ;
}

template <class T>
class ATL_NO_VTABLE CFixedPoint
{
public :
	T     m_tNumber ;
	BYTE  m_byFrac ;

	CFixedPoint(T tNum = 0, BYTE byFrac = 1) : m_tNumber(tNum), m_byFrac(byFrac) {}

	void SetFrac(BYTE byFrac)
	{
		if(byFrac == m_byFrac)
			return ;
		else if(m_tNumber == 0) 
			m_byFrac = byFrac ;
		else
		{
			int nFrac = (int)byFrac - (int)m_byFrac ;
			if(nFrac > 0)
				m_tNumber *= GetOrder(nFrac) ;
			else
				m_tNumber /= GetOrder(- nFrac) ;
			
			m_byFrac = byFrac ;
		}
	}

	T GetOrder(BYTE byFrac = 0xFF) const
	{
		if(byFrac == 0xFF) byFrac = m_byFrac ;
		T tOrder = 1 ;
		for(BYTE i = 0 ; i < byFrac ; ++ i, tOrder *= 10) ;
		return tOrder ;
	}

	inline T WholePart() const { return m_tNumber / GetOrder() ; }
	inline T FracPart() const { return abs(m_tNumber % GetOrder()) ; }
	inline T Trunc() const { return m_tNumber - m_tNumber % GetOrder() ; }
	inline T Frac() const { return m_tNumber % GetOrder() ; }
	inline BOOL IsSigned()  const { return  IsSignedType(m_tNumber) ; }

	operator T() const { return T ; }
	const CFixedPoint<T> &operator +=(T &tNum) { m_tNumber += tNum ; return m_tNumber ; }

	const CFixedPoint<T> &operator +=(const CFixedPoint<T> &fpNum)
	{
		return m_tNumber += fpNum.RiseTo(m_byFrac) ;
	}
	
	const CFixedPoint<T> &operator -=(T &tNum) { m_tNumber -= tNum ; return m_tNumber ; }

	const CFixedPoint<T> &operator -=(const CFixedPoint<T> &fpNum)
	{
		return m_tNumber -= fpNum.RiseTo(m_byFrac) ;
	}

	const CFixedPoint<T> &operator *=(const T tNum)
	{
		m_tNumber *= tNum ;
		return *this ;
	}

	const CFixedPoint<T> &operator *=(const CFixedPoint<T> &fpNum)
	{
		__int64 nRslt = m_tNumber * fpNum.m_tNumber ;
		T tFactOrder = fpNum.GetOrder() ;
		__int64 nRest = nRslt % tFactOrder ;
		nRslt /= nRest ;
		if(nRest >= 0)
			nRslt += 2 * nRest > tFactOrder ;
		else
			nRslt -= 2 * nRest <= - tFactOrder ;

		m_tNumber = (T)nRslt ;
		return *this ;
	}

	const CFixedPoint<T> &operator /=(const T tNum)
	{
		m_tNumber /= tNum ;
		return *this ;
	}

	const CFixedPoint<T> &operator /=(const CFixedPoint<T> &fpNum)
	{
		ATLASSERT(fpNum.m_tNumber != 0) ;

		__int64 nDeno = fpNum.m_tNumber ;
		__int64 nRslt = m_tNumber ;

		if(fpNum.m_byFrac >= m_byFrac)
			nRslt *= GetOrder(fpNum.m_byFrac - m_byFrac) ;
		else
			nDeno *= GetOrder(m_byFrac - fpNum.m_byFrac) ;
			
		__int64 nRest = nRslt % nDeno ;
		nRslt /= nDeno ;

		if(nRest > 0) 
			nRest += 2 * nRest >= abs(nDeno) ;
		else
			nRest -= 2 * nRest <= - abs(nDeno) ;

		m_tNumber = (T)nRslt ;
		return *this ;
	}


private :

	T RiseTo(BYTE byFrac) const
	{
		if(m_byFrac == byFrac) 
			return m_tNumber ;
		else
		{
			if(byFrac > m_byFrac)
				return m_tNumber * GetOrder(byFrac - m_byFrac) ;
			else 
			{
				T tOrder = GetOrder(m_byFrac - byFrac) ;
				T tRslt =  m_tNumber / tOrder ;
				if(tRslt >= 0)
					return tRslt + (2 * m_tNumber % tOrder >= tOrder) ;
				else
					return tRslt - (2 * m_tNumber % tOrder <= - tOrder) ;
			}
		}
	}
} ;

template <class T>
class CRecord
{
public:
	CRecord() : m_nLen(0), m_pT(NULL), m_nLast(0), m_nAllocated(0) {}

	CRecord(CRecord<T> &TRec)
	{
		if(AllocMem(TRec.Length())) Copy(TRec) ;
	}

	~CRecord()
	{
		Free() ;
	}

	void Free()
	{
		m_nAllocated = m_nLen = 0 ;
		delete m_pT ;
	}

	BOOL Allocate(int nLen)
	{
		if(AllocMem(nLen)) 
		{
			EnumMinMax() ;
			return TRUE ;
		}

		return FALSE ;
	}

	T GetAt(int n) const
	{
		ATLASSERT(0 <= n && n < m_nLen && m_pT != NULL) ;
		if(0 <= n && n < m_nLen)
			return m_pT[n] ;
		else
			return 0 ;
	}

	BOOL SetAt(int n, T t)
	{
		ATLASSERT(0 <= n && n < m_nLen && m_pT != NULL) ;

		BOOL bRslt = 0 <= n && n < m_nLen ;

		if(bRslt)
		{
			BOOL bReEnum = FALSE ;

			if(t > m_tMax)
			{
				m_tMax = t ; m_nMaxCnt = 1 ;
			}
			else if(t == m_tMax)
			{
				if(m_pT[n] != m_tMax) ++ m_nMaxCnt ;
			}
			else 
			{
				if(m_pT[n] == m_tMax)
					bReEnum = -- m_nMaxCnt == 0 ;	
			}

			if(t < m_tMin)
			{
				m_tMin = t ; m_nMinCnt = 1 ;
			}
			else if(t == m_tMin)
			{
				if(m_pT[n] != m_tMin) ++ m_nMinCnt ;
			}
			else 
			{
				if(m_pT[n] == m_tMin)
				{
					bReEnum = -- m_nMinCnt == 0 || bReEnum ;
				}
			}

			m_pT[n] = t ;
			m_nLast = n ;

			if(bReEnum) EnumMinMax() ;
		}
		
		return bRslt ;
	}

	void Reset()
	{
		ATLASSERT(m_pT != NULL) ;

		for(int i = 0 ; i < m_nLen ; ++ i)
			m_pT[i] = 0 ;

		m_tMin = 0 ; m_tMax = 0 ;
		m_nMinCnt = m_nLen ; m_nMaxCnt = 0 ;
	}

	inline int Lengh() const { return m_nLen ; }
	inline int Last() const { return m_nLast ; }
	inline T GetMax() const { return m_tMax ; }
	inline T GetMin() const { return m_tMin ; }

	operator const T*() const
	{
		return m_pT ;
	}

	inline T operator[](int n) const { return GetAt(n) ; }

	CRecord<T> &operator=(CRecord<T> &TRec)
	{
		if(AllocMem(TRec.Lengh())) Copy(TRec) ;
		return *this ;
	}

private:

	void Copy(CRecord &TRec)
	{
		memcpy(m_pT, TRec.m_pT, sizeof(T) * m_nLen) ;

		m_tMin = TRec.m_tMin ; m_nMinCnt = TRec.m_nMinCnt ;
		m_tMax = TRec.m_tMax ; m_nMaxCnt = TRec.m_nMaxCnt ;
		m_nLast = TRec.m_nLast ;
	}

	BOOL AllocMem(int nLen)
	{
		ATLASSERT(nLen > 0) ;

		if(m_nAllocated >= nLen) return TRUE ;

		if(m_pT) Free() ;
		m_pT = new T[nLen] ;
		
		if(m_pT) 
			m_nAllocated = m_nLen = nLen ; 

		return m_pT != NULL ;
	}

	BOOL EnumMinMax()
	{
		if(NULL == m_pT) return 0 ;

		m_tMin = GetMaxValue(*m_pT) ;
		m_tMax = GetMinValue(*m_pT) ;
		m_nMaxCnt = m_nMinCnt = 0 ;

		for(int i = 0 ; i < m_nLen ; ++ i)
		{
			if(m_pT[i] > m_tMax)
			{
				m_tMax = m_pT[i] ; m_nMaxCnt = 1 ;
			} 
			else if(m_pT[i] == m_tMax)
			{
				++ m_nMaxCnt ;
			}

			if(m_pT[i] < m_tMin)
			{
				m_tMin = m_pT[i] ; m_nMinCnt = 1 ;
			}
			else if(m_pT[i] == m_tMin)
			{
				++ m_nMinCnt ;
			}
		}

		return TRUE ;
	}

	T   *m_pT ;
	int  m_nLen ;

	T    m_tMin, m_tMax ;
	int  m_nMinCnt, m_nMaxCnt ;
	int  m_nLast ;
	int  m_nAllocated ;
} ;

#endif // !defined(AFX_RECORDS_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_)
