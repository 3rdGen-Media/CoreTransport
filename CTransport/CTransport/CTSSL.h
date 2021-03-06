//  ReqlSSL.h (SSL)

#ifndef CTSSL_H
#define CTSSL_H

#include "CTSystem.h"
#include "CTSocket.h"
#include "CTError.h"
#include "CTSSLKey.h"  //native cryptographic functions needed to associate key with ssl certificate

//#include "dns/dns.h"

#ifdef CTRANSPORT_USE_MBED_TLS	//MBED_TLS API
//mbded tls
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
typedef int CTSSLStatus;
typedef mbedtls_x509_crt CTSecCertificate;
typedef CTSecCertificate * CTSecCertificateRef;
//typedef mbedtls_ssl_context ReqlSSLContext;

//#pragma pack(push,1)
typedef struct CTSSLContext
{
	mbedtls_ssl_context			ctx;
	mbedtls_ssl_config 			conf;
	mbedtls_entropy_context		entropy;
	mbedtls_ctr_drbg_context	ctr_drbg;
}CTSSLContext;
//#pragma pack(pop)

typedef CTSSLContext * CTSSLContextRef;


#elif defined(__APPLE___)  //Darwin SecureTransport API
#import <Security/SecureTransport.h>
typedef SecCertificateRef ReqlSecCertificateRef;
#elif defined(_WIN32)	  //Win32 SCHANNEL API
//#include <windows.h>	  //Must include windows.h before sspi.h
#define SECURITY_WIN32
#define IO_BUFFER_SIZE  0x10000
#include <stdio.h>
#include <stdlib.h>
//#include <windows.h>
//#include <winsock2.h>
#include <wincrypt.h>
#include <wintrust.h>
#include <schnlsp.h>
#include <security.h>
#include <sspi.h>

typedef SECURITY_STATUS CTSSLStatus;

typedef enum CTSSLError
{
	CTSSLInternalError = SEC_E_INTERNAL_ERROR,
	//ReqlSSLDecryptError		 = -40, //An invalid input domain produced the error
	CTSSLContextExpired = SEC_I_CONTEXT_EXPIRED, //An invalid input domain produced the error
	CTSSLRenegotiate = SEC_I_RENEGOTIATE, //A bsd socket call produced the error
	CTSSLIncompleteMessage = SEC_E_INCOMPLETE_MESSAGE,
	CTSSLSuccess = 0   //No Error
}CTSSLError;

//4 or 8 bytes aligned
typedef struct CTSSLContext
{
	CredHandle					hCred;			//16 bytes
	CtxtHandle					hCtxt;			//16 bytes
	SecBuffer					ExtraData;		//16 bytes

	char * hBuffer;								//8 bytes
	char * tBuffer;								//8 bytes

	SecPkgContext_StreamSizes	Sizes;			//20 bytes

	char padding[4];
	//PCCERT_CONTEXT pClientServeContext;// = NULL;
}CTSSLContext;
typedef PCredHandle CTSecCertificateRef;
typedef CTSSLContext * CTSSLContextRef;

#endif


#if defined(__cplusplus) //|| defined(__OBJC__)
extern "C" {
#endif

	

//REQL_API REQL_INLINE void DisplayCertChain( PCCERT_CONTEXT  pServerCert, BOOL fLocal );

/***
 *	ReqlSSLInit
 *
 *	Load Security and Socket Libraries on WIN32 platforms
 ***/
CTRANSPORT_API CTRANSPORT_INLINE  void CTSSLInit(void);			//Load dll lib(s) associated with ssl
CTRANSPORT_API CTRANSPORT_INLINE  void CTSSLCleanup(void);			//cleanup ddl lib(s) associated with ssl

/***
 *  ReqlSSLCertificate
 *
 *	Create a platform specific SSL Security Certificate/Stoe from a file or buffer containing a DER or PEM formatted cert, respectiviely
 ***/
CTRANSPORT_API CTRANSPORT_INLINE  CTSecCertificateRef CTSSLCertificate(const char * caPath);

//REQL_API REQL_INLINE BOOL ReqlCredentialHandleCreate( ReqlSSLContextRef * sslContextRef );

/***
 *  ReqlSSLContext
 *
 *	Create a platform specific SSL Context with a platform provided socket
 ***/
CTRANSPORT_API CTRANSPORT_INLINE  CTSSLContextRef CTSSLContextCreate(CTSocket* socketfd, CTSecCertificateRef * certRef, const char * serverName);

/***
 *	ReqlSSLContextDestroy
 *
 *	Destroy a platform specific SSL Context attached to a platform provided socket
 ***/
CTRANSPORT_API CTRANSPORT_INLINE  void CTSSLContextDestroy(CTSSLContextRef sslContextRef);

/***
 *  CTSSLHandshake
 *
 *  Perform a custom SSL Handshake using the Apple Secure Transport API OR WIN32 CryptAPI SChannel
 *
 *  --Overrides PeerAuth step (errSSLPeerAuthCompleted) to validate trust against the input ca root certificate
 ***/
CTRANSPORT_API CTRANSPORT_INLINE int CTSSLHandshake(CTSocket socketfd, CTSSLContextRef sslContextRef, CTSecCertificateRef rootCertRef, const char * serverName);


//CTRANSPORT_API REQL_INLINE DWORD VerifyServerCertificate(PCCERT_CONTEXT pServerCert, char* pszServerName, DWORD dwCertFlags);

/***
 *  ReqlVerifyServerCertificate
 *
 *	Verify the platform specific Security Certificate Handle associated with a platform SSL Context
 ***/
CTRANSPORT_API CTRANSPORT_INLINE  int CTVerifyServerCertificate(CTSSLContextRef sslContextRef, char * pszServerName);


/***
 *	ReqlSSLDecrypt
 ***/
CTRANSPORT_API CTRANSPORT_INLINE CTSSLStatus CTSSLDecryptMessage(CTSSLContextRef sslContextRef, void*msg, unsigned long *msgLength);
CTRANSPORT_API CTRANSPORT_INLINE CTSSLStatus CTSSLDecryptMessage2(CTSSLContextRef sslContextRef, void*msg, unsigned long *msgLength, char**extraBuffer, unsigned long * bytesRemaining);

CTRANSPORT_API CTRANSPORT_INLINE CTSSLStatus CTSSLDecryptMessageInSitu(CTSSLContextRef sslContextRef, void**msg, unsigned long *msgLength);

/***
 *	ReqlSSLRead
/*****************************************************************************/
CTRANSPORT_API CTRANSPORT_INLINE CTSSLStatus CTSSLRead( CTSocket socketfd, CTSSLContextRef sslContextRef, void * msg, unsigned long * msgLength );

/***
 *	ReqlSSLEncryptMessage
 ***/
CTRANSPORT_API CTRANSPORT_INLINE CTSSLStatus CTSSLEncryptMessage(CTSSLContextRef sslContextRef, void*msg, unsigned long * msgLength);

/***
 *	ReqlSSLWrite
/*****************************************************************************/
CTRANSPORT_API CTRANSPORT_INLINE int CTSSLWrite( CTSocket socketfd, CTSSLContextRef sslContextRef, void * msg, unsigned long * msgLength );// * phContext, PBYTE pbIoBuffer, SecPkgContext_StreamSizes Sizes )



#ifdef __APPLE__
typedef SSLContextRef CTSSLContextRef;
typedef SecCertificateRef CTSecCertificateRef;
#endif


#if defined(__cplusplus) //|| defined(__OBJC__)
}
#endif


#endif