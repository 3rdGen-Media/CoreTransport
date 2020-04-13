//  ReqlSSL.h (SSL)

#ifndef ReqlSSL_h
#define ReqlSSL_h

#include "ReqlSocket.h"
#include "ReqlError.h"
#include "ReqlSSLKey.h"  //native cryptographic functions needed to associate key with ssl certificate

//#include "dns/dns.h"

#ifdef REQL_USE_MBED_TLS	//MBED_TLS API
//mbded tls
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
typedef int ReqlSSLStatus;
typedef mbedtls_x509_crt ReqlSecCertificate;
typedef ReqlSecCertificate * ReqlSecCertificateRef;
//typedef mbedtls_ssl_context ReqlSSLContext;

//#pragma pack(push,1)
typedef struct ReqlSSLContext
{
	mbedtls_ssl_context			ctx;
	mbedtls_ssl_config 			conf;
	mbedtls_entropy_context		entropy;
	mbedtls_ctr_drbg_context	ctr_drbg;
}ReqlSSLContext;
//#pragma pack(pop)

typedef ReqlSSLContext * ReqlSSLContextRef;


#elif defined(__APPLE___)  //Darwin SecureTransport API
#import <Security/SecureTransport.h>
typedef SecCertificateRef ReqlSecCertificateRef;
#elif defined(_WIN32)	  //Win32 SCHANNEL API
#include <windows.h>	  //Must include windows.h before sspi.h
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

typedef SECURITY_STATUS ReqlSSLStatus;

typedef enum ReqlSSLError
{
	ReqlSSLInternalError = SEC_E_INTERNAL_ERROR,
	//ReqlSSLDecryptError		 = -40, //An invalid input domain produced the error
	ReqlSSLContextExpired = SEC_I_CONTEXT_EXPIRED, //An invalid input domain produced the error
	ReqlSSLRenegotiate = SEC_I_RENEGOTIATE, //A bsd socket call produced the error
	ReqlSSLIncompleteMessage = SEC_E_INCOMPLETE_MESSAGE,
	ReqlSSLSuccess = 0   //No Error
}ReqlSSLError;

//4 or 8 bytes aligned
typedef struct ReqlSSLContext
{
	CredHandle					hCred;			//16 bytes
	CtxtHandle					hCtxt;			//16 bytes
	SecBuffer					ExtraData;		//16 bytes

	char * hBuffer;								//8 bytes
	char * tBuffer;								//8 bytes

	SecPkgContext_StreamSizes	Sizes;			//20 bytes

	char padding[4];
	//PCCERT_CONTEXT pClientServeContext;// = NULL;
}ReqlSSLContext;
typedef PCredHandle ReqlSecCertificateRef;
typedef ReqlSSLContext * ReqlSSLContextRef;

#endif


#if defined(__cplusplus) //|| defined(__OBJC__)
extern "C" {
#endif
/*
#ifndef __SECHANDLE_DEFINED__
#define SEC_FAR
	typedef struct _SecHandle
	{
		ULONG_PTR dwLower;
		ULONG_PTR dwUpper;
	} SecHandle, *PSecHandle;

#define __SECHANDLE_DEFINED__
	typedef SecHandle CredHandle;
	typedef PSecHandle PCredHandle;

	typedef SecHandle CtxtHandle;
	typedef PSecHandle PCtxtHandle;


	typedef struct _SecBuffer {
		unsigned long cbBuffer;             // Size of the buffer, in bytes
		unsigned long BufferType;           // Type of the buffer (below)
#ifdef MIDL_PASS
		[size_is(cbBuffer)] char * pvBuffer;                         // Pointer to the buffer
#else
		_Field_size_bytes_(cbBuffer) void SEC_FAR * pvBuffer;            // Pointer to the buffer
#endif
	} SecBuffer, *PSecBuffer;

	typedef struct _SecBufferDesc {
		unsigned long ulVersion;            // Version number
		unsigned long cBuffers;             // Number of buffers
#ifdef MIDL_PASS
		[size_is(cBuffers)]
#endif
		_Field_size_(cBuffers) PSecBuffer pBuffers;                // Pointer to array of buffers
	} SecBufferDesc, SEC_FAR * PSecBufferDesc;


	typedef struct _SecPkgContext_StreamSizes
	{
		unsigned long   cbHeader;
		unsigned long   cbTrailer;
		unsigned long   cbMaximumMessage;
		unsigned long   cBuffers;
		unsigned long   cbBlockSize;
	} SecPkgContext_StreamSizes, *PSecPkgContext_StreamSizes;

	typedef SecPkgContext_StreamSizes SecPkgContext_DatagramSizes;
	typedef PSecPkgContext_StreamSizes PSecPkgContext_DatagramSizes;

#endif // __SECHANDLE_DEFINED__
*/




//REQL_API REQL_INLINE void DisplayCertChain( PCCERT_CONTEXT  pServerCert, BOOL fLocal );

/***
 *	ReqlSSLInit
 *
 *	Load Security and Socket Libraries on WIN32 platforms
 ***/
REQL_API REQL_INLINE void ReqlSSLInit(void);			//Load dll lib(s) associated with ssl
REQL_API REQL_INLINE void ReqlSSLCleanup(void);			//cleanup ddl lib(s) associated with ssl

/***
 *  ReqlSSLCertificate
 *
 *	Create a platform specific SSL Security Certificate/Stoe from a file or buffer containing a DER or PEM formatted cert, respectiviely
 ***/
REQL_API REQL_INLINE ReqlSecCertificateRef ReqlSSLCertificate(const char * caPath);

//REQL_API REQL_INLINE BOOL ReqlCredentialHandleCreate( ReqlSSLContextRef * sslContextRef );

/***
 *  ReqlSSLContext
 *
 *	Create a platform specific SSL Context with a platform provided socket
 ***/
REQL_API REQL_INLINE ReqlSSLContextRef ReqlSSLContextCreate(REQL_SOCKET* socketfd, ReqlSecCertificateRef * certRef, const char * serverName);

/***
 *	ReqlSSLContextDestroy
 *
 *	Destroy a platform specific SSL Context attached to a platform provided socket
 ***/
REQL_API REQL_INLINE void ReqlSSLContextDestroy(ReqlSSLContextRef sslContextRef);

/***
 *  ReqlSSLHandshake
 *
 *  Perform a custom SSL Handshake using the Apple Secure Transport API OR WIN32 CryptAPI SChannel
 *
 *  --Overrides PeerAuth step (errSSLPeerAuthCompleted) to validate trust against the input ca root certificate
 ***/
REQL_API REQL_INLINE int ReqlSSLHandshake(REQL_SOCKET socketfd, ReqlSSLContextRef sslContextRef, ReqlSecCertificateRef rootCertRef);


//REQL_API REQL_INLINE DWORD VerifyServerCertificate(PCCERT_CONTEXT pServerCert, char* pszServerName, DWORD dwCertFlags);

/***
 *  ReqlVerifyServerCertificate
 *
 *	Verify the platform specific Security Certificate Handle associated with a platform SSL Context
 ***/
REQL_API REQL_INLINE int ReqlVerifyServerCertificate(ReqlSSLContextRef sslContextRef, char * pszServerName);


/***
 *	ReqlSSLDecrypt
 ***/
REQL_API REQL_INLINE ReqlSSLStatus ReqlSSLDecryptMessage(ReqlSSLContextRef sslContextRef, void*msg, unsigned long *msgLength);
REQL_API REQL_INLINE ReqlSSLStatus ReqlSSLDecryptMessageInSitu(ReqlSSLContextRef sslContextRef, void**msg, unsigned long *msgLength);

/***
 *	ReqlSSLRead
/*****************************************************************************/
REQL_API REQL_INLINE size_t ReqlSSLRead( REQL_SOCKET socketfd, ReqlSSLContextRef sslContextRef, void * msg, unsigned long * msgLength );

/***
 *	ReqlSSLEncryptMessage
 ***/
REQL_API REQL_INLINE ReqlSSLStatus ReqlSSLEncryptMessage(ReqlSSLContextRef sslContextRef, void*msg, unsigned long * msgLength);

/***
 *	ReqlSSLWrite
/*****************************************************************************/
REQL_API REQL_INLINE int ReqlSSLWrite( REQL_SOCKET socketfd, ReqlSSLContextRef sslContextRef, void * msg, unsigned long * msgLength );// * phContext, PBYTE pbIoBuffer, SecPkgContext_StreamSizes Sizes )


#ifdef __APPLE__
typedef SSLContextRef ReqlSSLContextRef;
typedef SecCertificateRef ReqlSecCertificateRef;
#endif


#if defined(__cplusplus) //|| defined(__OBJC__)
}
#endif


#endif