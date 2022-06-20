#ifndef CTCONNECTION_H
#define CTCONNECTION_H

#include "CTError.h"
#include "CTFile.h"
#include "CTSocket.h"
//#include "CTSSL.h"

#if defined(__cplusplus) //|| defined(__OBJC__)
extern "C" {
#endif


//Win32 -> BSD Overlapped Compatibility
#ifndef _WIN32
typedef int DWORD;
typedef struct WSABUF
{
	unsigned long len;
	char* 		  buf;
}WSABUF;
typedef struct WSAOVERLAPPED{
	struct kevent hEvent;
}WSAOVERLAPPED;
#define ZeroMemory(x, y) memset(x, 0, y)
#endif

//#pragma mark -- CTSSL Struct

/* * *
 *  ReqlSSL Object
 *
 *  This just wraps a string that points to root ca file on disk
 *  for NodeJS API conformance (eg ReqlService.ssl.ca)
 *
 *  char * ca = [path to self-signed Root Certificate Authority File in DER format]
 *
 *  If you only have a CA file in .pem format, use OpenSSL from the command line to convert to DER
 *
 * * */
typedef struct CTRANSPORT_PACK_ATTRIBUTE CTSSL
{
    char * ca;
#ifdef _WIN32
	CTSSLMethod method;
#else
	int method;
#endif
}CTSSL;

//#pragma mark -- CTDNS Struct

/* * *
 *  ReqlDNS Object
 *
 *  Allows customization of DNS Servers using Linux Style
 *  config files
 *
 *  char * resconf = [path to resolv.conf]
 *  char * nssconf = [path to nsswitch.conf
 *
 *  If you don't have these files on disk or do not have access to them (eg iOS or Windows)
 *  then create them and include them with your application distribution
 * * */
typedef struct CTRANSPORT_PACK_ATTRIBUTE CTDNS
{
    char * resconf;
    char * nssconf;
#ifdef _WIN32
	WORD wID;// = GetTickCount() % 65536;
#endif
}CTDNS;

//#pragma mark -- HTTPConnection Struct

typedef struct CTCursor;
typedef struct CTTarget;

typedef enum CTOverlappedStage
{
	CT_OVERLAPPED_STAGE_NULL	= -1,
	CT_OVERLAPPED_SCHEDULE,
	CT_OVERLAPPED_SCHEDULE_RECV,			//TCP
	CT_OVERLAPPED_SCHEDULE_RECV_DECRYPT,	//TCP + TLS
	CT_OVERLAPPED_SCHEDULE_RECV_FROM,		//UDP
	CT_OVERLAPPED_EXECUTE,
	CT_OVERLAPPED_RECV,						//TCP
	CT_OVERLAPPED_RECV_DECRYPT,				//TCP + TLS
	CT_OVERLAPPED_RECV_FROM,				//UDP
	CT_OVERLAPPED_SEND,						//TCP
	CT_OVERLAPPED_ENCRYPT_SEND,				//TLS + TCP
}CTOverlappedStage;

typedef struct CTOverlappedTarget
{
	WSAOVERLAPPED		Overlapped;
	struct CTTarget* target;
	//char* buf;
	DWORD				Flags;
	//unsigned long		len;
	//uint64_t			queryToken;
	//WSABUF			wsaBuf;
	struct CTCursor* cursor;
	CTOverlappedStage   stage;

	//struct CTOverlappedResponse *overlappedResponse;	//ptr to the next overlapped
}CTOverlappedTarget;

/* * *
 *  CTConnection [ReqlConnection]
 *
 *  The primary object handle for working with this API
 * * */
typedef struct CTConnection
{
    //BSD Socket TLS Connection
    CTSSLContextRef sslContext;

	//BSD Socket TCP Connection
    union{
		CTSocketContext socketContext;
		CTSocket socket;
	};

	//RethinkDB Reql Query Token
    volatile uint64_t token;
    
    //Increment assign query ids
	union {
		volatile uint64_t queryCount;
		volatile uint64_t requestCount;
	};

	volatile int64_t responseCount;

	//char response_overlap_buffer[65536L];
	size_t response_overlap_buffer_length;

	//a user defined context object
	//void * ctx;
	union {
		struct CTTarget * target;
		struct CTTarget * service;
	};
	
	void * object_wrapper;

	//BSD Socket Event Queue
	int event_queue;
	char padding[4];
}CTConnection;


//#pragma mark -- CTConnection API Callback Typedefs
/* * *
 *  CTConnectionCallback
 *
 *  A callback for receiving asynchronous connection results
 *  that gets scheduled back to the calling event loop on completion
 * * */

//typedef int (*CTConnectionClosure)(struct CTError* err, struct CTConnection* conn);
typedef int (^CTConnectionClosure)(struct CTError * err, struct CTConnection* conn);

//#pragma mark -- HTTPTarget [ReqlConnectionOptions]
/* * *
 *  ReqlService [alias: ReqlConnectionOptions]
 *
 *  Generally, an ReqlService object's lifetime is shortlived
 *  as it is only used to create an Re�MQL [ReqlConnection] object
 *
 *  --Property names will mimic those of NodeJS API, where appropriate
 *  --Properties are ordered from largest to smallest with variable length buffers placed at the end
 *  --The host_addr property is not present in the NodeJS API:
 *      The client can use this property to indicate they chose to resolve DNS manually
 *      before calling ReqlConnect if desired,
 *      thus, bypassing the ReqlConnectRoutine Hostname DNS Resolve Step
 *
 * * */
typedef struct CTTarget     //TO DO:  calculate size and alignment
{
	//Server host address
	CTURL		url;			//22 or 26 bytes
	CTURL       proxy;			//22 or 26 bytes

	//We place a socket on target struct to allow for transient async DNS over UDP and connectionless UDP pipeline
	CTSocket	socket;

	//TLS SSL: Root Certificate Authority File Path
	CTSSL		ssl;            //8 bytes

	//DNS:  Nameserver [resolv.conf] and NSSwitch.conf File Paths
	CTDNS		dns;            //16 bytes

	//UDP/TCP Socket Connection Options
	int64_t		timeout;        //-1 indicates wait forever

	//CTKernelQueue	cxQueue;	//The desired kernel queue for the socket connection to async connect AND async recv on 
	//CTKernelQueue	txQueue;	//The desired kernel queue for the socket connection to async send on
	//CTKernelQueue	rxQueue;	//The desired kernel queue for the socket connection to async send on

	//CTKernelQueue rxQueue;

	union  
	{
		struct{
			CTKernelQueueType cxQueue;
			CTKernelQueueType cxPipe[2];
		};
		CTKernelQueue cq;
	};

	union  
	{
		struct{
			CTKernelQueueType txQueue;
			CTKernelQueueType txPipe[2];
		};
		CTKernelQueue tq;
	};


	union  
	{
		struct{
			CTKernelQueueType rxQueue;
			CTKernelQueueType rxPipe[2];
		};
		CTKernelQueue rq;
	};

	CTConnectionClosure				callback;
//#ifdef _WIN32
	struct CTOverlappedTarget		overlappedTarget;
//#endif
	void* ctx;

	//RethinkDB SASL SCRAM SHA-256 Handshake Credentials; 
	//TO DO: These auth properties have more to do with the connection than the target but we need a way to pass them when connecting
	//		 so perhaps move them into the CTURL struct
	char* user;                    //8 bytes
	char* password;                //8 bytes

	//explicit padding
	unsigned char padding[2];
}CTTarget;
typedef CTTarget CTConnectionOptions;


typedef struct CTOverlappedResponse
{
	WSAOVERLAPPED		Overlapped;
	CTConnection *		conn;  //TO DO:  connection is already on cursor this is unnecessary
	char *				buf;
	DWORD				Flags;
	unsigned long		len;
	uint64_t			queryToken;
	WSABUF				wsaBuf;
	void *				cursor;
	CTOverlappedStage	stage;

	//struct CTOverlappedResponse *overlappedResponse;	//ptr to the next overlapped
}CTOverlappedResponse;

typedef enum CTQueueEventID
{
 	CTQUEUE_ACTIVE_CURSOR
}CTKernelQueueEventID;

/*
typedef struct CTOverlappedRequest
{
	WSAOVERLAPPED		Overlapped;	
	CTConnection	*	conn;
	char *				buf;
	DWORD				Flags;
	unsigned long		len;
	uint64_t			queryToken;
}CTOverlappedRequest;
*/

//client must provide a callback to return pointer to end of callback when requested
//this also notifies the client when the cursor has been read, before the end of the message has been read
typedef char* (*CTCursorHeaderLengthFunc)(struct CTCursor * cursor, char * buffer, unsigned long bufferLength);
//typedef void (*CTCursorCompletionClosure)(CTError* err, struct CTCursor* cursor);
typedef void (^CTCursorCompletionClosure)(struct CTError * err, struct CTCursor* cursor);

typedef struct CTCursor
{
    union { //8 bytes + sizeof(CTFile)
		//overlap the header for various defined protocols
        //ReqlQueryMessageHeader * header;
		struct{
	        void * buffer;
			size_t size;
		};
		CTFile file;
    };
	unsigned long				headerLength;
	unsigned long				contentLength;
    CTConnection*				conn;
	CTTarget*					target;
	struct CTOverlappedResponse		overlappedResponse;
	char						requestBuffer[65536L];
	uint64_t					queryToken;
	CTCursorHeaderLengthFunc	headerLengthCallback;
	CTCursorCompletionClosure	responseCallback;
	CTCursorCompletionClosure	queryCallback;

	//Reql
}CTCursor;
typedef int CTStatus;

//TO DO:  remove this
//extern CTConnection CTHandshakeConnection;

//create a pool of connections and cursors that we can create at compile time 
//	1)  to avoid connection copies...
//  2)  to have permanent memory that can be used for transiet async non-blocking pipeline routines
extern int			 CT_MAX_CONNECTIONS;			// Total number of available connections in the pool;
extern int			 CT_NUM_CONNECTIONS;			// Count of active connections strictly <= CT_MAX_CONNECTIONS
extern int			 CT_MAX_INFLIGHT_CONNECTIONS;	//A limit to the number of connections that can be scheduled/pending at any given time

extern int			 CT_NUM_CURSORS;
extern int			 CT_CURSOR_INDEX;

extern CTConnection *	CT_CONNECTION_POOL;
extern CTCursor		*	CT_CURSOR_POOL;

//#pragma mark -- CTConnection API Method Function Pointer Definitions
typedef int (*CTConnectFunc)(struct CTTarget * service, CTConnectionClosure callback);

#if defined(__cplusplus) //|| defined(__OBJC__)
}
#endif


#endif