#if !defined(AFX_TASKS_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_)
#define AFX_TASKS_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_

#define STD_STACK 16384

#define TSM_SAVE		WM_APP + 5		// Save the file, wPar == # of iteration
#define TSM_EXIT		WM_APP + 6
#define TSM_VALUE		WM_APP + 7		// wPar -- data
#define TSM_ERROR		WM_APP + 8		// wPar -- API error or lPar -- IO Error
#define TSM_OVER		WM_APP + 9		// Overflow event, wPar -- count value
#define TSM_MOTION		WM_APP + 10		// Motion state, wPar = TRUE/FALSE

struct TASKARGS
{
	PORTCFG SerCfg ;
	HWND	hMaster ;
	HANDLE	hCancel ;
} ;

extern DWORD WINAPI SerialTask(LPVOID) ;

struct PACKETARGS
{
	PORTCFG SerCfg ;
	HWND	hMaster ;
	HANDLE	hCancel ;
	DWORD	dwExpos ;
	DWORD   dwTicks ;
	DWORD	dwDelay ;
} ;

extern DWORD WINAPI PacketTask(LPVOID) ;

#endif // !defined(AFX_TASKS_H__8923A97E_4E9A_4B7D_B247_BE2583893605__INCLUDED_)
