//  ReqlSSL.h (SSL)

#ifndef CTSSL_H
#define CTSSL_H

#include "CTSystem.h"
#include "CTSocket.h"
#include "CTError.h"
#include "CTSSLKey.h"  //native cryptographic functions needed to associate key with ssl certificate

//#include "dns/dns.h"



typedef enum CTSSLMethod
{
    CTSSL_NONE,
    CTSSL_TLS_1_2
    /*
    /// Generic SSL version 2.
    sslv2,

    /// SSL version 2 client.
    sslv2_client,

    /// SSL version 2 server.
    sslv2_server,

    /// Generic SSL version 3.
    sslv3,

    /// SSL version 3 client.
    sslv3_client,

    /// SSL version 3 server.
    sslv3_server,

    /// Generic TLS version 1.
    tlsv1,

    /// TLS version 1 client.
    tlsv1_client,

    /// TLS version 1 server.
    tlsv1_server,

    /// Generic SSL/TLS.
    sslv23,

    /// SSL/TLS client.
    sslv23_client,

    /// SSL/TLS server.
    sslv23_server,

    /// Generic TLS version 1.1.
    tlsv11,

    /// TLS version 1.1 client.
    tlsv11_client,

    /// TLS version 1.1 server.
    tlsv11_server,

    /// Generic TLS version 1.2.
    tlsv12,

    /// TLS version 1.2 client.
    tlsv12_client,

    /// TLS version 1.2 server.
    tlsv12_server,

    /// Generic TLS version 1.3.
    tlsv13,

    /// TLS version 1.3 client.
    tlsv13_client,

    /// TLS version 1.3 server.
    tlsv13_server,

    /// Generic TLS.
    tls,

    /// TLS client.
    tls_client,

    /// TLS server.
    tls_server
    */
}CTSSLMethod;



#ifdef CTRANSPORT_WOLFSSL
//WOLF SSL
#include <wolfssl/options.h>
#include <wolfssl/ssl.h>
typedef int CTSSLStatus;
//typedef char CTSecCertificate;
typedef char* CTSecCertificateRef;
typedef struct CTSSLContext
{
	WOLFSSL*						ctx;
	WOLFSSL_CTX*					conf;
	WOLFSSL_BIO*					bio;

	//This struct allows bookkeeping of ssl encrypt/decrypt context header and trailer buffer sizes
	//For wolfssl, we just set these to 0 so they can be used interchangeably with calls to SCHANNEL on the decrypt thread(s)
	struct
	{
		unsigned long   cbHeader;
		unsigned long   cbTrailer;
	}Sizes;

	//mbedtls_ssl_config 			conf;
	//mbedtls_entropy_context		entropy;
	//mbedtls_ctr_drbg_context	ctr_drbg;
}CTSSLContext;
typedef CTSSLContext* CTSSLContextRef;

//TO DO:  work out alignment and padding of this struct
typedef struct CTSSLDecryptTransient
{
	int			  socketfd;
	unsigned long bytesToDecrypt;
	unsigned long totalBytesProcessed;
	char* buffer;
}CTSSLDecryptTransient;

typedef struct CTSSLEncryptTransient
{
	int			  wolf_socket_fd;
	//CTSocket	  ct_socket_fd;
	int			  messageLen;
	void		  *messageBuffer;
}CTSSLEncryptTransient;

#elif defined(CTRANSPORT_USE_MBED_TLS)	//MBED_TLS API
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
//CTRANSPORT_API CTRANSPORT_INLINE  CTSSLContextRef CTSSLContextCreate(CTSocket* socketfd, CTSecCertificateRef * certRef, const char * serverName, CTThreadQueue txQueue);
CTRANSPORT_API CTRANSPORT_INLINE  CTSSLContextRef CTSSLContextCreate(CTSocket* socketfd, CTSecCertificateRef* certRef, const char* serverName, void** firstMessage, int* firstMessageLen);

/***
 *	ReqlSSLContextDestroy
 *
 *	Destroy a platform specific SSL Context attached to a platform provided socket
 ***/
CTRANSPORT_API CTRANSPORT_INLINE  void CTSSLContextDestroy(CTSSLContextRef sslContextRef);


CTRANSPORT_API CTRANSPORT_INLINE void CTSSLHandshakeSendFirstMessage(CTSocket socketfd, CTSSLContextRef sslContextRef, void* firstMessageBuffer, int firstMessageLen);

//CTRANSPORT_API int CTSSLHandshakeProcessFirstResponse(CTSocket socketfd, CTSSLContextRef sslContextRef, char* responseBuffer);
//CTRANSPORT_API int CTSSLHandshakeRecvSecondResponse(CTSocket socketfd, CTSSLContextRef sslContextRef, CTSecCertificateRef rootCertRef, const char* serverName);

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