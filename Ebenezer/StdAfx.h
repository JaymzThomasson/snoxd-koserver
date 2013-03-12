#pragma once

#define _WIN32_WINNT _WIN32_WINNT_MAXVER
#define WINVER 0x0600
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#define _CRT_SECURE_NO_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <winsock2.h>
#include <afxtempl.h>

#include "DBAgent.h"
#include "DatabaseThread.h"

#include "EbenezerDlg.h"
#include "User.h"
#include "AIPacket.h"

#include <iostream>
#include <fstream>

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.