#ifndef CTERROR_H
#define CTERROR_H

#if defined(__cplusplus) //|| defined(__OBJC__)
extern "C" {
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


#include <WinSock2.h>
#endif


#ifdef CTRANSPORT_USE_MBED_TLS	//MBED_TLS API
//mbded tls
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#endif

//#pragma mark -- ReqlError Enums

typedef enum CTErrorClass
{
    CTCompileErrorClass,
    CTDriverErrorClass,
    CTRuntimeErrorClass
}CTErrorClass;

/* * *
 *  ReqlDriverError Enums
 *
 *  We custom define ReqlError(s) because it is not present in the protobuf definitions (or I cannot figure out how to acces it)
 *  We will use negative values for our custom error codes that do not exist in the official spec
 * * */

#ifndef _WIN32
#define WSA_IO_PENDING -140
#endif

typedef enum CTClientError
{
#ifdef CTRANSPORT_USE_MBED_TLS
	CTSocketWantRead				= MBEDTLS_ERR_SSL_WANT_READ,
	CTSocketWantWrite				= MBEDTLS_ERR_SSL_WANT_WRITE,
#endif

#if defined(_WIN32)
	CTSocketWouldBlock				= WSAEWOULDBLOCK,
	CTSocketDisconnect				= WSAEDISCON,
#else
	CTSocketWouldBlock				= EWOULDBLOCK,
	CTSocketDisconnet				= EDISCON,
#endif
	CTSocketIOPending				 = WSA_IO_PENDING,	 //ReqlAsyncSend is sending in an async state
    CTRunLoopError					 = -130,
    CTSysCallError					 = -120,
    CTSocketDNSError                 = -110, //Unable to resolve the ReqlService host address via DNS
    CTSCRAMEncryptionError			 = -100,
    CTSASLHandshakeError			 = -90, //An SSL/SCRAM handshake call produced the error
    CTSSLHandshakeError				 = -80, //An SSL handkshake call produced the error
    CTSSLKeychainError				 = -70, //A device keychain call produced the error
    CTSSLCertificateError            = -60, //An SSL Certificate call produced the error
    CTSecureTransportError           = -50, //An SSL/TLS layer call produced the error
    CTSocketEventError               = -40, //An unexpected kqueue/kevent behavior produced the error
    CTSocketConnectError             = -30, //A bsd socket connection call produced the error
    CTSocketDomainError              = -20, //An invalid input domain produced the error
    CTSocketError					 = -10, //A bsd socket call produced the error
    CTSuccess						 = 0,   //No Error
    CTAuthError					   	 = 10,  //Standard ReqlDriverError:ReqlAuthError
}CTClientError;

typedef enum CTFileError
{

	CTFileMapViewError		= -30,
	CTFileMapError			= -20,
	CTFileOpenError			= -10,
	CTFileSuccess			= 0
}CTFileError;

#pragma mark -- CTError Struct

/* * *
 *  ReqlError Object
 *
 * * */
typedef struct CTRANSPORT_PACK_ATTRIBUTE CTError
{
    CTErrorClass errClass;
    int32_t id;
    void *   description;
}CTError;

#ifndef OSStatus
typedef signed int OSStatus;
//static const OSStatus noErr = 0;
#endif


#if defined(__cplusplus) //|| defined(__OBJC__)
}
#endif



#endif