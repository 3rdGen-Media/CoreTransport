/***
**  
*	Core Transport -- HappyEyeballs
*
*	Description:  
*	
*
*	@author: Joe Moulton III
*	Copyright 2020 Abstract Embedded LLC DBA 3rdGen Multimedia
**
***/

//cr_display_sync is part of Core Render crPlatform library
#ifdef _DEBUG
#include <vld.h>                //Visual Leak Detector
#endif

#include "stdio.h"
#include "stdlib.h"
#include <process.h>            //threading routines for the CRT

//Core Transport ReQL C API
//#include <CoreTransport/CTransport.h>

//ctReQL can be optionally built with native SSL encryption
//or against MBEDTLS
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
#include <CoreTransport/CXURL.h>
#include <CoreTransport/CXReQL.h>

//CXReQL uses RapidJSON for json serialization internally
//#include "rapidjson/document.h"
//#include "rapidjson/error/en.h"
//#include "rapidjson/stringbuffer.h"
//#include <rapidjson/writer.h>

/***
 * Start CTransport (CTConnection) C API Example
 ***/

//Define a CTransport API CTTarget C style struct to initiate an HTTPS connection with a CTransport API CTConnection
static const char* http_server = "example.com";
static const unsigned short		http_port = 443;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char * rdb_server = "RETHINKDB_SERVER";
static const unsigned short rdb_port = 18773;
static const char * rdb_user = "RETHINKDB_USERNAME";
static const char * rdb_pass = "RETHINKDB_PASSWORD";

//Define SSL Certificate for ReqlService construction:
//	a)  For xplatform MBEDTLS support: The SSL certificate can be defined in a buffer OR a file AS a .cer in PEM format
//	b)  For Win32 SCHANNEL support:	   The SSL certificate must be pre-installed in a trusted root store on the system using MS .cer format
//  c)  For Darwin SecureTransport:	   The SSL certificate must be defined in a .der file with ... format

//const char * certFile = "../Keys/MSComposeSSLCertificate.cer";
const char * rdb_cert = "RETHINKDB_CERT_PATH_OR_STR\0";

/***
 * Start CXTransport C++ API Example
 ***/
using namespace CoreTransport;

//Define pointers to keep reference to our secure HTTPS and RethinkDB connections, respectively
CXConnection * _httpCXConn;
CXConnection * _reqlCXConn;

int _cxURLConnectionClosure(CTError *err, CXConnection * conn)
{
	if (err->id == CTSuccess && conn) 
	{
		_httpCXConn = conn;
		/*
		if (CTSocketCreateEventQueue(&(conn->connection()->socketContext)) < 0)
		{
			printf("ReqlSocketCreateEventQueue failed\n");
			err->id = (int)conn->connection()->event_queue;
			//goto CONN_CALLBACK;
		}
		*/
	}
	else { assert(1 == 0); } //process errors

	return err->id;
}

int _cxReQLConnectionClosure(ReqlError *err, CXConnection * conn)
{
	if( err->id == CTSuccess && conn ){ _reqlCXConn = conn; }
	else {} //process errors	
	return err->id;
}

void SendReqlQuery()
{
	//Send the ReQL Query to find some URLs to download
	//Demonstrate issue of query and callback lambda using CXReQL API
	auto queryCallback = [&] (ReqlError * error, std::shared_ptr<CXCursor> &cxCursor) {

		//The cursor's file mapping was closed so the file could be truncated to the size read from network, 
		//but the curor's file handle itself is still open/valid, so we just need to remap the cursor's file for reading
		CTCursorMapFileR(&cxCursor->_cursor);
		printf("Lambda callback response: %.*s\n", cxCursor->_cursor.file.size, (char*)(cxCursor->_cursor.file.buffer) + sizeof(ReqlQueryMessageHeader)); return;  
	};	
	CXReQL.db("MastryDB").table("Paintings").run(_reqlCXConn, NULL, queryCallback);
}

static int httpsRequestCount = 0;
void SendHTTPRequest()
{
	//file path to open
	char filepath[1024] = "https\0";

	_itoa(httpsRequestCount, filepath + 5, 10);

	strcat(filepath, ".txt");
	httpsRequestCount++;
	
	//Create a CXTransport API C++ CXURLRequest
	//std::shared_ptr<CXURLRequest> getRequest = CXURL.GET("/index.html", filepath);
	std::shared_ptr<CXURLRequest> getRequest = CXURL.GET("/index.html", filepath);

	//Add some HTTP headers to the CXURLRequest
	getRequest->setValueForHTTPHeaderField("Accept:", "*/*");

	
	//Define the response callback to return response buffers using CXTransport CXCursor object returned via a Lambda Closure
	auto requestCallback = [&] (CTError * error, std::shared_ptr<CXCursor> &cxCursor) {

		CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorMapFileR(cursor);
		printf("Lambda callback response:  %.*s\n", cxCursor->_cursor.headerLength, cxCursor->_cursor.requestBuffer);//%.*s\n", cursor->_cursor.length - sizeof(ReqlQueryMessageHeader), (char*)(cursor->_cursor.header) + sizeof(ReqlQueryMessageHeader)); return;  
		CTCursorCloseFile(&(cxCursor->_cursor));
	};

	//Pass the CXConnection and the lambda to populate the request buffer and asynchronously send it on the CXConnection
	//The lambda will be executed so the code calling the request can interact with the asynchronous response buffers
	getRequest->send(_httpCXConn, requestCallback);
	
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
			SendHTTPRequest();
		else if (key.uChar.AsciiChar == 'q')
			break;
	}
}

#define HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS 1024
CTConnection HAPPYEYEBALLS_CONNECTION_POOL[HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS];

#define CT_MAX_INFLIGHT_CURSORS 1024
CTCursor _httpCursor[CT_MAX_INFLIGHT_CURSORS];
CTCursor _reqlCursor;

int main(int argc, char **argv) 
{	
	//Static Definitions
	int i;

	HANDLE cxThread, txThread, rxThread;
	CTThreadQueue cxQueue, txQueue, rxQueue;

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;

	CTTarget httpTarget = {0};
	CTTarget reqlService = {0};
	//char* caPath = (char*)certStr;//"C:\\Development\\git\\CoreTransport\\bin\\Keys\\ComposeSSLCertficate.der";// .. / Keys / ComposeSSLCertificate.der";// MSComposeSSLCertificate.cer";

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

		cxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CX_Dequeue_Connect, cxQueue, 0, NULL);
		txThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CX_Dequeue_Encrypt_Send, txQueue, 0, NULL);
		rxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CX_Dequeue_Recv_Decrypt, rxQueue, 0, NULL);
	}


	//Define https connection target
	//We aren't doing any authentication via http
	httpTarget.host = (char*)http_server;
	httpTarget.port = http_port;
	httpTarget.ssl.ca = NULL;//(char*)caPath;
	//httpTarget.dns.resconf = (char*)resolvConfPath;
	//httpTarget.dns.nssconf = (char*)nsswitchConfPath;
	httpTarget.txQueue = txQueue;
	httpTarget.rxQueue = rxQueue;
	httpTarget.cxQueue = cxQueue;

	//Define RethinkDB service connection target
	//We are doing SCRAM authentication over TLS required by RethinkDB
	reqlService.host = (char*)rdb_server;
	reqlService.port = rdb_port;
	reqlService.user = (char*)rdb_user;
	reqlService.password = (char*)rdb_pass;
	reqlService.ssl.ca = (char*)rdb_cert;
	//reqlService.dns.resconf = (char*)resolvConfPath;
	//reqlService.dns.nssconf = (char*)nsswitchConfPath;

	//Use CXTransport CXURL C++ API to connect to our HTTPS target server
	CXURL.connect(&httpTarget, _cxURLConnectionClosure);

	//User CXTransport CXReQL C++ API to connect to our RethinkDB service
	//CXReQL.connect(&reqlService, _cxReQLConnectionClosure);

	//Keep the app running using platform defined run loop
	SystemKeyboardEventLoop();

	//Deleting CXConnection objects will
	//Clean up socket connections
	delete _httpCXConn;
	//delete _reqlCXConn;	
	
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
