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

#ifdef _WIN32

//cr_display_sync is part of Core Render crPlatform library
#ifdef _DEBUG
#include <vld.h>                //Visual Leak Detector
#endif

#include "stdio.h"
#include "stdlib.h"
#include <process.h>            //threading routines for the CRT
#include "assert.h"

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "secur32.lib")

#else

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

//CoreTransport CXURL and CXReQL C++ APIs use CTransport (CTConnection) API internally
#include <CoreTransport/CXURL.h>
#include <CoreTransport/CXReQL.h>

/***
 * Start CTransport (CTConnection) C API Example
 ***/

 //Define a CTransport API CTTarget C style struct to initiate an HTTPS connection with a CTransport API CTConnection
static const char* http_server = "3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com";//"example.com";// "vtransport - assets.s3.us - west - 1.amazonaws.com";//"example.com";//"mineralism.s3 - us - west - 2.amazonaws.com";
static const unsigned short	http_port = 443;

//Proxies use a prefix to specify the proxy protocol, defaulting to HTTP Proxy
static const char* proxy_server = "http://172.20.10.1";//"54.241.100.168";
static const unsigned short	proxy_port = 443;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char* rdb_server = "rdb.server.net";
static const unsigned short rdb_port = 28015;
static const char* rdb_user = "rdb_username";
static const char* rdb_pass = "rdb_password";

//These are only relevant for custom DNS resolution libdill, loading the data from these files has not yet been implemented for WIN32 platforms
char* resolvConfPath = "./resolv.conf";
char* nsswitchConfPath = "./nsswitch.conf";

//Define SSL Certificate for ReqlService construction:
//	a)  For xplatform MBEDTLS support: The SSL certificate can be defined in a buffer OR a file AS a .cer in PEM format
//	b)  For Win32 SCHANNEL support:	   The SSL certificate must be pre-installed in a trusted root store on the system using MS .cer format
//  c)  For Darwin SecureTransport:	   The SSL certificate must be defined in a .der file with ... format
//	d)  For xplatform WolfSSL support: ...
//
char* caPath = NULL;

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

void LoadGameStateQuery()
{
	//Send the ReQL Query to find some URLs to download
	//Demonstrate issue of query and callback lambda using CXReQL API
	auto queryCallback = [&] (CTError * error, std::shared_ptr<CXCursor> cxCursor) {

		printf("LoadGameState response:  \n\n%.*s\n\n", cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);
		CTCursorCloseMappingWithSize(cxCursor->_cursor, cxCursor->_cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
		CTCursorCloseFile(cxCursor->_cursor);
	};
	CXReQL.db("GameState").table("Scene").get("BasicCharacterTest").run(_reqlCXConn, NULL, queryCallback);
}

static int httpsRequestCount = 0;
void SendHTTPRequest()
{
	//file path to open
#ifndef _WIN32
	char filepath[1024] = "/home/jmoulton/3rdGen/CoreTransport/bin/img/http\0";
#else
	char filepath[1024] = "C:\\3rdGen\\CoreTransport\\bin\\img\\http\0";
#endif

	char* urlPath = "/index.html";

	if (httpsRequestCount % 3 == 0)
		urlPath = "/img/textures/wood.png";
	else if (httpsRequestCount % 3 == 1)
		urlPath = "/img/textures/brickwall.jpg";
	else if (httpsRequestCount % 3 == 2)
		urlPath = "/img/textures/brickwall_normal.jpg";

	//_itoa(httpsRequestCount, filepath + 5, 10);
	snprintf(filepath + strlen(filepath), strlen(filepath), "%d", httpsRequestCount);

	//strcat(filepath, ".txt");
	
	if (httpsRequestCount % 3 == 0)
		strcat(filepath, ".png");
	else
		strcat(filepath, ".jpg");
	
	httpsRequestCount++;
	
	//Create a CXTransport API C++ CXURLRequest
	//std::shared_ptr<CXURLRequest> getRequest = CXURL.GET("/index.html", filepath);
	std::shared_ptr<CXURLRequest> getRequest = CXURL.GET(urlPath, filepath);

	getRequest->setValueForHTTPHeaderField("Host", "3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com:443");
	//Add some HTTP headers to the CXURLRequest
	getRequest->setValueForHTTPHeaderField("Accept", "*/*");

	
	//Define the response callback to return response buffers using CXTransport CXCursor object returned via a Lambda Closure
	auto requestCallback = [&] (CTError * error, std::shared_ptr<CXCursor> cxCursor) {

		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorMapFileR(cursor);
		fprintf(stderr, "SendHTTPRequest response header:  %.*s\n", (int)cxCursor->_cursor->headerLength, cxCursor->_cursor->requestBuffer);//%.*s\n", cursor->_cursor.length - sizeof(ReqlQueryMessageHeader), (char*)(cursor->_cursor.header) + sizeof(ReqlQueryMessageHeader)); return;  
		fprintf(stderr, "SendHTTPRequest response body:  %.*s\n", (int)cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));

		CTCursorCloseMappingWithSize(cxCursor->_cursor, cxCursor->_cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
		CTCursorCloseFile(cxCursor->_cursor);
	};

	//Pass the CXConnection and the lambda to populate the request buffer and asynchronously send it on the CXConnection
	//The lambda will be executed so the code calling the request can interact with the asynchronous response buffers
	getRequest->send(_httpCXConn, requestCallback);
	
}

int _cxURLConnectionClosure(CTError* err, CXConnection* conn) {
	//CXConnectionClosure _cxURLConnectionClosure = ^ int(CTError * err, CXConnection * conn) {
	if (err->id == CTSuccess && conn)
	{
		_httpCXConn = conn;
	}
	else { assert(1 == 0); } //process errors

	fprintf(stderr, "HTTP Connection Success\n");

	//for (int i = 0; i < 30; i++)
	//	SendHTTPRequest();

	return err->id;
};

int _cxReQLConnectionClosure(CTError* err, CXConnection* conn) {
	//CTConnectionClosure _cxReQLConnectionClosure = ^ int(CTError * err, CTConnection * conn) {

	if (err->id == CTSuccess && conn)
	{
		_reqlCXConn = conn;
	}
	else { assert(1 == 0); } //process errors

	fprintf(stderr, "ReQL Connection Success\n");

	//for (int i = 0; i < 30; i++)
	//	SendHTTPRequest();

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
	printf("\nStarting SystemKeyboardEventLoop...\n");
	//int64_t timestamp = 12345678912345;

	KEY_EVENT_RECORD key;
	for (;;)
	{
		yield();

		memset(&key, 0, sizeof(KEY_EVENT_RECORD));
		getconchar(&key);

		//if (key.uChar.AsciiChar == 's')
		//	StartTransientsChangefeedUpdate();
		//CreateUserQuery(user_email, user_name, user_password, _reqlCXConn);

		if (key.uChar.AsciiChar == 'g')
			SendHTTPRequest();
		else if (key.uChar.AsciiChar == 'r')		
			LoadGameStateQuery();		
		else if (key.uChar.AsciiChar == 'y')
			yield();			`
		else if (key.uChar.AsciiChar == 'q')
			break;

		yield();

	}
}

#define HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS 1024
CTConnection HAPPYEYEBALLS_CONNECTION_POOL[HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS];

#define CT_MAX_INFLIGHT_CURSORS 1024
CTCursor _httpCursor[CT_MAX_INFLIGHT_CURSORS];

int main(int argc, char **argv) 
{	
	//Static Definitions
	int index;

	//int64_t time = ct_system_utc();
	//printf("time = %lld", time);

	//Declare Coroutine Bundle Handle 
	//Note:  Needed for dill global couroutine malloc cleanup on application exit, while just a meaningless int on Win32 platforms
	CTRoutine coroutine_bundle;

	//Declare thread handles for thread pools
	CTThread cxThread, txThread, rxThread;

	//Declare dedicated Kernel Queues for dequeueing socket connect, read and write operations in our network application
	CTKernelQueue cq, tq, rq;

	//Declare Targets (ie specify TCP connection endpoints + options)
	CTTarget		httpTarget	= { 0 };
	CTTarget		reqlService	= { 0 };
	CTKernelQueue	emptyQueue  = { 0 };

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
		cxThread = CTThreadCreate(&cq, CX_Dequeue_Resolve_Connect_Handshake);
		rxThread = CTThreadCreate(&rq, CX_Dequeue_Recv_Decrypt);
		txThread = CTThreadCreate(&tq, CX_Dequeue_Encrypt_Send);

		//TO DO:  check thread failure
	}

	//Define https connection target
	httpTarget.url.host = (char*)http_server;
	httpTarget.url.port = http_port;
	//httpTarget.proxy.host = (char*)proxy_server;
	//httpTarget.proxy.port = proxy_port;
	httpTarget.ssl.ca = NULL;//(char*)caPath;
	httpTarget.ssl.method = CTSSL_TLS_1_2;
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
	reqlService.ssl.ca = NULL;//(char*)caPath;
	reqlService.ssl.method = CTSSL_TLS_1_2;
	reqlService.dns.resconf = (char*)resolvConfPath;
	reqlService.dns.nssconf = (char*)nsswitchConfPath;
	reqlService.cq = cq;
	reqlService.tq = tq;
	reqlService.rq = rq;

	//Use CXTransport CXURL C++ API to connect to our HTTPS target server
	//coroutine_bundle = CXURL.connect(&httpTarget, _cxURLConnectionClosure);

	//User CXTransport CXReQL C++ API to connect to our RethinkDB service
	coroutine_bundle = CXReQL.connect(&reqlService, _cxReQLConnectionClosure);

	//Keep the app running using platform defined run loop
	SystemKeyboardEventLoop();

	//TO DO:  Wait for asynchronous threads to shutdown

	//Deleting CXConnection objects will
	//Clean up socket connections
	//delete _httpCXConn;
	delete _reqlCXConn;	
	
	//Clean  Up Auth Memory
	ca_scram_cleanup();

	//Clean Up SSL Memory
	CTSSLCleanup();

	//Cleanup Thread Handles
	CTThreadClose(&cxThread);  //stop new connections first
	CTThreadClose(&txThread);  //stop sending next
	CTThreadClose(&rxThread);  //stop receiving last

	//close the dill coroutine bundle to clean up dill mallocs
	CTRoutineClose(coroutine_bundle);

	return 0;
}
