// User.h: interface for the CUser class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USER_H__44B01102_A28D_4527_BCBC_1815DD57BCB0__INCLUDED_)
#define AFX_USER_H__44B01102_A28D_4527_BCBC_1815DD57BCB0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IOCPSocket2.h"

class CVersionManagerDlg;
class CUser : public CIOCPSocket2  
{
public:
	void Initialize();
	void Parsing(Packet & pkt);
	void HandleVersion(Packet & pkt);
	void HandlePatches(Packet & pkt);
	void HandleLogin(Packet & pkt);
	void HandleServerlist(Packet & pkt);
	void HandleNews(Packet & pkt);
#if __VERSION >= 1453
	void HandleSetEncryptionPublicKey(Packet & pkt);
	void HandleUnkF7(Packet & pkt);
#endif

	CVersionManagerDlg* m_pMain;
};

void InitPacketHandlers(void);
typedef void (CUser::*LSPacketHandler)(Packet &);

#endif // !defined(AFX_USER_H__44B01102_A28D_4527_BCBC_1815DD57BCB0__INCLUDED_)
