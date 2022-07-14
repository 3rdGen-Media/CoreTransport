
//#ifndef __BLOCKS__
//#error must be compiled with -fblocks option enabled
//#endif

#ifdef _WIN32

#ifdef _DEBUG
#include <vld.h>                //Visual Leak Detector
#endif

#include "stdio.h"
#include "stdlib.h"
#include <process.h>            // threading routines for the CRT
#include "assert.h"

//CoreTransport can be optionally built with native SSL encryption OR against MBEDTLS
//#pragma comment(lib, "BlocksRuntime.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "secur32.lib")
//#pragma comment(lib, "ntdll.lib")
#else

/* blocks-test.c */

#include <Block.h>
#include <sys/thr.h>//FreeBSD threads
#include <pthread.h>//Unix pthreads

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#endif

//Keyboard Event Loop Includes and Definitions (TO DO: Move this to its own header file)
#ifndef _WIN32
//#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>

 #include <limits.h>
#include <stdlib.h>
#include <wchar.h>
#include <stdbool.h>
#include <string.h>
//#include "../../Git/libdispatch/dispatch/dispatch.h"

#include "/usr/include/sys/thr.h"

struct termios orig_termios;
typedef struct KEY_EVENT_RECORD
{
	bool bKeyDown;
	unsigned short wRepeatCount;
	unsigned short wVirtualKeyCode;
	unsigned short wVirtualScanCode;
	union {
		wchar_t UnicodeChar;
		char   AsciiChar;
	} uChar;
	unsigned long dwControlKeyState;
}KEY_EVENT_RECORD;

void reset_terminal_mode()
{
	tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode()
{
	struct termios new_termios;

	/* take two copies - one for now, one for later */
	tcgetattr(0, &orig_termios);
	memcpy(&new_termios, &orig_termios, sizeof(new_termios));

	/* register cleanup handler, and set the new terminal mode */
	atexit(reset_terminal_mode);
	cfmakeraw(&new_termios);
	tcsetattr(0, TCSANOW, &new_termios);
}

int kbhit()
{
	struct timeval tv = { 0L, 0L };
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &tv) > 0;
}

#endif //ifndef _WIN32

//CoreTransport CXURL and ReQL C++ APIs uses CTransport (CTConnection) API internally
#include <CoreTransport/CTransport.h>

static int httpRequestCount = 0;

#define HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS 1024
CTConnection HAPPYEYEBALLS_CONNECTION_POOL[HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS];

#define CT_MAX_INFLIGHT_CURSORS 1024
CTCursor _httpCursor[CT_MAX_INFLIGHT_CURSORS];
CTCursor _reqlCursor;

/***
 * Start CTransport (CTConnection) C API Example
 ***/

//Define a CTransport API CTTarget C style struct to initiate an HTTPS connection with a CTransport API CTConnection
static const char* 			http_server  = "3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com";//"example.com";// "vtransport - assets.s3.us - west - 1.amazonaws.com";//"example.com";//"mineralism.s3 - us - west - 2.amazonaws.com";
static const unsigned short	http_port	 = 80;

//Proxies use a prefix to specify the proxy protocol, defaulting to HTTP Proxy
static const char*			proxy_server = "http://172.20.10.1";//"54.241.100.168";
static const unsigned short	proxy_port	 = 443;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char* rdb_server = "rdb.server.net";
static const unsigned short rdb_port = 28015;
static const char* rdb_user = "rdb_username";
static const char* rdb_pass = "rdb_password";

//These are only relevant for custom DNS resolution libdill, loading the data from these files has not yet been implemented for WIN32 platforms
char * resolvConfPath = "./resolv.conf";
char * nsswitchConfPath = "./nsswitch.conf";

//Define SSL Certificate for ReqlService construction:
//	a)  For xplatform MBEDTLS support: The SSL certificate can be defined in a buffer OR a file AS a .cer in PEM format
//	b)  For Win32 SCHANNEL support:	   The SSL certificate must be pre-installed in a trusted root store on the system using MS .cer format
//  c)  For Darwin SecureTransport:	   The SSL certificate must be defined in a .der file with ... format
//	d)  For xplatform WolfSSL support: ...
//
char* caPath = NULL;
//const char * certPath = "RETHINKDB_CERT_PATH_OR_STR\0";

//Predefine ReQL query to use with CTransport C Style API (because message support is not provided at the CTransport/C level)
char* gamestateTableQuery = "[1,[15,[[14,[\"GameState\"]],\"Scene\"]]]\0";

//Define pointers and/or structs to keep reference to our secure CTransort HTTPS and RethinkDB connections, respectively
CTConnection _httpConn;
CTConnection _reqlConn;

char* httpHeaderLengthCallback(struct CTCursor* pCursor, char* buffer, unsigned long length)
{
	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = strstr(buffer, "\r\n\r\n");
	
	if (!endOfHeader)
	{
		pCursor->headerLength = 0;
		pCursor->contentLength = 0;
		return NULL;
	}
	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	if( pCursor->queryToken%3 == 0)
		pCursor->contentLength = 1641845;
	else if( pCursor->queryToken%3 == 1)
		pCursor->contentLength = 198744;
	else if( pCursor->queryToken%3 == 2)
		pCursor->contentLength = 572443;

	//pCursor->contentLength = 1256;
	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}

CTCursorCompletionClosure httpResponseClosure = ^void(CTError * err, CTCursor* cursor)
{
	fprintf(stderr, "httpResponseCallback (%d) header:  \n\n%.*s\n\n", (int)cursor->queryToken, (int)cursor->headerLength, cursor->requestBuffer);
	CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	CTCursorCloseFile(cursor);
};


uint64_t CTCursorRunQueryOnQueue(CTCursor* cursor, uint64_t queryToken)
{
	unsigned long queryHeaderLength, queryMessageLength;
	ReqlQueryMessageHeader* queryHeader = (ReqlQueryMessageHeader*)cursor->requestBuffer;

	//unsigned long requestHeaderLength, requestLength;

	//get the buffer for sending with async iocp
	//char * queryBuffer = cursor->requestBuffer;
	char* queryBuffer = cursor->requestBuffer + sizeof(ReqlQueryMessageHeader);
	char* baseBuffer = cursor->requestBuffer;

	//calulcate the request length
	queryMessageLength = strlen(queryBuffer);//requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

	//Populate the network message buffer with a header and the serialized query JSON string
	queryHeader->token = queryToken;//(conn->queryCount)++;//conn->token;
	queryHeader->length = queryMessageLength;

	//print the request for debuggin
	//printf("CXURLSendRequestWithTokenOnQueue::request (%d) = %.*s\n", (int)queryMessageLength, (int)queryMessageLength, baseBuffer);
	printf("CTCursorRunQueryOnQueue::queryBuffer (%d) = %.*s", (int)queryMessageLength, (int)queryMessageLength, cursor->requestBuffer + sizeof(ReqlQueryMessageHeader));
	queryMessageLength += sizeof(ReqlQueryMessageHeader);// + queryMessageLength;

	//cursor->overlappedResponse.cursor = cursor;
	cursor->conn = &_reqlConn;
	cursor->queryToken = queryToken;

	//Send the HTTP(S) request
	return CTCursorSendOnQueue(cursor, (char**)&baseBuffer, queryMessageLength);//, &overlappedResponse);
}


uint64_t CTCursorSendRequestOnQueue(CTCursor * cursor, uint64_t requestToken)	
{
	//a string/stream for serializing the json query to char*	
	unsigned long requestLength;

	//get the buffer for sending with async iocp
	char * requestBuffer = cursor->requestBuffer;
	char * baseBuffer = requestBuffer;
	
	//calculate the request length
	requestLength = strlen(requestBuffer);//requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

	//print the request for debuggin
	fprintf(stderr, "CTCursorSendRequestOnQueue::request (%d) = %.*s\n", (int)requestLength, (int)requestLength, baseBuffer);

	//cursor->overlappedResponse.cursor = cursor;
	cursor->conn = &_httpConn;
	cursor->queryToken = requestToken;

	//Send the HTTP(S) request
	return CTCursorSendOnQueue(cursor, (char **)&baseBuffer, requestLength);//, &overlappedResponse);
}

void createCursorResponseBuffers(CTCursor* cursor, const char* filepath)
{
	//unsigned long ctBufferSize = ct_system_allocation_granularity(); //+ sizeof(CTOverlappedResponse); 

	if (!filepath)
		filepath = "map.jpg\0";
	cursor->file.path = (char*)filepath;

	//file path to open
	CTCursorCreateMapFileW(cursor, cursor->file.path, ct_system_allocation_granularity() * 256UL);

	memset(cursor->file.buffer, 0, ct_system_allocation_granularity() * 256UL);
	
#ifdef _DEBUG
	assert(cursor->file.buffer);
#endif
}
void sendHTTPRequest(CTCursor* cursor)
{
	int requestIndex;

	//send buffer ptr/length
	char* queryBuffer;// = _gQueryBuffer;
	unsigned long queryStrLength;

	//send request
	char GET_REQUEST[1024] = "\0";
	char * imgName = "wood.png";

	if(httpRequestCount % 3 == 0)
		imgName = "wood.png";
	else if(httpRequestCount % 3 == 1)
		imgName = "brickwall.jpg";
	else if(httpRequestCount % 3 == 2)
		imgName = "brickwall_normal.jpg";

	//char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";
	sprintf(GET_REQUEST, "GET /img/textures/%s HTTP/1.1\r\nHost: 3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n", imgName);
	//GET_REQUEST = "GET /img/textures/wood.png HTTP/1.1\r\nHost: 3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";

	//file path to open
#ifndef _WIN32
	char filepath[1024] = "/home/jmoulton/3rdGen/CoreTransport/bin/img/http\0";
#else
	char filepath[1024] = "C:\\3rdGen\\CoreTransport\\bin\\img\\http\0";
#endif
	
	memset(cursor, 0, sizeof(CTCursor));

	//itoa(httpRequestCount, filepath + strlen(filepath), 10);
	snprintf(filepath + strlen(filepath), strlen(filepath), "%d", httpRequestCount);
	
	if(httpRequestCount % 3 == 0)
		strcat(filepath, ".png");
	else
		strcat(filepath, ".jpg");
		
	httpRequestCount++;

	//CoreTransport provides support for each cursor to read responses into memory mapped files
	//however, if this is not desired, simply cursor's file.buffer slot to your own memory
	createCursorResponseBuffers(cursor, filepath);

	assert(cursor->file.buffer);

	//cursor->file.buffer = cursor->requestBuffer;

	cursor->headerLengthCallback =  httpHeaderLengthCallback;
	cursor->responseCallback = 		httpResponseClosure;

	//printf("_conn.sslContextRef->Sizes.cbHeader = %d\n", _conn.sslContext->Sizes.cbHeader);
	//printf("_conn.sslContextRef->Sizes.cbTrailer = %d\n", _conn.sslContext->Sizes.cbTrailer);

	for (requestIndex = 0; requestIndex < 1; requestIndex++)
	{
		queryStrLength = strlen(GET_REQUEST);
		queryBuffer = cursor->requestBuffer;
		memcpy(queryBuffer, GET_REQUEST, queryStrLength);
		queryBuffer[queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminato
		fprintf(stderr, "sendHTTPRequest(): \n%s\n", queryBuffer);
		//CTSend(&_httpConn, queryBuffer, queryStrLength);
		CTCursorSendRequestOnQueue(cursor, _httpConn.queryCount);
	}
}

//int _httpConnectionClosure(CTError* err, CTConnection* conn) {
CTConnectionClosure _httpConnectionClosure = ^int(CTError * err, CTConnection * conn) {
	
	int i = 0;
	//TO DO:  Parse error status
	assert(err->id == CTSuccess);

	//Copy CTConnection object memory from coroutine stack
	//to appplication memory (or the connection will go out of scope when this funciton exits)
	//FYI, do not use the pointer memory after this!!!
	_httpConn = *conn;
	_httpConn.response_overlap_buffer_length = 0;
	
	fprintf(stderr, "HTTP Connection Success\n");
	
	//for(i=0; i<30; i++)
	//	sendHTTPRequest(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);

	return err->id;
};



char* reqlHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer + sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	cursor->contentLength = ((ReqlQueryMessageHeader*)buffer)->length;
	uint64_t token = ((ReqlQueryMessageHeader*)buffer)->token;
	assert(token == cursor->queryToken);

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}


CTCursorCompletionClosure reqlResponseClosure = ^void(CTError * err, CTCursor * cursor)
{
	printf("reqlCompletionCallback body:  \n\n%.*s\n\n", cursor->contentLength, cursor->file.buffer);
	//fprintf(stderr, "reqlResponseClosure (%d) header:  \n\n%.*s\n\n", (int)cursor->queryToken, (int)cursor->headerLength, cursor->requestBuffer);
	CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	CTCursorCloseFile(cursor);
};

void sendReqlQuery(CTCursor* cursor)
{
	int queryIndex;

	//send buffer ptr/length
	char* queryBuffer;
	unsigned long queryStrLength;

	//send request
	//char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";
	//char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";

	//file path to open
	char filepath[1024] = "C:\\3rdGen\\CoreTransport\\Samples\\HappyEyeballs\\reql\0";

	memset(cursor, 0, sizeof(CTCursor));

	_itoa(httpRequestCount, filepath + strlen(filepath), 10);

	strcat(filepath, ".txt");
	httpRequestCount++;

	//CoreTransport provides support for each cursor to read responses into memory mapped files
	//however, if this is not desired, simply cursor's file.buffer slot to your own memory
	createCursorResponseBuffers(cursor, filepath);
	//cursor->file.buffer = cursor->requestBuffer;
	
	assert(cursor->file.buffer);
	//if (!cursor->file.buffer)
	//	return;

	cursor->headerLengthCallback = reqlHeaderLengthCallback;
	cursor->responseCallback = reqlResponseClosure;

	//printf("_conn.sslContextRef->Sizes.cbHeader = %d\n", _conn.sslContext->Sizes.cbHeader);
	//printf("_conn.sslContextRef->Sizes.cbTrailer = %d\n", _conn.sslContext->Sizes.cbTrailer);

	//send query
	for (queryIndex = 0; queryIndex < 1; queryIndex++)
	{
		queryStrLength = (unsigned long)strlen(gamestateTableQuery);
		queryBuffer = cursor->requestBuffer;
		memcpy(queryBuffer + sizeof(ReqlQueryMessageHeader), gamestateTableQuery, queryStrLength);
		queryBuffer[sizeof(ReqlQueryMessageHeader) + queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminator
		printf("queryBuffer = %s\n", queryBuffer + sizeof(ReqlQueryMessageHeader));

		//CTCursorSendRequestOnQueue(cursor, _reqlConn.queryCount++);

		CTCursorRunQueryOnQueue(cursor, _reqlConn.queryCount++);
		////CTReQLRunQueryOnQueue(&_reqlConn, (const char**)&queryBuffer, queryStrLength, _reqlConn.queryCount++);
	}

}


//int _reqlHandshakeClosure(CTError* err, ReqlConnection* conn) {
CTConnectionClosure _reqlHandshakeClosure = ^ int(CTError * err, CTConnection * conn) {

	assert(conn == &_reqlConn);
	if (err->id == 0 && conn) { /*_reqlConn = conn;*/ }
	else
	{
		printf("CTReQLAsyncLHandshake failed!\n");
		CTCloseConnection(&_reqlConn);
		CTSSLCleanup();
		err->id = CTSASLHandshakeError;
		return err->id;
	}
	//createConnectionResponsBufferQueue(_reqlConn);
	printf("CTReQLAsyncLHandshake Success\n");

	return err->id;
};

//int _reqlConnectionClosure(CTError* err, CTConnection* conn) {
CTConnectionClosure _reqlConnectionClosure = ^ int(CTError * err, CTConnection * conn) {

	//status
	int status;

	//TO DO:  Parse error status
	assert(err->id == ReqlSuccess);

	//Copy CTConnection object memory from coroutine stack
	//to application memory (or the connection will go out of scope when this funciton exits)
	//FYI, do not use the pointer memory after this!!!
	_reqlConn = *conn;
	_reqlConn.response_overlap_buffer_length = 0;

	fprintf(stderr, "ReQL Connection Success\n");

	//  Now Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
	//  It is always safe to use Async ReQL Handshake after connection
	if (1)
	{
		if ((status = CTReQLAsyncHandshake(&_reqlConn, _reqlConn.target, _reqlHandshakeClosure)) != CTSuccess)
		{
			assert(1 == 0);
		}
	}
	else if ( (status = CTReQLHandshake(&_reqlConn, _reqlConn.target)) != CTSuccess)
	{
		printf("CTReQLSASLHandshake failed!\n");
		CTCloseConnection(&_reqlConn);
		CTSSLCleanup();
		err->id = CTSASLHandshakeError;
		return err->id;
	}

	/*
	 createCursorResponseBuffers(&_reqlCursor, filepath);
	 _reqlCursor.headerLengthCallback = reqlHeaderLengthCallback;
	 _reqlCursor.responseCallback = reqlResponseCallback;

	//1  Create a single worker thread, respectively, for the CTConnection's (Socket) Tx and Rx Queues
	//   We associate our associated processing callbacks for each by associating the callback with the thread(s) we associate with the queues
	//   Note:  On Win32, "Queues" are actually IO Completion Ports
	for( i = 0; i <1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CT_Dequeue_Recv_Decrypt, _reqlConn.socketContext.rxQueue, 0, NULL);
		CloseHandle(hThread);

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CT_Dequeue_Encrypt_Send, _reqlConn.socketContext.txQueue, 0, NULL);
		CloseHandle(hThread);
	}
	*/

	printf("REQL Connection Success\n");
	return err->id;
};

//Define crossplatform keyboard event loop handler
bool getconchar(KEY_EVENT_RECORD* krec)
{
#ifdef _WIN32
	DWORD cc;
	INPUT_RECORD irec;
	KEY_EVENT_RECORD* eventRecPtr;

	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	if (h == NULL) return false; // console not found

	for (;;)
	{
		//On Windows, we read console input and then ask if it's key down
		ReadConsoleInput(h, &irec, 1, &cc);
		eventRecPtr = (KEY_EVENT_RECORD*)&(irec.Event);
		if (irec.EventType == KEY_EVENT && eventRecPtr->bKeyDown)//&& ! ((KEY_EVENT_RECORD&)irec.Event).wRepeatCount )
		{
			*krec = *eventRecPtr;
			return true;
		}
	}
#else
	//On Linux we detect a keyboard hit using select and then read the console input
	if (kbhit())
	{
		int r;
		unsigned char c;

		//printf("\nkbhit!\n");
		if ((r = read(0, &c, sizeof(c))) == 1) {
			krec->uChar.AsciiChar = c;
			return true;
		}
	}
	//else	
	//	printf("\n!kbhit\n");
#endif
	return false; //future ????
}
void SystemKeyboardEventLoop()
{
#ifndef _WIN32	
	//user linux termios to set terminal mode from line read to char read
	set_conio_terminal_mode();
#endif
	fprintf(stderr, "\nStarting SystemKeyboardEventLoop...\n");
	KEY_EVENT_RECORD key;
	for (;;)
	{
		yield();

		memset(&key, 0, sizeof(KEY_EVENT_RECORD));
		getconchar(&key);
		if (key.uChar.AsciiChar == 's')
			sendHTTPRequest(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);
		if (key.uChar.AsciiChar == 'r')
			sendReqlQuery(CTGetNextPoolCursor());// &_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);
		else if (key.uChar.AsciiChar == 'y')
			yield();
		else if (key.uChar.AsciiChar == 'q')
		{
			break;
		}
		yield();
	}
}

int main(void) {

	//Declare intermediate variables
	int index;

	//Declare Coroutine Bundle Handle 
	//Note:  Needed for dill global couroutine malloc cleanup on application exit, while just a meaningless int on Win32 platforms
	CTRoutine coroutine_bundle;

	//Declare dedicated Kernel Queues for dequeueing socket connect, read and write operations in our network application
	CTKernelQueue cq, tq, rq;

	//Declare thread handles for thread pools
	CTThread cxThread, txThread, rxThread;

	//Declare Targets (ie specify TCP connection endpoints + options)
    CTTarget httpTarget  = { 0 };
	CTTarget reqlService = { 0 };	

	//Connection & Cursor Pool Initialization
	CTCreateConnectionPool(&(HAPPYEYEBALLS_CONNECTION_POOL[0]), HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS);
	CTCreateCursorPool(&(_httpCursor[0]), CT_MAX_INFLIGHT_CURSORS);
	
	//Kernel FileDescriptor Event Queue Initialization
	//If cq is *NOT* specified as input to a connection target, [DNS + Connect + TLS Handshake] will occur on the current thread.
	//If cq is specified as input to a connection target, [DNS + Connect] will occur asynchronously on a background thread(pool) and handshake delegated to tq + rq.
	//If tq or rq are not specified, CTransport internal queues will be assigned to the connection (ie async read/write operation operation post connection pipeline via tq + rq is *ALWAYS* mandatory).   
	cq = CTKernelQueueCreate();
	tq = CTKernelQueueCreate();
	rq = CTKernelQueueCreate();

	//Initialize Thread Pool for dequeing socket operations on associated kernel queues
	for (index = 0; index < 1; ++index)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool
		cxThread = CTThreadCreate(&cq, CT_Dequeue_Resolve_Connect_Handshake);
		rxThread = CTThreadCreate(&rq, CT_Dequeue_Recv_Decrypt);
		txThread = CTThreadCreate(&tq, CT_Dequeue_Encrypt_Send);

		//TO DO:  check thread failure
	}
    
	CTKernelQueue emptyQueue = {0};
	//Define https connection target
	httpTarget.url.host = (char*)http_server;
	httpTarget.url.port = http_port;
	//httpTarget.proxy.host = (char*)proxy_server;
	//httpTarget.proxy.port = proxy_port;
	httpTarget.ssl.ca = NULL;//(char*)caPath;
	httpTarget.ssl.method = CTSSL_NONE;//CTSSL_TLS_1_2;
	httpTarget.dns.resconf = (char*)resolvConfPath;
	httpTarget.dns.nssconf = (char*)nsswitchConfPath;
	httpTarget.cq = cq;
	httpTarget.tq = tq;
	httpTarget.rq = rq;


	//Define RethinkDB service connection target
	//We are doing SCRAM authentication over TLS required by RethinkDB
	//For SCHANNEL SSL Cert Verification, the certficate needs to be installed in the system CA trusted authorities store
	//For WolfSSL and SecureTransport Cert Verification, the path to the certificate can be passed 
	reqlService.url.host = (char*)rdb_server;
	reqlService.url.port = rdb_port;
	//reqlService.proxy.host = (char*)proxy_server;
	//reqlService.proxy.port = proxy_port;
	reqlService.user = (char*)rdb_user;
	reqlService.password = (char*)rdb_pass;
	reqlService.ssl.ca = (char*)caPath;
	reqlService.ssl.method = CTSSL_TLS_1_2;
	reqlService.dns.resconf = (char*)resolvConfPath;
	reqlService.dns.nssconf = (char*)nsswitchConfPath;
	reqlService.cq = cq;
	reqlService.tq = tq;
	reqlService.rq = rq;

	//Demonstrate CTransport C API Connection (No JSON support)
	//coroutine_bundle = CTransport.connect(&httpTarget, _httpConnectionClosure);
	coroutine_bundle = CTransport.connect(&reqlService, _reqlConnectionClosure);
	
	//Start a runloop on the main thread for the duration of the application
	SystemKeyboardEventLoop();

	//TO DO:  Wait for asynchronous threads to shutdown

	//Clean up socket connections (Note: closing a socket will remove all associated kevents on kqueues)
	//CTCloseConnection(&_httpConn);
	CTCloseConnection(&_reqlConn);

	//Clean Up Global/Thread Auth Memory
	ca_scram_cleanup();

	//Clean Up Global SSL Memory
	CTSSLCleanup();

	//Cleanup Thread Handles
	CTThreadClose(&cxThread);  //stop new connections first
	CTThreadClose(&txThread);  //stop sending next
	CTThreadClose(&rxThread);  //stop receiving last

	//close the dill coroutine bundle to clean up dill mallocs
	CTRoutineClose(coroutine_bundle);

    return 0;
}