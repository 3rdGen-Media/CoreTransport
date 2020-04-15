//
//  ctHypertextAPI.h
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#ifndef CTSOCKETAPI_H
#define CTSOCKETAPI_H

#include "ct_scram.h"

//uint64_t includes
#include <stdint.h>

//Proto Includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//SSL Includes
//#include "ReqlSSL.h"

//OpenSSL Includes
#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifndef _WIN32
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
#ifndef _WIN32
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

#include "CTError.h"
#include "CTCoroutine.h"
#include "CTConnection.h"
#include "CTReQL.h"

//#include "tls_api.h"

//use protobuf-c's extern C helper macro because it is there
//PROTOBUF_C__BEGIN_DECLS

#if defined(__cplusplus) //|| defined(__OBJC__)
extern "C" {
#endif


#pragma mark -- CTConnect API Methods
//Connect to a RethinkDB Service + Init/Alloc ReØMQL [ReqlConnection] object
CTRANSPORT_API CTRANSPORT_INLINE int CTConnect(  CTTarget * service, CTConnectionClosure callback);
CTRANSPORT_API CTRANSPORT_INLINE int CTReQLSASLHandshake( CTConnection * r, CTTarget * service);
CTRANSPORT_API CTRANSPORT_INLINE int CTCloseConnection( CTConnection * conn );
CTRANSPORT_API CTRANSPORT_INLINE int CTCloseSSLSocket(CTSSLContextRef sslContextRef, CTSocket socketfd);

CTRANSPORT_API CTRANSPORT_INLINE unsigned long CTPageSize();

//Blocking Send/Receive network buffer over CTConnection dedicated platform TCP socket
CTRANSPORT_API CTRANSPORT_INLINE int	 CTSend(CTConnection * conn, void * msg, unsigned long msgLength );
CTRANSPORT_API CTRANSPORT_INLINE void* CTRecv(     CTConnection * conn, void * msg, unsigned long * msgLength );

//Async Send/Receive network buffer over CTConnection dedicated platform TCP socket (and tag with a queryToken)
//CTRANSPORT_API CTRANSPORT_INLINE CTClientError CTSendWithQueue(CTConnection* conn, void * msg, unsigned long * msgLength);
CTRANSPORT_API CTRANSPORT_INLINE uint64_t CTSendOnQueue(CTConnection * conn, char ** queryBufPtr, unsigned long queryStrLength, uint64_t queryToken);
CTRANSPORT_API CTRANSPORT_INLINE CTClientError CTAsyncRecv(CTConnection* conn, void * msg, unsigned long * msgLength);

//Reql Send/Receive wrappers
CTRANSPORT_API CTRANSPORT_INLINE uint64_t CTReQLRunQueryOnQueue(CTConnection * conn, const char ** queryBufPtr, unsigned long queryStrLength, uint64_t queryToken);


#pragma mark -- Global ReqlClientDriver Object
typedef struct CTClientDriver
{
    //The Client Driver Object can create Reql Connections
    CTConnectFunc             connect;
    
    //The Client Driver Object can do top level queries
    //ReqlDatabaseQueryFunc       db;
}CTClientDriver;

static const CTClientDriver CTransport = {CTConnect};//, ReqlDatabaseQuery};



#if defined(__cplusplus) //|| defined(__OBJC__)
}
#endif
//PROTOBUF_C__END_DECLS

#endif /* ReØMQL_h */
