#include "../CXURL.h"
#include <memory>

using namespace CoreTransport;

void CXURLInterface::connect(CTTarget * target, CXConnectionClosure callback)
{
	CXURLSession * sharedSession = sharedSession->getInstance();
	sharedSession->connect(target, callback);
	//return CXReQLInterfaceSession.connect;
}

