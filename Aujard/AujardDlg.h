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

using namespace std;

/////////////////////////////////////////////////////////////////////////////
// CAujardDlg dialog

struct OdbcError;
class CAujardDlg : public CDialog
{
// Construction
public:
	CAujardDlg(CWnd* pParent = NULL);	// standard constructor

	BOOL InitializeMMF();
	void ReportSQLError(OdbcError *pError);
	void WriteLogFile( char* pData );

	void AccountLogIn(Packet & pkt);
	void SelectNation(Packet & pkt);
	void AllCharInfoReq(Packet & pkt);
	void CreateNewChar(Packet & pkt);
	void DeleteChar(Packet & pkt);
	void SelectCharacter(Packet & pkt);
	void UserLogOut(Packet & pkt);
	void UserDataSave(Packet & pkt);
	void KnightsPacket(Packet & pkt);
	void CreateKnights(Packet & pkt);
	void JoinKnights(Packet & pkt);
	void WithdrawKnights(Packet & pkt);
	void ModifyKnightsMember(Packet & pkt, uint8 command);
	void DestroyKnights(Packet & pkt);
	void AllKnightsMember(Packet & pkt);
	void KnightsList(Packet & pkt);
	void LoadKnightsAllList(Packet & pkt);
	void SetLogInInfo(Packet & pkt);
	void UserKickOut(Packet & pkt);
	void BattleEventResult(Packet & pkt);
	void ShoppingMall(Packet & pkt);
	void LoadWebItemMall(Packet & pkt);
	void SkillDataProcess(Packet & pkt);
	void SkillDataSave(Packet & pkt, short uid);
	void SkillDataLoad(Packet & pkt, short uid);
	void FriendProcess(Packet & pkt);
	void RequestFriendList(Packet & pkt);
	void AddFriend(Packet & pkt);
	void RemoveFriend(Packet & pkt);

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
