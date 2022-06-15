

#ifdef _WIN32

#ifdef _DEBUG
#include <vld.h>                //Visual Leak Detector
#endif

#include "stdio.h"
#include "stdlib.h"
#include <process.h>            // threading routines for the CRT
#include "assert.h"

//CoreTransport can be optionally built with native SSL encryption OR against MBEDTLS
#pragma comment(lib, "crypt32.lib")
//#pragma comment(lib, "user32.lib")
#pragma comment(lib, "secur32.lib")

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

#include "../../Git/libdispatch/dispatch/dispatch.h"

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

static const char* aws_access_key = "AKIA267MLRQDRJ2IIARW";
static const char* aws_access_secrt = "0SakIbVLOM2E+ck7xPw1tlJSN519ev2mcMx2KTG/";
/***
 * Start CTransport (CTConnection) C API Example
 ***/

//Define a CTransport API CTTarget C style struct to initiate an HTTPS connection with a CTransport API CTConnection
static const char* http_server = "3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com";//"example.com";// "vtransport - assets.s3.us - west - 1.amazonaws.com";//"example.com";//"mineralism.s3 - us - west - 2.amazonaws.com";
static const unsigned short	http_port = 80;

//Proxies use a prefix to specify the proxy protocol, defaulting to HTTP Proxy
static const char* proxy_server = "socks5://172.20.10.1";// "54.241.100.168";
static const unsigned short		proxy_port = 443;// 1080;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char* rdb_server = "rdb.3rd-gen.net";
static const unsigned short rdb_port = 28015;
static const char* rdb_user = "admin";
static const char* rdb_pass = "3rdgen.rdb.4.$";

//These are only relevant for custom DNS resolution libdill, loading the data from these files has not yet been implemented for WIN32 platforms
char * resolvConfPath = "./resolv.conf";
char * nsswitchConfPath = "/nsswitch.conf";

//Define SSL Certificate for ReqlService construction:
//	a)  For xplatform MBEDTLS support: The SSL certificate can be defined in a buffer OR a file AS a .cer in PEM format
//	b)  For Win32 SCHANNEL support:	   The SSL certificate must be pre-installed in a trusted root store on the system using MS .cer format
//  c)  For Darwin SecureTransport:	   The SSL certificate must be defined in a .der file with ... format
//	d)  For xplatform WolfSSL support: ...
//
char* caPath = NULL;
//const char * certPath = "RETHINKDB_CERT_PATH_OR_STR\0";

//Predefine ReQL query to use with CTransport C Style API (because message support is not provided at the CTransport/C level)
//char * paintingTableQuery = "[1,[15,[[14,[\"GameState\"]],\"Scene\"]]]\0";

//Define pointers and/or structs to keep reference to our secure CTransort HTTPS and RethinkDB connections, respectively
CTConnection _httpConn;
CTConnection _reqlConn;

#ifndef _WIN32
//Declare libdispatch socket source(s)
dispatch_source_t _rxSource;

//Declare libdispatch queues to use with libdispatch socket source(s)
//static const char * kReqlConnectionQueue = "com.3rdgen.HappyEyeballs.ConnectionQueue";
static const char * kTransportReadQueue      = "com.3rdgen.CoreTransport.Dispatch.ReadQueue";
static const char * kTransportWriteQueue     = "com.3rdgen.CoreTransport.Dispatch.WriteQueue";
dispatch_queue_t _rxQueue;
dispatch_queue_t _txQueue;




CTDispatchSourceHandler _rxSourceHandler = ^(void)
{
        unsigned long bytesAvailable = dispatch_source_get_data(_rxSource);
        fprintf(stderr, "_rxSource bytes bytes available: %lu\n", bytesAvailable);
		
		pthread_t ptid = pthread_self();
		long tid = 0;
		thr_self(&tid);
		fprintf(stderr, "HappyEyeballs rxSource Thread ID:  (%ld, %p)\n", tid, ptid);

		char recv_buffer[32768];
		unsigned long buffer_size = bytesAvailable;
		memset(recv_buffer, 0, bytesAvailable);

		// Sync Wait for query response on ReqlConnection socket
		char * responsePtr = CTRecv( &_httpConn, recv_buffer, &buffer_size );
		assert(responsePtr);
		assert(buffer_size > 0);
		fprintf( stderr, "Response:\n\n%.*s", (int)strlen(responsePtr), responsePtr);
		
		//dispatch_source_cancel(_rxSource);
};


void CreateDispatchQueues()
{
    dispatch_queue_attr_t attr = DISPATCH_QUEUE_SERIAL;//dispatch_queue_attr_create();
	//dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INTERACTIVE, 0);
    _rxQueue    		= dispatch_queue_create(kTransportReadQueue,attr);
    _txQueue         	= dispatch_queue_create(kTransportWriteQueue,attr);
    //_changeQueue        = dispatch_queue_create(kReqlChangeQueue,attr);
}

dispatch_source_t CreateConnectionDispatchSource(CTConnection * conn, dispatch_queue_t dispatchQueue)
{
    //Add ReqlConnection socket as dispatch read source on NSReQLSession.queryQueue
    _rxSource = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, conn->socket, 0, dispatchQueue);
    assert(_rxSource != NULL);
    
    //__weak typeof(self) weakSelf = self;
    //Create shared memory with mmap for reading
    
    //NSLog(@"vm_kernel_page_size = %lu", vm_kernel_page_size);
    //NSLog(@"getpagesize() = %d", getpagesize());
    //NSLog(@"sysconf(_SC_PAGESIZE) = %ld", sysconf(_SC_PAGE_SIZE));
    
    
    //create some shared memory that can be used to receive socket messages
    static const size_t NUM_PAGES = 128;
    __block size_t bufferSize = sysconf(_SC_PAGE_SIZE) * NUM_PAGES;
    //__block char * readBuffer = (char*)mmap(NULL, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0 );
    //assert( readBuffer && readBuffer != MAP_FAILED);
    
    dispatch_source_set_event_handler(_rxSource, _rxSourceHandler);
    
    /* GCD requires that any source dealing with a file descriptor have a
     * cancellation handler. Because GCD needs to keep the file descriptor
     * around to monitor it, the cancellation handler is the client's signal
     * that GCD is done with the file descriptor, and thus the client is safe
     * to close it out. Remember, file descriptors aren't ref counted.
     */
    dispatch_source_set_cancel_handler(_rxSource, ^(void)
    {
        fprintf(stderr, "NSReQLConnection DISPATCH_SOURCE_TYPE_READ canceled\n" );
        //TO DO:  ReqlConnectionClose
        dispatch_release(_rxSource);
        //(void)close(fd);
    });
   dispatch_resume(_rxSource);
    return _rxSource;
}

#endif




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

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}

#ifndef _WIN32
CTCursorCompletionClosure httpResponseClosure = ^void(CTError * err, CTCursor* cursor)
{
	//CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	//CTCursorMapFileR(cursor);
	fprintf(stderr, "httpResponseCallback (%d) header:  \n\n%.*s\n\n", (int)cursor->queryToken, (int)cursor->headerLength, cursor->requestBuffer);
	CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	CTCursorCloseFile(cursor);
};
#else
void httpResponseClosure(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "httpResponseCallback (%d) header:  \n\n%.*s\n\n", (int)cursor->queryToken, (int)cursor->headerLength, cursor->requestBuffer);
	CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	CTCursorCloseFile(cursor);

}
#endif


uint64_t CTCursorSendRequestOnQueue(CTCursor * cursor, uint64_t requestToken)	
{
	//a string/stream for serializing the json query to char*	
	unsigned long requestHeaderLength, requestLength;

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
	//recv
	//LPVOID queryPtr, responsePtr;
	unsigned long ctBufferSize = ct_system_allocation_granularity(); //+ sizeof(CTOverlappedResponse); 

	if (!filepath)
		filepath = "map.jpg\0";
	cursor->file.path = (char*)filepath;

	//file path to open
	CTCursorCreateMapFileW(cursor, cursor->file.path, ct_system_allocation_granularity() * 256UL);

	memset(cursor->file.buffer, 0, ct_system_allocation_granularity() * 256UL);
	//cursor->overlappedResponse.buf = cursor->file.buffer;
	//cursor->overlappedResponse.len = ctBufferSize;
	//cursor->overlappedResponse.wsaBuf.buf = cursor->file.buffer;
	//cursor->overlappedResponse.wsaBuf.len = ctBufferSize;

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
	//char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";
	char GET_REQUEST[1024] = "\0";
	char * imgName = "wood.png";

	if(httpRequestCount % 3 == 0)
		imgName = "wood.png";
	else if(httpRequestCount % 3 == 1)
		imgName = "brickwall.jpg";
	else if(httpRequestCount % 3 == 2)
		imgName = "brickwall_normal.jpg";

	sprintf(GET_REQUEST, "GET /img/textures/%s HTTP/1.1\r\nHost: 3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0", imgName);
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
	//cursor->file.buffer = &(responseBuffers[httpRequestCount - 1][0]);

	assert(cursor->file.buffer);
	//if (!cursor->file.buffer)
	//	return;

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

#ifndef _WIN32
CTConnectionClosure _httpConnectionClosure = ^int(CTError * err, CTConnection * conn)
#else
int _httpConnectionClosure(CTError* err, CTConnection* conn)
#endif
{
    
	//TO DO:  Parse error status
	assert(err->id == CTSuccess);

	//Copy CTConnection object memory from coroutine stack
	//to appplication memory (or the connection will go out of scope when this funciton exits)
	//FYI, do not use the pointer memory after this!!!
	_httpConn = *conn;
	_httpConn.response_overlap_buffer_length = 0;

	//CreateConnectionDispatchSource( &_httpConn, _rxQueue);
	
	fprintf(stderr, "HTTP Connection Success\n");
	

	for(int i =0; i<100; i++)
		sendHTTPRequest(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);

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
	//yield();

	KEY_EVENT_RECORD key;
	for (;;)
	{
		yield();

		memset(&key, 0, sizeof(KEY_EVENT_RECORD));
		getconchar(&key);
		if (key.uChar.AsciiChar == 's')
			sendHTTPRequest(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);
		//if(key.uChar.AsciiChar == 'r')
		//	sendReqlQuery(CTGetNextPoolCursor());// &_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);
		else if (key.uChar.AsciiChar == 'y')
			yield();
		else if (key.uChar.AsciiChar == 'q')
		{
			break;
		}
		yield();

	}
	
	//yield();

}

CTKernelQueuePipePair g_kQueuePipePair;

int main(void) {

	//Declare intermediate variables
	int i;

	//Declare Coroutine Bundle Handle (Needed for dill global couroutine malloc cleanup on application exit)
#ifdef CTRoutine //check for coroutine support
	CTRoutine coroutine_bundle;
#endif

	//Declare dedicated Kernel Queues for dequeueing socket connect, read and write operations in our network application
	CTKernelQueue cxQueue, txQueue, rxQueue, oxPipe[2];

	//Declare thread handles for thread pools
#ifdef _WIN32
	HANDLE cxThread, txThread, rxThread;
#else
	pthread_attr_t  _thread_options = {0};
	/*
	struct pthread_attr _pthread_attr_default = 
	{
		.sched_policy = SCHED_OTHER,
		.sched_inherit = PTHREAD_INHERIT_SCHED,
		.prio = 0,
		.suspend = THR_CREATE_RUNNING,
		.flags = PTHREAD_SCOPE_SYSTEM,
		.stackaddr_attr = NULL,
		.stacksize_attr = THR_STACK_DEFAULT,
		.guardsize_attr = 0,
		.cpusetsize = 0,
		.cpuset = NULL
	};
	*/
	pthread_t cxThread, txThread, rxThread;
#endif

	//Declare Targets (ie specify TCP connection endpoints + options)
    CTTarget httpTarget = { 0 };
	//CTTarget reqlService = { 0 };	

	//libdispatch initialization
	//CreateDispatchQueues();

	//Connection & Cursor Pool Initialization
	CTCreateConnectionPool(&(HAPPYEYEBALLS_CONNECTION_POOL[0]), HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS);
	CTCreateCursorPool(&(_httpCursor[0]), CT_MAX_INFLIGHT_CURSORS);
	
	//Kernel FileDescriptor Event Queue Initialization
	cxQueue = CTKernelQueueCreate();//CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	rxQueue = CTKernelQueueCreate();//CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	txQueue = CTKernelQueueCreate();//CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
#ifndef _WIN32
	pipe(oxPipe);// = CTKernelQueueCreate();//CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);\
	g_kQueuePipePair.q = rxQueue;
	g_kQueuePipePair.p[0] = oxPipe[0];
	g_kQueuePipePair.p[1] = oxPipe[1];
#endif

	//Initialize Thread Pool for dequeing socket operations on associated kernel queues
	
	for (i = 0; i < 1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool

#ifdef _WIN32
		cxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CT_Dequeue_Connect, cxQueue, 0, NULL);		
		rxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CT_Dequeue_Recv_Decrypt, rxQueue, 0, NULL);
		txThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CT_Dequeue_Encrypt_Send, txQueue, 0, NULL);
		//TO DO:  check thread failure
#else
		if( pthread_create(&rxThread, &_thread_options, CT_Dequeue_Recv_Decrypt, (void*)&g_kQueuePipePair) != 0)
			assert(1==0);
		if( pthread_create(&txThread, &_thread_options, CT_Dequeue_Encrypt_Send, (void*)&txQueue) != 0)
			assert(1==0);
#endif
	}
    

	//Define https connection target
	//We aren't doing any authentication via http
	//SCHANNEL can find the certficate in the system CA keychain for us, MBEDTLS cannot
	httpTarget.url.host = (char*)http_server;
	httpTarget.url.port = http_port;
	httpTarget.proxy.host = NULL;//(char*)proxy_server;
	httpTarget.proxy.port = 0;//proxy_port;
	httpTarget.ssl.ca = NULL;//(char*)caPath;
	httpTarget.ssl.method = 0;// CTSSL_TLS_1_2;
	httpTarget.dns.resconf = (char*)resolvConfPath;
	httpTarget.dns.nssconf = (char*)nsswitchConfPath;
	httpTarget.cxQueue = cxQueue;
	httpTarget.rxQueue = rxQueue;
	httpTarget.txQueue = txQueue;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
#ifndef _WIN32
	httpTarget.oxPipe[0] = oxPipe[0];// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	httpTarget.oxPipe[1] = oxPipe[1];// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
#endif
	
	//pthread_t ptid = pthread_self();
	//long tid = 0;
	//thr_self(&tid);
	//fprintf(stderr, "HappyEyeballs Main Thread ID:  (%ld, %p)\n", tid, ptid);
	
	//Demonstrate CTransport CT C API Connection (No JSON support)
	//On Darwin platforms, the Core Transport connection routine operates asynchronously on a single thread to by integrating libdill coroutines with a thread's CFRunLoop operation (so the connection can be started from the main thread before cocoa app starts)
	//On WIN32, we only have synchronous/blocking connection support, but this can be easily thrown onto a background thread at startup (even in a UWP app?)
	coroutine_bundle = CTransport.connect(&httpTarget, _httpConnectionClosure);
	//CTransport.connect(&reqlService, _reqlConnectionClosure);

	//for(int i = 0; i<1;i++)
	//	sendReqlQuery(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);

	SystemKeyboardEventLoop();

	//Clean up socket connections (Note: closing a socket will remove all associated kevents on kqueues)
	//CTCloseConnection(&_httpConn);
	CTCloseConnection(&_httpConn);

	//cancel the libdispatch socket read source *ONLY* after removing the socket from kqueues and closing the socket
	//dispatch_source_cancel(_rxSource);

	//Clean Up Global/Thread Auth Memory
	ca_scram_cleanup();

	//close the dill coroutine bundle to clean up dill mallocs
//#ifdef CTRoutine //check for coroutine support
	CTRoutineClose(coroutine_bundle);
//#endif


	//Clean Up SSL Memory
	//CTSSLCleanup();

	//TO DO:  Shutdown threads \

    /*
	//Cleanup Thread Handles
	if (cxThread && cxThread != INVALID_HANDLE_VALUE)
		CloseHandle(cxThread);

	if (txThread && txThread != INVALID_HANDLE_VALUE)
		CloseHandle(txThread);

	if (rxThread && rxThread != INVALID_HANDLE_VALUE)
		CloseHandle(rxThread);
    */

    return 0;
}