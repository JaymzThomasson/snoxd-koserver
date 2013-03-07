#include "stdafx.h"
#include "Ebenezer.h"
#include "EbenezerDlg.h"

BEGIN_MESSAGE_MAP(CEbenezerApp, CWinApp)
	//{{AFX_MSG_MAP(CEbenezerApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

CEbenezerApp::CEbenezerApp() {}
CEbenezerApp theApp;

BOOL CEbenezerApp::InitInstance()
{
	AfxEnableControlContainer();

	CEbenezerDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
