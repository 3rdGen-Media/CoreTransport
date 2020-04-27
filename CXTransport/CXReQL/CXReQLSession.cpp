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


static int _CXReQLSessionConnectionCallback(CTError* err, CTConnection * conn)
{
	//conn should always exist, so it can contain the input CTTarget pointer
	//which contains the client context
	assert(conn);

	//Return variables
	CXConnectionClosure callback;
	CXConnection * cxConnection = NULL;

	//Retrieve CXReQLSession caller object from ReQLService input client context property
	CXReQLSession * cxReQLSession = (CXReQLSession*)(conn->target->ctx);

	//retrieve the client connection callback and target ctx (replace target context on target)
	std::pair<CXConnectionClosure, void*> targetCallbackContext = cxReQLSession->pendingConnectionForKey(conn->target);
	callback = targetCallbackContext.first;
	conn->target->ctx = targetCallbackContext.second;

	//  Complete the RethinkDB 2.3+ connection: Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
    err->id = CTReQLSASLHandshake(conn, conn->target);

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
		u_long nonblockingMode = 0;
		int iResult = ioctlsocket(conn->socket, FIONBIO, &nonblockingMode );
		if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);

		//For WIN32 each CTConnection gets its own pair of tx/rx iocp queues internal to the ReQL C API (but the client must assign thread(s) to these)
		//For Darwin + GCD, these queue handles are currently owned by NSReQLSession and still need to be moved into ReQLConnection->socketContext	
		//*Note:  On platforms where the SSL Context is not thread safe, such as Darwin w/ SecureTransport, we need to manually specify the same [GCD] thread queue for send and receive
		//because the SSL Context API (SecureTransport) is not thread safe when reading and writing simultaneously

		//Wrap the CTConnection in a C++ CXConnection Object
		//A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
		cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);

		//  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
		if (cxConnection) cxReQLSession->addConnection(conn->target, cxConnection);
		//  Add to error connections for retry?
		//else

	}

	//  Always remove pending connections
	cxReQLSession->removePendingConnection(conn->target);

	//issue the client callback
	callback(err, cxConnection);

	return err->id;
}

std::pair<CXConnectionClosure, void*> CXReQLSession::pendingConnectionForKey(CTTarget* target)
{
	return _pendingConnections.at(target);
}

void CXReQLSession::addPendingConnection(CTTarget* target, std::pair<CXConnectionClosure, void *> &targetCallbackContext)
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

void CXReQLSession::connect(ReqlService * service, CXConnectionClosure callback)
{
#ifdef _WIN32
	//Create a connection key for caching the connection and add to hash as a pending connection

	//convert port short to c string
	char port[6];
	_itoa((int)service->port, port, 10);

	//hijack the input target ctx variable
	void * clientCtx = service->ctx;
	service->ctx = this;

	//create connection key
	std::string connectionKey(service->host);
	connectionKey.append(":");
	connectionKey.append( port );

	//wrap ReQLService c struct ptr in CXReQLService object
	//CXReQLService * cxService = new CXReQLService(target);

	//Add target to map of pending connections
	addPendingConnection(service, std::make_pair(callback, clientCtx));
	

	//Issue the connection using CTC API 
	CTransport.connect(service, _CXReQLSessionConnectionCallback);

#elif defined(__APPLE__ )


#endif
	return;
}
