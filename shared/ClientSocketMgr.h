#pragma once

#include <map>
#include "SocketMgr.h"
#include "KOSocket.h"

typedef std::map<uint16, KOSocket *> SessionMap;

template <class T>
class ClientSocketMgr : public KOSocketMgr<T>
{
public:
	ClientSocketMgr<T>() {}

 	virtual Socket *AssignSocket(SOCKET socket) { return NULL; }

	virtual ~ClientSocketMgr() {}
};
