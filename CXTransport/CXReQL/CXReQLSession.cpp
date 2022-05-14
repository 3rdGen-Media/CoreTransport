#include "../CXReQL.h"
//#include "CXReQLSession.h"

using namespace CoreTransport;

CXReQLSession::CXReQLSession() 
{ 
	//_pendingConnections = new std::map<CTTarget*, std::pair<CXConnectionClosure, void*>>;
	//_connections = new std::map<CTTarget*, CXConnection*>;
}

CXReQLSession::~CXReQLSession()
{
	//delete _connections;
	//delete _pendingConnections;
}


std::pair<CXConnectionClosureFunc, void*> CXReQLSession::pendingConnectionForKey(CTTarget* target)
{
	return _pendingConnections.at(target);
}

void CXReQLSession::addPendingConnection(CTTarget* target, std::pair<CXConnectionClosureFunc, void *> targetCallbackContext)
{
	//assert(_pendingConnections);
	_pendingConnections.insert(std::make_pair(target, targetCallbackContext));
}

void CXReQLSession::removePendingConnection(CTTarget * target)
{
	_pendingConnections.erase(target);
}

void CXReQLSession::addConnection(CTTarget *target, CXConnection * cxConnection)
{
	_connections.insert(std::make_pair(target, cxConnection));
}


void CXReQLSession::removeConnection(CTTarget *target)
{
	_connections.erase(target);	
}
