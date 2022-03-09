
/***
**  
*	Core Transport -- HappyEyeballs
*
*	Description:  A test application demonstrating usage of CoreTransport C API
*	
*
*	@author: Joe Moulton
*	Copyright 2020 Abstract Embedded LLC DBA 3rdGen Multimedia
**
***/

//cr_display_sync is part of Core Render crPlatform library
#ifdef _DEBUG
#include <vld.h>                //Visual Leak Detector
#endif

#include "stdio.h"
#include "stdlib.h"
#include <process.h>            // threading routines for the CRT
#include "assert.h"

//CoreTransport can be optionally built with native SSL encryption OR against MBEDTLS
#ifndef CTRANSPORT_USE_MBED_TLS
#pragma comment(lib, "crypt32.lib")
//#pragma comment(lib, "user32.lib")
#pragma comment(lib, "secur32.lib")
#else
// mbded tls
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#endif


//CoreTransport CXURL and ReQL C++ APIs uses CTransport (CTConnection) API internally
#include <CoreTransport/CTransport.h>

#define HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS 1024
CTConnection HAPPYEYEBALLS_CONNECTION_POOL[HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS];

#define CT_MAX_INFLIGHT_CURSORS 1024
CTCursor _httpCursor[CT_MAX_INFLIGHT_CURSORS];
CTCursor _reqlCursor;

/***
 * Start CTransport (CTConnection) C API Example
 ***/

//Define a CTransport API CTTarget C style struct to initiate an HTTPS connection with a CTransport API CTConnection
static const char* http_server = "learnopengl.com";//"mineralism.s3 - us - west - 2.amazonaws.com";
static const unsigned short	http_port = 443;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char *			rdb_server = "RETHINKDB_SERVER";
static const unsigned short rdb_port = 18773;
static const char *			rdb_user = "RETHINKDB_USERNAME";
static const char *			rdb_pass = "RETHINKDB_PASSWORD";

//Define SSL Certificate for ReqlService construction:
//	a)  For xplatform MBEDTLS support: The SSL certificate can be defined in a buffer OR a file AS a .cer in PEM format
//	b)  For Win32 SCHANNEL support:	   The SSL certificate must be pre-installed in a trusted root store on the system using MS .cer format
//  c)  For Darwin SecureTransport:	   The SSL certificate must be defined in a .der file with ... format

//const char * certFile = "../Keys/MSComposeSSLCertificate.cer";
const char * certStr = "RETHINKDB_CERT_PATH_OR_STR\0";

//Predefine ReQL query to use with CTransport C Style API (because message support is not provided at the CTransport/C level)
char * paintingTableQuery = "[1,[15,[[14,[\"MastryDB\"]],\"Paintings\"]]]\0";

//Define pointers and/or structs to keep reference to our secure CTransort HTTPS and RethinkDB connections, respectively
CTConnection _httpConn;
CTConnection _reqlConn;


void createCursorResponseBuffers(CTCursor* cursor, const char* filepath)
{
#ifdef _WIN32
	//recv
	LPVOID queryPtr, responsePtr;
	unsigned long ctBufferSize = ct_system_allocation_granularity(); //+ sizeof(CTOverlappedResponse); 

	if (!filepath)
		filepath = "map.jpg\0";
	cursor->file.path = (char*)filepath;

	//file path to open
	CTCursorCreateMapFileW(cursor, cursor->file.path, ct_system_allocation_granularity() * 256L);

	cursor->overlappedResponse.buf = cursor->file.buffer;
	cursor->overlappedResponse.len = ctBufferSize;
	cursor->overlappedResponse.wsaBuf.buf = cursor->file.buffer;
	cursor->overlappedResponse.wsaBuf.len = ctBufferSize;
#endif

#ifdef _DEBUG
	assert(cursor->file.buffer);
#endif
}

uint64_t CTCursorRunQueryOnQueue(CTCursor * cursor, uint64_t queryToken)	
{
	unsigned long queryHeaderLength, queryMessageLength;
    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)cursor->requestBuffer;
    
	//unsigned long requestHeaderLength, requestLength;

	//get the buffer for sending with async iocp
	//char * queryBuffer = cursor->requestBuffer;
	char* queryBuffer = cursor->requestBuffer + sizeof(ReqlQueryMessageHeader);
	char * baseBuffer = cursor->requestBuffer;
	
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
	return CTCursorSendOnQueue(cursor, (char **)&baseBuffer, queryMessageLength);//, &overlappedResponse);
}

uint64_t CTCursorSendRequestOnQueue(CTCursor * cursor, uint64_t requestToken)	
{
	//a string/stream for serializing the json query to char*	
	unsigned long requestHeaderLength, requestLength;

	//get the buffer for sending with async iocp
	char * requestBuffer = cursor->requestBuffer;
	char * baseBuffer = requestBuffer;
	
	//calulcate the request length
	requestLength = strlen(requestBuffer);//requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

	//print the request for debuggin
	printf("CXURLSendRequestWithTokenOnQueue::request (%d) = %.*s\n", (int)requestLength, (int)requestLength, baseBuffer);

	//cursor->overlappedResponse.cursor = cursor;
	cursor->conn = &_httpConn;
	cursor->queryToken = requestToken;

	//Send the HTTP(S) request
	return CTCursorSendOnQueue(cursor, (char **)&baseBuffer, requestLength);//, &overlappedResponse);
}




char* httpHeaderLengthCallback(struct CTCursor* pCursor, char* buffer, unsigned long length)
{
	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = strstr(buffer, "\r\n\r\n");
#ifdef _DEBUG
	if (!endOfHeader)
		assert(1 == 0);
#endif
	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	pCursor->contentLength = 1641845;

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}


char* reqlHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer + sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	cursor->contentLength = ((ReqlQueryMessageHeader*)buffer)->length;
	assert(((ReqlQueryMessageHeader*)buffer)->token == cursor->queryToken);

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}

void httpResponseCallback(CTError* err, CTCursor* cursor)
{
	CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	//CTCursorMapFileR(cursor);
	printf("httpResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	CTCursorCloseFile(cursor);
}


void reqlResponseCallback(CTError* err, CTCursor* cursor)
{
	CTCursorMapFileR(cursor);
	printf("reqlCompletionCallback body:  \n\n%.*s\n\n", cursor->file.size, cursor->file.buffer);
	CTCursorCloseFile(cursor);
}

static int httpRequestCount = 0;
char responseBuffers [CT_MAX_INFLIGHT_CURSORS][65536];

void sendHTTPRequest(CTCursor* cursor)
{
	int requestIndex;

	//send buffer ptr/length
	char* queryBuffer;
	unsigned long queryStrLength;

	//send request
	char GET_REQUEST[1024] = "GET /img/textures/wood.png HTTP/1.1\r\nHost: learnopengl.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";
	//char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";

	//file path to open
	char filepath[1024] = "C:\\3rdGen\\CoreTransport\\Samples\\HappyEyeballs\\https\0";

	memset(cursor, 0, sizeof(CTCursor));

	_itoa(httpRequestCount, filepath + strlen(filepath), 10);

	strcat(filepath, ".png");
	httpRequestCount++;

	//CoreTransport provides support for each cursor to read responses into memory mapped files
	//however, if this is not desired, simply cursor's file.buffer slot to your own memory
	createCursorResponseBuffers(cursor, filepath);
	//cursor->file.buffer = &(responseBuffers[httpRequestCount - 1][0]);

	assert(cursor->file.buffer);
	//if (!cursor->file.buffer)
	//	return;

	cursor->headerLengthCallback = httpHeaderLengthCallback;
	cursor->responseCallback = httpResponseCallback;

	//printf("_conn.sslContextRef->Sizes.cbHeader = %d\n", _conn.sslContext->Sizes.cbHeader);
	//printf("_conn.sslContextRef->Sizes.cbTrailer = %d\n", _conn.sslContext->Sizes.cbTrailer);

	for (requestIndex = 0; requestIndex < 1; requestIndex++)
	{
		queryStrLength = strlen(GET_REQUEST);
		queryBuffer = cursor->requestBuffer;//cursor->file.buffer;
		memcpy(queryBuffer, GET_REQUEST, queryStrLength);
		queryBuffer[queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminato
		printf("\n%s\n", queryBuffer);
		CTCursorSendRequestOnQueue(cursor, _httpConn.queryCount++);
	}

}

void sendReqlQuery(CTCursor * cursor)
{
	int queryIndex;

	//send buffer ptr/length
	char * queryBuffer;
	unsigned long queryStrLength;

	//send query
	for( queryIndex = 0; queryIndex < 1; queryIndex++ )
	{
		queryStrLength = (unsigned long)strlen(paintingTableQuery);
		queryBuffer = cursor->requestBuffer;
		memcpy(queryBuffer + sizeof(ReqlQueryMessageHeader), paintingTableQuery, queryStrLength);
		queryBuffer[sizeof(ReqlQueryMessageHeader) + queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminator
		printf("queryBuffer = %s\n", queryBuffer+sizeof(ReqlQueryMessageHeader));
		CTCursorRunQueryOnQueue(cursor, _reqlConn.queryCount++);
		//CTReQLRunQueryOnQueue(&_reqlConn, (const char**)&queryBuffer, queryStrLength, _reqlConn.queryCount++);
	}

}

int _httpConnectionClosure(CTError* err, CTConnection* conn)
{
	//thread
	HANDLE hThread;
	DWORD i;

	//TO DO:  Parse error status
	assert(err->id == CTSuccess);

	//Copy CTConnection object memory from coroutine stack
	//to appplication memory (or the connection will go out of scope when this funciton exits)
	//FYI, do not use the pointer memory after this!!!
	_httpConn = *conn;
	_httpConn.response_overlap_buffer_length = 0;
	
	//_httpConn.socketContext = conn->socketContext;
	//allocate mapped file buffer on cursor for http request response
	//createCursorResponseBuffers(&_httpCursor, filepath);
	//_httpCursor.headerLengthCallback = httpHeaderLengthCallback;
	//_httpCursor.responseCallback = httpResponseCallback;

   //1  Create a single worker thread, respectively, for the CTConnection's (Socket) Tx and Rx Queues
   //   We associate our associated processing callbacks for each by associating the callback with the thread(s) we associate with the queues
   //   Note:  On Win32, "Queues" are actually IO Completion Ports
	/*
	for (i = 0; i < 1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CTDecryptResponseCallback, _httpConn.socketContext.rxQueue, 0, NULL);
		CloseHandle(hThread);

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CTEncryptQueryCallback, _httpConn.socketContext.txQueue, 0, NULL);
		CloseHandle(hThread);
	}
	*/

	printf("HTTP Connection Success\n");
	return err->id;
}

int _reqlConnectionClosure(CTError *err, CTConnection * conn)
{
	//file path to open
	char *filepath = "ReQLQuery.txt\0";

	//thread
	HANDLE hThread;
	DWORD i;

	//send
	char * queryBuffer;
	unsigned long queryStrLength;
	
	//status
	int status;

	//TO DO:  Parse error status
	assert(err->id == ReqlSuccess);

	//Copy CTConnection object memory from coroutine stack
	//to appplication memory (or the connection will go out of scope when this funciton exits)
	//FYI, do not use the pointer memory after this!!!
	 _reqlConn = *conn;

	//  Now Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
    if( (status = CTReQLSASLHandshake(&_reqlConn, _reqlConn.target)) != CTSuccess )
    {
        printf("CTReQLSASLHandshake failed!\n");
		CTCloseConnection(&_reqlConn);
		CTSSLCleanup();
		err->id = CTSASLHandshakeError;
		return err->id;
	}

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

	printf("REQL Connection Success\n");
	return err->id;
}

//Define crossplatform keyboard event loop handler
bool getconchar(KEY_EVENT_RECORD* krec)
{
#ifdef _WIN32
	DWORD cc;
	INPUT_RECORD irec;
	KEY_EVENT_RECORD* eventRecPtr;

	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

	if (h == NULL)
	{
		return false; // console not found
	}

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
			krec.uChar.AsciiChar = c;
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
	printf("\nStarting SystemKeyboardEventLoop...\n");

	KEY_EVENT_RECORD key;
	for (;;)
	{
		memset(&key, 0, sizeof(KEY_EVENT_RECORD));
		getconchar(&key);
		if (key.uChar.AsciiChar == 's')
			sendHTTPRequest(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);
		else if (key.uChar.AsciiChar == 'q')
			break;
	}
}


int main(int argc, char **argv) 
{
	//Static Definitions
	int i;

	HANDLE cxThread, txThread, rxThread;
	CTThreadQueue cxQueue, txQueue, rxQueue;

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
	
	CTTarget httpTarget = { 0 };
	CTTarget reqlService = { 0 };	
	char* caPath = (char*)certStr;//"C:\\Development\\git\\CoreTransport\\bin\\Keys\\ComposeSSLCertficate.der";// .. / Keys / ComposeSSLCertificate.der";// MSComposeSSLCertificate.cer";

	//These are only relevant for custom DNS resolution, which has not been implemented yet for WIN32 platforms
	//char * resolvConfPath = "../Keys/resolv.conf";
	//char * nsswitchConfPath = "../Keys/nsswitch.conf";

	//Connection & Cursor Pool Initialization
	CTCreateConnectionPool(&(HAPPYEYEBALLS_CONNECTION_POOL[0]), HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS);
	CTCreateCursorPool(&(_httpCursor[0]), CT_MAX_INFLIGHT_CURSORS);
	
	//Thread Pool Initialization
	cxQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	txQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	rxQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);

	for (i = 0; i < 1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool

		cxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CT_Dequeue_Connect, cxQueue, 0, NULL);		
		txThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CT_Dequeue_Encrypt_Send, txQueue, 0, NULL);
		rxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CT_Dequeue_Recv_Decrypt, rxQueue, 0, NULL);
	}

	//Define https connection target
	//We aren't doing any authentication via http
	//SCHANNEL can find the certficate in the system CA keychain for us, MBEDTLS cannot
	httpTarget.host = (char*)http_server;
	httpTarget.port = http_port;
	httpTarget.ssl.ca = NULL;//(char*)caPath;
	//httpTarget.dns.resconf = (char*)resolvConfPath;
	//httpTarget.dns.nssconf = (char*)nsswitchConfPath;
	httpTarget.txQueue = txQueue;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	httpTarget.rxQueue = rxQueue;
	httpTarget.cxQueue = cxQueue;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	//CloseHandle(cxThread);

	//Define RethinkDB service connection target
	//We are doing SCRAM authentication over TLS required by RethinkDB
	//For SCHANNEL SSL Cert Verification, the certficate needs to be installed in the system CA trusted authorities store
	//For WolfSSL, MBEDTLS and SecureTransport Cert Verification, the path to the certificate can be passed 
	reqlService.host = (char*)rdb_server;
	reqlService.port = rdb_port;
	reqlService.user = (char*)rdb_user;
	reqlService.password = (char*)rdb_pass;
	reqlService.ssl.ca = (char*)caPath;
	//reqlService.dns.resconf = (char*)resolvConfPath;
	//reqlService.dns.nssconf = (char*)nsswitchConfPath;

	//Demonstrate CTransport CT C API Connection (No JSON support)
	//On Darwin platforms, the Core Transport connection routine operates asynchronously on a single thread to by integrating libdill coroutines with a thread's CFRunLoop operation (so the connection can be started from the main thread before cocoa app starts)
	//On WIN32, we only have synchronous/blocking connection support, but this can be easily thrown onto a background thread at startup (even in a UWP app?)
	CTransport.connect(&httpTarget, _httpConnectionClosure);
	//CTransport.connect(&reqlService, _reqlConnectionClosure);

	//for(i=0;i<10000;i++)	
	//	sendHTTPRequest(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);

	SystemKeyboardEventLoop();

	
	//Clean up socket connections
	CTCloseConnection(&_httpConn);
	//CTCloseConnection(&_reqlConn);

	//Clean  Up Auth Memory
	ct_scram_cleanup();

	//Clean Up SSL Memory
	CTSSLCleanup();

	//TO DO:  Shutdown threads

	//Cleanup Thread Handles
	if (cxThread && cxThread != INVALID_HANDLE_VALUE)
		CloseHandle(cxThread);

	if (txThread && txThread != INVALID_HANDLE_VALUE)
		CloseHandle(txThread);

	if (rxThread && rxThread != INVALID_HANDLE_VALUE)
		CloseHandle(rxThread);

	return 0;
}
