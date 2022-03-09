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


static int _CXURLSessionConnectionCallback(CTError* err, CTConnection * conn)
{
	//conn should always exist, so it can contain the input CTTarget pointer
	//which contains the client context
	assert(conn);

	//Return variables
	CXConnectionClosure callback;
	CXConnection * cxConnection = NULL;

	//Retrieve CXURLSession caller object from ReQLService input client context property
	CXURLSession * cxURLSession = (CXURLSession*)(conn->target->ctx);

	//retrieve the client connection callback and target ctx (replace target context on target)
	std::pair<CXConnectionClosure, void*> targetCallbackContext = cxURLSession->pendingConnectionForKey(conn->target);
	callback = targetCallbackContext.first;
	conn->target->ctx = targetCallbackContext.second;

	//  Complete the RethinkDB 2.3+ connection: Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
    //err->id = CTReQLSASLHandshake(conn, conn->target);

	//Parse any errors
	if (err->id != CTSuccess)
	{
		//The connection failed
		//parse the Error and bail out
		assert(1 == 0);

	}
	else //Handle successfull connection!!!
	//if (conn)
	{
		//For WIN32 each CTConnection gets its own pair of tx/rx iocp queues internal to the ReQL C API (but the client must assign thread(s) to these)
		//For Darwin + GCD, these queue handles are currently owned by NSReQLSession and still need to be moved into ReQLConnection->socketContext	
		//*Note:  On platforms where the SSL Context is not thread safe, such as Darwin w/ SecureTransport, we need to manually specify the same [GCD] thread queue for send and receive
		//because the SSL Context API (SecureTransport) is not thread safe when reading and writing simultaneously

		//if the client didn't supply tx/rx thread queues, create them now because CXConnection object requires them for initialization
		if (!conn->socketContext.txQueue)
		{
			if (CTSocketCreateEventQueue(&(conn->socketContext)) < 0)
			{
				printf("_CXURLSessionConnectionCallback::CTSocketCreateEventQueue failed\n");
				err->id = (int)conn->event_queue;
			}
		}

		assert(conn->socketContext.rxQueue);
		assert(conn->socketContext.txQueue);

		//Wrap the CTConnection in a C++ CXConnection Object
		//A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
		cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);

		//  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
		if (cxConnection) cxURLSession->addConnection(conn->target, cxConnection);
		//  Add to error connections for retry?
		//else

	}

	//  Always remove pending connections
	cxURLSession->removePendingConnection(conn->target);

	//issue the client callback
	callback(err, cxConnection);

	return err->id;
}

std::pair<CXConnectionClosure, void*> CXURLSession::pendingConnectionForKey(CTTarget* target)
{
	int x = 1;
	return _pendingConnections.at(target);
}

void CXURLSession::addPendingConnection(CTTarget* target, std::pair<CXConnectionClosure, void *> &targetCallbackContext)
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

void CXURLSession::connect(CTTarget * target, CXConnectionClosure callback)
{
#ifdef _WIN32
	//Create a connection key for caching the connection and add to hash as a pending connection

	//convert port short to c string
	char port[6];
	_itoa((int)target->port, port, 10);

	//hijack the input target ctx variable
	void * clientCtx = target->ctx;
	target->ctx = this;

	//create connection key
	std::string connectionKey(target->host);
	connectionKey.append(":");
	connectionKey.append( port );

	//wrap ReQLService c struct ptr in CXReQLService object
	//CXReQLService * cxService = new CXReQLService(target);

	//Add target to map of pending connections
	addPendingConnection(target, std::make_pair(callback, clientCtx));
	

	//Issue the connection using CTC API 
	CTransport.connect(target, _CXURLSessionConnectionCallback);

#elif defined(__APPLE__ )


#endif
	return;
}
