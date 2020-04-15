#include "../CXReQL.h"
//#include "CXReQLSession.h"

using namespace CoreTransport;

CXReQLSession::CXReQLSession() 
{ 
	//_pendingConnections = new std::map<ReqlService*, std::pair<CXReQLConnectionClosure, void*>>;
	//_connections = new std::map<ReqlService*, CXReQLConnection*>;
}

CXReQLSession::~CXReQLSession()
{
	//delete _connections;
	//delete _pendingConnections;
}


static int _CXReQLSessionConnectionCallback(ReqlError* err, CTConnection * conn)
{
	//conn should always exist, so it can contain the input ReqlService pointer
	//which contains the client context
	assert(conn);

	//Return variables
	CXReQLConnectionClosure callback;
	CXReQLConnection * cxConnection = NULL;

	//Retrieve CXReQLSession caller object from ReQLService input client context property
	CXReQLSession * cxReQLSession = (CXReQLSession*)(conn->service->ctx);

	//retrieve the client connection callback and service ctx (replace service context on service)
	std::pair<CXReQLConnectionClosure, void*> serviceCallbackContext = cxReQLSession->pendingConnectionForKey(conn->service);
	callback = serviceCallbackContext.first;
	conn->service->ctx = serviceCallbackContext.second;

	//  Complete the RethinkDB 2.3+ connection: Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
    err->id = CTReQLSASLHandshake(conn, conn->service);

	//Parse any errors
	if (err->id != ReqlSuccess)
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

		//Wrap the CTConnection in a C++ CXReQLConnection Object
		//A CXReQLConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
		cxConnection = new CXReQLConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);

		//  Store CXReQLConnection in internal NXReQLSession connections array/map/hash or whatever
		if (cxConnection) cxReQLSession->addConnection(conn->service, cxConnection);
		//  Add to error connections for retry?
		//else

	}

	//  Always remove pending connections
	cxReQLSession->removePendingConnection(conn->service);

	//issue the client callback
	callback(err, cxConnection);

	return err->id;
}

std::pair<CXReQLConnectionClosure, void*> CXReQLSession::pendingConnectionForKey(ReqlService* service)
{
	return _pendingConnections.at(service);
}

void CXReQLSession::addPendingConnection(ReqlService* service, std::pair<CXReQLConnectionClosure, void *> &serviceCallbackContext)
{
	//assert(_pendingConnections);
	_pendingConnections.insert(std::make_pair(service, serviceCallbackContext));
}

void CXReQLSession::removePendingConnection(ReqlService * service)
{
	_pendingConnections.erase(service);
}

void CXReQLSession::addConnection(ReqlService *service, CXReQLConnection * cxConnection)
{
	_connections.insert(std::make_pair(service, cxConnection));
}


void CXReQLSession::removeConnection(ReqlService *service)
{
	_connections.erase(service);	
}

void CXReQLSession::connect(ReqlService * service, CXReQLConnectionClosure callback)
{
#ifdef _WIN32
	//Create a connection key for caching the connection and add to hash as a pending connection

	//convert port short to c string
	char port[6];
	_itoa((int)service->port, port, 10);

	//hijack the input service ctx variable
	void * clientCtx = service->ctx;
	service->ctx = this;

	//create connection key
	std::string connectionKey(service->host);
	connectionKey.append(":");
	connectionKey.append( port );

	//wrap ReQLService c struct ptr in CXReQLService object
	//CXReQLService * cxService = new CXReQLService(service);

	//Add service to map of pending connections
	addPendingConnection(service, std::make_pair(callback, clientCtx));
	

	//Issue the connection using ReqlC API 
	RethinkDB.connect(service, _CXReQLSessionConnectionCallback);

#elif defined(__APPLE__ )


#endif
	return;
}
