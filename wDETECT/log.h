#ifndef _LOG_H_
#define _LOG_H_

#ifdef LOGFACILITY

extern void LogOpen(LPCTSTR lpName) ;
extern void LogString(LPCSTR lpcStr) ;
extern void LogError(LPCSTR lpcPlace, DWORD dwErrCode) ;
extern DWORD LogErrorCode(LPCSTR lpcPlace) ;
extern void LogBytes(BYTE *lpBytes, int nLen) ;
extern void LogClose() ;

#define LOG_OPEN(name)			LogOpen((name))
#define LOG_STRING(str)			LogString((str))
#define LOG_ERROR(str , code)	LogError((str) , (code))
#define LOG_ERRORCODE(str)		LogErrorCode((str))
#define LOG_BYTES(bts , len)	LogBytes((bts) , (len))
#define LOG_CLOSE()				LogClose()

#else

#define LOG_OPEN(name)			{}
#define LOG_STRING(str)			{}
#define LOG_ERROR(str, code)	{}
#define LOG_ERRORCODE(str)		GetLastError() 
#define LOG_BYTES(bts , len)	{}
#define LOG_CLOSE()				{}

#endif		// LOGFACILITY

#endif // _LOG_H_
