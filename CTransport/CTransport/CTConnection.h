#ifndef CTCONNECTION_H
#define CTCONNECTION_H

#include "CTError.h"
#include "CTSocket.h"
#include "CTSSL.h"

#if defined(__cplusplus) //|| defined(__OBJC__)
extern "C" {
#endif


#pragma mark -- CTSSL Struct

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
}CTSSL;

#pragma mark -- CTDNS Struct

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
}CTDNS;

#pragma mark -- HTTPTarget [ReqlConnectionOptions]
/* * *
 *  ReqlService [alias: ReqlConnectionOptions]
 *
 *  Generally, an ReqlService object's lifetime is shortlived
 *  as it is only used to create an ReØMQL [ReqlConnection] object
 *
 *  --Property names will mimic those of NodeJS API, where appropriate
 *  --Properties are ordered from largest to smallest with variable length buffers placed at the end
 *  --The host_addr property is not present in the NodeJS API:
 *      The client can use this property to indicate they chose to resolve DNS manually
 *      before calling ReqlConnect if desired,
 *      thus, bypassing the ReqlConnectRoutine Hostname DNS Resolve Step
 *
 * * */
typedef struct CTTarget      //72 bytes, 8 byte data alignment
{

#ifdef _WIN32

	//Pre-resolved socket address input or storage for async resolve
    struct sockaddr_in host_addr;   //16 bytes

#endif

    //Server host address string
    char * host;                    //8 bytes
    
    //RethinkDB SASL SCRAM SHA-256 Handshake Credentials
    char * user;                    //8 bytes
    char * password;                //8 bytes
    
    //TLS SSL: Root Certificate Authority File Path
    CTSSL ssl;                    //8 bytes
    
    //DNS:  Nameserver [resolv.conf] and NSSwitch.conf File Paths
    CTDNS dns;                    //16 bytes
    
    //ReqlConnectionCallback * callback;  //8 bytes
    
    //TCP Socket Connection Input
    int64_t timeout;                //-1 indicates wait forever

	void	* ctx;
    unsigned short port;
    
    //explicit padding
    unsigned char padding[2];
}CTTarget;
typedef CTTarget CTConnectionOptions;

#pragma mark -- HTTPConnection Struct


/* * *
 *  CTConnection [ReqlConnection]
 *
 *  The primary object handle for working with this API
 * * */
typedef struct CTConnection
{
    //ReqlService * service;

    //BSD Socket TLS Connection
    CTSSLContextRef sslContext;
    //SSLContextRef sslWriteContext;

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

	union{
		char * query_buffers;
		char * request_buffers;
	};
	char * response_buffers;

	//a user defined context object
	//void * ctx;
	union {
		CTTarget * target;
		CTTarget * service;
	};
	
	//BSD Socket Event Queue
	int event_queue;
	char padding[4];
}CTConnection;


#ifdef _WIN32
typedef struct CTOverlappedResponse
{
	WSAOVERLAPPED		Overlapped;
	CTConnection *	conn;
	char *				buf;
	DWORD				Flags;
	unsigned long		len;
	uint64_t			queryToken;
	WSABUF				wsaBuf;
}CTOverlappedResponse;

typedef struct CTOverlappedRequest
{
	WSAOVERLAPPED		Overlapped;	
	CTConnection	*	conn;
	char *				buf;
	DWORD				Flags;
	unsigned long		len;
	uint64_t			queryToken;
}CTOverlappedRequest;

#endif

#pragma mark -- CTConnection API Callback Typedefs
/* * *
 *  CTConnectionCallback
 *
 *  A callback for receiving asynchronous connection results
 *  that gets scheduled back to the calling event loop on completion
 * * */
typedef void (*CTConnectionCallback)(struct CTError * err, struct CTConnection* conn);

//A completion block handler that returns the HTTP(S) status code
//and the relevant JSON documents from the database store associated with the request
//typedef void (^ReqlConnectionClosure)(ReqlError * err, ReqlConnection * conn);
typedef int (*CTConnectionClosure)(struct CTError * err, struct CTConnection * conn);

#pragma mark -- CTConnection API Method Function Pointer Definitions
typedef int (*CTConnectFunc)(struct CTTarget * service, CTConnectionClosure callback);

#if defined(__cplusplus) //|| defined(__OBJC__)
}
#endif


#endif