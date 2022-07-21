#include "../CXURL.h"
//#include "CXURLSession.h"

using namespace CoreTransport;

CXURLSession::CXURLSession() 
{ 
	//_pendingConnections = new std::map<CTTarget*, std::pair<CXConnectionClosure, void*>>;
	//_connections = new std::map<CTTarget*, CXConnection*>;
}

CXURLSession::~CXURLSession()
{
	//delete _connections;
	//delete _pendingConnections;
}


std::pair<CXConnectionClosureFunc, void*> CXURLSession::pendingConnectionForKey(CTTarget* target)
{
	return _pendingConnections.at(target);
}

void CXURLSession::addPendingConnection(CTTarget* target, std::pair<CXConnectionClosureFunc, void *> targetCallbackContext)
{
	//assert(_pendingConnections);
	_pendingConnections.insert(std::make_pair(target, targetCallbackContext));
	return;
}


void CXURLSession::removePendingConnection(CTTarget * target)
{
	_pendingConnections.erase(target);
}

void CXURLSession::addConnection(CTTarget *target, CXConnection * cxConnection)
{
	_connections.insert(std::make_pair(target, cxConnection));
}


void CXURLSession::removeConnection(CTTarget *target)
{
	_connections.erase(target);	
}
