//
//  CoReQL.h
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//


#ifndef ReØMQL_h
#define ReØMQL_h

#include "ct_scram.h"

//uint64_t includes
#include <stdint.h>

//Proto Includes
#include "../protobuf/ql2.pb-c.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//SSL Includes
#include "ReqlSSL.h"

//OpenSSL Includes
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
//#include <malloc.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#endif
//#include <openssl/ssl.h>
//#include <openssl/err.h>

//Nanomsg Includes
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

//kqueue
#ifndef WIN32
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>
#endif

#ifdef _WIN32
#include <stdint.h>
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;

//#ifndef uintptr_t
//typedef unsigned intptr_t;
//#endif
//#endif

#endif


#include "ReqlCoroutine.h"


//#include "tls_api.h"

//use protobuf-c's extern C helper macro because it is there
//PROTOBUF_C__BEGIN_DECLS

#if defined(__cplusplus) //|| defined(__OBJC__)
extern "C" {
#endif


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/


//Tunable Params

#ifndef PAGE_SIZE
#define REQL_PAGE_SIZE 4096
#else
#define REQL_PAGE_SIZE PAGE_SIZE
#endif

//static const unsigned char REQL_V1_0_MAGIC_NUMBER[4] = {0xc3, 0xbd, 0xc2, 0x34};
static const char * REQL_V1_0_MAGIC_NUMBER = "\xc3\xbd\xc2\x34";//{0xc3, 0xbd, 0xc2, 0x34};

static const uintptr_t REQL_EVT_TIMEOUT = UINTPTR_MAX;
static const uint32_t  REQL_MAX_TIMEOUT = UINT_MAX;

static const char * REQL_CHANGE_TYPE_INITIAL = "initial";
static const char * REQL_CHANGE_TYPE_CHANGE = "change";
static const char * REQL_CHANGE_TYPE_REMOVE = "remove";

#pragma mark -- Reql Object Type Enums

/***
 *  ReqlQueryType Enums
 *
 *  We map our own identifiers to the TERM__TERM_TYPE__[QueryType] definitions
 *  in protobuf ql2.pb-c.h
 ***/
typedef enum ReqlQueryType
{
    REQL_START          = QUERY__QUERY_TYPE__START,         //1
    REQL_CONTINUE       = QUERY__QUERY_TYPE__CONTINUE,      //2
    REQL_STOP           = QUERY__QUERY_TYPE__STOP,          //3
    REQL_NOREPLY_WAIT   = QUERY__QUERY_TYPE__NOREPLY_WAIT,  //4
    REQL_SERVER_INFO    = QUERY__QUERY_TYPE__SERVER_INFO,   //5
}ReqlQueryType;


/***
 *  ReqlTermType Enums
 *
 *  We map our own identifiers to the TERM__TERM_TYPE__[QueryType] definitions
 *  in protobuf ql2.pb-c.h
 ***/
typedef enum ReqlTermType
{
    REQL_EXPR               = 0,
    REQL_MAKE_ARRAY         = TERM__TERM_TYPE__MAKE_ARRAY,      //2
    REQL_MAKE_OBJECT        = TERM__TERM_TYPE__MAKE_OBJ,        //3
    REQL_VAR                = TERM__TERM_TYPE__VAR,             //10
    REQL_IMPLICIT_VAR       = TERM__TERM_TYPE__IMPLICIT_VAR,    //13
    REQL_DB                 = TERM__TERM_TYPE__DB,              //14
    REQL_TABLE              = TERM__TERM_TYPE__TABLE,           //15
    REQL_GET                = TERM__TERM_TYPE__GET,             //16
    REQL_EQ                 = TERM__TERM_TYPE__EQ,              //17
    REQL_NE                 = TERM__TERM_TYPE__NE,              //18
    REQL_LT                 = TERM__TERM_TYPE__LT,              //19
    REQL_GT                 = TERM__TERM_TYPE__GT,              //21
    REQL_GE                 = TERM__TERM_TYPE__GE,              //22
    REQL_NOT                = TERM__TERM_TYPE__NOT,             //23
    REQL_ADD                = TERM__TERM_TYPE__ADD,             //24
    REQL_SUB                = TERM__TERM_TYPE__SUB,             //25
    REQL_APPEND             = TERM__TERM_TYPE__APPEND,          //29
    REQL_GET_FIELD          = TERM__TERM_TYPE__GET_FIELD,       //31
    REQL_HAS_FIELDS         = TERM__TERM_TYPE__HAS_FIELDS,      //32
    REQL_PLUCK              = TERM__TERM_TYPE__PLUCK,           //33
    REQL_WITHOUT            = TERM__TERM_TYPE__WITHOUT,         //34
    REQL_MERGE              = TERM__TERM_TYPE__MERGE,           //35
    REQL_FILTER             = TERM__TERM_TYPE__FILTER,          //39
    REQL_ORDER_BY           = TERM__TERM_TYPE__ORDER_BY,        //41
    REQL_COUNT              = TERM__TERM_TYPE__COUNT,           //43
    REQL_COERCE_TO          = TERM__TERM_TYPE__COERCE_TO,       //51
    REQL_UPDATE             = TERM__TERM_TYPE__UPDATE,          //53
    REQL_DELETE             = TERM__TERM_TYPE__DELETE,          //54
    REQL_INSERT             = TERM__TERM_TYPE__INSERT,          //56
    REQL_DO                 = TERM__TERM_TYPE__FUNCALL,         //64
    REQL_BRANCH             = TERM__TERM_TYPE__BRANCH,          //65
    REQL_OR                 = TERM__TERM_TYPE__OR,              //66
    REQL_AND                = TERM__TERM_TYPE__AND,             //67
    REQL_FUNC               = TERM__TERM_TYPE__FUNC,            //69
    REQL_LIMIT              = TERM__TERM_TYPE__LIMIT,           //71
    REQL_ASC                = TERM__TERM_TYPE__ASC,             //73
    REQL_DESC               = TERM__TERM_TYPE__DESC,            //74
    REQL_SET_INSERT         = TERM__TERM_TYPE__SET_INSERT,      //88
    REQL_DEFAULT            = TERM__TERM_TYPE__DEFAULT,         //92
    REQL_MATCH              = TERM__TERM_TYPE__MATCH,           //97
    REQL_ISO8601            = TERM__TERM_TYPE__ISO8601,         //99
    REQL_EPOCH              = TERM__TERM_TYPE__EPOCH_TIME,          //101
    REQL_TO_EPOCH           = TERM__TERM_TYPE__TO_EPOCH_TIME,       //102
    REQL_SUM                = TERM__TERM_TYPE__SUM,                 //145
    REQL_CHANGES            = TERM__TERM_TYPE__CHANGES,             //152
    REQL_POINT              = TERM__TERM_TYPE__POINT,               //159
    REQL_LINE               = TERM__TERM_TYPE__LINE,                //160
    REQL_POLYGON            = TERM__TERM_TYPE__POLYGON,             //161
    REQL_BRACKET            = TERM__TERM_TYPE__BRACKET,             //170

    REQL_GET_INTERSECTING   =  TERM__TERM_TYPE__GET_INTERSECTING
}ReqlTermType;

typedef enum ReqlResponseType
{
    REQL_SUCCESS_ATOM       = RESPONSE__RESPONSE_TYPE__SUCCESS_ATOM,    //1
    REQL_SUCCESS_SEQ        = RESPONSE__RESPONSE_TYPE__SUCCESS_SEQUENCE,//2
    REQL_SUCCESS_PARTIAL    = RESPONSE__RESPONSE_TYPE__SUCCESS_PARTIAL, //3
    REQL_WAIT_COMPLETE      = RESPONSE__RESPONSE_TYPE__WAIT_COMPLETE,   //4
    //REQL_SERVER_INFO      = RESPONSE__RESPONSE_TYPE__SERVER_INFO,     //5
    REQL_CLIENT_ERROR       = RESPONSE__RESPONSE_TYPE__CLIENT_ERROR,    //16
    REQL_COMPILE_ERROR      = RESPONSE__RESPONSE_TYPE__COMPILE_ERROR,   //17
    REQL_RUNTIME_ERROR      = RESPONSE__RESPONSE_TYPE__RUNTIME_ERROR    //18
    //1 SUCCESS_ATOM: The whole query has been returned and the result is in the first (and only) element of r.
    //2 SUCCESS_SEQUENCE: Either the whole query has been returned in r, or the last section of a multi-response query has been returned.
    //3 SUCCESS_PARTIAL: The query has returned a stream, which may or may not be complete. To retrieve more results for the query, send a CONTINUE message (see below).
    //4 WAIT_COMPLETE: This ResponseType indicates all queries run in noreply mode have finished executing. r will be empty.
    //5 SERVER_INFO: The response to a SERVER_INFO request. The data will be in the first (and only) element of r.
    //16 CLIENT_ERROR: The server failed to run the query due to a bad client request. The error message will be in the first element of r.
    //17 COMPILE_ERROR: The server failed to run the query due to an ReQL compilation error. The error message will be in the first element of r.
    //18 RUNTIME_ERROR: The query compiled correctly, but failed at runtime. The error message will be in the first element of r.
}ReqlResponseType;

#pragma mark -- ReqlError Struct

/* * *
 *  ReqlError Object
 *
 * * */
typedef struct REQL_PACK_ATTRIBUTE ReqlError
{
    ReqlErrorClass errClass;
    int32_t id;
    void *   description;
}ReqlError;

#pragma mark -- ReqlSSL Struct

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
typedef struct REQL_PACK_ATTRIBUTE ReqlSSL
{
    char * ca;
}ReqlSSL;

#pragma mark -- ReqlDNS Struct

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
typedef struct REQL_PACK_ATTRIBUTE ReqlDNS
{
    char * resconf;
    char * nssconf;
}ReqlDNS;

#pragma mark -- ReqlService [ReqlConnectionOptions]
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
typedef struct ReqlService      //72 bytes, 8 byte data alignment
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
    ReqlSSL ssl;                    //8 bytes
    
    //DNS:  Nameserver [resolv.conf] and NSSwitch.conf File Paths
    ReqlDNS dns;                    //16 bytes
    
    //ReqlConnectionCallback * callback;  //8 bytes
    
    //TCP Socket Connection Input
    int64_t timeout;                //-1 indicates wait forever

	void	* ctx;
    unsigned short port;
    
    //explicit padding
    unsigned char padding[2];
}ReqlService;
typedef ReqlService ReqlConnectionOptions;

#pragma mark -- ReqlConnection Struct
/*
typedef enum ReqlConnectionState
{
    
    
    
}ReqlConnectionState;
*/

/* * *
 *  ReØMQL [ReqlConnection]
 *
 *  The primary object handle for working with this API
 * * */
typedef struct ReqlConnection
{
    //ReqlService * service;

    //BSD Socket TLS Connection
    ReqlSSLContextRef sslContext;
    //SSLContextRef sslWriteContext;

	//BSD Socket TCP Connection
    union{
		ReqlSocketContext socketContext;
		REQL_SOCKET socket;
	};

   
    //RethinkDB Reql Query Token
    volatile uint64_t token;
    
    //Increment assign query ids
    volatile uint64_t queryCount;
    

    
	///Unusued:
    // version sent ?
    //int version_sent;
    // buffer for the query
    char * query_buffers;
	char * response_buffers;
	//void * query_buf[2];
    // buffer for the response
    //void *response_buf[2];

	//a user defined context object
	//void * ctx;
	ReqlService * service;
	//BSD Socket Event Queue
	int event_queue;
	char padding[4];
}ReqlConnection;
typedef ReqlConnection ReØMQL;

#ifdef _WIN32
typedef struct ReqlOverlappedResponse
{
	WSAOVERLAPPED		Overlapped;
	ReqlConnection *	conn;
	char *				buf;
	DWORD				Flags;
	unsigned long		len;
	uint64_t			queryToken;
	WSABUF				wsaBuf;
}ReqlOverlappedResponse;

typedef struct ReqlOverlappedQuery
{
	WSAOVERLAPPED		Overlapped;	
	ReqlConnection *	conn;
	char *				buf;
	DWORD				Flags;
	unsigned long		len;
	uint64_t			queryToken;
}ReqlOverlappedQuery;

#endif

/*
typedef struct ReqlResponse
{
    int32_t         t;      //the ResponseType, as defined in ql2.proto
    void *          r;      //data from the result, as a JSON array
    void *          b;      //a backtrace if t is an error type; this field will not be present otherwise
    void *          p;      //a profile if the global optarg profile: true was specified; this field will not be present otherwise
    void *          n;      //an optional array of ResponseNote values, as defined in ql2.proto
    size_t length;
}ReqlResponse;
*/

#pragma mark -- ReqlGeometry

typedef enum ReqlGeomType
{
    REQL_GEOM_POINT,
    REQL_GEOM_LINE,
    REQL_GEOM_POLY
}ReqlGeomType;


REQL_DECLSPEC REQL_ALIGN(8) typedef union ReqlPoint{    
    struct { double x, y; };
    struct { double lon, lat;};
}ReqlPoint;

/*
typedef struct REQL_ALIGN(8) ReqlLine{
    double x, y;
}ReqlPoint;

typedef struct REQL_ALIGN(8) ReqlPoint{
    double x, y;
}ReqlPoint;
*/

#pragma mark -- ReqlConnection API Callback Typedefs
/* * *
 *  ReqlConnectionCallback
 *
 *  A callback for receiving asynchronous connection results
 *  that gets scheduled back to the calling event loop on completion
 * * */
typedef void (*ReqlConnectionCallback)(ReqlError * err, ReqlConnection * conn);

//A completion block handler that returns the HTTP(S) status code
//and the relevant JSON documents from the database store associated with the request
//typedef void (^ReqlConnectionClosure)(ReqlError * err, ReqlConnection * conn);
typedef int (*ReqlConnectionClosure)(ReqlError * err, ReqlConnection * conn);

#pragma mark -- ReqlConnection API Method Function Pointer Definitions
typedef int (*ReqlConnectFunc)(ReqlService * service, ReqlConnectionClosure callback);

#pragma mark -- ReqlConnect API Methods
//Connect to a RethinkDB Service + Init/Alloc ReØMQL [ReqlConnection] object
REQL_API REQL_INLINE int ReqlConnect(  ReqlService * service, ReqlConnectionClosure callback);
REQL_API REQL_INLINE int ReqlSASLHandshake(ReØMQL * r, ReqlService * service);
REQL_API REQL_INLINE int ReqlCloseConnection( ReqlConnection * conn );

//Send/Receive message over ReqlConnection dedicated BSD TCP socket
REQL_API REQL_INLINE int ReqlSend(     ReqlConnection * conn, void * msg, unsigned long msgLength );
REQL_API REQL_INLINE ReqlDriverError ReqlSendWithQueue(ReqlConnection* conn, void * msg, unsigned long * msgLength);

REQL_API REQL_INLINE void* ReqlRecv(     ReqlConnection * conn, void * msg, unsigned long * msgLength );

REQL_API REQL_INLINE ReqlDriverError ReqlAsyncRecv(ReqlConnection* conn, void * msg, unsigned long * msgLength);

REQL_API REQL_INLINE void ReqlReinitSecurityContext(ReqlConnection * conn);

REQL_API REQL_INLINE int ReqlRecvQueryResponse(ReqlConnection* conn, void * msg, size_t * msgLength);


#pragma mark -- ReqlQuery API Definitions
typedef struct ReqlQueryContext ReqlQueryContext;
struct ReqlQueryContext* ReqlQueryGetContext(void);
void ReqlQuerySetContext(ReqlQueryContext * ctx);
void ReqlQueryClearContext(ReqlQueryContext* ctx);

static const uint64_t REQL_MAX_QUERY_LENGTH = 256;

typedef struct ReqlCursor      ReqlCursor;      //forward declare ReqlCursor struct
typedef struct ReqlQueryObject ReqlQueryObject; //forward Declare ReqlQueryObject struct

#pragma mark -- ReqlQuery API Function Pointer Typedefs

//A completion block handler thant returns ReqlError and/or
//the relevant JSON documents for the database store associated with the ReqlQuery
//typedef void (^ReqlQueryClosure)(ReqlError * err, ReqlCursor cursor);
typedef void (*ReqlQueryClosure)(ReqlError * err, ReqlCursor cursor);

//typedef void*               (*ReqlQueryMakePtrRet)    (void * query);
//typedef ReqlQueryMakePtrRet (*ReqlQueryMakePtr)       (void * query);
//typedef ReqlQueryObject           (*ReqlDatabaseQueryFunc)  (void * name);        //input:  db name
//typedef ReqlQueryObject           (*ReqlTableQueryFunc)     (void * name);        //input:  table name
//typedef ReqlQueryObject           (*ReqlFilterQueryFunc)    (void * predicates);  //input:  JSON Dict of values to filter by
//typedef ReqlQueryObject           (*ReqlInsertQueryFunc)    (void * documents);   //input:  JSON array of JSON dictionaries (ie JSON Document Objects)
//typedef ReqlQueryObject           (*ReqlDeleteQueryFunc)    (void * options);     //input:  JSON dict of options for the delete query
//typedef ReqlQueryObject           (*ReqlChangesQueryFunc)   (void * options);     //input:  JSON dict of options for the delete query
//typedef void                      (*ReqlRunQueryFunc)       (ReqlConnection * conn, void * options, ReqlQueryClosure callback);

/***
 *  ReqlQuery
 *
 *  A C-Style object that can be used to store query data for serialization
 *  and function pointers for NodeJS style syntax operation
 *
 *  A RetinkDB Query Language Query is just a formatted string of the form:
 *
 *      [QueryType, ["QueryData"], {QueryOptions}]
 ***/

/*
struct
{
    ReqlRunQueryFunc        run;
    ReqlTableQueryFunc      table;
    ReqlFilterQueryFunc     filter;
    ReqlInsertQueryFunc     insert;
    ReqlDeleteQueryFunc     delete;
}ReqlQueryFunctions;
*/
/*
typedef struct ReqlQueryFuncTable
{
    ReqlRunQueryFunc        run;
    ReqlTableQueryFunc      table;
    ReqlFilterQueryFunc     filter;
    ReqlInsertQueryFunc     insert;
    ReqlDeleteQueryFunc     delete;
    ReqlChangesQueryFunc    changes;
}ReqlQueryFuncTable;
*/

#pragma pack(push, 1)
typedef struct REQL_PACK_ATTRIBUTE ReqlQueryMessageHeader
{
    uint64_t token;
    int32_t  length;
}ReqlQueryMessageHeader;
#pragma pack(pop)

#pragma mark -- ReqlCursor

/***
 *  ReqlCursor
 *
 *  A cursor is just a wrapper to a socket receive buffer
 ***/
typedef struct ReqlCursor
{
    union { //8 bytes
        ReqlQueryMessageHeader * header;
        void * buffer;
    };
    size_t length;
    ReqlConnection *conn;
    //Reql
}ReqlCursor;

REQL_API REQL_INLINE uint64_t ReqlCursorContinue(ReqlCursor *cursor, void* options);
REQL_API REQL_INLINE uint64_t ReqlCursorStop(ReqlCursor *cursor, void* options);


REQL_API REQL_INLINE ReqlResponseType ReqlCursorGetResponseType(ReqlCursor * cursor);

/***
 *  ReqlQuery
 ***/
typedef struct ReqlQuery
{
    ReqlTermType            type;                       //8 bytes
    void*                   data;                       //8 bytes
    void*                   options;                    //8 bytes
}ReqlQuery;


/***
 *  ReqlQueryContext
 ***/
typedef struct ReqlQueryContext
{
    ReqlQuery stack[10];
    int numQueries;
}ReqlQueryContext;

typedef struct ReqlQueryCtx
{
    ReqlQuery stack[10];
    int numQueries;
}ReqlQueryCtx;

/***
 *  ReqlQueryObject
 ***/
/*
typedef struct ReqlQueryObject
{
    union {
        struct ReqlQuery query;
        struct {
            ReqlTermType            type;                       //8 bytes
            void*                   data;                       //8 bytes
            void*                   options;                    //8 bytes
        };
    };
    
    union {
        struct ReqlQueryFuncTable   vTable;
        struct{
            ReqlRunQueryFunc        run;
            ReqlTableQueryFunc      table;
            ReqlFilterQueryFunc     filter;
            ReqlInsertQueryFunc     insert;
            ReqlDeleteQueryFunc     delete;
            ReqlChangesQueryFunc    changes;
        };
    };
    
}ReqlQueryObject;

//typedef struct ReqlQueryFunctions ReqlQueryFunctions;

//RealTime Thread Agnostic Zero Copy Socket RethinkDB Query Language Client Driver
//#define ReqlQueryMake( queryType, queryData, queryOptions)
//REQL_API REQL_INLINE dill_coroutine void* ReqlQueryChain( void * query);
//REQL_API REQL_INLINE dill_coroutine ReqlQueryMakePtrRet ReqlQueryMake(void * query);//, const char * queryOptions

//ReqlQueryMake Convenience Methods
REQL_API REQL_INLINE ReqlQueryObject ReqlDatabaseQuery   ( void * name );             //eg [14, ["db name"]]
REQL_API REQL_INLINE ReqlQueryObject ReqlTableQuery      ( void * name );             //eg [15, [...], ["table name"]]
REQL_API REQL_INLINE ReqlQueryObject ReqlGetQuery        ( void * name );             //eg [15, [...], ["table name"]]
REQL_API REQL_INLINE ReqlQueryObject ReqlPluckQuery      ( void * name );             //eg [15, [...], ["table name"]]

REQL_API REQL_INLINE ReqlQueryObject ReqlCountQuery      (void);
REQL_API REQL_INLINE ReqlQueryObject ReqlMergeQuery      ( void * predicateString );
REQL_API REQL_INLINE ReqlQueryObject ReqlFilterQuery     ( void * predicates );       //eg [39, [...], {"name": "Michel"}]]
REQL_API REQL_INLINE ReqlQueryObject ReqlGetFieldQuery   ( void * name );
REQL_API REQL_INLINE ReqlQueryObject ReqlCoerceToQuery   ( void * name );

REQL_API REQL_INLINE ReqlQueryObject ReqlAppendQuery     ( void * jsonObjectString );//ReqlConnection * conn, char *dbname, char *tablename, char *datacenter, char *pkey, int use_outdated )

REQL_API REQL_INLINE ReqlQueryObject ReqlRowQuery        ( void* name );
REQL_API REQL_INLINE ReqlQueryObject ReqlDoQuery         ( void* queryString );
REQL_API REQL_INLINE ReqlQueryObject ReqlExprQuery       ( void* queryString );

REQL_API REQL_INLINE ReqlQueryObject ReqlMatchQuery      ( void* name );

REQL_API REQL_INLINE ReqlQueryObject ReqlOrderByQuery    ( void * field );            //eg [39, [...], {"name": "Michel"}]]

REQL_API REQL_INLINE ReqlQueryObject ReqlInsertQuery     ( void * jsonString );       //eg [56, [...]]]
REQL_API REQL_INLINE ReqlQueryObject ReqlUpdateQuery      ( void * predicateString );
REQL_API REQL_INLINE ReqlQueryObject ReqlDeleteQuery     ( void * optionString );     //eg [54, [...]]]
REQL_API REQL_INLINE ReqlQueryObject ReqlChangesQuery    ( void * options );          //eg [56, [...]]]
*/
//Geometry Query Convenience Methods
//REQL_API REQL_INLINE ReqlQueryObject ReqlGeoIntersectionQuery( void * geometryString, void * geospatialIndexString );

//REQL_API REQL_INLINE ReqlQueryObject ReqlPointQuery

/***
 *  ReqlRunQuery
 *
 *  [Asynchronously] Run a query on a connection. The callback will get either an error,
 *  a single JSON result, or a cursor, depending on the query.
 ***/
//REQL_API REQL_INLINE uint64_t ReqlRunQuery       ( ReqlConnection * conn, void * options, ReqlQueryClosure callback );
REQL_API REQL_INLINE uint64_t ReqlRunQuery          (const char ** queryBufPtr, ReqlConnection * conn);//, void * options);//, ReqlQueryClosure callback)
REQL_API REQL_INLINE uint64_t ReqlRunQueryWithToken (const char ** queryBufPtr, ReqlConnection * conn, uint64_t queryToken);
REQL_API REQL_INLINE uint64_t ReqlRunQueryWithTokenOnQueue(const char ** queryBufPtr, unsigned long queryStrLength, ReqlConnection * conn, uint64_t queryToken);

REQL_API REQL_INLINE uint64_t ReqlRunQueryCtx      ( ReqlQueryContext * ctx, ReqlConnection * conn, void * options );//, ReqlQueryClosure callback);
#ifdef __APPLE__
REQL_API REQL_INLINE uint64_t ReqlRunQueryCtxOnQueue(ReqlQueryContext * context, ReqlConnection * conn, void * options, dispatch_queue_t queryQueue);//, ReqlQueryClosure callback)
#endif
REQL_API REQL_INLINE uint64_t ReqlContinueQuery    ( ReqlConnection * conn, uint64_t queryToken, void * options );
REQL_API REQL_INLINE uint64_t ReqlContinueQueryCtx ( ReqlQueryContext * ctx, ReqlConnection * conn, uint64_t queryToken, void * options );
REQL_API REQL_INLINE uint64_t ReqlStopQuery        ( ReqlConnection * conn, uint64_t queryToken, void * options );
REQL_API REQL_INLINE uint64_t ReqlStopQueryCtx     ( ReqlQueryContext* ctx, ReqlConnection * conn, uint64_t queryToken, void * options );

//REQL_API REQL_INLINE const struct ReqlQueryFuncTable _ReqlQueryObjectFuncTable;// = {ReqlRunQuery, ReqlTableQuery, ReqlFilterQuery, ReqlInsertQuery, ReqlDeleteQuery, ReqlChangesQuery};

REQL_API REQL_INLINE int ReqlSendQuery(ReØMQL * r, Query * query);


#pragma mark -- Global ReqlClientDriver Object
typedef struct ReqlClientDriver
{
    //The Client Driver Object can create Reql Connections
    ReqlConnectFunc             connect;
    
    //The Client Driver Object can do top level queries
    //ReqlDatabaseQueryFunc       db;
}ReqlClientDriver;

static const ReqlClientDriver RethinkDB = {ReqlConnect};//, ReqlDatabaseQuery};

#ifdef __APPLE__
extern CFRunLoopSourceContext REQL_RUN_LOOP_SRC_CTX;
extern const CFOptionFlags REQL_RUN_LOOP_WAITING;// = kCFRunLoopBeforeWaiting | kCFRunLoopAfterWaiting;
extern const CFOptionFlags REQL_RUN_LOOP_RUNNING;//  = kCFRunLoopAllActivities;

extern CFRunLoopObserverRef ReqlCreateRunLoopObserver(void);
extern CFRunLoopSourceRef ReqlCreateRunLoopSource(void);
#endif 

//#ifdef _WIN32
//#pragma pack(pop)
//#endif

#if defined(__cplusplus) //|| defined(__OBJC__)
}
#endif
//PROTOBUF_C__END_DECLS

#endif /* ReØMQL_h */
