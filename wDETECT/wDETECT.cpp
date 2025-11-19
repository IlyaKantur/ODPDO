// wDETECT.cpp : main source file for wDETECT.exe
//

#include "stdafx.h"
#include "autores.h"

#include "resource.h"
#include "records.h"
#include "Cfg.h"
#include "view.h"
#include "aboutdlg.h"
#include "tasks.h"
#include "MainFrm.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	if(!ConfigInit()) {
		ErrorMsgBox(NULL, ::GetLastError(), IDS_INIERROR) ;
	}

	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;

	CRect rc(g_Cfg.m_GUI.m_ptOrigin.x, g_Cfg.m_GUI.m_ptOrigin.y, CW_USEDEFAULT, CW_USEDEFAULT) ; 	  

	if(wndMain.CreateEx(NULL, &rc) == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();

	CongfigDone() ;
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
