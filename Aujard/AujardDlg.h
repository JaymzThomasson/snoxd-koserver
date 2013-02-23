// AujardDlg.h : header file
//

#if !defined(AFX_AUJARDDLG_H__B5274041_22AE_464F_86F6_53F992C2BF54__INCLUDED_)
#define AFX_AUJARDDLG_H__B5274041_22AE_464F_86F6_53F992C2BF54__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../shared/SharedMem.h"
#include "DBAgent.h"
#include "define.h"

struct OdbcError;
class CAujardDlg : public CDialog
{
// Construction
public:
	CAujardDlg(CWnd* pParent = NULL);	// standard constructor

	BOOL InitializeMMF();
	void ReportSQLError(OdbcError *pError);
	void WriteLogFile( char* pData );

	void AccountLogIn(Packet & pkt, int16 uid);
	void SelectNation(Packet & pkt, int16 uid);
	void AllCharInfoReq(Packet & pkt, int16 uid);
	void ChangeHairReq(Packet & pkt, int16 uid);
	void CreateNewChar(Packet & pkt, int16 uid);
	void DeleteChar(Packet & pkt, int16 uid);
	void SelectCharacter(Packet & pkt, int16 uid);
	void UserLogOut(Packet & pkt, int16 uid);
	void UserDataSave(Packet & pkt, int16 uid);
	void KnightsPacket(Packet & pkt, int16 uid);
	void CreateKnights(Packet & pkt, int16 uid);
	void JoinKnights(Packet & pkt, int16 uid);
	void WithdrawKnights(Packet & pkt, int16 uid);
	void ModifyKnightsMember(Packet & pkt, uint8 command, int16 uid);
	void DestroyKnights(Packet & pkt, int16 uid);
	void AllKnightsMember(Packet & pkt, int16 uid);
	void KnightsList(Packet & pkt, int16 uid);
	void SetLogInInfo(Packet & pkt, int16 uid);
	void UserKickOut(Packet & pkt);
	void BattleEventResult(Packet & pkt);
	void ShoppingMall(Packet & pkt, int16 uid);
	void LoadWebItemMall(Packet & pkt, int16 uid);
	void SkillDataProcess(Packet & pkt, int16 uid);
	void SkillDataSave(Packet & pkt, int16 uid);
	void SkillDataLoad(Packet & pkt, int16 uid);
	void FriendProcess(Packet & pkt, int16 uid);
	void RequestFriendList(Packet & pkt, int16 uid);
	void AddFriend(Packet & pkt, int16 uid);
	void RemoveFriend(Packet & pkt, int16 uid);

	void SaveUserData();
	void ConCurrentUserCount();
	_USER_DATA* GetUserPtr(const char* struserid, short & uid);
	void AllSaveRoutine();

	CSharedMemQueue	m_LoggerSendQueue;
	CSharedMemQueue	m_LoggerRecvQueue;

	HANDLE	m_hReadQueueThread;
	HANDLE	m_hMMFile;
	char*	m_lpMMFile;

	CDBAgent	m_DBAgent;

	int	m_nServerNo, m_nZoneNo;
	char m_strGameDSN[32], m_strAccountDSN[32];
	char m_strGameUID[32], m_strAccountUID[32];
	char m_strGamePWD[32], m_strAccountPWD[32];

	CFile					m_LogFile;

	int m_iLogFileDay;

// Dialog Data
	//{{AFX_DATA(CAujardDlg)
	enum { IDD = IDD_AUJARD_DIALOG };
	CListBox	m_OutputList;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAujardDlg)
	public:
	virtual BOOL DestroyWindow();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CAujardDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AUJARDDLG_H__B5274041_22AE_464F_86F6_53F992C2BF54__INCLUDED_)
