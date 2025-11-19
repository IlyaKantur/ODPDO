// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_)
#define AFX_STDAFX_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_

// Change these values to use different versions
#define WINVER		0x0400
//#define _WIN32_WINNT	0x0400
#define _WIN32_IE	0x0500
#define _RICHEDIT_VER	0x0100

#define _WTL_NEW_PAGE_NOTIFY_HANDLERS
#define _WTL_USE_CSTRING

#include <stdio.h>
#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>

#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlcrack.h>
#include <atlgdi.h>
#include <atlmisc.h>
#include <atlddx.h>
#include <atlctrlx.h>

#include <wininet.h>
#undef	BEGIN_MSG_MAP
#define	BEGIN_MSG_MAP(x) BEGIN_MSG_MAP_EX(x)

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_)
