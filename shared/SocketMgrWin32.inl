#ifdef CONFIG_USE_IOCP

	INLINE HANDLE GetCompletionPort() { return m_completionPort; }
	INLINE void SetCompletionPort(HANDLE cp) { m_completionPort = cp; }
	void CreateCompletionPort();

	static void SetupWinsock();
	static void CleanupWinsock();
	
	static DWORD WINAPI SocketWorkerThread(LPVOID lpParam);

	HANDLE m_completionPort;

#endif