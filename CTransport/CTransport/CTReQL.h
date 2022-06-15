#ifndef CTREQL_H
#define CTREQL_H

#include "CTConnection.h"

//Proto Includes
#include "../protobuf/ql2.pb-c.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef PAGE_SIZE
#define REQL_PAGE_SIZE 4096
#else
#define REQL_PAGE_SIZE PAGE_SIZE
#endif

//TO DO:  move the static const char defines into implementation file
//static const unsigned char REQL_V1_0_MAGIC_NUMBER[4] = {0xc3, 0xbd, 0xc2, 0x34};
static const char * REQL_V1_0_MAGIC_NUMBER = "\xc3\xbd\xc2\x34";//{0xc3, 0xbd, 0xc2, 0x34};

static const char* REQL_JSON_PROTOCOL_VERSION = "{\"protocol_version\":0,\0";
static const char* REQL_JSON_AUTH_METHOD = "\"authentication_method\":\"SCRAM-SHA-256\",\0";

static const char* REQL_SUCCESS_KEY = "\"success\"";
static const char* REQL_AUTH_KEY = "\"authentication\"";

#define CT_REQL_NUM_NONCE_BYTES 20
static const char * REQL_CLIENT_KEY = "Client Key";
static const char * REQL_SERVER_KEY = "Server Key";


static const uintptr_t REQL_EVT_TIMEOUT = UINTPTR_MAX;
static const uint32_t  REQL_MAX_TIMEOUT = UINT_MAX;

static const char * REQL_CHANGE_TYPE_INITIAL = "initial";
static const char * REQL_CHANGE_TYPE_CHANGE = "change";
static const char * REQL_CHANGE_TYPE_REMOVE = "remove";



//#pragma mark -- Reql Object Type Enums

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
    REQL_GET_ALL            = TERM__TERM_TYPE__GET_ALL,         //78
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


//ReQL Protocol Definitions
#pragma pack(push, 1)
typedef struct CTRANSPORT_PACK_ATTRIBUTE ReqlQueryMessageHeader
{
    uint64_t token;
    int32_t  length;
}ReqlQueryMessageHeader;
#pragma pack(pop)

//#pragma mark -- ReqlCursor

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
    CTConnection *conn;
    //Reql
}ReqlCursor;

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

typedef CTError ReqlError;
typedef CTTarget ReqlService;
typedef CTClientError ReqlDriverError;
typedef CTKernelQueue ReqlThreadQueue;
typedef CTDispatchSource ReqlDispatchSource;
typedef CTConnection ReqlConnection;

//error redefs
#define ReqlIOPending CTSocketIOPending
#define ReqlSSLStatus CTSSLStatus
#define ReqlSuccess	  CTSuccess

#define ReqlCompileErrorClass CTCompileErrorClass
#define ReqlDriverErrorClass CTDriverErrorClass
#define ReqlRuntimeErrorClass CTRuntimeErrorClass 

#define RethinkDB CTransport

//#ifdef _WIN32
//#ifdef CTOverlappedRequest
typedef CTOverlappedResponse ReqlOverlappedQuery;
typedef CTOverlappedResponse ReqlOverlappedResponse;
//#endif
//#endif


//Cursor API
CTRANSPORT_API CTRANSPORT_INLINE uint64_t ReqlCursorContinue(ReqlCursor *cursor, void* options);
CTRANSPORT_API CTRANSPORT_INLINE uint64_t ReqlCursorStop(ReqlCursor *cursor, void* options);
CTRANSPORT_API CTRANSPORT_INLINE ReqlResponseType ReqlCursorGetResponseType(ReqlCursor * cursor);

CTRANSPORT_API CTRANSPORT_INLINE uint64_t ReqlContinueQuery    ( ReqlConnection * conn, uint64_t queryToken, void * options );
CTRANSPORT_API CTRANSPORT_INLINE uint64_t ReqlContinueQueryCtx ( ReqlQueryContext * ctx, ReqlConnection * conn, uint64_t queryToken, void * options );
CTRANSPORT_API CTRANSPORT_INLINE uint64_t ReqlStopQuery        ( ReqlConnection * conn, uint64_t queryToken, void * options );
CTRANSPORT_API CTRANSPORT_INLINE uint64_t ReqlStopQueryCtx     ( ReqlQueryContext* ctx, ReqlConnection * conn, uint64_t queryToken, void * options );


CTRANSPORT_API CTRANSPORT_INLINE int   CTReQLHandshakeProcessMagicNumberResponse (char* mnBuffer, size_t mnBufferLen);
CTRANSPORT_API CTRANSPORT_INLINE void* CTReQLHandshakeProcessFirstMessageResponse(char* sFirstMessagePtr, size_t sFirstMessageLen, char* password);
CTRANSPORT_API CTRANSPORT_INLINE int CTReQLHandshakeProcessFinalMessageResponse(char* sFinalMessagePtr, size_t sFinalMessageLen, char* base64SS);

#endif
