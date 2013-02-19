#pragma once

template <class T>
DWORD WINAPI ListenSocketThread(LPVOID lpParam)
{
	ListenSocket<T> * ls = (ListenSocket<T> *)lpParam;
	return ls->runnable() ? 0 : 1;
}

template<class T>
class ListenSocket
{
public:
	ListenSocket(SocketMgr *socketMgr, const char * ListenAddress, uint32 Port) : m_threadRunning(false), m_hThread(NULL)
	{
		m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

		// Enable blocking on the socket
		u_long arg = 0;
		ioctlsocket(m_socket, FIONBIO, &arg);

		m_address.sin_family = AF_INET;
		m_address.sin_port = ntohs((u_short)Port);
		m_address.sin_addr.s_addr = htonl(INADDR_ANY);
		m_opened = false;

		if (strcmp(ListenAddress, "0.0.0.0"))
		{
			struct hostent * hostname = gethostbyname(ListenAddress);
			if (hostname != NULL)
				memcpy(&m_address.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
		}

		// bind. well, attempt to...
		int ret = ::bind(m_socket, (const sockaddr*)&m_address, sizeof(m_address));
		if (ret != 0)
		{
			TRACE("Bind unsuccessful on port %u.\n", Port);
			return;
		}

		ret = listen(m_socket, 5);
		if (ret != 0) 
		{
			TRACE("Unable to listen on port %u.\n", Port);
			return;
		}

		m_opened = true;
		m_cp = socketMgr->GetCompletionPort();
		m_socketMgr = socketMgr;
	}

	~ListenSocket() { Close(); }

	bool run()
	{
		if (m_hThread != NULL)
			return false;

		DWORD id;
		m_hThread = CreateThread(NULL, 0, ListenSocketThread<T>, this, NULL, &id);
		return m_hThread != NULL;
	}

	void suspend()
	{
		if (m_hThread != NULL)
			SuspendThread(m_hThread);
	}

	void resume()
	{
		if (m_hThread != NULL)
			ResumeThread(m_hThread);
	}

	bool runnable()
	{
		struct sockaddr_in m_tempAddress;
		uint32 len = sizeof(sockaddr_in);
		m_threadRunning = true;

		// Remove blocking on the socket
		//u_long arg = 1;
		//ioctlsocket(m_socket, FIONBIO, &arg);
		while (m_opened && m_threadRunning)
		{
			//SOCKET aSocket = accept(m_socket, (sockaddr*)&m_tempAddress, (socklen_t*)&len);
			SOCKET aSocket = WSAAccept(m_socket, (sockaddr*)&m_tempAddress, (socklen_t*)&len, NULL, NULL);
			if (aSocket == INVALID_SOCKET)
			{
				//Sleep(10); // Don't kill the CPU!
				continue;
			}

			// Attempt to assign the socket to an available session
			Socket *socket = m_socketMgr->AssignSocket(aSocket);

			// No available sessions... unfortunately, we're going to have to let you go.
			if (socket == NULL)
			{
				shutdown(aSocket, SD_BOTH);
				closesocket(aSocket);
				continue;
			}
			socket->SetCompletionPort(m_cp);
			socket->Accept(&m_tempAddress);
		}
		return true;
	}

	void Close()
	{
		// prevent a race condition here.
		bool mo = m_opened;
		m_opened = false;

		if (mo)
		{
			shutdown(m_socket, SD_BOTH);
			closesocket(m_socket);
		}
	}

	__forceinline bool IsOpen() { return m_opened; }
	__forceinline HANDLE GetCompletionPort() { return m_cp; }

private:
	bool m_threadRunning;
	HANDLE m_hThread, m_cp;
	SocketMgr *m_socketMgr;
	SOCKET m_socket;
	struct sockaddr_in m_address;
	bool m_opened;
};
