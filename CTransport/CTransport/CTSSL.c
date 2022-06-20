//#include "stdafx.h"
#include "../CTransport.h"

#include "CTSSL.h"
//#include "ReqlFile.h"

#include <stdio.h>
#include <assert.h>

#ifdef CTRANSPORT_WOLFSSL
	static int g_SSLInitialized = 0;
#elif defined(CTRANSPORT_USE_MBED_TLS)

#include <stdio.h>
#include <stdarg.h>
//#include <args.h>
static void Reql_fprintf(stderr, char *format, ...)
{
	char REQL_PRINT_BUFFER[4096];

/*
	va_list args;
	va_start(args, format);
#if defined(_WIN32)//WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
	vsprintf_s(REQL_PRINT_BUFFER, 1, format, args);
#else
	vprintf( stderr, format, args);
#endif
	va_end(args);

#if defined(_WIN32)//WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
	OutputDebugStringA(REQL_PRINT_BUFFER);
#endif
*/
	
	int nBuf;
	va_list args;
	va_start(args, format);
	
	vprintf( stderr, format, args);
	//TCHAR szBuffer[512]; // get rid of this hard-coded buffer
	//nBuf = _vsnfprintf(stderr, REQL_PRINT_BUFFER, 4095, format, args);
	//OutputDebugStringA(REQL_PRINT_BUFFER);
	va_end(args);
	

}


static void ReqlMBEDTLSDebug(void *ctx, int level, const char *file, int line, const char *str)
{
	((void)level);
	Reql_fprintf(stderr, "%s:%04d: %s", file, line, str);
}

#elif defined(_WIN32)

#define DLL_NAME TEXT("secur32.dll")
#define NT4_DLL_NAME TEXT("Security.dll")
#define SEC_SUCCESS(Status) ((Status) >= 0)

//Load libs
//#pragma comment(lib, "ws2_32.lib")
//#pragma comment(lib, "WSock32.lib")
//#pragma comment(lib, "crypt32.lib")
//#pragma comment(lib, "user32.lib")
//#pragma comment(lib, "secur32.lib")
//#pragma comment(lib, "MSVCRTD.lib")

//Globals

// SCHANNEL related Globals.
BOOL    fVerbose        = FALSE; // FALSE; // TRUE;

//Server and Port Definitions
//INT     iPortNumber     = 465; // gmail TLS
//LPSTR   pszServerName   = "smtp.gmail.com"; // DNS name of server
//LPSTR   pszUser         = 0; // if specified, a certificate in "MY" store is searched for

//Protocol definitions
//DWORD   dwProtocol      = SP_PROT_TLS1_2_CLIENT; // SP_PROT_TLS1; // SP_PROT_PCT1; SP_PROT_SSL2; SP_PROT_SSL3; 0=default
//ALG_ID  aiKeyExch       = CALG_RSA_KEYX; // = default; CALG_DH_EPHEM; CALG_RSA_KEYX;

//RethinkDB Client Driver:  

//EECDH+AESGCM:EDH+AESGCM:AES256+EECDH:AES256+EDH:AES256-SHA

//SCHANNEL Key Exchange, Signature, Cipher, HASHING ALgorithm
//TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 

#ifndef CALG_ECDH_EPHEM
#define CALG_ECDH_EPHEM 0x0000ae06
#endif

//static const DWORD            cSupportedAlgs = 3;
ALG_ID			 rgbSupportedAlgs[3] = { CALG_ECDH_EPHEM, CALG_RSA_SIGN, CALG_SHA_384};//, 0x2200, 0x2203};
//ALG_ID			 rgbSupportedAlgs[15] = {0x660e, 0x6610, 0x6801, 0x6603, 0x6601, 0x8003, 0x8004, 0x800c, 0x800d, 0x800e, 0x2400, 0xaa02, 0xae06, 0x2200, 0x2203};
ALG_ID           allAlgs[47] = { CALG_MD2, CALG_MD4, CALG_MD5, CALG_SHA, CALG_SHA1, CALG_MAC, CALG_RSA_SIGN, CALG_DSS_SIGN, CALG_NO_SIGN, CALG_RSA_KEYX, CALG_DES, CALG_3DES_112, CALG_3DES, CALG_DESX, CALG_RC2, CALG_RC4, CALG_SEAL, CALG_DH_SF, CALG_DH_EPHEM, CALG_AGREEDKEY_ANY, CALG_KEA_KEYX, CALG_HUGHES_MD5, \
CALG_SKIPJACK, CALG_TEK, CALG_CYLINK_MEK, CALG_SSL3_SHAMD5, CALG_SSL3_MASTER, CALG_SCHANNEL_MASTER_HASH, CALG_SCHANNEL_MAC_KEY, CALG_SCHANNEL_ENC_KEY, CALG_PCT1_MASTER, CALG_SSL2_MASTER, CALG_TLS1_MASTER, CALG_RC5, CALG_HMAC, CALG_TLS1PRF, CALG_HASH_REPLACE_OWF, CALG_AES_128, CALG_AES_192, CALG_AES_256,\
CALG_AES, CALG_SHA_256, CALG_SHA_384, CALG_SHA_512, CALG_ECDH, /*CALG_ECDH_EPHEM,*/ CALG_ECMQV, CALG_ECDSA};//, CALG_NULLCIPHER };

//Proxy Definitions?
//BOOL    fUseProxy       = FALSE;
//LPSTR   pszProxyServer  = "proxy";
//INT     iProxyPort      = 80;

//Certificate Keychain Definitions
HCERTSTORE hMyCertStore = NULL;

//SCHANNEL_CRED SchannelCred;

//SSPI Security Module and corresponding funciton table
HMODULE	   g_hCrypto		= NULL;
HMODULE	   g_hSecurity     = NULL;
PSecurityFunctionTable g_pSSPI = NULL;

DWORD dwTlsSecurityLibrary; 
DWORD dwTlsSecurityInterface; 

//WIN32 SSPI LIBRARY

/*****************************************************************************/
PSecurityFunctionTable LoadSecurityLibrary( void ) // load SSPI.DLL, set up a special table - PSecurityFunctionTable
{
    //INIT_SECURITY_INTERFACE pInitSecurityInterface;
//  QUERY_CREDENTIALS_ATTRIBUTES_FN pQueryCredentialsAttributes;
    //OSVERSIONINFO VerInfo;
    //UCHAR lpszDLL[MAX_PATH];
	
	//PSecurityFunctionTable pSSPI = NULL;
	//HMODULE hSecurity = NULL;

	/*
	if( !g_pSSPI )
	{
		TlsFree(dwTlsSecurityInterface);
   
		// Allocate a TLS index. 
	   if ((dwTlsSecurityInterface = TlsAlloc()) == TLS_OUT_OF_INDEXES) 
			fprintf(stderr, "\nTlsAlloc failed\n"); 
	}
	*/
    //  Find out which security DLL to use, depending on
    //  whether we are on Win2K, NT or Win9x

	/*
    VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if ( !GetVersionEx (&VerInfo) ) return FALSE;

    if ( VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT  &&  VerInfo.dwMajorVersion == 4 )
    {
        strcpy ((char*)lpszDLL, NT4_DLL_NAME ); // NT4_DLL_NAME TEXT("Security.dll")
    }
    else if ( VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ||
              VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
    {
        strcpy((char*)lpszDLL, DLL_NAME); // DLL_NAME TEXT("Secur32.dll")
    }
    else
        { fprintf(stderr,  "System not recognized\n" ); return FALSE; }


    //  Load Global handle to Security DLL (if not already loaded)
	if( !g_hSecurity )
	{
	    g_hSecurity = LoadLibrary((char*)lpszDLL);
		if(g_hSecurity== NULL) { fprintf(stderr,  "Error 0x%x loading %s.\n", GetLastError(), lpszDLL ); return FALSE; }
	}

	//Load InitSecurityInterfaceA function from DLL
    pInitSecurityInterface = (INIT_SECURITY_INTERFACE)GetProcAddress( g_hSecurity, TEXT("InitSecurityInterfaceA") );
    if(pInitSecurityInterface == NULL) { fprintf(stderr,  "Error 0x%x reading InitSecurityInterface entry point.\n", GetLastError() ); return FALSE; }

	//Call InitSecurityInterfaceA to get a pointer to the security function table
	g_pSSPI = pInitSecurityInterface(); // call InitSecurityInterfaceA(void);
    if(g_pSSPI == NULL) { fprintf(stderr, "Error 0x%x reading security interface.\n", GetLastError()); return FALSE; }


	
    //  Load Global handle to Security DLL (if not already loaded)
	if( !g_hCrypto )
	{
	    g_hCrypto = LoadLibrary(TEXT("crypt32.dll"));
		if(g_hCrypto== NULL) { fprintf(stderr,  "Error 0x%x loading %s.\n", GetLastError(), TEXT("crypt32.dll")); return FALSE; }
	}
	*/
	InitSecurityInterfaceA();

	// Initialize the TLS index for this thread. 
	//lpvData = (LPVOID) LocalAlloc(LPTR, 256); 
	//if (! TlsSetValue(dwTlsSecurityLibrary, hSecurity))
    //  fprintf(stderr, "\nTlsSetValue dwTlsSecurityLibrary error"); 
 	//if (! TlsSetValue(dwTlsSecurityInterface, pSSPI))
    //  fprintf(stderr, "\nTlsSetValue dwTlsSecurityInterface error"); 

	//the first thread that loads will store as the global security interface
	//if( !g_pSSPI )
	//{
		//g_hSecurity = hSecurity;
		//g_pSSPI = pSSPI;
	//}

    return g_pSSPI; // and PSecurityFunctionTable
}


/*****************************************************************************/
void UnloadSecurityLibrary(void)
{
   //TlsFree(dwTlsSecurityInterface);
   FreeLibrary(g_hSecurity);
   g_hSecurity = NULL;
}
//Security Credentials and Keychain

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)f


////#pragma mark -- SCHANNEL Utility Functions


/*****************************************************************************/
static void DisplayWinVerifyTrustError(DWORD Status)
{
	LPSTR pszName = NULL;

	switch (Status)
	{
	case CERT_E_EXPIRED:                pszName = "CERT_E_EXPIRED";                 break;
	case CERT_E_VALIDITYPERIODNESTING:  pszName = "CERT_E_VALIDITYPERIODNESTING";   break;
	case CERT_E_ROLE:                   pszName = "CERT_E_ROLE";                    break;
	case CERT_E_PATHLENCONST:           pszName = "CERT_E_PATHLENCONST";            break;
	case CERT_E_CRITICAL:               pszName = "CERT_E_CRITICAL";                break;
	case CERT_E_PURPOSE:                pszName = "CERT_E_PURPOSE";                 break;
	case CERT_E_ISSUERCHAINING:         pszName = "CERT_E_ISSUERCHAINING";          break;
	case CERT_E_MALFORMED:              pszName = "CERT_E_MALFORMED";               break;
	case CERT_E_UNTRUSTEDROOT:          pszName = "CERT_E_UNTRUSTEDROOT";           break;
	case CERT_E_CHAINING:               pszName = "CERT_E_CHAINING";                break;
	case TRUST_E_FAIL:                  pszName = "TRUST_E_FAIL";                   break;
	case CERT_E_REVOKED:                pszName = "CERT_E_REVOKED";                 break;
	case CERT_E_UNTRUSTEDTESTROOT:      pszName = "CERT_E_UNTRUSTEDTESTROOT";       break;
	case CERT_E_REVOCATION_FAILURE:     pszName = "CERT_E_REVOCATION_FAILURE";      break;
	case CERT_E_CN_NO_MATCH:            pszName = "CERT_E_CN_NO_MATCH";             break;
	case CERT_E_WRONG_USAGE:            pszName = "CERT_E_WRONG_USAGE";             break;
	default:                            pszName = "(unknown)";                      break;
	}
	fprintf(stderr, "Error 0x%x (%s) returned by CertVerifyCertificateChainPolicy!\n", Status, pszName);
}


/*****************************************************************************/
static void DisplayWinSockError(DWORD ErrCode)
{
	LPSTR pszName = NULL; // http://www.sockets.com/err_lst1.htm#WSANO_DATA

	switch (ErrCode) // http://msdn.microsoft.com/en-us/library/ms740668(VS.85).aspx
	{
	case     10035:  pszName = "WSAEWOULDBLOCK    "; break;
	case     10036:  pszName = "WSAEINPROGRESS    "; break;
	case     10037:  pszName = "WSAEALREADY       "; break;
	case     10038:  pszName = "WSAENOTSOCK       "; break;
	case     10039:  pszName = "WSAEDESTADDRREQ   "; break;
	case     10040:  pszName = "WSAEMSGSIZE       "; break;
	case     10041:  pszName = "WSAEPROTOTYPE     "; break;
	case     10042:  pszName = "WSAENOPROTOOPT    "; break;
	case  10043:  pszName = "WSAEPROTONOSUPPORT"; break;
	case  10044:  pszName = "WSAESOCKTNOSUPPORT"; break;
	case     10045:  pszName = "WSAEOPNOTSUPP     "; break;
	case     10046:  pszName = "WSAEPFNOSUPPORT   "; break;
	case     10047:  pszName = "WSAEAFNOSUPPORT   "; break;
	case     10048:  pszName = "WSAEADDRINUSE     "; break;
	case     10049:  pszName = "WSAEADDRNOTAVAIL  "; break;
	case     10050:  pszName = "WSAENETDOWN       "; break;
	case     10051:  pszName = "WSAENETUNREACH    "; break;
	case     10052:  pszName = "WSAENETRESET      "; break;
	case     10053:  pszName = "WSAECONNABORTED   "; break;
	case     10054:  pszName = "WSAECONNRESET     "; break;
	case     10055:  pszName = "WSAENOBUFS        "; break;
	case     10056:  pszName = "WSAEISCONN        "; break;
	case     10057:  pszName = "WSAENOTCONN       "; break;
	case     10058:  pszName = "WSAESHUTDOWN      "; break;
	case     10059:  pszName = "WSAETOOMANYREFS   "; break;
	case     10060:  pszName = "WSAETIMEDOUT      "; break;
	case     10061:  pszName = "WSAECONNREFUSED   "; break;
	case     10062:  pszName = "WSAELOOP          "; break;
	case     10063:  pszName = "WSAENAMETOOLONG   "; break;
	case     10064:  pszName = "WSAEHOSTDOWN      "; break;
	case     10065:  pszName = "WSAEHOSTUNREACH   "; break;
	case     10066:  pszName = "WSAENOTEMPTY      "; break;
	case     10067:  pszName = "WSAEPROCLIM       "; break;
	case     10068:  pszName = "WSAEUSERS         "; break;
	case     10069:  pszName = "WSAEDQUOT         "; break;
	case     10070:  pszName = "WSAESTALE         "; break;
	case     10071:  pszName = "WSAEREMOTE        "; break;
	case     10091:  pszName = "WSASYSNOTREADY    "; break;
	case  10092:  pszName = "WSAVERNOTSUPPORTED"; break;
	case     10093:  pszName = "WSANOTINITIALISED "; break;
	case     11001:  pszName = "WSAHOST_NOT_FOUND "; break;
	case     11002:  pszName = "WSATRY_AGAIN      "; break;
	case     11003:  pszName = "WSANO_RECOVERY    "; break;
	case     11004:  pszName = "WSANO_DATA        "; break;
	}
	fprintf(stderr, "Error 0x%x (%s)\n", ErrCode, pszName);
}

/*****************************************************************************/
static void DisplaySECError(DWORD ErrCode)
{
	LPSTR pszName = NULL; // WinError.h

	switch (ErrCode)
	{
	case     SEC_E_BUFFER_TOO_SMALL:
		pszName = "SEC_E_BUFFER_TOO_SMALL - The message buffer is too small. Used with the Digest SSP.";
		break;

	case     SEC_E_CRYPTO_SYSTEM_INVALID:
		pszName = "SEC_E_CRYPTO_SYSTEM_INVALID - The cipher chosen for the security context is not supported. Used with the Digest SSP.";
		break;
	case     SEC_E_INCOMPLETE_MESSAGE:
		pszName = "SEC_E_INCOMPLETE_MESSAGE - The data in the input buffer is incomplete. The application needs to read more data from the server and call DecryptMessage (General) again.";
		break;

	case     SEC_E_INVALID_HANDLE:
		pszName = "SEC_E_INVALID_HANDLE - A context handle that is not valid was specified in the phContext parameter. Used with the Digest and Schannel SSPs.";
		break;

	case     SEC_E_INVALID_TOKEN:
		pszName = "SEC_E_INVALID_TOKEN - The buffers are of the wrong type or no buffer of type SECBUFFER_DATA was found. Used with the Schannel SSP.";
		break;

	case     SEC_E_MESSAGE_ALTERED:
		pszName = "SEC_E_MESSAGE_ALTERED - The message has been altered. Used with the Digest and Schannel SSPs.";
		break;

	case     SEC_E_OUT_OF_SEQUENCE:
		pszName = "SEC_E_OUT_OF_SEQUENCE - The message was not received in the correct sequence.";
		break;

	case     SEC_E_QOP_NOT_SUPPORTED:
		pszName = "SEC_E_QOP_NOT_SUPPORTED - Neither confidentiality nor integrity are supported by the security context. Used with the Digest SSP.";
		break;

	case     SEC_I_CONTEXT_EXPIRED:
		pszName = "SEC_I_CONTEXT_EXPIRED - The message sender has finished using the connection and has initiated a shutdown.";
		break;

	case     SEC_I_RENEGOTIATE:
		pszName = "SEC_I_RENEGOTIATE - The remote party requires a new handshake sequence or the application has just initiated a shutdown.";
		break;

	case     SEC_E_ENCRYPT_FAILURE:
		pszName = "SEC_E_ENCRYPT_FAILURE - The specified data could not be encrypted.";
		break;

	case     SEC_E_DECRYPT_FAILURE:
		pszName = "SEC_E_DECRYPT_FAILURE - The specified data could not be decrypted.";
		break;

	}
	fprintf(stderr, "Error 0x%x %s \n", ErrCode, pszName);
}


/*****************************************************************************/
void DisplayCertChain(PCCERT_CONTEXT pServerCert, BOOL fLocal)
{

	typedef DWORD(WINAPI* fCertNameToStrA)(DWORD, PCERT_NAME_BLOB, DWORD, LPSTR, DWORD);

	CHAR szName[512];
	DWORD csz = 100;
	PCCERT_CONTEXT pCurrentCert = NULL;
	PCCERT_CONTEXT pIssuerCert = NULL;
	DWORD dwVerificationFlags;



	//fCertNameToStrA pCertNameToStrA = NULL;
	//pCertNameToStrA = (fCertNameToStrA)GetProcAddress( g_hCrypto, TEXT("CertNameToStrA") );
	//if(pCertNameToStrA == NULL) { fprintf(stderr,  "Error 0x%x reading pCertNameToStrA entry point.\n", GetLastError() ); return FALSE; }

	//memset(szName, 0,csz);

	fprintf(stderr, "\n");

	//-------------------------------------------------------------------
	// Get and display the name of subject of the certificate.

	if (CertGetNameString(
		pServerCert,
		CERT_NAME_FRIENDLY_DISPLAY_TYPE,
		0,
		NULL,
		szName,
		128))
	{
		fprintf(stderr, "\nCertificate for %s found.\n", szName);
	}
	else
	{
		fprintf(stderr, "\nCertGetName failed.\n");
	}

	if (CertGetNameString(
		pServerCert,
		CERT_NAME_FRIENDLY_DISPLAY_TYPE | CERT_NAME_ISSUER_FLAG,
		0,
		NULL,
		szName,
		128))
	{
		fprintf(stderr, "\nCertificate for %s found.\n", szName);
	}
	else
	{
		fprintf(stderr, "\nCertGetName failed.\n");
	}


	/*
	// display leaf name
	if( !CertNameToStr( X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
												&(pServerCert->pCertInfo->Subject),
												CERT_X500_NAME_STR,
												szName, csz ) )
	{ fprintf(stderr, "**** Error 0x%x building subject name\n", GetLastError()); }

	if(fLocal) fprintf(stderr, "Client subject: %s\n", szName);
	else fprintf(stderr, "Server subject: %s\n", szName);



	if( !CertNameToStr( (pServerCert)->dwCertEncodingType,
												&(pServerCert)->pCertInfo->Issuer,
												CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
												(LPSTR)szName, sizeof(szName) ) )
	{ fprintf(stderr, "**** Error 0x%x building issuer name\n", GetLastError()); }

	if(fLocal) fprintf(stderr, "Client issuer: %s\n", szName);
	else fprintf(stderr, "Server issuer: %s\n\n", szName);
	*/

	// display certificate chain
	pCurrentCert = pServerCert;
	while (pCurrentCert != NULL)
	{
		fprintf(stderr, "Displaying cert chain...\n");
		dwVerificationFlags = 0;
		pIssuerCert = CertGetIssuerCertificateFromStore((pServerCert)->hCertStore, pCurrentCert, NULL, &dwVerificationFlags);
		if (pIssuerCert == NULL)
		{
			fprintf(stderr, "CertGetIssuerCertificateFromStore failed\n");
			if (pCurrentCert != pServerCert) CertFreeCertificateContext(pCurrentCert);
			break;
		}

		if (!CertNameToStrA(pIssuerCert->dwCertEncodingType,
			&(pIssuerCert->pCertInfo->Subject),
			CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
			szName, sizeof(szName)))
		{
			fprintf(stderr, "**** Error 0x%x building subject name\n", GetLastError());
		}

		fprintf(stderr, "CA subject: %s\n", szName);

		if (!CertNameToStrA(pIssuerCert->dwCertEncodingType,
			&(pIssuerCert->pCertInfo->Issuer),
			CERT_X500_NAME_STR | CERT_NAME_STR_NO_PLUS_FLAG,
			szName, sizeof(szName)))
		{
			fprintf(stderr, "**** Error 0x%x building issuer name\n", GetLastError());
		}

		fprintf(stderr, "CA issuer: %s\n\n", szName);

		if (pCurrentCert != pServerCert) CertFreeCertificateContext(pCurrentCert);
		pCurrentCert = pIssuerCert;
		pIssuerCert = NULL;
	}
}

/*****************************************************************************/
static void DisplayConnectionInfo(CtxtHandle *phContext)
{

	SECURITY_STATUS Status;
	SecPkgContext_ConnectionInfo ConnectionInfo;
	/*
						PSecurityFunctionTable pSSPI = NULL;
	// Retrieve a data pointer for the current thread's security function table
	pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
	if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
	  fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/
	Status = QueryContextAttributes(phContext, SECPKG_ATTR_CONNECTION_INFO, (PVOID)&ConnectionInfo);
	if (Status != SEC_E_OK) { fprintf(stderr, "Error 0x%x querying connection info\n", Status); return; }

	fprintf(stderr, "\n");

	switch (ConnectionInfo.dwProtocol)
	{
	case SP_PROT_TLS1_CLIENT:
		fprintf(stderr, "Protocol: TLS1.0\n");
		break;

	case SP_PROT_TLS1_1_CLIENT:
		fprintf(stderr, "Protocol: TLS1.1\n");
		break;

	case SP_PROT_TLS1_2_CLIENT:
		fprintf(stderr, "Protocol: TLS1.2\n");
		break;


	case SP_PROT_SSL3_CLIENT:
		fprintf(stderr, "Protocol: SSL3\n");
		break;

	case SP_PROT_PCT1_CLIENT:
		fprintf(stderr, "Protocol: PCT\n");
		break;

	case SP_PROT_SSL2_CLIENT:
		fprintf(stderr, "Protocol: SSL2\n");
		break;

	default:
		fprintf(stderr, "Protocol: 0x%x\n", ConnectionInfo.dwProtocol);
	}

	switch (ConnectionInfo.aiCipher)
	{
	case CALG_RSA_KEYX:
	case CALG_RSA_SIGN:
		fprintf(stderr, "Cipher: RSA\n");
		break;

	case CALG_RC4:
		fprintf(stderr, "Cipher: RC4\n");
		break;

	case CALG_3DES:
		fprintf(stderr, "Cipher: Triple DES\n");
		break;

	case CALG_RC2:
		fprintf(stderr, "Cipher: RC2\n");
		break;

	case CALG_DES:
	case CALG_CYLINK_MEK:
		fprintf(stderr, "Cipher: DES\n");
		break;

	case CALG_SKIPJACK:
		fprintf(stderr, "Cipher: Skipjack\n");
		break;

	case CALG_AES:
		fprintf(stderr, "Cipher: AES\n");
		break;
	case CALG_AES_128:
		fprintf(stderr, "Cipher: AES_128\n");
		break;
	case CALG_AES_192:
		fprintf(stderr, "Cipher: AES_192\n");
		break;
	case CALG_AES_256:
		fprintf(stderr, "Cipher: AES_256\n");
		break;

	default:
		fprintf(stderr, "Cipher: 0x%x\n", ConnectionInfo.aiCipher);
	}

	fprintf(stderr, "Cipher strength: %d\n", ConnectionInfo.dwCipherStrength);

	switch (ConnectionInfo.aiHash)
	{
	case CALG_MD5:
		fprintf(stderr, "Hash: MD5\n");
		break;

	case CALG_SHA:
		fprintf(stderr, "Hash: SHA\n");
		break;

	case CALG_SHA_256:
		fprintf(stderr, "Hash: SHA_256\n");
		break;
	case CALG_SHA_384:
		fprintf(stderr, "Hash: SHA_384\n");
		break;
	case CALG_SHA_512:
		fprintf(stderr, "Hash: SHA_512\n");
		break;

	default:
		fprintf(stderr, "Hash: 0x%x\n", ConnectionInfo.aiHash);
	}

	fprintf(stderr, "Hash strength: %d\n", ConnectionInfo.dwHashStrength);

	switch (ConnectionInfo.aiExch)
	{
	case CALG_RSA_KEYX:
	case CALG_RSA_SIGN:
		fprintf(stderr, "Key exchange: RSA\n");
		break;

	case CALG_KEA_KEYX:
		fprintf(stderr, "Key exchange: KEA\n");
		break;

	case CALG_AES:
		fprintf(stderr, "Key exchange: AES\n");
		break;
	case CALG_AES_128:
		fprintf(stderr, "Key exchange: AES_128\n");
		break;
	case CALG_AES_192:
		fprintf(stderr, "Key exchange: AES_192\n");
		break;
	case CALG_AES_256:
		fprintf(stderr, "Key exchange: AES_256\n");
		break;

	case CALG_ECDH_EPHEM:
		fprintf(stderr, "Key exchange: ECDH_EPHEM\n");
		break;

	default:
		fprintf(stderr, "Key exchange: 0x%x\n", ConnectionInfo.aiExch);
	}

	fprintf(stderr, "Key exchange strength: %d\n", ConnectionInfo.dwExchStrength);
}


//TO DO:  Consider replacing with ct_scram_hex
void PrintHexDump(
	DWORD length,
	PBYTE buffer)
{
	DWORD i, count, index;
	CHAR rgbDigits[] = "0123456789abcdef";
	CHAR rgbLine[100];
	char cbLine;

	for (index = 0; length;
		length -= count, buffer += count, index += count)
	{
		count = (length > 16) ? 16 : length;

		sprintf_s(rgbLine, 100, "%4.4x  ", index);
		cbLine = 6;

		for (i = 0; i < count; i++)
		{
			rgbLine[cbLine++] = rgbDigits[buffer[i] >> 4];
			rgbLine[cbLine++] = rgbDigits[buffer[i] & 0x0f];
			if (i == 7)
			{
				rgbLine[cbLine++] = ':';
			}
			else
			{
				rgbLine[cbLine++] = ' ';
			}
		}
		for (; i < 16; i++)
		{
			rgbLine[cbLine++] = ' ';
			rgbLine[cbLine++] = ' ';
			rgbLine[cbLine++] = ' ';
		}

		rgbLine[cbLine++] = ' ';

		for (i = 0; i < count; i++)
		{
			if (buffer[i] < 32 || buffer[i] > 126)
			{
				rgbLine[cbLine++] = '.';
			}
			else
			{
				rgbLine[cbLine++] = buffer[i];
			}
		}

		rgbLine[cbLine++] = 0;
		fprintf(stderr, "%s\n", rgbLine);
	}
}

/*****************************************************************************/
static void PrintText(DWORD length, PBYTE buffer) // handle unprintable charaters
{
	int i; //

	fprintf(stderr, "\n"); // "length = %d bytes \n", length);
	for (i = 0; i < (int)length; i++)
	{
		if (buffer[i] == 10 || buffer[i] == 13)
			fprintf(stderr, "%c", (char)buffer[i]);
		else if (buffer[i] < 32 || buffer[i] > 126 || buffer[i] == '%')
			continue;//fprintf(stderr, "%c", '.');
		else
			fprintf(stderr, "%c", (char)buffer[i]);
	}
	fprintf(stderr, "\n\n");
}





/*
void addCipherToSCHANNEL()
{
	SECURITY_STATUS Status = ERROR_SUCCESS;
	LPWSTR wszCipher = (L"TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384");

	Status = BCryptAddContextFunction(
		CRYPT_LOCAL,
		L"SSL",
		NCRYPT_SCHANNEL_INTERFACE,
		wszCipher,
		CRYPT_PRIORITY_TOP);

}

void listSCHANNELCipherSuites(void)
{
	HRESULT Status = ERROR_SUCCESS;
	DWORD   cbBuffer = 0;
	PCRYPT_CONTEXT_FUNCTIONS pBuffer = NULL;

	Status = BCryptEnumContextFunctions(
		CRYPT_LOCAL,
		L"SSL",
		NCRYPT_SCHANNEL_INTERFACE,
		&cbBuffer,
		&pBuffer);
	if (FAILED(Status))
	{
		printf_s("\n**** Error 0x%x returned by BCryptEnumContextFunctions\n", Status);
		return;
	}

	if (pBuffer == NULL)
	{
		printf_s("\n**** Error pBuffer returned from BCryptEnumContextFunctions is null");
		//BCryptFreeBuffer(pBuffer);
		return;
	}

	printf_s("\n\n Listing Cipher Suites (%d) ", (int)pBuffer->cFunctions);
	assert(pBuffer->rgpszFunctions);
	for (UINT index = 0; index < pBuffer->cFunctions; ++index)
	{
		printf_s("\n%S", pBuffer->rgpszFunctions[index]);
	}

	
	if(pBuffer!=NULL)
		BCryptFreeBuffer(pBuffer);

}
*/



static void printProtocolsForDWORD(DWORD protocols)
{
	if( protocols & SP_PROT_PCT1_SERVER )
		fprintf(stderr, "\nSP_PROT_PCT1_SERVER");
		if( protocols & SP_PROT_PCT1_CLIENT )
		fprintf(stderr, "\nSP_PROT_PCT1_CLIENT");
	if( protocols & SP_PROT_PCT1 )
		fprintf(stderr, "\nSP_PROT_PCT1");
	if( protocols & SP_PROT_SSL2_SERVER)
		fprintf(stderr, "\nSP_PROT_SSL2_SERVER");
	if( protocols & SP_PROT_SSL2_CLIENT )
		fprintf(stderr, "\nSP_PROT_SSL2_CLIENT");
	if( protocols & SP_PROT_SSL2 )
		fprintf(stderr, "\nSP_PROT_SSL2");
	if( protocols & SP_PROT_SSL3_CLIENT )
		fprintf(stderr, "\nSP_PROT_SSL3_CLIENT");
	if( protocols & SP_PROT_SSL3_SERVER )
		fprintf(stderr, "\nSP_PROT_SSL3_SERVER");
	if( protocols & SP_PROT_SSL3)
		fprintf(stderr, "\nSP_PROT_SSL3");
	if( protocols & SP_PROT_TLS1_SERVER)
		fprintf(stderr, "\nSP_PROT_TLS1_SERVER");
	if( protocols & SP_PROT_TLS1_CLIENT )
		fprintf(stderr, "\nSP_PROT_TLS1_SERVER");
	if( protocols & SP_PROT_TLS1 )
		fprintf(stderr, "\nSP_PROT_TLS1");
	if( protocols & SP_PROT_SSL3TLS1_CLIENTS)
		fprintf(stderr, "\nSP_PROT_SSL3TLS1_CLIENTS");
	if( protocols & SP_PROT_SSL3TLS1_SERVERS )
		fprintf(stderr, "\nSP_PROT_SSL3TLS1_SERVERS");
	if( protocols & SP_PROT_SSL3TLS1 )
		fprintf(stderr, "\nSP_PROT_SSL3TLS1");
	if( protocols & SP_PROT_UNI_SERVER)
		fprintf(stderr, "\nSP_PROT_UNI_SERVER");
	if( protocols & SP_PROT_UNI_CLIENT )
		fprintf(stderr, "\nSP_PROT_UNI_CLIENT");
	if( protocols & SP_PROT_UNI )
		fprintf(stderr, "\nSP_PROT_UNI");
	if( protocols & SP_PROT_ALL)
		fprintf(stderr, "\nSP_PROT_ALL");
	if( protocols & SP_PROT_NONE )
		fprintf(stderr, "\nSP_PROT_NONE");
	if( protocols & SP_PROT_CLIENTS)
		fprintf(stderr, "\nSP_PROT_CLIENTS");
	if( protocols & SP_PROT_SERVERS)
		fprintf(stderr, "\nSP_PROT_SERVERS");
	if( protocols & SP_PROT_TLS1_0_SERVER)
		fprintf(stderr, "\nSP_PROT_TLS1_0_SERVER");
	if( protocols & SP_PROT_TLS1_0_CLIENT )
		fprintf(stderr, "\nSP_PROT_TLS1_0_CLIENT");
	if( protocols & SP_PROT_TLS1_0 )
		fprintf(stderr, "\nSP_PROT_TLS1_0");

	if( protocols & SP_PROT_TLS1_1_SERVER)
		fprintf(stderr, "\nSP_PROT_TLS1_1_SERVER");
	if( protocols & SP_PROT_TLS1_1_CLIENT )
		fprintf(stderr, "\nSP_PROT_TLS1_1_CLIENT");
	if( protocols & SP_PROT_TLS1_1 )
		fprintf(stderr, "\nSP_PROT_TLS1_1");

	if( protocols & SP_PROT_TLS1_2_SERVER)
		fprintf(stderr, "\nSP_PROT_TLS1_2_SERVER");
	if( protocols & SP_PROT_TLS1_2_CLIENT )
		fprintf(stderr, "\nSP_PROT_TLS1_2_CLIENT");
	if( protocols & SP_PROT_TLS1_2 )
		fprintf(stderr, "\nSP_PROT_TLS1_2");


	if( protocols & SP_PROT_TLS1_1PLUS_SERVER)
		fprintf(stderr, "\nSP_PROT_TLS1_1PLUS_SERVER");
	if( protocols & SP_PROT_TLS1_1PLUS_CLIENT )
		fprintf(stderr, "\nSP_PROT_TLS1_1PLUS_CLIENT");
	if( protocols & SP_PROT_TLS1_1PLUS )
		fprintf(stderr, "\nSP_PROT_TLS1_1PLUS");


	if( protocols & SP_PROT_TLS1_X_SERVER)
		fprintf(stderr, "\nSP_PROT_TLS1_X_SERVER");
	if( protocols & SP_PROT_TLS1_X_CLIENT )
		fprintf(stderr, "\nSP_PROT_TLS1_X_CLIENT");
	if( protocols & SP_PROT_TLS1_X)
		fprintf(stderr, "\nSP_PROT_TLS1_X");



	if( protocols & SP_PROT_SSL3TLS1_X_CLIENTS)
		fprintf(stderr, "\nSP_PROT_SSL3TLS1_X_CLIENTS");
	if( protocols & SP_PROT_SSL3TLS1_X_SERVERS )
		fprintf(stderr, "\nSP_PROT_SSL3TLS1_X_SERVERS");
	if( protocols & SP_PROT_SSL3TLS1_X)
		fprintf(stderr, "\nSP_PROT_SSL3TLS1_X");

	if( protocols & SP_PROT_X_CLIENTS)
		fprintf(stderr, "\nSP_PROT_X_CLIENTS");
	if( protocols & SP_PROT_X_SERVERS )
		fprintf(stderr, "\nSP_PROT_X_SERVERS");

	fprintf(stderr, "\n");
}


BOOL CTCredentialHandleCreate( CTSSLContextRef * sslContextRef )
{
	SCHANNEL_CRED SchannelCred;
	SECURITY_STATUS  Status;
	TimeStamp tsExpiry = {0};
	//PCCERT_CONTEXT serverCerts[1] = {0};
	//PCredHandle pCredHandle = NULL;
	//listSCHANNELCipherSuites();
	//addCipherToSCHANNEL();
	/*
					PSecurityFunctionTable pSSPI = NULL;
	// Retrieve a data pointer for the current thread's security function table 
    pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface); 
    if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
      fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/

	// SCHANNEL_CRED
    // Build Schannel credential structure. Currently, this sample only
    // specifies the protocol to be used (and optionally the certificate,
    // of course). Real applications may wish to specify other parameters as well.
	    ZeroMemory( &tsExpiry, sizeof(TimeStamp) );

	ZeroMemory( &SchannelCred, sizeof(SCHANNEL_CRED) );
    SchannelCred.dwVersion  = SCHANNEL_CRED_VERSION;

	
	SchannelCred.dwMinimumCipherStrength = 0;
	SchannelCred.dwMaximumCipherStrength = 0;
    //if(pImportCertContext)
    //{
        SchannelCred.cCreds     = 0;
        SchannelCred.paCred     = NULL;//&pImportCertContext;
    //}

		SchannelCred.grbitEnabledProtocols = 0;// SP_PROT_X_CLIENTS;// | SP_PROT_TLS1_1PLUS_CLIENT | SP_PROT_CLIENTS;

    //if(aiKeyExch) rgbSupportedAlgs[cSupportedAlgs++] = aiKeyExch;
    //if(1)
    //{
		SchannelCred.cSupportedAlgs = 0;
		SchannelCred.palgSupportedAlgs = NULL;
    //}
	
		//dwFlags = SCH_USE_STRONG_CRYPTO | SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION | SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT | SCH_CRED_IGNORE_NO_REVOCATION_CHECK;
		//memcpy((char*)&SchannelCred - sizeof(DWORD) * 2, &dwFlags, sizeof(DWORD));

	//TO DO:  Turn these flags back on?
	SchannelCred.dwFlags = SCH_CRED_NO_DEFAULT_CREDS | SCH_CRED_MANUAL_CRED_VALIDATION | SCH_CRED_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT;// | SCH_CRED_IGNORE_NO_REVOCATION_CHECK;
	//SchannelCred.dwCredFormat = SCH_CRED_MAX_SUPPORTED_ALGS;
    // The SCH_CRED_MANUAL_CRED_VALIDATION flag is specified because
    // this sample verifies the server certificate manually.
    // Applications that expect to run on WinNT, Win9x, or WinME
    // should specify this flag and also manually verify the server
    // certificate. Applications running on newer versions of Windows can
    // leave off this flag, in which case the InitializeSecurityContext
    // function will validate the server certificate automatically.
    //SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;
	//SchannelCred.dwFlags |= SCH_CRED_IGNORE_NO_REVOCATION_CHECK;

	//allocate memory backing Credential pointer to pass to ACH
	//caCertRef = (ReqlSecCertificateRef)malloc( sizeof(CredHandle) );

	//fprintf(stderr, "sizeof(ReqlSSLContextRef) = %d\n", sizeof(ReqlSSLContextRef)); 
	//fprintf(stderr, "sizeof(ReqlSSLContext) = %d\n", sizeof(ReqlSSLContext)); 
	//fprintf(stderr, "sizeof(CredHandle) = %d\n", sizeof(CredHandle)); 
	//fprintf(stderr, "sizeof(CtxtHandle) = %d\n", sizeof(CtxtHandle));
	//fprintf(stderr, "sizeof(SecBuffer) = %d\n", sizeof(SecBuffer)); 
	//fprintf(stderr, "sizeof(SecPkgContext_StreamSizes) = %d\n", sizeof(SecPkgContext_StreamSizes)); 

    // Create an SSPI credential.
	//assert(g_pSSPI);
	//g_pSSPI = InitSecurityInterface();

    Status = AcquireCredentialsHandleA( NULL,                 // Name of principal    
                                                                                                 TEXT(SCHANNEL_NAME_A),      // Name of package
                                                                                                 SECPKG_CRED_OUTBOUND, // Flags indicating use
                                                                                                 NULL,                 // Pointer to logon ID
                                                                                                 &SchannelCred,        // Package specific data
                                                                                                 NULL,                 // Pointer to GetKey() func
                                                                                                 NULL,                 // Value to pass to GetKey()
                                                                                                 &((*sslContextRef)->hCred),              // (out) Cred Handle
                                                                                                 NULL );          // (out) Lifetime (optional)


	//caCertRef = &hCreds;
    if(Status != SEC_E_OK) {
		fprintf(stderr, "**** Error 0x%x (%ld) (returned by AcquireCredentialsHandle\n", Status, Status);
		//free(caCertRef);
		//caCertRef = NULL;
		return FALSE;
	}

	return TRUE;

}



/*****************************************************************************/
static void GetNewClientCredentials(CredHandle *phCreds, CtxtHandle *phContext)
{
	SCHANNEL_CRED SchannelCred;
	CredHandle                                            hCreds;
	SecPkgContext_IssuerListInfoEx    IssuerListInfo;
	PCCERT_CHAIN_CONTEXT                        pChainContext;
	CERT_CHAIN_FIND_BY_ISSUER_PARA    FindByIssuerPara;
	PCCERT_CONTEXT                                    pCertContext;
	//PTimeStamp                                         tsExpiry;
	SECURITY_STATUS                                    Status;

	PSecurityFunctionTable pSSPI = NULL;
	// Retrieve a data pointer for the current thread's security function table 
	pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
	if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
		fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
		pSSPI = g_pSSPI;
	}


	// Read list of trusted issuers from schannel.
	Status = pSSPI->QueryContextAttributes(phContext, SECPKG_ATTR_ISSUER_LIST_EX, (PVOID)&IssuerListInfo);
	if (Status != SEC_E_OK) { fprintf(stderr, "Error 0x%x querying issuer list info\n", Status); return; }

	// Enumerate the client certificates.
	ZeroMemory(&FindByIssuerPara, sizeof(CERT_CHAIN_FIND_BY_ISSUER_PARA));

	FindByIssuerPara.cbSize = sizeof(CERT_CHAIN_FIND_BY_ISSUER_PARA);
	FindByIssuerPara.pszUsageIdentifier = szOID_PKIX_KP_CLIENT_AUTH;
	FindByIssuerPara.dwKeySpec = 0;
	FindByIssuerPara.cIssuer = IssuerListInfo.cIssuers;
	FindByIssuerPara.rgIssuer = IssuerListInfo.aIssuers;

	pChainContext = NULL;

	while (TRUE)
	{   // Find a certificate chain.
		pChainContext = CertFindChainInStore(hMyCertStore,
			X509_ASN_ENCODING,
			0,
			CERT_CHAIN_FIND_BY_ISSUER,
			&FindByIssuerPara,
			pChainContext);
		if (pChainContext == NULL) { fprintf(stderr, "Error 0x%x finding cert chain\n", GetLastError()); break; }

		fprintf(stderr, "\ncertificate chain found\n");

		// Get pointer to leaf certificate context.
		pCertContext = pChainContext->rgpChain[0]->rgpElement[0]->pCertContext;

		// Create schannel credential.
		SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
		SchannelCred.cCreds = 1;
		SchannelCred.paCred = &pCertContext;

		Status = AcquireCredentialsHandleA(NULL,                   // Name of principal
			TEXT(SCHANNEL_NAME_A),           // Name of package
			SECPKG_CRED_OUTBOUND,   // Flags indicating use
			NULL,                   // Pointer to logon ID
			&SchannelCred,          // Package specific data
			NULL,                   // Pointer to GetKey() func
			NULL,                   // Value to pass to GetKey()
			&hCreds,                // (out) Cred Handle
			NULL);            // (out) Lifetime (optional)

		if (Status != SEC_E_OK) { fprintf(stderr, "**** Error 0x%x returned by AcquireCredentialsHandle\n", Status); continue; }

		fprintf(stderr, "\nnew schannel credential created\n");

		pSSPI->FreeCredentialsHandle(phCreds); // Destroy the old credentials.

		*phCreds = hCreds;

	}
}

/***
 *  VerifyServerCertificate
 *
 *
 *
 ***/
 /*****************************************************************************/
DWORD VerifyServerCertificate(PCCERT_CONTEXT pServerCert, char* pszServerName, DWORD dwCertFlags)
{
	HTTPSPolicyCallbackData  polHttps;
	CERT_CHAIN_POLICY_PARA   PolicyPara;
	CERT_CHAIN_POLICY_STATUS PolicyStatus;
	CERT_CHAIN_PARA          ChainPara;
	PCCERT_CHAIN_CONTEXT     pChainContext = NULL;
	DWORD                                         cchServerName, Status;
	LPSTR rgszUsages[] = { szOID_PKIX_KP_SERVER_AUTH,
							   szOID_SERVER_GATED_CRYPTO,
							   szOID_SGC_NETSCAPE };

	DWORD cUsages = sizeof(rgszUsages) / sizeof(LPSTR);

	PWSTR   pwszServerName = NULL;

	//memset(&polHttps, 0 , sizeof(HTTPSPolicyCallbackData));
	//memset(&PolicyPara, 0 , sizeof(CERT_CHAIN_POLICY_PARA));
	//memset(&PolicyStatus, 0 , sizeof(CERT_CHAIN_POLICY_STATUS));
	//memset(&ChainPara, 0 , sizeof(CERT_CHAIN_PARA));

	fprintf(stderr, "VerifyServerCertificate::Hello\n");
	if (pServerCert == NULL)
	{
		Status = SEC_E_WRONG_PRINCIPAL;   fprintf(stderr, "VerifyServerCertificate::SEC_E_INSUFFICIENT_MEMORY 1\n"); goto cleanup;
	}

	// Convert server name to unicode.
	if (pszServerName == NULL || strlen(pszServerName) == 0)
	{
		Status = SEC_E_WRONG_PRINCIPAL; fprintf(stderr, "VerifyServerCertificate::SEC_E_INSUFFICIENT_MEMORY 2\n");  goto cleanup;
	}

	cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName, -1, NULL, 0);
	pwszServerName = (PWSTR)LocalAlloc(LMEM_FIXED, cchServerName * sizeof(WCHAR));
	if (pwszServerName == NULL)
	{
		Status = SEC_E_INSUFFICIENT_MEMORY; fprintf(stderr, "VerifyServerCertificate::SEC_E_INSUFFICIENT_MEMORY\n"); goto cleanup;
	}

	cchServerName = MultiByteToWideChar(CP_ACP, 0, pszServerName, -1, pwszServerName, cchServerName);
	if (cchServerName == 0)
	{
		Status = SEC_E_WRONG_PRINCIPAL; fprintf(stderr, "VerifyServerCertificate::SEC_E_WRONG_PRINCIPAL\n"); goto cleanup;
	}


	// Build certificate chain.
	ZeroMemory(&ChainPara, sizeof(CERT_CHAIN_PARA));
	ChainPara.cbSize = sizeof(CERT_CHAIN_PARA);
	ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
	ChainPara.RequestedUsage.Usage.cUsageIdentifier = cUsages;
	ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = rgszUsages;

	fprintf(stderr, "Before CertGetCertificateChain\n");
	if (!CertGetCertificateChain(NULL,
		pServerCert,
		NULL,
		NULL,
		&ChainPara,
		0,
		NULL,
		&pChainContext))
	{
		Status = GetLastError();
		fprintf(stderr, "Error 0x%x returned by CertGetCertificateChain!\n", Status);
		goto cleanup;
	}
	fprintf(stderr, "After CertGetCertificateChain\n");


	// Validate certificate chain.
	ZeroMemory(&polHttps, sizeof(HTTPSPolicyCallbackData));
	polHttps.cbStruct = sizeof(HTTPSPolicyCallbackData);
	polHttps.dwAuthType = AUTHTYPE_SERVER;
	polHttps.fdwChecks = dwCertFlags;
	polHttps.pwszServerName = pwszServerName;

	memset(&PolicyPara, 0, sizeof(CERT_CHAIN_POLICY_PARA));
	PolicyPara.cbSize = sizeof(CERT_CHAIN_POLICY_PARA);
	PolicyPara.pvExtraPolicyPara = &polHttps;

	memset(&PolicyStatus, 0, sizeof(CERT_CHAIN_POLICY_STATUS));
	PolicyStatus.cbSize = sizeof(CERT_CHAIN_POLICY_STATUS);

	if (!CertVerifyCertificateChainPolicy(CERT_CHAIN_POLICY_SSL,
		pChainContext,
		&PolicyPara,
		&PolicyStatus))
	{
		Status = GetLastError();
		fprintf(stderr, "Error 0x%x returned by CertVerifyCertificateChainPolicy!\n", Status);
		goto cleanup;
	}

	if (PolicyStatus.dwError)
	{
		fprintf(stderr, "VerifyServerCertificate::Policy Status Error\n");
		Status = PolicyStatus.dwError;
		DisplayWinVerifyTrustError(Status);
		goto cleanup;
	}
	fprintf(stderr, "VerifyServerCertificate::OK\n");
	Status = SEC_E_OK;


cleanup:
	if (pChainContext)  CertFreeCertificateChain(pChainContext);
	if (pwszServerName) LocalFree(pwszServerName);

	return Status;
}

#endif //WIN32

void CTSSLCertificateDestroy(CTSecCertificateRef * certRef)
{
	assert(*certRef);
#ifdef CTRANSPORT_USE_MBED_TLS
	free(*certRef);
#elif defined( _WIN32 )
	free(*certRef);
#elif defined (__APPLE__)
	CFRelease(*certRef);
#endif
	*certRef = NULL;
}


/***
 *	CTlSSLInit
 *
 *	Load Security Libraries for specified SSL Implementation
 * 
 *  Note:  This function is actually only used in the case of WolfSSL to call wolfSSL_Init
 ***/
void CTSSLInit(void)
{
#ifdef CTRANSPORT_WOLFSSL
	 /* Initialize wolfSSL */
	if (!g_SSLInitialized)
	{
		wolfSSL_Init();
		g_SSLInitialized = 1;
	}
#elif defined(CTRANSPORT_USE_MBED_TLS)
	
/*
	mbedtls_entropy_init(&entropy);
	if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
		(const unsigned char *)pers,
		strlen(pers) ) ) != 0 )
	{
		Reql_fprintf(stderr, "ReqlSSLInit::mbedtls_ctr_drbg_seed failed with return %d\n", ret);
		return;
	}
*/
#elif defined(_WIN32)
	//Load
    //if( !LoadSecurityLibrary() )
	//{ fprintf(stderr, "Error initializing the security library\n"); }//goto cleanup; } //
	//fprintf(stderr, "----- SSPI Initialized\n");
#endif

}

void CTSSLCleanup(void)
{
#ifdef CTRANSPORT_WOLFSSL
	wolfSSL_Cleanup();          //Cleanup the wolfSSL environment
#elif defined(CTRANSPORT_USE_MBED_TLS)
#elif defined(_WIN32)
	//UnloadSecurityLibrary();
#endif
}


void CTSSLContextDestroy(CTSSLContextRef sslContextRef)
{
#ifdef CTRANSPORT_WOLFSSL //DESTROY WOLFSSL CONTEXT
	if (sslContextRef)
	{
		if (sslContextRef->bio)
			wolfSSL_BIO_free(sslContextRef->bio);
		if (sslContextRef->conf)
			wolfSSL_free(sslContextRef->ctx);  // Free the wolfSSL context object
		if (sslContextRef->conf)
			wolfSSL_CTX_free(sslContextRef->conf);  // Free the wolfSSL context object

		sslContextRef->bio = NULL;
		sslContextRef->ctx = NULL;
		sslContextRef->conf = NULL;
	}

	free(sslContextRef);

#elif defined(CTRANSPORT_USE_MBED_TLS)	//DESTROY MBEDTLS CONTEXT
	//TO DO
	mbedtls_entropy_free(&(sslContextRef->entropy));
	mbedtls_ctr_drbg_free(&(sslContextRef->ctr_drbg));
	//mbedtls_x509_crt_free(&(sslContextRef->cacert));
	mbedtls_ssl_free(&(sslContextRef->ctx));
	mbedtls_ssl_config_free(&(sslContextRef->conf));
	free(sslContextRef);
#elif defined(_WIN32)		//DESTROY SCHANNEL CONTEXT
	//PSecurityFunctionTable pSSPI = NULL;
	//g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
 
	 //sslContextRef->hCtxt = 0;
	if (sslContextRef)
	{
		if (sslContextRef->hBuffer)
			free(sslContextRef->hBuffer);
		if (sslContextRef->tBuffer)
			free(sslContextRef->tBuffer);

		sslContextRef->hBuffer = NULL;
		sslContextRef->tBuffer = NULL;

		/*
		// Retrieve a data pointer for the current thread's security function table
		pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
		if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
		{
		  fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
		  pSSPI = g_pSSPI;
		}
		*/
		DeleteSecurityContext(&(sslContextRef->hCtxt));

		free(sslContextRef);
	}

	sslContextRef = NULL;
#elif defined(__APPLE__)	//DESTROY SECURE TRANPORT CONTEXT
	SSLClose(sslContextRef);
#endif
}


/***
 *  ReqlSSLCertificate
 *
 *  Read CA file in DER format from disk to create an Apple Secure Transport API SecCertificateRef
 *  and Add the SecCertificateRef to the device keychain (or delete and re-add the certificate -- which works on iOS but not OSX)
 *
 *  -- Non (password protected) PEM files can be converted to DER format offline with OpenSSL or in app with by removing PEM headers/footers and converting (from or to?) BASE64.  DER data may need headers chopped of in some instances.
 *  -- Password protected PEM files must have their private/public key pairs generated using PCK12.  This can be done offline with OpenSSL or in app using Secure Transport
 ***/
CTSecCertificateRef CTSSLCertificate( const char * caCertPath )
{
	CTSecCertificateRef caCert = NULL;

#ifdef CTRANSPORT_WOLFSSL
	assert(1 == 0);
#elif defined( CTRANSPORT_USE_MBED_TLS )
	int ret = 1;// , len;

	caCert = (CTSecCertificateRef)malloc(sizeof(CTSecCertificate));
	//memset(caCert, 0, sizeof(caCert));
	mbedtls_x509_crt_init(caCert);

	//first try loading as a der cert file
	//ret = mbedtls_x509_crt_parse_file(caCert, caCertPath);
	//if (ret < 0)
	//{
	//	Reql_fprintf(stderr, "ReqlSSLCertificate::mbedtls_x509_crt_parse_file failed and returned %d (pathsize = %d)\n)", ret, (int)strlen(caCertPath));

		//then try loading as a cert buffer
		ret = mbedtls_x509_crt_parse(caCert, (const unsigned char*)caCertPath, strlen(caCertPath)+1);
		if (ret < 0)
		{
			Reql_fprintf(stderr, "ReqlSSLCertificate::mbedtls_x509_crt_parse failed and returned %d (buffer size = %d)\n)", ret, (int)strlen(caCertPath)+1);
			free(caCert);
			return NULL;
		}
	//}
#elif defined(__WIN32)
	assert(1 == 0);
#elif defined(__APPLE__)
    //7.1 Read CA chain to buffer from file on disk
    uint64_t filesize;
    char * filebuffer;
    int fileDescriptor = cr_file_open(caCertPath);
    filesize = cr_file_size(fileDescriptor);
    
    //7.2 Map to the file buffer for reading -- MMAP IS OVERKILL FOR SMALL FILES LIKE THIS
    //    (Typically you would also want to touch all pages but it is already less than one page)
    filebuffer = (char*)cr_file_map_to_buffer(&(filebuffer), filesize, PROT_READ,  MAP_SHARED | MAP_NORESERVE, fileDescriptor, 0);
    
    if (madvise(filebuffer, (size_t)filesize, MADV_SEQUENTIAL | MADV_WILLNEED ) == -1) {
        fprintf(stderr, "\nread madvise failed\n");
    }
    
    //7.3 close file after mapping, we don't need the fd anymore (remember to unmap the filebuffer when finished with it)
    cr_file_close(fileDescriptor);
    //fprintf(stderr, "\nFile Size =  %llu bytes\n", filesize);
    
    //7.4  Create a CFData object from the mmapped file buffer
    CFDataRef caData = CFDataCreateWithBytesNoCopy( kCFAllocatorDefault, (UInt8*)filebuffer, filesize, kCFAllocatorNull);
    assert(caData);
    
    //7.5  Create a Security Transport Certificate from the file data
    SecCertificateRef caCert = SecCertificateCreateWithData(NULL, caData);
    assert(caCert);
    
    //unmap the buffer
    munmap(filebuffer, filesize);
    filebuffer = NULL;
    
    //Return the SecCertificateRef
#endif
	return caCert;
}


#if defined(__APPLE__)

/***
 *  ReqlSaveCertToKeychain
 *
 *  Read CA file in DER format from disk to create an Apple Secure Transport API SecCertificateRef
 *  and Add the SecCertificateRef to the device keychain (or delete and re-add the certificate -- which works on iOS but not OSX)
 *
 *  Returns an OSStatus since calls in this mehtodare strictly limited to the Apple Secure Transport API
 ***/
static OSStatus ReqlSaveCertToKeychain( SecCertificateRef cert )
{
    OSStatus status;

    //7.6 Create a dictionary so we can add certificate to system keychain using SecItemAdd
    const void *ccq_keys[] =    { kSecClass,            kSecValueRef, kSecReturnPersistentRef,  kSecReturnAttributes    };
    const void *ccq_values[] =  { kSecClassCertificate, cert,     kCFBooleanTrue,           kCFBooleanTrue          };
    CFDictionaryRef certCreateQuery = CFDictionaryCreate(kCFAllocatorDefault, ccq_keys, ccq_values, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    assert(certCreateQuery);
    
    //7.7 Check to see if the certificate we are loading is already in the device system keychain
    CFDictionaryRef certCopyResult = NULL;
    
    if( (status = SecItemCopyMatching(certCreateQuery, (CFTypeRef *)&certCopyResult)) != errSecSuccess )
    {
        if( status != errSecItemNotFound )
        {
            fprintf(stderr, "SecItemCopyMatching failed with error: %d\n", status);
            return status;
        }
    }
    
    //7.8 If the certificate matching the dict query was already in the device system keychain, then delete it and re-add it (or directly attempt to get the identity cert with it)
    /*
    if( certCopyResult )
    {
        //Delete SecurityCertificateRef matching query from keychain
        if( (status = SecItemDelete( certCreateQuery )) != errSecSuccess )
        {
            fprintf(stderr, "SecItemDelete failed with error:  %d\n", status);
            return status;
        }
        else
        {
            fprintf(stderr, "SecItemDelete:  Success\n");
        }
        
        //release, we are about to use this pointer for an allocated copy again
        CFRelease(certCopyResult);
        certCopyResult = NULL;
    }
    */
    
    //7.9 Add the SecurityCertificateRef to the device keychain
    if( !certCopyResult && (status = SecItemAdd( certCreateQuery, (CFTypeRef *)&certCopyResult)) != errSecSuccess )
    {
        if( status == errSecDuplicateItem)
        {
            //by design we should not arrive here unless our calls to delete above failed
            fprintf(stderr, "SecItemAdd:  Duplicate KeyChain Item\n");
            return -1;
        }
        else
        {
            fprintf(stderr, "SecItemAdd failed with error:  %d", status);
            return -1;
        }
    }
    
    //7.10  Create Certificate Section Cleanup
    if( certCopyResult )
        CFRelease(certCopyResult);
    CFRelease(certCreateQuery);
    
    //Return an OSStatus
    return status;
}

#endif


#ifdef CTRANSPORT_USE_MBED_TLS

/**
	* Send callback for mbed TLS
	*/
int mbedWriteToSocket(void *ctx, const unsigned char *buf, size_t len) {

	int ret = -1;// MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;
	//int fd = *((int*)ctx);
	int fd = (CTSocket)ctx;//((mbedtls_net_context *)ctx)->fd;


	assert(fd >= 0);
	fprintf(stderr, "fd = %d", fd);
	if (fd < 0)
		return(MBEDTLS_ERR_NET_INVALID_CONTEXT);

	ret = (int)send(fd, (char*)buf, len,0);

	if (ret < 0)
	{
		//if (net_would_block(ctx) != 0)
		ret = (WSAGetLastError() == WSAEWOULDBLOCK);
		if( ret !=0 )
			return MBEDTLS_ERR_SSL_WANT_WRITE;

#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && !defined(EFI32)
		if (WSAGetLastError() == WSAECONNRESET)
			return(MBEDTLS_ERR_NET_CONN_RESET);
#else
		if (errno == EPIPE || errno == ECONNRESET)
			return(MBEDTLS_ERR_NET_CONN_RESET);

		if (errno == EINTR)
			return(MBEDTLS_ERR_SSL_WANT_WRITE);
#endif

		//return(MBEDTLS_ERR_NET_SEND_FAILED);
		//return ret;
		return WSAGetLastError();
	}

	return(ret);

}

/**
 * Receive callback for mbed TLS
 */
static int mbedReadFromSocket(void *ctx, unsigned char *buf, size_t len)
{
	size_t status = -1;
	//int sockfd = *((int*)ctx);
	size_t bytesRequested = len;
	//unsigned long numBytesAvailable = 0;
	

//#ifdef _WIN32
//	DWORD numBytesRecv = len;
//	DWORD flags = 0;
//	WSABUF wsaBuf = {len, (CHAR*)buf};
//#endif
	
	int sockfd = (CTSocket)ctx;//((mbedtls_net_context *)ctx)->fd;

	fprintf(stderr, "**** mbedReadFromSocket::bytesRequested = %d\n", (int)bytesRequested);

	assert(sockfd >= 0);
	if (sockfd < 0)
		return(MBEDTLS_ERR_NET_INVALID_CONTEXT);

/*
#ifdef _WIN32	
	status = WSARecv(sockfd, (LPWSABUF)&wsaBuf, 1, &numBytesRecv, &flags, NULL, NULL);
	//WSA_IO_PENDING
	if( status == SOCKET_ERROR )
	{
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			fprintf(stderr,  "****mbedReadFromSocket::WSARecv failed with error %d \n",  WSAGetLastError() );	
		} 
		
		//forward the winsock system error to the client
		return (CTClientError)WSAGetLastError();
	}

	if( status == 0 )
	{
		fprintf(stderr, "**** mbedReadFromSocket::WSARecv return numBytesRecv = %d\n", (int)numBytesRecv);
		return numBytesRecv;
	}
#else
*/
	status = (int)recv(sockfd, (char*)buf, bytesRequested, 0 );
//#endif

	/*
	if (status > 0)
	{
		//*dataLength = status;
		//if (bytesRequested > status)
		//	return MBEDTLS_ERR_SSL_WANT_READ;// WSAEWOULDBLOCK; //errSSLWouldBlock
		//else
		//	return noErr;
	}
	else */if (0 == status) {
		//*dataLength = 0;
				fprintf(stderr, "**** mbedReadFromSocket::Status = 0 (%d)\n", WSAGetLastError());

		return WSAEDISCON;// errSSLClosedGraceful;
	}
	else if (status  == SOCKET_ERROR)
	{
		fprintf(stderr, "**** mbedReadFromSocket::Error %d reading data from server\n", WSAGetLastError());
		if (WSAGetLastError() == WSAEWOULDBLOCK)
			return MBEDTLS_ERR_SSL_WANT_READ;

#if ( defined(_WIN32) || defined(_WIN32_WCE) ) && !defined(EFIX64) && !defined(EFI32)
		if (WSAGetLastError() == WSAECONNRESET)
			return(MBEDTLS_ERR_NET_CONN_RESET);
#else
		if (errno == EPIPE || errno == ECONNRESET)
			return(MBEDTLS_ERR_NET_CONN_RESET);

		if (errno == EINTR)
			return(MBEDTLS_ERR_SSL_WANT_READ);
#endif

		return(MBEDTLS_ERR_NET_RECV_FAILED);
	}

	fprintf(stderr, "**** mbedReadFromSocket::return numBytesRecv = %d\n", (int)status);
	return (int)status;
}

/*
void onError(TCPSocket *s, int error) {
	fprintf(stderr, "MBED: Socket Error: %d\r\n", error);
	s->close();
	_error = true;
}
*/

#elif defined(__APPLE__)
OSStatus readFromSocket(SSLConnectionRef connection, void *data, size_t *dataLength) {

	//fprintf(stderr, "readFromSocket\n");
	//mach_port_t machTID = pthread_mach_thread_np(pthread_self());
	//fprintf(stderr, "readFromSocket current thread: %x", machTID);
	//fprintf(stderr, "readFromSocket (%d bytes): \n\n%.*s\n", *dataLength, *dataLength, (char*)data );

	int sockfd = (int)connection;
	//kevent kev;
	//uintptr_t ret_event = kqueue_wait_with_timeout(rdb_conn->event_queue, &kev, /*EVFILT_READ |*/ EVFILT_READ, -1, -2, sockfd, socketfd, UINT_MAX);

	size_t bytesRequested = *dataLength;
	ssize_t status = read(sockfd, data, bytesRequested);

	/*
	char buf[1024];
	struct aiocb aiocb;
	memset(&aiocb, 0, sizeof(struct aiocb));
	aiocb.aio_fildes = sockfd;
	aiocb.aio_buf = data;
	aiocb.aio_nbytes = *dataLength;
	aiocb.aio_lio_opcode = LIO_READ;
	if (aio_read(&aiocb) == -1) {
		fprintf(stderr, " Error at aio_read(): %s\n",strerror(errno));
		return errSSLClosedAbort;

	}

	int err;
	ssize_t status;

	//Wait until end of transaction
	while ((err = aio_error (&aiocb)) == EINPROGRESS);
	err = aio_error(&aiocb);
	status = aio_return(&aiocb);
	if (err != 0) {
		fprintf(stderr, " Error at aio_error() : %s\n", strerror (err));
		return errSSLClosedAbort;

	}
	if (status != 1024) {
		fprintf(stderr, " Error at aio_return()\n");
		return errSSLClosedAbort;
	}
	// check it //
	fprintf(stderr, "aio_buf = \n\n%.*s\n", status, buf);
	*/
	if (status > 0) {
		*dataLength = status;
		if (bytesRequested > *dataLength)
			return errSSLWouldBlock;
		else
			return noErr;
	}
	else if (0 == status) {
		*dataLength = 0;
		return errSSLClosedGraceful;
	}
	else {
		*dataLength = 0;
		switch (errno) {
		case ENOENT:
			return errSSLClosedGraceful;
		case EAGAIN:
			return errSSLWouldBlock;
		case ECONNRESET:
			return errSSLClosedAbort;
		default:
			return errno;
		}
		return noErr;
	}
}

OSStatus writeToSocket(SSLConnectionRef connection, const void *data, size_t *dataLength) {
	//fprintf(stderr, "writeToSocket\n");

	if (!data || *dataLength < 1)
		return 0;


	//fprintf(stderr, "writeToSocket (%d bytes): \n\n%.*s\n", *dataLength, *dataLength, (char*)data );

	int sockfd = (int)connection;
	size_t bytesToWrite = *dataLength;
	ssize_t status = write(sockfd, data, bytesToWrite);
	//fprintf(stderr, "bytes written  = %ld\n", status);
	if (status < 1)
	{
		fprintf(stderr, "errno = %d\n", errno);
		return errno;
	}
	if (status > 0) {
		*dataLength = status;
		if (bytesToWrite > *dataLength)
			return errSSLWouldBlock;
		else
			return noErr;
	}
	else if (0 == status) {
		*dataLength = 0;
		return errSSLClosedGraceful;
	}
	else {
		*dataLength = 0;
		if (EAGAIN == errno) {
			return errSSLWouldBlock;
		}
		else {
			return errSecIO;
		}
	}
}

#elif defined(CTRANSPORT_WOLFSSL)

int CTWolfSSLRecv(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
	/* By default, ctx will be a pointer to the file descriptor to read from.
	 * This can be changed by calling wolfSSL_SetIOReadCtx(). */
	int sockfd = *(int*)ctx;
	CTSSLDecryptTransient* transient = (CTSSLDecryptTransient*)ctx;
	int recvd;
	unsigned long remainingBytes;
	unsigned long recvdBytes;
	//fprintf(stderr, "CTWolfSSLRecv Callback: attempting to recv %d bytes on socket (%d)...\n", sz, sockfd);

	//WSABUF wsaBuf = { sz, buff };
	//DWORD flags = 0;
	//OVERLAPPED ol;
	//ZeroMemory(&ol, sizeof(OVERLAPPED));
	//Receive a buffer of encrypted bytes from a buffer passed to wolfSSL_read by CoreTransport
	if (sockfd == 0)
	{
		//if WolfSSL is asking for more bytes than CoreTransport has provided...
		if (sz > transient->bytesToDecrypt)
		{
			transient->bytesToDecrypt = sz;
			return WOLFSSL_CBIO_ERR_WANT_READ;  //will cause wolfSSL_read return with the positive version of this error to CoreTransport
		}
		else //copy bytes from CoreTransport to WolfSSL for decryption
		{
			//fprintf(stderr, "CTWolfSSLRecv Callback: received %d bytes from CoreTransport\n", sz);
			memcpy(buff, transient->buffer + transient->totalBytesProcessed, sz);
			transient->totalBytesProcessed += sz;
			transient->bytesToDecrypt -= sz;
			return sz;
		}
	}
	//Receive a buffer of encrypted bytes by reading directly from the socket
	//else if ((recvd = WSARecv(sockfd, &wsaBuf, 1, sz, &flags, &ol, NULL)) == SOCKET_ERROR){
#ifdef _WIN32
	else if ((recvd = recv(sockfd, buff, sz, 0)) == SOCKET_ERROR) {
#else
	else if ((recvd = recv(sockfd, buff, sz, 0)) < 0) {
#endif
		//DWORD numBytes;
		//BOOL ok = GetOverlappedResult((HANDLE)sockfd, &ol, &numBytes, TRUE);
		//if (ok)
		//	fprintf(stderr, "WSARecv succeeded\n");

		//error encountered. Be responsible and report it in wolfSSL terms
		fprintf(stderr, stderr, "IO RECEIVE ERROR: ");

		int ret = CTSocketError();
		switch (ret){

#if EAGAIN != EWOULDBLOCK
		case EAGAIN: /* EAGAIN == EWOULDBLOCK on some systems, but not others */
#endif
		case EWOULDBLOCK:
			if (!wolfSSL_dtls(ssl) || wolfSSL_get_using_nonblock(ssl)) {
				fprintf(stderr, stderr, "would block\n");
				return WOLFSSL_CBIO_ERR_WANT_READ;
			}
			else {
				fprintf(stderr, stderr, "socket timeout\n");
				return WOLFSSL_CBIO_ERR_TIMEOUT;
			}
#ifdef _WIN32
		case WSAEWOULDBLOCK:
			assert(1 == 0);
#endif
		case ECONNRESET:
			fprintf(stderr, stderr, "connection reset\n");
			return WOLFSSL_CBIO_ERR_CONN_RST;
		case EINTR:
			fprintf(stderr, stderr, "socket interrupted\n");
			return WOLFSSL_CBIO_ERR_ISR;
		case ECONNREFUSED:
			fprintf(stderr, stderr, "connection refused\n");
			return WOLFSSL_CBIO_ERR_WANT_READ;
		case ECONNABORTED:
			fprintf(stderr, stderr, "connection aborted\n");
			return WOLFSSL_CBIO_ERR_CONN_CLOSE;
		default:
			fprintf(stderr, stderr, "general error\n");
			return WOLFSSL_CBIO_ERR_GENERAL;
		}
	}
	else if (recvd == 0) {
		fprintf(stderr, "Connection closed\n");
		return WOLFSSL_CBIO_ERR_CONN_CLOSE;
	}

	// successful receive
	fprintf(stderr, "CTWolfSSLRecv: received %d bytes from %d\n", sz, transient->socketfd);
	return recvd;
}


char* WolfSSLFirstMessageHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "WolfSSLFirstMessageHeaderLengthCallback headerLengthCallback...\n\n");

	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer;// +sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	cursor->contentLength = length;// ((ReqlQueryMessageHeader*)buffer)->length;

	//The cursor headerLength is calculated as follows after this function returns
	return endOfHeader;
}



void WolfSSLFirstMessageResponseCallback(CTError * err, CTCursor * cursor)
{
	fprintf(stderr, "WolfSSLFirstMessageResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

	/*
	int ret = 0;
	if ((ret = CTSSLHandshakeProcessFirstResponse(cursor, cursor->conn->socket, cursor->conn->sslContext)) != noErr)
	{
		if (ret == SSL_ERROR_WANT_READ)
		{
			//assert(1 == 0);
			return CTSuccess;
		}
		else if (ret == SSL_ERROR_WANT_WRITE)
		{
			//assert(1 == 0);
			return CTSuccess;
		}

		//ReqlSSLCertificateDestroy(&rootCertRef);
		fprintf(stderr, "CTSSLHandshake failed with status:  %d\n", ret);
		err->id = CTSSLHandshakeError;
		CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
		cursor->conn->target->callback(err, cursor->conn);
	}
	*/	
}


int CTWolfSSLSend(WOLFSSL* ssl, char* buff, int sz, void* ctx)
{
	/* By default, ctx will be a pointer to the file descriptor to write to.
	 * This can be changed by calling wolfSSL_SetIOWriteCtx(). */
	int sockfd = *(int*)ctx;
	CTSSLEncryptTransient* transient = (CTSSLEncryptTransient*)ctx;
	int sent;

	//fprintf(stderr, "CTWolfSSLSend: attempting to send %d bytes...\n", sz);

	//Send a buffer of bytes encrypted by WOLFSSL as a result of CoreTransport calling wolfssl_write
	
	if (sockfd > 0)//wolf_handshake_complete)
	{
		//fprintf(stderr, "CTWolfSSLSend: sent %d encrypted message bytes on CoreTransport provided socket %d\n", sz, sockfd);
		sent = send(sockfd, buff, sz, 0);
		if (sent != sz)
		{
			//fprintf(stderr, stderr, "IO SEND ERROR: ");
			switch (CTSocketError()) {
#if EAGAIN != EWOULDBLOCK
			case EAGAIN: // EAGAIN == EWOULDBLOCK on some systems, but not others
#endif
			case EWOULDBLOCK:
				//fprintf(stderr, stderr, "would block\n");
				return WOLFSSL_CBIO_ERR_WANT_WRITE;
			case ECONNRESET:
				//fprintf(stderr, stderr, "connection reset\n");
				return WOLFSSL_CBIO_ERR_CONN_RST;
			case EINTR:
				//fprintf(stderr, stderr, "socket interrupted\n");
				return WOLFSSL_CBIO_ERR_ISR;
			case EPIPE:
				//fprintf(stderr, stderr, "socket EPIPE\n");
				return WOLFSSL_CBIO_ERR_CONN_CLOSE;
			default:
				//fprintf(stderr, stderr, "general error\n");
				return WOLFSSL_CBIO_ERR_GENERAL;
			}

		}

		if (sent == 0) {
			fprintf(stderr, "Connection closed\n");
			return 0;
		}

		return sent;
	}

	else //if (transient->ct_socket_fd != 0)
	{
		//we are notifying WolfSSL that we asynchronously finished sending bytes
		if (transient->messageLen > 0)
		{
			int cpLen = transient->messageLen;
			transient->messageLen = 0;
			return cpLen;
		}
		transient->messageLen = sz;
		transient->messageBuffer = buff;

		return WOLFSSL_CBIO_ERR_WANT_WRITE;
	}
	/*
	else if ((sent = send(sockfd, buff, sz, 0)) == -1) {
		// error encountered. Be responsible and report it in wolfSSL terms 

		
		//fprintf(stderr, stderr, "IO SEND ERROR: ");
		switch (errno) {
#if EAGAIN != EWOULDBLOCK
		case EAGAIN: // EAGAIN == EWOULDBLOCK on some systems, but not others
#endif
		case EWOULDBLOCK:
			//fprintf(stderr, stderr, "would block\n");
			return WOLFSSL_CBIO_ERR_WANT_WRITE;
		case ECONNRESET:
			//fprintf(stderr, stderr, "connection reset\n");
			return WOLFSSL_CBIO_ERR_CONN_RST;
		case EINTR:
			//fprintf(stderr, stderr, "socket interrupted\n");
			return WOLFSSL_CBIO_ERR_ISR;
		case EPIPE:
			//fprintf(stderr, stderr, "socket EPIPE\n");
			return WOLFSSL_CBIO_ERR_CONN_CLOSE;
		default:
			//fprintf(stderr, stderr, "general error\n");
			return WOLFSSL_CBIO_ERR_GENERAL;
		}
		
	}
	else if (sent == 0) {
		fprintf(stderr, "Connection closed\n");
		return 0;
	}
	*/

	assert(1 == 0);
	/* successful send */
	//fprintf(stderr, "CTWolfSSLSend: sent %d handshake bytes on WolfSSL provided socket %d\n", sz, transient->wolf_socket_fd);
	return sent;
}

CTSSLStatus CTSSLDecryptMessage2(CTSSLContextRef sslContextRef, void* msg, unsigned long* msgLength, char** extraBuffer, unsigned long* extraBytes)
{
	CTSSLStatus scRet;
	int err = 0;
	char buffer[80];
	//int sockfd = wolfSSL_get_fd(sslContextRef->ctx);
	
	//WOLFSSL callbacks must be overridden to allow encrypt or decrypt without writing to/reading from socket, respectively
	//HOWEVER, we must still support traditional blocking send/recv calls in our callbacks initially to allow
	//WOLFSSL to handle the the ssl handshake via wolfSSL_connect()
	// 
	//so...we will key off the fact that we are passing a socket descriptor equal to 0 in the callback
	//to determine when to do decryption without allowing wolfssl to read from socket after the handshake has finished
	CTSSLDecryptTransient transient = { 0, *msgLength, 0, msg};

	//fprintf(stderr, "CTSSLDecryptMessage2::CoreTransport supplying (%lu) bytes to WolfSSL for decryption...\n", *msgLength);
	
	//Let WolfSSL Decrypt as many bytes from our encrypted input buffer ass possible in a single pass to wolfSSL_read
	wolfSSL_SetIOReadCtx(sslContextRef->ctx, &transient);
	scRet = wolfSSL_read(sslContextRef->ctx, (char*)msg /* + totalDecryptedBytes*/, *msgLength);//- totalDecryptedBytes);
	if (scRet <= 0) //WolfSSL returned error
	{		
		err = wolfSSL_get_error(sslContextRef->ctx, 0);
		wolfSSL_ERR_error_string(err, buffer);
		//fprintf(stderr, stderr, "CTSSLDecryptMessage2: wolfSSL failed to read (%d): \n\n%s\n\n", wolfSSL_get_error(sslContextRef->ctx, 0), buffer);

		//HANDLE INCOMPLETE MESSAGE: the buffer didn't have as many bytes as WolfSSL needed for decryption
		if (err == WOLFSSL_ERROR_WANT_READ )
		{
			//SCHANNEL IMPLEMENTATION FOR REFERENCE
			//*msgLength = hasData * Buffers[1].cbBuffer + hasMissing * Buffers[0].cbBuffer;
			//*extraBuffer = (char*)(hasExtra * (uintptr_t)Buffers[3].pvBuffer);
			//*extraBytes = (scRet != SEC_E_OK) * (*extraBytes) + hasExtra * Buffers[3].cbBuffer;

			//WOLFSSL ANALOG IMPLEMENTATION
			*msgLength = *msgLength - transient.totalBytesProcessed;// transient.bytesToDecrypt;// totalDecryptedBytes;
			*extraBytes = transient.bytesToDecrypt;// transient.bytesToDecrypt;
			*extraBuffer = (char*)msg + transient.totalBytesProcessed;// : NULL;
			scRet = SEC_E_INCOMPLETE_MESSAGE;
		}		
	}
	else //WolfSSL returned decrypted bytes
	{
		*extraBytes = *msgLength - transient.totalBytesProcessed;
		*extraBuffer = *extraBytes > 0 ? (char*)msg + transient.totalBytesProcessed : NULL;
		*msgLength = (unsigned long)scRet;
		scRet = SEC_E_OK;	//play nice with our existing SCHANNEL API
	}
	//wolfSSL_SetIOReadCtx(sslContextRef->ctx, &sockfd);
	return scRet;
}
#elif defined(_WIN32)
/***
 *	ReqlSSLDecrypt
 ***/
CTSSLStatus CTSSLDecryptMessage(CTSSLContextRef sslContextRef, void*msg, unsigned long *msgLength)
{
	int i;
	unsigned long length;
	CTSSLStatus scRet;				// unsigned long cbBuffer;    // Size of the buffer, in bytes
	SecBuffer          Buffers[4];		// void    SEC_FAR * pvBuffer;   // Pointer to the buffer
	SecBufferDesc      Message;			// unsigned long BufferType;  // Type of the buffer (below)
	SecBuffer          *pDataBuffer, *pExtraBuffer;
	PBYTE              buff;
	DWORD cbIoBuffer;
	/*
	PSecurityFunctionTable pSSPI = NULL;
	// Retrieve a data pointer for the current thread's security function table
	pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
	if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
	  fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/

	// Decrypt the received data.
	Buffers[0].pvBuffer = msg;
	Buffers[0].cbBuffer = *msgLength;
	Buffers[0].BufferType = SECBUFFER_DATA;  // Initial Type of the buffer 1
	Buffers[1].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 2
	Buffers[2].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 3
	Buffers[3].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 4


	Message.ulVersion = SECBUFFER_VERSION;    // Version number
	Message.cBuffers = 4;                                    // Number of buffers - must contain four SecBuffer structures.
	Message.pBuffers = Buffers;                        // Pointer to array of buffers
	
	*msgLength = 0;
	scRet = DecryptMessage(&(sslContextRef->hCtxt), &Message, 0, NULL);

	//if( scRet == SEC_I_CONTEXT_EXPIRED ) return scRet;//break; // Server signalled end of session
	//if( scRet == SEC_E_INCOMPLETE_MESSAGE - Input buffer has partial encrypted record, read more

	// Locate data and (optional) extra buffers.
	pDataBuffer = NULL;
	pExtraBuffer = NULL;
	for (i = 1; i < 4; i++)
	{
		if (pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA) pDataBuffer = &Buffers[i];
		if (pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA) pExtraBuffer = &Buffers[i];
	}

		//TO DO:  eliminate this branch
		//these are cases we don't consider decryption errors
	if (scRet != SEC_E_OK &&
		scRet != SEC_I_RENEGOTIATE &&
		scRet != SEC_I_CONTEXT_EXPIRED)
		//&& scRet != SEC_E_INCOMPLETE_MESSAGE)
	{
		//if( scRet == SEC_E_INCOMPLETE_MESSAGE)
		//{
			assert(!pDataBuffer);
			assert(!pExtraBuffer);
			//*msgLength = pExtraBuffer->cbBuffer;
			*msgLength = 0;
		//}
		fprintf(stderr, "**** DecryptMessage ");
		DisplaySECError((DWORD)scRet);
		return scRet;
	}

	

	// Display the decrypted data.
	if (pDataBuffer)
	{
		length = pDataBuffer->cbBuffer;
		//totalDecryptedBytes += (size_t)length;
		if (length) // check if last two chars are CR LF
		{
			buff = (PBYTE)pDataBuffer->pvBuffer; // fprintf(stderr,  "n-2= %d, n-1= %d \n", buff[length-2], buff[length-1] );
			*msgLength += length;
			fprintf(stderr, "Decrypted data: %d bytes", length); PrintText(length, buff);
			//if(fVerbose) { PrintHexDump(length, buff); fprintf(stderr, "\n"); }
			//if( buff[length-2] == 13 && buff[length-1] == 10 ) return CTSSLSuccess; // fprintf(stderr, "Found CRLF\n");
			//return ReqlSSLSuccess;
		}
	}

	// Move any "extra" data to the input buffer.
	if (pExtraBuffer)
	{
		fprintf(stderr, "\nReqlSSLDecryptMessage::Handling Extra Buffer Data (%d bytes)!!!\n", pExtraBuffer->cbBuffer);
		MoveMemory((char*)msg, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
		cbIoBuffer = pExtraBuffer->cbBuffer; // fprintf(stderr, "cbIoBuffer= %d  \n", cbIoBuffer);
		scRet = CTSSLDecryptMessage(sslContextRef, msg, &(cbIoBuffer));

			if (scRet != SEC_E_OK &&
		scRet != SEC_I_RENEGOTIATE &&
		scRet != SEC_I_CONTEXT_EXPIRED
		&& scRet != SEC_E_INCOMPLETE_MESSAGE)
		{
			//if( scRet == SEC_E_INCOMPLETE_MESSAGE)
			//	*msgLength = pExtraBuffer->cbBuffer;
			
			fprintf(stderr, "**** DecryptMessage 2");
			DisplaySECError((DWORD)scRet);
			return scRet;
		}

		//In order to avoid copying the message buffer for subsequent SCHANNEL decryption passes,
		//We are going add whitespace based on the connection's message protocol

		//memset( (char*)msg + sslContextRef->Sizes.cbHeader + *msgLength, ' ', sslContextRef->Sizes.cbHeader + sslContextRef->Sizes.cbTrailer - 1);
		//memcpy( (char*)msg + sslContextRef->Sizes.cbHeader + *msgLength + sslContextRef->Sizes.cbHeader + sslContextRef->Sizes.cbTrailer - 1, "\n", 1);

		//memcpy( (char*)msg + sslContextRef->Sizes.cbHeader + *msgLength - 2, "Padding: ", sizeof("Padding: "));
		//memset( (char*)msg + sslContextRef->Sizes.cbHeader + *msgLength, 'X', sslContextRef->Sizes.cbHeader + sslContextRef->Sizes.cbTrailer - sizeof("\r\n\r\n"));
		//memcpy( (char*)msg + sslContextRef->Sizes.cbHeader + *msgLength + sslContextRef->Sizes.cbHeader + sslContextRef->Sizes.cbTrailer - sizeof("\r\n\r\n"), "\r\n\r\n", sizeof("\r\n\r\n"));

		*msgLength += sslContextRef->Sizes.cbTrailer + sslContextRef->Sizes.cbHeader + cbIoBuffer;
		return scRet;
	}
	/*
	else if ( scRet == SEC_E_INCOMPLETE_MESSAGE )
	{

	}
	*/
	//else
	//  cbIoBuffer = 0;
	//return CTSSLSuccess;

	return scRet;
}

CTSSLStatus CTSSLDecryptMessage2(CTSSLContextRef sslContextRef, void*msg, unsigned long *msgLength, char **extraBuffer, unsigned long * extraBytes)
{
	int i;
	unsigned long length;
	CTSSLStatus scRet;				// unsigned long cbBuffer;    // Size of the buffer, in bytes
	SecBuffer* pDataBuffer, * pExtraBuffer;
	PBYTE              buff;
	DWORD cbIoBuffer;

	unsigned long hasData, hasExtra, hasMissing;

	SecBuffer          Buffers[4] = { {*msgLength, SECBUFFER_DATA, msg},0,0,0 };		// void    SEC_FAR * pvBuffer;   // Pointer to the buffer
	SecBufferDesc      Message = { SECBUFFER_VERSION, 4, Buffers };

	// Decrypt the received data.
	//Buffers[0].pvBuffer = msg;
	//Buffers[0].cbBuffer = *msgLength;
	//Buffers[0].BufferType = SECBUFFER_DATA;  // Initial Type of the buffer 1
	//Buffers[1].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 2
	//Buffers[2].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 3
	//Buffers[3].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 4
	//Message.ulVersion = SECBUFFER_VERSION;    // Version number
	//Message.cBuffers = 4;                                    // Number of buffers - must contain four SecBuffer structures.
	//Message.pBuffers = Buffers;                        // Pointer to array of buffers

	//*extraBytes = 0;
	//*extraBuffer = NULL;
	//*msgLength = 0;

	scRet = DecryptMessage(&(sslContextRef->hCtxt), &Message, 0, NULL);

#ifdef _DEBUGS
	if (scRet != SEC_E_OK && scRet != SEC_I_RENEGOTIATE && scRet != SEC_I_CONTEXT_EXPIRED)
	{
		fprintf(stderr, "**** DecryptMessage ");
		DisplaySECError((DWORD)scRet);
		//return scRet;
	}
#endif
	/*
	pDataBuffer = NULL;
	pExtraBuffer = NULL;
	for (i = 1; i < 4; i++)
	{
		if (pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA) pDataBuffer = &Buffers[i];
		if (pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA) pExtraBuffer = &Buffers[i];
	}


	*msgLength = 0;
	*extraBuffer = NULL;
	if( scRet  == SEC_E_OK || scRet == SEC_E_INCOMPLETE_MESSAGE )
	{

		if(pDataBuffer )
		{
			*msgLength = pDataBuffer->cbBuffer;
		}


		if( pExtraBuffer )
		{
			*extraBuffer = (char*)pExtraBuffer->pvBuffer;
			*extraBytes = pExtraBuffer->cbBuffer;
		}
	}
	*/

	hasMissing = (Buffers[0].BufferType == SECBUFFER_MISSING);
	hasData = (Buffers[1].BufferType == SECBUFFER_DATA);
	hasExtra = (Buffers[3].BufferType == SECBUFFER_EXTRA);

	if (scRet == SEC_E_INCOMPLETE_MESSAGE && Buffers[0].BufferType == SECBUFFER_MISSING)
	{
		//*extraBytes = Buffers[0].cbBuffer;
		fprintf(stderr, "CTSSLDecryptMessage2::SEC_E_INCOMPLETE_MESSAGE remaining bytes need to be read: %lu\n", Buffers[0].cbBuffer);
	}

	//*extraBytes = (*extraBytes) * !hasMissing + Buffers[0].cbBuffer * hasMissing;

	*msgLength = hasData * Buffers[1].cbBuffer + (scRet != SEC_E_OK) * (*msgLength);// +hasMissing * Buffers[0].cbBuffer;
	*extraBuffer = (char*)(hasExtra * (uintptr_t)Buffers[3].pvBuffer + !hasExtra * (uintptr_t)msg);
	*extraBytes = (scRet != SEC_E_OK) * (*extraBytes + hasMissing * Buffers[0].cbBuffer) + hasExtra * Buffers[3].cbBuffer;

	return scRet;
}

CTSSLStatus ReqlSSLDecryptMessageInSitu(CTSSLContextRef sslContextRef, void**msg, unsigned long *msgLength)
{
	int i;
	unsigned long length;
	CTSSLStatus scRet;				// unsigned long cbBuffer;    // Size of the buffer, in bytes
	SecBuffer          Buffers[4];		// void    SEC_FAR * pvBuffer;   // Pointer to the buffer
	SecBufferDesc      Message;			// unsigned long BufferType;  // Type of the buffer (below)
	SecBuffer          *pDataBuffer, *pExtraBuffer;
	PBYTE              buff;

	/*
	PSecurityFunctionTable pSSPI = NULL;
	// Retrieve a data pointer for the current thread's security function table
	pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
	if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
	  fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/

	// Decrypt the received data.
	Buffers[0].pvBuffer = *msg;
	Buffers[0].cbBuffer = *msgLength;
	Buffers[0].BufferType = SECBUFFER_DATA;  // Initial Type of the buffer 1
	Buffers[1].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 2
	Buffers[2].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 3
	Buffers[3].BufferType = SECBUFFER_EMPTY; // Initial Type of the buffer 4

	Message.ulVersion = SECBUFFER_VERSION;    // Version number
	Message.cBuffers = 4;                                    // Number of buffers - must contain four SecBuffer structures.
	Message.pBuffers = Buffers;                        // Pointer to array of buffers
	scRet = DecryptMessage(&(sslContextRef->hCtxt), &Message, 0, NULL);

	//if( scRet == SEC_I_CONTEXT_EXPIRED ) return scRet;//break; // Server signalled end of session
	//if( scRet == SEC_E_INCOMPLETE_MESSAGE - Input buffer has partial encrypted record, read more

		//TO DO:  eliminate this branch
		//these are cases we don't consider decryption errors
	if (scRet != SEC_E_OK &&
		scRet != SEC_I_RENEGOTIATE &&
		scRet != SEC_I_CONTEXT_EXPIRED)
	{
		fprintf(stderr, "**** DecryptMessage ");
		DisplaySECError((DWORD)scRet);
		return scRet;
	}

	// Locate data and (optional) extra buffers.
	pDataBuffer = NULL;
	pExtraBuffer = NULL;
	for (i = 1; i < 4; i++)
	{
		if (pDataBuffer == NULL && Buffers[i].BufferType == SECBUFFER_DATA) pDataBuffer = &Buffers[i];
		if (pExtraBuffer == NULL && Buffers[i].BufferType == SECBUFFER_EXTRA) pExtraBuffer = &Buffers[i];
	}

	// Display the decrypted data.
	if (pDataBuffer)
	{
		length = pDataBuffer->cbBuffer;
		//totalDecryptedBytes += (size_t)length;
		if (length) // check if last two chars are CR LF
		{
			buff = (PBYTE)pDataBuffer->pvBuffer; // fprintf(stderr,  "n-2= %d, n-1= %d \n", buff[length-2], buff[length-1] );
			*msgLength = length;
			fprintf(stderr, "Decrypted data: %d bytes", length); PrintText(length, buff);
			//if(fVerbose) { PrintHexDump(length, buff); fprintf(stderr, "\n"); }
			if( buff[length-2] == 13 && buff[length-1] == 10 ) return CTSSLSuccess; // fprintf(stderr, "Found CRLF\n");
			//return ReqlSSLSuccess;
		}
	}

	// Move any "extra" data to the input buffer.
	if (pExtraBuffer)
	{
		fprintf(stderr, "\nReqlSSLDecryptMessage::Handling Extra Buffer Data!!!\n");
		//Question:  Where does the pbBuffer memory live?  In the sslContext?
		//			 And, secondly, why does copying it back to the front of the msg buffer work?
		MoveMemory(*msg, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
		//cbIoBuffer = pExtraBuffer->cbBuffer; // fprintf(stderr, "cbIoBuffer= %d  \n", cbIoBuffer);
		return CTSSLDecryptMessage(sslContextRef, msg, &(pExtraBuffer->cbBuffer));
	}
	*msg = buff;
	//else
	//  cbIoBuffer = 0;
	return CTSSLSuccess;

	return scRet;
}



/***
 *	ReqlSSLEncryptMessage
 *
 *
 ***/
CTSSLStatus CTSSLEncryptMessage(CTSSLContextRef sslContextRef, void*msg, unsigned long * msgLength)
{
	CTSSLStatus scRet;				// unsigned long cbBuffer;    // Size of the buffer, in bytes

	//SECURITY_STATUS			scRet;            // unsigned long cbBuffer;    // Size of the buffer, in bytes
	SecBufferDesc			Message;        // unsigned long BufferType;  // Type of the buffer (below)
	SecBuffer               Buffers[4];    // void    SEC_FAR * pvBuffer;   // Pointer to the buffer
	DWORD                   cbMessage;//, cbData;
	//PBYTE                 pbHeader;//, pbMessage;
	//BYTE pbMessage[4096];
	size_t bufSize = sslContextRef->Sizes.cbHeader + sslContextRef->Sizes.cbTrailer;

	//pbMessage = (PBYTE)LocalAlloc(LMEM_FIXED, msgLength + bufSize);
	//memset(pbMessage, 0, msgLength + bufSize );
	//pbMessage = (PBYTE)((char*)msg + sslContextRef->Sizes.cbHeader); // Offset by "header size"
	cbMessage = (DWORD)*msgLength;//(DWORD)strlen((char*)pbMessage);
	//fprintf(stderr, "Sending %d bytes of plaintext:\n", cbMessage); PrintText(cbMessage, (PBYTE)msg);
	//if(fVerbose) { PrintHexDump(cbMessage, (PBYTE)msg); fprintf(stderr, "\n"); }

	//memcpy( pbMessage, msg, sslContextRef->Sizes.cbHeader);
	//memcpy( pbMessage + sslContextRef->Sizes.cbHeader, msg, msgLength);
	//memcpy( pbMessage + sslContextRef->Sizes.cbHeader + sslContextRef->Sizes.cbTrailer, msg, sslContextRef->Sizes.cbTrailer);
	//pbMessage = LocalAlloc(LMEM_FIXED, msgLength);


	memset(sslContextRef->hBuffer, 0, sslContextRef->Sizes.cbHeader);
	memset(sslContextRef->tBuffer, 0, sslContextRef->Sizes.cbTrailer);

	// Encrypt the HTTP request.
	Buffers[0].pvBuffer = sslContextRef->hBuffer;                                // Pointer to buffer 1
	Buffers[0].cbBuffer = sslContextRef->Sizes.cbHeader;                        // length of header
	Buffers[0].BufferType = SECBUFFER_STREAM_HEADER;    // Type of the buffer

	Buffers[1].pvBuffer = msg;                                // Pointer to buffer 2
	Buffers[1].cbBuffer = cbMessage;                                // length of the message
	Buffers[1].BufferType = SECBUFFER_DATA;                        // Type of the buffer

	Buffers[2].pvBuffer = sslContextRef->tBuffer;        // Pointer to buffer 3
	Buffers[2].cbBuffer = sslContextRef->Sizes.cbTrailer;                    // length of the trailor
	Buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;    // Type of the buffer

	Buffers[3].pvBuffer = SECBUFFER_EMPTY;                    // Pointer to buffer 4
	Buffers[3].cbBuffer = SECBUFFER_EMPTY;                    // length of buffer 4
	Buffers[3].BufferType = SECBUFFER_EMPTY;                    // Type of the buffer 4

	memset(&Message, 0, sizeof(SecBufferDesc));
	Message.ulVersion = SECBUFFER_VERSION;    // Version number
	Message.cBuffers = 4;                                    // Number of buffers - must contain four SecBuffer structures.
	Message.pBuffers = Buffers;                        // Pointer to array of buffers

	scRet = EncryptMessage(&(sslContextRef->hCtxt), 0, &Message, 0); // must contain four SecBuffer structures.
	*msgLength = Buffers[1].cbBuffer;
	if (FAILED(scRet)) { fprintf(stderr, "**** Error 0x%x returned by EncryptMessage\n", scRet); }
	return scRet;
}




#endif //_WIN32 SCHANNEL ENCRYPT, DECRYPT



SECURITY_STATUS CTSSLHandshakeProcessSecondResponse(CTCursor* cursor, CTSocket socketfd, CTSSLContextRef sslContextRef)
{
	int ret = 0;
	int err = 0;
	char buffer[80];
#ifdef CTRANSPORT_WOLFSSL

	CTClientError scRet;

	CTSSLDecryptTransient dTransient = { 0, cursor->headerLength + cursor->contentLength, 0, cursor->file.buffer };
	CTSSLEncryptTransient eTransient = { 0, 0, NULL };
	//CTSSLEncryptTransient* eTransient = wolfSSL_GetIOWriteCtx(sslContextRef->ctx);
	//assert(eTransient);

	//eTransient->wolf_socket_fd = 0;
	//eTransient->messageLen = 0;
	//eTransient->messageBuffer = NULL;

	fprintf(stderr, "CTSSLHandshakeProcessSecondResponse::CoreTransport supplying (%lu) handshake bytes to WolfSSL...\n", cursor->headerLength + cursor->contentLength);

	//we are finished sending the first response, reset the send context for the second client message after the recv
	wolfSSL_SetIOWriteCtx(cursor->conn->sslContext->ctx, &eTransient);

	//Let WolfSSL Decrypt as many bytes from our encrypted input buffer ass possible in a single pass to wolfSSL_read
	wolfSSL_SetIOReadCtx(sslContextRef->ctx, &dTransient);

	/* Connect to wolfSSL on the server side */
	if ((ret = wolfSSL_connect(sslContextRef->ctx)) != SSL_SUCCESS) {

		err = wolfSSL_get_error(sslContextRef->ctx, 0);
		wolfSSL_ERR_error_string(err, buffer);
		fprintf(stderr, "CTSSLHandshakeProcessSecondResponse: wolfSSL_connect failed to read (%d): \n\n%s\n\n", wolfSSL_get_error(sslContextRef->ctx, 0), buffer);

		//HANDLE INCOMPLETE MESSAGE: the buffer didn't have as many bytes as WolfSSL needed for decryption
		if (err == WOLFSSL_ERROR_WANT_READ)
		{
			//SCHANNEL IMPLEMENTATION FOR REFERENCE
			//*msgLength = hasData * Buffers[1].cbBuffer + hasMissing * Buffers[0].cbBuffer;
			//*extraBuffer = (char*)(hasExtra * (uintptr_t)Buffers[3].pvBuffer);
			//*extraBytes = (scRet != SEC_E_OK) * (*extraBytes) + hasExtra * Buffers[3].cbBuffer;

			//WOLFSSL ANALOG IMPLEMENTATION
			//*msgLength = *msgLength - transient.totalBytesProcessed;// transient.bytesToDecrypt;// totalDecryptedBytes;
			//*extraBytes = transient.bytesToDecrypt;// transient.bytesToDecrypt;
			//*extraBuffer = (char*)msg + transient.totalBytesProcessed;// : NULL;
			//scRet = SEC_E_INCOMPLETE_MESSAGE;

			//
			//	Handle INCOMPLETE ENCRYPTED MESSAGE AS FOLLOWS:
			// 
			//	1)  copy the remaining encrypted bytes to the start of our buffer so we can 
			//		append to them when receiving from socket on the next async iocp iteration
			//  
			//	2)  use NumBytesRecv var as an offset input parameter CTCursorAsyncRecv
			//
			//	3)  use NumByteRemaining var as input to CTCursorAsyncRecv to request num bytes to recv from socket
			//		(PrevBytesRecvd will be subtracted from NumBytesRemaining internal to CTAsyncRecv

			//For SCHANNEL:  CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer
			//For WolfSSL:   CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer + the size of the ssl header wolfssl already consumed

			CTOverlappedResponse* overlappedResponse = &(cursor->overlappedResponse);
			assert(overlappedResponse);

			//request exactly the amount to complete the message NumBytesRemaining 
			//populated by CTSSLDecryptMessage2 above or request more if desired
			unsigned long NumBytesRecv = cursor->headerLength + cursor->contentLength - dTransient.totalBytesProcessed;
			unsigned long NumBytesRemaining = dTransient.bytesToDecrypt - NumBytesRecv;   //request exactly the amount to complete the message NumBytesRemaining 

			assert(NumBytesRemaining > 0);
			if (dTransient.totalBytesProcessed > 0)
				memcpy((char*)cursor->file.buffer, cursor->file.buffer + dTransient.totalBytesProcessed, NumBytesRecv);


			overlappedResponse->buf = (char*)(cursor->file.buffer);
			//overlappedResponse->len = sslFirstMessageLen;

			cursor->headerLength = 0;
			cursor->contentLength = 0;// transient.bytesToDecrypt;
			NumBytesRemaining = 32768;// NumBytesRecv + NumBytesRemaining;

			if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), cursor->file.buffer, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)
			{
				//the async operation returned immediately
				if (scRet == CTSuccess)
				{
					fprintf(stderr, "CTSSLHandshakeProcessSecondResponse::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
					//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
				}
				else //failure
				{
					fprintf(stderr, "CTSSLHandshakeProcessSecondResponse::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
					assert(1 == 0);
				}
			}
			return SSL_ERROR_WANT_READ;// CTSuccess;
		}
		else if (err == SSL_ERROR_WANT_WRITE)
		{
			assert(1 == 0);
			return SSL_ERROR_WANT_WRITE;
		}
		/*
		err = wolfSSL_get_error(sslContextRef->ctx, ret);
		if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
		{
			fprintf(stderr, stderr, "ERROR: wolfSSL handshake failed!\n");
		}
		return err;
		*/
	}
	//wolf_handshake_complete = 1;
#elif defined(_WIN32)
	SecBufferDesc   OutBuffer, InBuffer;
	SecBuffer       InBuffers[2], OutBuffers[1];
	DWORD           dwSSPIFlags, dwSSPIOutFlags, cbData, cbIoBuffer;
	//TimeStamp       tsExpiry;
	SECURITY_STATUS scRet;
	PUCHAR          IoBuffer;
	BOOL            fDoRead;
	/*
	PSecurityFunctionTable pSSPI = NULL;
	// Retrieve a data pointer for the current thread's security function table
	pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
	if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
	  fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/

	dwSSPIFlags = ISC_REQ_CONFIDENTIALITY | ISC_REQ_STREAM | ISC_REQ_ALLOCATE_MEMORY   /*   | ISC_REQ_REPLAY_DETECT     | ISC_REQ_CONFIDENTIALITY   |
				  ISC_RET_EXTENDED_ERROR    | | ISC_REQ_STREAM */;

				  // Allocate data buffer.
	IoBuffer = cursor->file.buffer;// (PUCHAR)LocalAlloc(LMEM_FIXED, IO_BUFFER_SIZE);
	if (IoBuffer == NULL) { fprintf(stderr, "**** Out of memory (1)\n"); return SEC_E_INTERNAL_ERROR; }
	cbIoBuffer = cursor->headerLength + cursor->contentLength;
	fDoRead = 1;//fDoInitialRead;

	// Loop until the handshake is finished or an error occurs.
	scRet = SEC_I_CONTINUE_NEEDED;

	while (scRet == SEC_I_CONTINUE_NEEDED ||
		scRet == SEC_E_INCOMPLETE_MESSAGE ||
		scRet == SEC_I_INCOMPLETE_CREDENTIALS)
	{

		/*
		if (0 == cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE) // Read data from server.
		{
			if (fDoRead)
			{
				cbData = recv(socketfd, (char*)IoBuffer + cbIoBuffer, IO_BUFFER_SIZE - cbIoBuffer, 0);
				if (cbData == SOCKET_ERROR)
				{
					fprintf(stderr, "**** Error %d reading data from server\n", WSAGetLastError());
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				else if (cbData == 0)
				{
					fprintf(stderr, "**** Server unexpectedly disconnected\n");
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				fprintf(stderr, "%d bytes of handshake data received\n", cbData);
				if (fVerbose) { PrintHexDump(cbData, IoBuffer + cbIoBuffer); fprintf(stderr, "\n"); }
				cbIoBuffer += cbData;
			}
			else
				fDoRead = TRUE;
		}
		*/

		// Set up the input buffers. Buffer 0 is used to pass in data
		// received from the server. Schannel will consume some or all
		// of this. Leftover data (if any) will be placed in buffer 1 and
		// given a buffer type of SECBUFFER_EXTRA.
		InBuffers[0].pvBuffer = IoBuffer;
		InBuffers[0].cbBuffer = cbIoBuffer;
		InBuffers[0].BufferType = SECBUFFER_TOKEN;

		InBuffers[1].pvBuffer = NULL;
		InBuffers[1].cbBuffer = 0;
		InBuffers[1].BufferType = SECBUFFER_EMPTY;

		InBuffer.cBuffers = 2;
		InBuffer.pBuffers = InBuffers;
		InBuffer.ulVersion = SECBUFFER_VERSION;


		// Set up the output buffers. These are initialized to NULL
		// so as to make it less likely we'll attempt to free random
		// garbage later.
		OutBuffers[0].pvBuffer = NULL;
		OutBuffers[0].BufferType = SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;


		// Call InitializeSecurityContext.
		scRet = InitializeSecurityContextA(&(sslContextRef->hCred),
			&(sslContextRef->hCtxt),
			(SEC_CHAR*)(cursor->conn->target->url.host),
			dwSSPIFlags,
			0,
			SECURITY_NATIVE_DREP,
			&InBuffer,
			0,
			&(sslContextRef->hCtxt),
			&OutBuffer,
			&dwSSPIOutFlags,
			NULL);


		if (scRet == SEC_E_OK ||
			scRet == SEC_I_CONTINUE_NEEDED ||
			FAILED(scRet) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR))
		{
			// Check for fatal error.
			//if (FAILED(scRet)) { fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet); }
			fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet);

			assert((LONG)scRet != 2148074278);

			//assert(scRet != SEC_I_CONTINUE_NEEDED);
			/*
			if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
			{
				cbData = send(socketfd, (const char*)OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
				if (cbData == SOCKET_ERROR || cbData == 0)
				{
					fprintf(stderr, "**** Error %d sending data to server (2)\n", WSAGetLastError());
					FreeContextBuffer(OutBuffers[0].pvBuffer);
					DeleteSecurityContext(&(sslContextRef->hCtxt));
					return SEC_E_INTERNAL_ERROR;
				}
				fprintf(stderr, "%d bytes of handshake data sent\n", cbData);
				if (fVerbose) { PrintHexDump(cbData, (PBYTE)OutBuffers[0].pvBuffer); fprintf(stderr, "\n"); }

				// Free output buffer.
				FreeContextBuffer(OutBuffers[0].pvBuffer);
				OutBuffers[0].pvBuffer = NULL;
			}
			*/
			if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
				break;

			if (scRet == SEC_I_CONTINUE_NEEDED && InBuffers[1].BufferType == SECBUFFER_EXTRA)
			{
				// Copy any leftover data from the "extra" buffer, and go around again.
				//if (InBuffers[1].BufferType == SECBUFFER_EXTRA)
				//{
				MoveMemory(IoBuffer, IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);
				cbIoBuffer = InBuffers[1].cbBuffer;
				//}
				//else
				//	cbIoBuffer = 0;

				continue;
			}
			else if (scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_CONTINUE_NEEDED)
			{

				/*
				cursor->headerLengthCallback = SSLFirstMessageHeaderLengthCallback;
				cursor->responseCallback = SSLFirstMessageResponseCallback;

				cursor->file.buffer = cursor->requestBuffer;
				cursor->overlappedResponse.buf = (char*)(sslFirstMessageBuffer);
				handshakeCursor->overlappedResponse.len = sslFirstMessageLen;

				handshakeCursor->conn = conn; //assume conn is permanent memory from core transport connection pool
				handshakeCursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to bypass encrypt bc sthis is the ssl handshake
				assert(handshakeCursor->overlappedResponse.buf);
				CTCursorSendOnQueue(handshakeCursor, (char**)&(handshakeCursor->overlappedResponse.buf), handshakeCursor->overlappedResponse.len);
				return CTSuccess;
				*/

				CTOverlappedResponse* olResponse = &(cursor->overlappedResponse);
				unsigned long NumBytesRemaining = ct_system_allocation_granularity();// 32768;   //request exactly the amount to complete the message NumBytesRemaining 
								   //populated by CTSSLDecryptMessage2 above or request more if desired

				//char* buffer = olResponse->buf;

				//if (scRet == SEC_E_INCOMPLETE_MESSAGE)
				//	NumBytesRemaining -= cbIoBuffer;
				if (scRet == SEC_I_CONTINUE_NEEDED)
					cbIoBuffer = 0;
				//buffer = cursor->file.buffer;;

			//fprintf(stderr, " CTSSLHandhsakeProcessFirstResponse:: recvd %d bytes of handshake data \n\n", cursor->overlappedResponse.buf - cursor->file.buffer);
				if ((ret = CTCursorAsyncRecv(&olResponse, cursor->file.buffer, cbIoBuffer, &NumBytesRemaining)) != CTSocketIOPending)
				{
					//the async operation returned immediately
					if (ret == CTSuccess)
					{
						//fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
						//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
					}
					else //failure
					{
						fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
						break;
					}

				}
				return scRet;

			}

		}

		// If InitializeSecurityContext returned SEC_E_INCOMPLETE_MESSAGE,
		// then we need to read more data from the server and try again.
		if (scRet == SEC_E_INCOMPLETE_MESSAGE) { fprintf(stderr, "SEC_E_INCOMPLETE_MESSAGE"); continue; }


		// If InitializeSecurityContext returned SEC_E_OK, then the
		// handshake completed successfully.
		if (scRet == SEC_E_OK)
		{
			// If the "extra" buffer contains data, this is encrypted application
			// protocol layer stuff. It needs to be saved. The application layer
			// will later decrypt it with DecryptMessage.
			fprintf(stderr, "Handshake was successful\n");

			if (InBuffers[1].BufferType == SECBUFFER_EXTRA)
			{
				sslContextRef->ExtraData.pvBuffer = LocalAlloc(LMEM_FIXED, InBuffers[1].cbBuffer);
				if (sslContextRef->ExtraData.pvBuffer == NULL) { fprintf(stderr, "**** Out of memory (2)\n"); return SEC_E_INTERNAL_ERROR; }

				MoveMemory(sslContextRef->ExtraData.pvBuffer,
					IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer),
					InBuffers[1].cbBuffer);

				sslContextRef->ExtraData.cbBuffer = InBuffers[1].cbBuffer;
				sslContextRef->ExtraData.BufferType = SECBUFFER_TOKEN;

				fprintf(stderr, "%d bytes of app data was bundled with handshake data\n", sslContextRef->ExtraData.cbBuffer);
			}
			else
			{
				sslContextRef->ExtraData.pvBuffer = NULL;
				sslContextRef->ExtraData.cbBuffer = 0;
				sslContextRef->ExtraData.BufferType = SECBUFFER_EMPTY;
			}
			break; // Bail out to quit
		}



		// Check for fatal error.
		if (FAILED(scRet)) { fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet); break; }

		// If InitializeSecurityContext returned SEC_I_INCOMPLETE_CREDENTIALS,
		// then the server just requested client authentication.
		if (scRet == SEC_I_INCOMPLETE_CREDENTIALS)
		{
			// Busted. The server has requested client authentication and
			// the credential we supplied didn't contain a client certificate.
			// This function will read the list of trusted certificate
			// authorities ("issuers") that was received from the server
			// and attempt to find a suitable client certificate that
			// was issued by one of these. If this function is successful,
			// then we will connect using the new certificate. Otherwise,
			// we will attempt to connect anonymously (using our current credentials).
			GetNewClientCredentials(&(sslContextRef->hCred), &(sslContextRef->hCtxt));

			// Go around again.
			fDoRead = FALSE;
			scRet = SEC_I_CONTINUE_NEEDED;
			continue;
		}

		// Copy any leftover data from the "extra" buffer, and go around again.
		if (InBuffers[1].BufferType == SECBUFFER_EXTRA)
		{
			MoveMemory(IoBuffer, IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);
			cbIoBuffer = InBuffers[1].cbBuffer;
		}
		else
			cbIoBuffer = 0;
	}

	//LocalFree(IoBuffer); 	//This buffer does not need to be explicitly freed in async pipeline?  
	// Delete the security context in the case of a fatal error.
	if (FAILED(scRet))
	{
		fprintf(stderr, "ReqlSSLhandshake::Deleting Securitiy Context\n");
		DeleteSecurityContext(&(sslContextRef->hCtxt)); return scRet;
	}
#endif
	return SEC_E_OK;
}


char* SSLSecondMessageHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "SSLSecondMessageHeaderLengthCallback headerLengthCallback...\n\n");

	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer;// +sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	cursor->contentLength = length;// ((ReqlQueryMessageHeader*)buffer)->length;

	//The cursor headerLength is calculated as follows after this function returns
	return endOfHeader;
}

//void SSLSecondMessageResponseCallback(CTError* err, CTCursor* cursor) {
CTCursorCompletionClosure SSLSecondMessageResponseCallback = ^ void(CTError * err, CTCursor * cursor){

	fprintf(stderr, "SSLSecondMessageResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

	SECURITY_STATUS ret = 0;
	if ((ret = CTSSLHandshakeProcessSecondResponse(cursor, cursor->conn->socket, cursor->conn->sslContext)) != SEC_E_OK)
	{
#ifdef CTRANSPORT_WOLFSSL

		if (ret == SSL_ERROR_WANT_READ)
		{
			//assert(1 == 0);
			return;// CTSuccess;
		}
		else if (ret == SSL_ERROR_WANT_WRITE)
		{
			assert(1 == 0);
			return;// CTSuccess;
		}
#elif defined( _WIN32 )
		if (ret == SEC_E_INCOMPLETE_MESSAGE || ret == SEC_I_CONTINUE_NEEDED)
			return;// CTSuccess;
#endif


		fprintf(stderr, "SSLSecondMessageResponseCallback::CTSSLHandshakeProcessSecondResponse failed with status:  %d\n", ret);
		err->id = CTSSLHandshakeError;
		CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
	}

	if ((ret = CTVerifyServerCertificate(cursor->conn->sslContext, cursor->conn->target->url.host)) != noErr)
	{
		//ReqlSSLCertificateDestroy(&rootCertRef);
		fprintf(stderr, "CTVerifyServerCertificate with status:  %d\n", ret);
		CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
		return;// CTSSLHandshakeError;
	}

	cursor->conn->responseCount = 0;  //we incremented this for the handshake, it iscritical to reset this for the client before returning the connection
	cursor->conn->queryCount = 0;
	CT_CURSOR_INDEX = 0;

	cursor->conn->target->callback(err, cursor->conn);
};


//void SSLSecondMessageQueryCallback(CTError* err, CTCursor* cursor)
CTCursorCompletionClosure SSLSecondMessageQueryCallback = ^ void(CTError * err, CTCursor * cursor) {

	int ret = 0;
	int wolfErr = 0;
	char buffer[80];
	CTClientError scRet;

	fprintf(stderr, "SSLSecondMessageQueryCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

#ifdef CTRANSPORT_WOLFSSL

	CTSSLDecryptTransient dTransient = { 0, cursor->headerLength + cursor->contentLength, 0, cursor->file.buffer };
	CTSSLEncryptTransient* eTransient = NULL;// { cursor->conn->socket, 0, NULL, 0 };

	fprintf(stderr, "SSLSecondMessageQueryCallback::CoreTransport supplying (%lu) handshake bytes to WolfSSL...\n", cursor->headerLength + cursor->contentLength);

	//we are finished sending the first response, reset the send context for the second client message after the recv

	//Let WolfSSL Decrypt as many bytes from our encrypted input buffer ass possible in a single pass to wolfSSL_read
	wolfSSL_SetIOReadCtx(cursor->conn->sslContext->ctx, &dTransient);


	/* Connect to wolfSSL on the server side */
	while ((ret = wolfSSL_connect(cursor->conn->sslContext->ctx)) != SSL_SUCCESS) {

		wolfErr = wolfSSL_get_error(cursor->conn->sslContext->ctx, 0);
		wolfSSL_ERR_error_string(wolfErr, buffer);
		//fprintf(stderr, stderr, "SSLSecondMessageQueryCallback: wolfSSL_connect failed to send/read handshake data (%d): \n\n%s\n\n", wolfSSL_get_error(cursor->conn->sslContext->ctx, 0), buffer);

		//HANDLE INCOMPLETE MESSAGE: the buffer didn't have as many bytes as WolfSSL needed for decryption
		if (wolfErr == WOLFSSL_ERROR_WANT_READ)
		{
			//assert(1 == 0);
			//eTransient = (CTSSLEncryptTransient*)wolfSSL_GetIOWriteCtx(cursor->conn->sslContext->ctx);
			//eTransient->ct_socket_fd = cursor->conn->socket;
			wolfSSL_SetIOWriteCtx(cursor->conn->sslContext->ctx, &(cursor->conn->socket));
			return;// CTSuccess;
		}
		else if (wolfErr == SSL_ERROR_WANT_WRITE)
		{
			//wolfSSL_SetIOWriteCtx(cursor->conn->sslContext->ctx, &eTransient);
			//assert(1 == 0);

			eTransient = (CTSSLEncryptTransient*)wolfSSL_GetIOWriteCtx(cursor->conn->sslContext->ctx);
			eTransient->wolf_socket_fd = cursor->conn->socket;
			//eTransient->ct_socket_fd = 0;
			/*
			assert(eTransient && eTransient->messageLen == 6);
			//the second client handshake message
			cursor->headerLengthCallback = SSLSecondMessageHeaderLengthCallback;
			cursor->responseCallback = SSLSecondMessageResponseCallback;
			cursor->queryCallback = SSLSecondMessageQueryCallback;
			cursor->contentLength = cursor->headerLength = 0;
			cursor->conn->responseCount = 0;
			cursor->file.buffer = cursor->requestBuffer;
			cursor->overlappedResponse.buf = eTransient->messageBuffer;// (char*)(OutBuffers[0].pvBuffer);
			cursor->overlappedResponse.len = eTransient->messageLen;//(char*)(OutBuffers[0].cbBuffer);
			cursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to send but bypass encrypt bc this is the ssl handshake
			assert(cursor->overlappedResponse.buf);
			CTCursorSendOnQueue(cursor, (char**)&(cursor->overlappedResponse.buf), cursor->overlappedResponse.len);
			return CTSuccess;
			*/
			continue;

		}
		/*
		err = wolfSSL_get_error(sslContextRef->ctx, ret);
		if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
		{
			fprintf(stderr, stderr, "ERROR: wolfSSL handshake failed!\n");
		}
		return err;
		*/
	}
	//wolf_handshake_complete = 1;
#elif defined(_WIN32)

	BOOL    fVerbose = FALSE; // FALSE; // TRUE;
	//if (fVerbose) { PrintHexDump(cbData, (PBYTE)(overlappedRequest->buf)); fprintf(stderr, "\n"); }

	//always free the handshake buffer
	FreeContextBuffer((PVOID)cursor->overlappedResponse.buf);// overlappedRequest->buf);

	//the error or the amount of data read was placed in the error.id
	//if (cbData == SOCKET_ERROR || cbData == 0)
	if (err->id == SOCKET_ERROR || err->id == 0)
	{
		//FreeContextBuffer((PVOID)cursor->overlappedResponse.buf);// overlappedRequest->buf);
		DeleteSecurityContext(&(cursor->conn->sslContext->hCtxt));

		free(cursor->conn->sslContext);
		//return;//SEC_E_INTERNAL_ERROR;
	}

	return;
#endif
};




/*
char* SSLFirstMessageHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	fprintf(stderr, "SSLFirstMessageResponseCallback headerLengthCallback...\n\n");

	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer;// +sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	cursor->contentLength = length;// ((ReqlQueryMessageHeader*)buffer)->length;

	//The cursor headerLength is calculated as follows after this function returns
	return endOfHeader;
}

void SSLFirstMessageResponseCallback(CTError* err, CTCursor* cursor)
{
	fprintf(stderr, "SSLFirstMessageResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	int ret = 0;
	if ((ret = CTSSLHandshakeProcessFirstResponse(cursor, cursor->conn->socket, cursor->conn->sslContext)) != noErr)
	{
		//ReqlSSLCertificateDestroy(&rootCertRef);
		fprintf(stderr, "CTSSLHandshake with status:  %d\n", ret);
		err->id = CTSSLHandshakeError;
		CTCloseSSLSocket(cursor->conn->sslContext, cursor->conn->socket);
		cursor->conn->target->callback(err, cursor->conn);
	}
}
*/

void CTSSLHandshakeSendFirstMessage(CTSocket socketfd, CTSSLContextRef sslContextRef, void* firstMessageBuffer, int firstMessageLen)
{
#ifdef CTRANSPORT_WOLFSSL
	assert(1 == 0);
#elif defined(_WIN32) //SCHANNEL
	/*
	if (txQueue)
	{
		memset(&CTHandshakeCursor, 0, sizeof(CTCursor));

		CTHandshakeCursor.headerLengthCallback = SSLFirstMessageHeaderLengthCallback;
		CTHandshakeCursor.responseCallback = SSLFirstMessageResponseCallback;

		CTHandshakeCursor.file.buffer = CTHandshakeCursor.requestBuffer;
		CTHandshakeCursor.overlappedResponse.buf = (char*)(OutBuffers[0].pvBuffer);
		CTHandshakeCursor.overlappedResponse.len = (char*)(OutBuffers[0].cbBuffer);

		//CTHandshakeCursor.queryToken = 4;
	}
	else
	{
	*/
		DWORD cbData = send(socketfd, (char*)firstMessageBuffer, firstMessageLen, 0);
		if (cbData == SOCKET_ERROR || cbData == 0)
		{
			fprintf(stderr, "**** Error %d sending data to server (1)\n", WSAGetLastError());
			FreeContextBuffer(firstMessageBuffer);
			DeleteSecurityContext(&(sslContextRef->hCtxt));

			free(sslContextRef);
			return;// NULL;//SEC_E_INTERNAL_ERROR;
		}
		fprintf(stderr, "%d bytes of handshake data sent\n", cbData);
		if (fVerbose) { PrintHexDump(cbData, (PBYTE)(firstMessageBuffer)); fprintf(stderr, "\n"); }
		FreeContextBuffer(firstMessageBuffer); // Free output buffer.
		firstMessageBuffer = NULL;
	//}
#endif
}

/***
 *  CTSSLContextCreate
 *
 *
 *
 ***/
CTSSLContextRef CTSSLContextCreate(CTSocket *socketfd, CTSecCertificateRef * certRef, const char * serverName, void ** firstMessage, int * firstMessageLen)
{
	//OSStatus status;
	CTSSLContextRef sslContextRef = NULL;// = {0};

#ifdef CTRANSPORT_WOLFSSL

	sslContextRef = (CTSSLContextRef)malloc(sizeof(CTSSLContext));
	memset(sslContextRef, 0, sizeof(CTSSLContext));

	wolfSSL_Debugging_ON();
	/* Create and initialize WOLFSSL_CTX */
	if ((sslContextRef->conf = wolfSSL_CTX_new(wolfTLSv1_2_client_method())) == NULL) {
		fprintf(stderr, stderr, "ERROR: failed to create WOLFSSL_CTX\n");
		assert(sslContextRef->conf);
	}

	//Load client certificates into WOLFSSL_CTX
	if (certRef && strlen(certRef) > 0 && wolfSSL_CTX_load_verify_locations(sslContextRef->conf, certRef, NULL) != WOLFSSL_SUCCESS)
	{
		fprintf(stderr, stderr, "ERROR: failed to load %s, please check the file.\n", certRef);
		assert(1 == 0);
	}

	//if( certRef == NULL )
	wolfSSL_CTX_set_verify(sslContextRef->conf, WOLFSSL_VERIFY_NONE, NULL);

	wolfSSL_SetIOSend(sslContextRef->conf, CTWolfSSLSend);
	wolfSSL_SetIORecv(sslContextRef->conf, CTWolfSSLRecv);

	// Create a WOLFSSL object
	if ((sslContextRef->ctx = wolfSSL_new(sslContextRef->conf)) == NULL) {
		fprintf(stderr, stderr, "ERROR: failed to create WOLFSSL object\n");
		assert(sslContextRef->ctx);
	}

	// Attach wolfSSL to the socket
	wolfSSL_set_fd(sslContextRef->ctx, *socketfd);

	/* make wolfSSL object nonblocking */
	wolfSSL_set_using_nonblock(sslContextRef->ctx, 1);
	/*
	fprintf(stderr, "creating new WOLFSSL BIO type to write and read with\n");
	if( (sslContextRef->bio = wolfSSL_BIO_new(wolfSSL_BIO_f_ssl())) == NULL)
	{
		assert(sslContextRef->bio);
	}
	assert(sslContextRef->bio);

	wolfSSL_BIO_set_ssl(sslContextRef->bio, sslContextRef->ctx, 0); // with BIO_CLOSE wolfSSL_BIO_free will free ssl
	*/
#elif defined(CTRANSPORT_USE_MBED_TLS)
	int ret;
	//mbedtls_entropy_context entropy;
	//mbedtls_ctr_drbg_context ctr_drbg;
	//mbedtls_ssl_context ssl;
	//mbedtls_ssl_config conf;
	//mbedtls_x509_crt cacert;

	const char *pers = "ssl_client1";

#if defined(MBEDTLS_DEBUG_C)
	mbedtls_debug_set_threshold( 1 );
#endif

	//0. Initialize the RNG and the session data
	//mbedtls_net_init(&server_fd);

	//allocate memory for the tls object
	sslContextRef = (CTSSLContextRef)malloc(sizeof(CTSSLContext));
	memset(sslContextRef, 0, sizeof(CTSSLContext));
	//initialize mbedtls contexts

	//mbedtls_x509_crt_init(&cacert);

	mbedtls_ssl_init(&(sslContextRef->ctx));
	mbedtls_ssl_config_init(&(sslContextRef->conf));

	mbedtls_ctr_drbg_init(&(sslContextRef->ctr_drbg));
	mbedtls_entropy_init(&(sslContextRef->entropy));

	if ((ret = mbedtls_ctr_drbg_seed(&(sslContextRef->ctr_drbg), mbedtls_entropy_func, &(sslContextRef->entropy),
		(const unsigned char *)pers,
		strlen(pers))) != 0)
	{
		Reql_fprintf(stderr, "ReqlSSLContextCreate::mbedtls_ctr_drbg_seed failed and returned %d\n\n", ret);
		free(sslContextRef);
		return NULL;
	}

	//OutputDebugStringA("  . Setting up the SSL/TLS structure...");	
	if ((ret = mbedtls_ssl_config_defaults(&(sslContextRef->conf),
		MBEDTLS_SSL_IS_CLIENT,
		MBEDTLS_SSL_TRANSPORT_STREAM,
		MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
	{
		Reql_fprintf(stderr, "ReqlSSLContextCreate::mbedtls_ssl_config_defaults failed and returned %d\n\n", ret);
		free(sslContextRef);
		return NULL;
	}

	//mbedtls_ssl_conf_authmode(&(sslContextRef->conf), MBEDTLS_SSL_VERIFY_NONE);
	//  OPTIONAL is not optimal for security,
	//but makes interop easier in this simplified example
	//mbedtls_net_set_nonblock((mbedtls_net_context*)socketfd);
	mbedtls_ssl_conf_authmode(&(sslContextRef->conf), MBEDTLS_SSL_VERIFY_NONE);
	mbedtls_ssl_conf_ca_chain(&(sslContextRef->conf), *certRef, NULL);
	mbedtls_ssl_conf_rng(&(sslContextRef->conf), mbedtls_ctr_drbg_random, &(sslContextRef->ctr_drbg));
	mbedtls_ssl_conf_dbg(&(sslContextRef->conf), ReqlMBEDTLSDebug, stdout);

	if ((ret = mbedtls_ssl_setup(&(sslContextRef->ctx), &(sslContextRef->conf))) != 0)
	{
		Reql_fprintf(stderr, "ReqlSSLContextCreate::mbedtls_ssl_setup failed and returned %d\n\n", ret);
		free(sslContextRef);
		return NULL;
	}

	//mbedtls doesn't seeem to be propogating this
	//sslContextRef->ctx.conf = &(sslContextRef->conf);

	if ((ret = mbedtls_ssl_set_hostname(&(sslContextRef->ctx), serverName)) != 0)
	{
		Reql_fprintf(stderr, "ReqlSSLContextCreate::mbedtls_ssl_set_hostname failed and returned %d\n\n", ret);
		free(sslContextRef);
		return NULL;
	}

	//setup the sslread and sslwrite callbacks
	mbedtls_ssl_set_bio(&(sslContextRef->ctx), (void*)*socketfd, mbedWriteToSocket, mbedReadFromSocket, NULL);

	/*
	while ((ret = mbedtls_ssl_handshake(&(sslContextRef->ctx))) != 0)
	{
		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			fprintf(stderr, " failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", ret);
			return -1;
		}
	}

	OutputDebugStringA(" ok\n");


	//5. Verify the server certificate
	OutputDebugStringA("  . Verifying peer X.509 certificate...");
	uint32_t flags = 0;
	//In real life, we probably want to bail out when ret != 0
	if ((flags = mbedtls_ssl_get_verify_result(&(sslContextRef->ctx))) != 0)
	{
		char vrfy_buf[4096];

		OutputDebugStringA(" failed\n");

		mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

		OutputDebugStringA(vrfy_buf);
		OutputDebugStringA("\n");
	}
	else
		OutputDebugStringA(" ok\n");
	*/
	/*
	char REQL_MAGIC_NUMBER_BUF[5] = "\xc3\xbd\xc2\x34";

	int errOrBytesWritten = mbedtls_ssl_write((sslContextRef), (const unsigned char *)REQL_MAGIC_NUMBER_BUF, (size_t)4);
	if (errOrBytesWritten < 0) {
		if (errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_READ &&
			errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_WRITE) {

			fprintf(stderr, "ReqlSSLWrite::mbedtls_ssl_write = %d\n", errOrBytesWritten);

		}
	}
	fprintf(stderr, "ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);
	//fprintf(stderr, "ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);

	system("pause");
	*/

	fprintf(stderr, "Socket = %d\n", socketfd);
	fprintf(stderr, "sslContextRef = %llu\n", sslContextRef);

	assert(sslContextRef == &(sslContextRef->ctx));
	return sslContextRef;
#elif defined(_WIN32)	//SCHANNEL

	SecBufferDesc   OutBuffer;
	SecBuffer       OutBuffers[1];
	DWORD           dwSSPIFlags, dwSSPIOutFlags, cbData;
	SECURITY_STATUS ss;
	TimeStamp     tsExpiry = { 0 };
	//PSecurityFunctionTable pSSPI = NULL;
	//PCtxtHandle pCtxHandle = NULL;
	dwSSPIFlags =  ISC_REQ_STREAM | ISC_REQ_ALLOCATE_MEMORY;
	dwSSPIOutFlags = 0;

	//  Initiate a ClientHello message and generate a token.
	OutBuffers[0].pvBuffer = NULL;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer = 0;

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	/*
	// Retrieve a data pointer for the current thread's security function table
	pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
	if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
	  fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/
	//Apple SSLCreateContext allocates memory for the ReqlSSLContextRef (SSLContextRef) object for us
	//But on on WIN32 (since we are trying to recreate Apple behavior) we must allocate the pointer ourselves
	if (!(sslContextRef = (CTSSLContextRef)malloc(sizeof(CTSSLContext))))
	{
		fprintf(stderr, "ReqlSSLContextRef Memory allocation failed!");
		return NULL;
	}
	memset(sslContextRef, 0, sizeof(CTSSLContext));
	//assert( *certRef );
	//sslContextRef->hCtxt = {0};

	//Create the SCHANNEL TLS Credential Handle
	if (!CTCredentialHandleCreate(&sslContextRef))
	{
		free(sslContextRef);
		fprintf(stderr, "ReqlCredentialHandleCreate failed!");
		return NULL;
	}

	ss = 0;
	ss = InitializeSecurityContextA(&(sslContextRef->hCred),//*certRef,
		NULL,
		(SEC_CHAR*)serverName,
		dwSSPIFlags,
		0,
		SECURITY_NATIVE_DREP,
		NULL,
		0,
		&(sslContextRef->hCtxt),
		&OutBuffer,
		&dwSSPIOutFlags,
		NULL);

	if (ss != SEC_I_CONTINUE_NEEDED) { fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (1)\n", ss); }

	// Send response to server if there is one.
	if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
	{
		*firstMessage = OutBuffers[0].pvBuffer;
		*firstMessageLen = OutBuffers[0].cbBuffer;
	}
	else
	{
		fprintf(stderr, "OPutbuffers is NULL");
	}

#elif defined(__APPLE__)
	//1  Create Darwin Secure Transport SSL Context
	if ((sslContextRef = SSLCreateContext(kCFAllocatorDefault, kSSLClientSide, kSSLStreamType)) == NULL) {
		fprintf(stderr, "SSLCreateContextkCFAllocatorDefault, kSSLClientSide, kSSLStreamType) failed with error:", errno);
		ReqlCloseSSLSocket(sslContext, socketfd);
		return NULL;
	}

	//2  Set SSL Secure Transport Callback functions for reading and writing
	if ((status = SSLSetIOFuncs(sslContextRef, readFromSocket, writeToSocket)) != noErr)
	{
		fprintf(stderr, "SSLSetIOFuncs failed with err status %d", status);
		ReqlCloseSSLSocket(sslContextRef, socketfd);
		return NULL;
	}

	//3  Set the socket file descriptor on the ssl context
	if ((status = SSLSetConnection(sslContextRef, (SSLConnectionRef)(intptr_t)socketfd)) != noErr)
	{
		fprintf(stderr, "SSLSetConnection failed with err status %d", status);
		ReqlCloseSSLSocket(sslContextRef, socketfd);
		return NULL;
	}

	//4  Set minimum TLS version 1.2
	if ((status = SSLSetProtocolVersionMin(sslContextRef, kTLSProtocol12)) != noErr)
	{
		fprintf(stderr, "SSLSetProtocolVersionMin(sslContext, kTLSProtocol12) failed with err status:  %d", status);
		ReqlCloseSSLSocket(sslContext, socketfd);
		return NULL;
	}

	//5  Set options to custom control the SSL handshake procedure
	if ((status = SSLSetSessionOption(sslContextRef, kSSLSessionOptionBreakOnServerAuth, true)) != noErr)
	{
		fprintf(stderr, "SSLSetSessionOption(sslContext, kSSLSessionOptionBreakOnServerAuth, true) failed with err status %d", status);
		ReqlCloseSSLSocket(sslContextRef, socketfd);
		return NULL;
	}

	//this doesn't seem to trigger anything in the handshake...
	//which means our server is not requesting client cert
	if ((status = SSLSetSessionOption(sslContextRef, kSSLSessionOptionBreakOnCertRequested, true)) != noErr)
	{
		fprintf(stderr, "SSLSetSessionOption (kSSLSessionOptionBreakOnCertRequested) failed with err status %d", status);
		ReqlCloseSSLSocket(sslContextRef, socketfd);
		return NULL;
	}
#endif
	//return the successfully created ssl context
	return sslContextRef;
}

static int count = 0;

SECURITY_STATUS CTSSLHandshakeProcessFirstResponse(CTCursor * cursor, CTSocket socketfd, CTSSLContextRef sslContextRef)
{
	int ret = 0;
	int err = 0;
	char buffer[80];
#ifdef CTRANSPORT_WOLFSSL
	CTClientError scRet;
	
	CTSSLDecryptTransient dTransient = { 0, cursor->headerLength + cursor->contentLength, 0, cursor->file.buffer};
	CTSSLEncryptTransient* eTransient = NULL;// { socketfd, socketfd, NULL, 0 };

	fprintf(stderr, "CTSSLHandshakeProcessFirstResponse::CoreTransport supplying (%lu) handshake bytes to WolfSSL...\n", cursor->headerLength + cursor->contentLength);

	//we are finished sending the first response, reset the send context for the second client message after the recv
	
	//Let WolfSSL Decrypt as many bytes from our encrypted input buffer ass possible in a single pass to wolfSSL_read
	wolfSSL_SetIOReadCtx(sslContextRef->ctx, &dTransient);

	//wolfSSL_SetIOWriteCtx(sslContextRef->ctx, &eTransient);

	/* Connect to wolfSSL on the server side */
	if ((ret = wolfSSL_connect(sslContextRef->ctx)) != SSL_SUCCESS) {

		err = wolfSSL_get_error(sslContextRef->ctx, 0);
		wolfSSL_ERR_error_string(err, buffer);
		fprintf(stderr, stderr, "CTSSLHandshakeProcessFirstResponse: wolfSSL_connect failed to read (%d): \n\n%s\n\n", wolfSSL_get_error(sslContextRef->ctx, 0), buffer);

		//HANDLE INCOMPLETE MESSAGE: the buffer didn't have as many bytes as WolfSSL needed for decryption
		if (err == WOLFSSL_ERROR_WANT_READ)
		{
			//SCHANNEL IMPLEMENTATION FOR REFERENCE
			//*msgLength = hasData * Buffers[1].cbBuffer + hasMissing * Buffers[0].cbBuffer;
			//*extraBuffer = (char*)(hasExtra * (uintptr_t)Buffers[3].pvBuffer);
			//*extraBytes = (scRet != SEC_E_OK) * (*extraBytes) + hasExtra * Buffers[3].cbBuffer;

			//WOLFSSL ANALOG IMPLEMENTATION
			//*msgLength = *msgLength - transient.totalBytesProcessed;// transient.bytesToDecrypt;// totalDecryptedBytes;
			//*extraBytes = transient.bytesToDecrypt;// transient.bytesToDecrypt;
			//*extraBuffer = (char*)msg + transient.totalBytesProcessed;// : NULL;
			//scRet = SEC_E_INCOMPLETE_MESSAGE;

			//
			//	Handle INCOMPLETE ENCRYPTED MESSAGE AS FOLLOWS:
			// 
			//	1)  copy the remaining encrypted bytes to the start of our buffer so we can 
			//		append to them when receiving from socket on the next async iocp iteration
			//  
			//	2)  use NumBytesRecv var as an offset input parameter CTCursorAsyncRecv
			//
			//	3)  use NumByteRemaining var as input to CTCursorAsyncRecv to request num bytes to recv from socket
			//		(PrevBytesRecvd will be subtracted from NumBytesRemaining internal to CTAsyncRecv

			//For SCHANNEL:  CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer
			//For WolfSSL:   CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer + the size of the ssl header wolfssl already consumed

			CTOverlappedResponse* overlappedResponse = &(cursor->overlappedResponse);
			assert(overlappedResponse);

			//request exactly the amount to complete the message NumBytesRemaining 
			//populated by CTSSLDecryptMessage2 above or request more if desired
			unsigned long NumBytesRecv = cursor->headerLength + cursor->contentLength - dTransient.totalBytesProcessed;
			unsigned long NumBytesRemaining = dTransient.bytesToDecrypt - NumBytesRecv;   //request exactly the amount to complete the message NumBytesRemaining 

			assert(NumBytesRemaining > 0);
			if(dTransient.totalBytesProcessed > 0 )
				memcpy((char*)cursor->file.buffer, cursor->file.buffer + dTransient.totalBytesProcessed, NumBytesRecv);

			
			overlappedResponse->buf = (char*)(cursor->file.buffer);
			//overlappedResponse->len = sslFirstMessageLen;

			cursor->headerLength = 0;
			cursor->contentLength = 0;// transient.bytesToDecrypt;
			NumBytesRemaining = 32768;// NumBytesRecv + NumBytesRemaining;

			if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), cursor->file.buffer, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)
			{
				//the async operation returned immediately
				if (scRet == CTSuccess)
				{
					fprintf(stderr, "CTSSLHandshakeProcessFirstResponse::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
					//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
				}
				else //failure
				{
					fprintf(stderr, "CTSSLHandshakeProcessFirstResponse::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
					assert(1 == 0);
				}
			}
			return CTSuccess;
		}
		else if (err == SSL_ERROR_WANT_WRITE)
		{
			eTransient = (CTSSLEncryptTransient*)wolfSSL_GetIOWriteCtx(sslContextRef->ctx);
			assert(eTransient && eTransient->messageLen == 75);
			//the second client handshake message
			cursor->headerLengthCallback = SSLSecondMessageHeaderLengthCallback;
			cursor->responseCallback = SSLSecondMessageResponseCallback;
			cursor->queryCallback = SSLSecondMessageQueryCallback;
			cursor->contentLength = cursor->headerLength = 0;
			//cursor->conn->responseCount = 0;
			cursor->file.buffer = cursor->requestBuffer;
			cursor->overlappedResponse.buf = eTransient->messageBuffer;// (char*)(OutBuffers[0].pvBuffer);
			cursor->overlappedResponse.len = eTransient->messageLen;//(char*)(OutBuffers[0].cbBuffer);
			cursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to send but bypass encrypt bc this is the ssl handshake
			assert(cursor->overlappedResponse.buf);
			CTCursorSendOnQueue(cursor, (char**)&(cursor->overlappedResponse.buf), cursor->overlappedResponse.len);
			return CTSuccess;
		}
		/*
		err = wolfSSL_get_error(sslContextRef->ctx, ret);
		if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
		{
			fprintf(stderr, stderr, "ERROR: wolfSSL handshake failed!\n");
		}
		return err;
		*/
	}
	//wolf_handshake_complete = 1;
#elif defined( CTRANSPORT_USE_MBED_TLS )
	while ((ret = mbedtls_ssl_handshake(&(sslContextRef->ctx))) != 0)
	{
		Reql_fprintf(stderr, "%s", "CTSSLHandshake\n");

		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			Reql_fprintf(stderr, "CTSSLHandshake::mbedtls_ssl_handshake failed and returned %d\n\n", ret);
			return ret;
		}
	}
#elif defined(_WIN32)

	SecBufferDesc   OutBuffer, InBuffer;
	SecBuffer       InBuffers[2], OutBuffers[1];
	DWORD           dwSSPIFlags, dwSSPIOutFlags, cbData, cbIoBuffer;
	//TimeStamp       tsExpiry;
	SECURITY_STATUS scRet;
	PUCHAR          IoBuffer;
	BOOL            fDoRead;

	/*
	PSecurityFunctionTable pSSPI = NULL;
	// Retrieve a data pointer for the current thread's security function table
	pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
	if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
	  fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/

	dwSSPIFlags = ISC_REQ_STREAM | ISC_REQ_ALLOCATE_MEMORY | ISC_RET_EXTENDED_ERROR  /*   | ISC_REQ_REPLAY_DETECT     | ISC_REQ_CONFIDENTIALITY   |
				  ISC_RET_EXTENDED_ERROR    | | ISC_REQ_STREAM */;

				  // Allocate data buffer.
	//IoBuffer = (PUCHAR)LocalAlloc(LMEM_FIXED, IO_BUFFER_SIZE);
	IoBuffer = cursor->file.buffer;

	if (IoBuffer == NULL) { fprintf(stderr, "**** Out of memory (1)\n"); return SEC_E_INTERNAL_ERROR; }
	cbIoBuffer = cursor->overlappedResponse.buf - cursor->file.buffer;// cursor->headerLength + cursor->contentLength;
	fDoRead = 1;//fDoInitialRead;

	// Loop until the handshake is finished or an error occurs.
	scRet = SEC_I_CONTINUE_NEEDED;

	fprintf(stderr, " CTSSLHandhsakeProcessFirstResponse:: recvd %d bytes of handshake data \n\n", cursor->overlappedResponse.buf - cursor->file.buffer);

	while (scRet == SEC_I_CONTINUE_NEEDED ||
		scRet == SEC_E_INCOMPLETE_MESSAGE ||
		scRet == SEC_I_INCOMPLETE_CREDENTIALS)
	{
		/*
		if (0 == cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE) // Read data from server.
		{
			if (fDoRead)
			{
				cbData = recv(socketfd, (char*)IoBuffer + cbIoBuffer, IO_BUFFER_SIZE - cbIoBuffer, 0);
				if (cbData == SOCKET_ERROR)
				{
					fprintf(stderr, "**** Error %d reading data from server\n", WSAGetLastError());
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				else if (cbData == 0)
				{
					fprintf(stderr, "**** Server unexpectedly disconnected\n");
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				fprintf(stderr, "%d bytes of handshake data received\n", cbData);
				if (fVerbose) { PrintHexDump(cbData, IoBuffer + cbIoBuffer); fprintf(stderr, "\n"); }
				cbIoBuffer += cbData;
			}
			else
				fDoRead = TRUE;
		}
		*/

		// Set up the input buffers. Buffer 0 is used to pass in data
		// received from the server. Schannel will consume some or all
		// of this. Leftover data (if any) will be placed in buffer 1 and
		// given a buffer type of SECBUFFER_EXTRA.
		InBuffers[0].pvBuffer = IoBuffer;
		InBuffers[0].cbBuffer = cbIoBuffer;
		InBuffers[0].BufferType = SECBUFFER_TOKEN;

		InBuffers[1].pvBuffer = NULL;
		InBuffers[1].cbBuffer = 0;
		InBuffers[1].BufferType = SECBUFFER_EMPTY;

		InBuffer.cBuffers = 2;
		InBuffer.pBuffers = InBuffers;
		InBuffer.ulVersion = SECBUFFER_VERSION;


		// Set up the output buffers. These are initialized to NULL
		// so as to make it less likely we'll attempt to free random
		// garbage later.
		OutBuffers[0].pvBuffer = NULL;
		OutBuffers[0].BufferType = SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer = 0;

		OutBuffer.cBuffers = 1;
		OutBuffer.pBuffers = OutBuffers;
		OutBuffer.ulVersion = SECBUFFER_VERSION;

		static int done = 1;
		// Call InitializeSecurityContext.
		scRet = InitializeSecurityContextA(&(cursor->conn->sslContext->hCred),
			&(cursor->conn->sslContext->hCtxt),
			(SEC_CHAR*)(cursor->conn->target->url.host),
			dwSSPIFlags,
			0,
			SECURITY_NATIVE_DREP,
			&InBuffer,
			0,
			&(cursor->conn->sslContext->hCtxt),
			&OutBuffer,
			&dwSSPIOutFlags,
			NULL);

		done = 0;

		// If InitializeSecurityContext was successful (or if the error was
		// one of the special extended ones), send the contends of the output
		// buffer to the server.
		if (scRet == SEC_E_OK ||
			scRet == SEC_I_CONTINUE_NEEDED ||
			FAILED(scRet) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR))
		{
			// Check for fatal error.
			//if (FAILED(scRet)) { fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet); }
			fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet);

			assert((LONG)scRet != 2148074278);

			//assert(scRet != SEC_I_CONTINUE_NEEDED);
			/*
			if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
			{
				cbData = send(socketfd, (const char*)OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
				if (cbData == SOCKET_ERROR || cbData == 0)
				{
					fprintf(stderr, "**** Error %d sending data to server (2)\n", WSAGetLastError());
					FreeContextBuffer(OutBuffers[0].pvBuffer);
					DeleteSecurityContext(&(sslContextRef->hCtxt));
					return SEC_E_INTERNAL_ERROR;
				}
				fprintf(stderr, "%d bytes of handshake data sent\n", cbData);
				if (fVerbose) { PrintHexDump(cbData, (PBYTE)OutBuffers[0].pvBuffer); fprintf(stderr, "\n"); }

				// Free output buffer.
				FreeContextBuffer(OutBuffers[0].pvBuffer);
				OutBuffers[0].pvBuffer = NULL;
			}
			*/
			if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
				break;

			if (scRet == SEC_I_CONTINUE_NEEDED && InBuffers[1].BufferType == SECBUFFER_EXTRA)
			{
				// Copy any leftover data from the "extra" buffer, and go around again.
				//if (InBuffers[1].BufferType == SECBUFFER_EXTRA)
				//{
					MoveMemory(IoBuffer, IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);
					cbIoBuffer = InBuffers[1].cbBuffer;
				//}
				//else
				//	cbIoBuffer = 0;

				continue;
			}
			else if (scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_CONTINUE_NEEDED)
			{

				/*
				cursor->headerLengthCallback = SSLFirstMessageHeaderLengthCallback;
				cursor->responseCallback = SSLFirstMessageResponseCallback;

				cursor->file.buffer = cursor->requestBuffer;
				cursor->overlappedResponse.buf = (char*)(sslFirstMessageBuffer);
				handshakeCursor->overlappedResponse.len = sslFirstMessageLen;

				handshakeCursor->conn = conn; //assume conn is permanent memory from core transport connection pool
				handshakeCursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to bypass encrypt bc sthis is the ssl handshake
				assert(handshakeCursor->overlappedResponse.buf);
				CTCursorSendOnQueue(handshakeCursor, (char**)&(handshakeCursor->overlappedResponse.buf), handshakeCursor->overlappedResponse.len);
				return CTSuccess;
				*/

				CTOverlappedResponse* olResponse = &(cursor->overlappedResponse);
				unsigned long NumBytesRemaining = ct_system_allocation_granularity();// 32768;   //request exactly the amount to complete the message NumBytesRemaining 
								   //populated by CTSSLDecryptMessage2 above or request more if desired

				//char* buffer = olResponse->buf;

				//if (scRet == SEC_E_INCOMPLETE_MESSAGE)
				//	NumBytesRemaining -= cbIoBuffer;
				if (scRet == SEC_I_CONTINUE_NEEDED)
					cbIoBuffer = 0;
					//buffer = cursor->file.buffer;;

				//fprintf(stderr, " CTSSLHandhsakeProcessFirstResponse:: recvd %d bytes of handshake data \n\n", cursor->overlappedResponse.buf - cursor->file.buffer);
				if ((ret = CTCursorAsyncRecv(&olResponse, cursor->file.buffer, cbIoBuffer, &NumBytesRemaining)) != CTSocketIOPending)
				{
					//the async operation returned immediately
					if (ret == CTSuccess)
					{
						//fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
						//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
					}
					else //failure
					{
						fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
						break;
					}

				}
				return scRet;

			}
			
		}

		// If InitializeSecurityContext returned SEC_E_INCOMPLETE_MESSAGE,
		// then we need to read more data from the server and try again.
		if (scRet == SEC_E_INCOMPLETE_MESSAGE ) { fprintf(stderr, "SEC_E_INCOMPLETE_MESSAGE"); assert(1==0); }

		// Copy any leftover data from the "extra" buffer, and go around again.
		if (InBuffers[1].BufferType == SECBUFFER_EXTRA)
		{
			assert(1 == 0);
			MoveMemory(IoBuffer, IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);
			cbIoBuffer = InBuffers[1].cbBuffer;
		}

	}


	if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
	{
		//Setup to send second message, reuse the same cursor that started the handshake

		cursor->headerLengthCallback = SSLSecondMessageHeaderLengthCallback;
		cursor->responseCallback = SSLSecondMessageResponseCallback;
		cursor->queryCallback = SSLSecondMessageQueryCallback;

		cursor->file.buffer = cursor->requestBuffer;
		cursor->overlappedResponse.buf = (char*)(OutBuffers[0].pvBuffer);
		cursor->overlappedResponse.len = (char*)(OutBuffers[0].cbBuffer);

		cursor->overlappedResponse.stage = CT_OVERLAPPED_SEND; //we need to send but bypass encrypt bc this is the ssl handshake
		assert(cursor->overlappedResponse.buf);
		CTCursorSendOnQueue(cursor, (char**)&(cursor->overlappedResponse.buf), cursor->overlappedResponse.len);

	//}
	/*
	else
	{

		cbData = send(cursor->conn->socketfd, (const char*)OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
		if (cbData == SOCKET_ERROR || cbData == 0)
		{
			fprintf(stderr, "**** Error %d sending data to server (2)\n", WSAGetLastError());
			FreeContextBuffer(OutBuffers[0].pvBuffer);
			DeleteSecurityContext(&(sslContextRef->hCtxt));
			return SEC_E_INTERNAL_ERROR;
		}
		fprintf(stderr, "%d bytes of handshake data sent\n", cbData);
		if (fVerbose) { PrintHexDump(cbData, (PBYTE)OutBuffers[0].pvBuffer); fprintf(stderr, "\n"); }

		// Free output buffer.
		FreeContextBuffer(OutBuffers[0].pvBuffer);
		OutBuffers[0].pvBuffer = NULL;

	}
	*/
	}

#endif
	return SEC_E_OK;
}


/***
 *  CTSSLHandshake
 *
 *  Perform a custom SSL Handshake using the Apple Secure Transport API
 *
 *  --Overrides PeerAuth step (errSSLPeerAuthCompleted) to validate trust against the input ca root certificate
 ***/
int CTSSLHandshake(CTSocket socketfd, CTSSLContextRef sslContextRef, CTSecCertificateRef rootCertRef, const char * serverName)
{
	int ret = 0;
	int err = 0;

#ifdef CTRANSPORT_WOLFSSL

	/* Connect to wolfSSL on the server side */
	if ((ret = wolfSSL_connect(sslContextRef->ctx)) != SSL_SUCCESS) {

		err = wolfSSL_get_error(sslContextRef->ctx, ret);
		if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
		{
			fprintf(stderr, stderr, "ERROR: wolfSSL handshake failed!\n");
			assert(1 == 0);
		}
		return err;
	}
	//wolf_handshake_complete = 1;
#elif defined( CTRANSPORT_USE_MBED_TLS )
	while ((ret = mbedtls_ssl_handshake(&(sslContextRef->ctx))) != 0)
	{
		Reql_fprintf(stderr, "%s", "CTSSLHandshake\n");

		if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
		{
			Reql_fprintf(stderr, "CTSSLHandshake::mbedtls_ssl_handshake failed and returned %d\n\n", ret);
			return ret;
		}
	}
#elif defined(_WIN32)

    SecBufferDesc   OutBuffer, InBuffer;
    SecBuffer       InBuffers[2], OutBuffers[1];
    DWORD           dwSSPIFlags, dwSSPIOutFlags, cbData, cbIoBuffer;
    //TimeStamp       tsExpiry;
    SECURITY_STATUS scRet;
    PUCHAR          IoBuffer;
    BOOL            fDoRead;

	/*
	PSecurityFunctionTable pSSPI = NULL;
	// Retrieve a data pointer for the current thread's security function table 
    pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface); 
    if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
      fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/

    dwSSPIFlags = ISC_REQ_STREAM | ISC_REQ_ALLOCATE_MEMORY |ISC_RET_EXTENDED_ERROR  /*   | ISC_REQ_REPLAY_DETECT     | ISC_REQ_CONFIDENTIALITY   |
                  ISC_RET_EXTENDED_ERROR    | | ISC_REQ_STREAM */ ;

    // Allocate data buffer.
    IoBuffer = (PUCHAR)LocalAlloc(LMEM_FIXED, IO_BUFFER_SIZE);
    if(IoBuffer == NULL) { fprintf(stderr, "**** Out of memory (1)\n"); return SEC_E_INTERNAL_ERROR; }
    cbIoBuffer = 0;
    fDoRead = 1;//fDoInitialRead;

    // Loop until the handshake is finished or an error occurs.
    scRet = SEC_I_CONTINUE_NEEDED;

    while( scRet == SEC_I_CONTINUE_NEEDED        ||
           scRet == SEC_E_INCOMPLETE_MESSAGE     ||
           scRet == SEC_I_INCOMPLETE_CREDENTIALS )
   {
        if(0 == cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE) // Read data from server.
        {
            if(fDoRead)
            {
                cbData = recv(socketfd, (char*)IoBuffer + cbIoBuffer, IO_BUFFER_SIZE - cbIoBuffer, 0 );
                if(cbData == SOCKET_ERROR)
                {
                    fprintf(stderr, "**** Error %d reading data from server\n", WSAGetLastError());
                    scRet = SEC_E_INTERNAL_ERROR;
                    break;
                }
                else if(cbData == 0)
                {
                    fprintf(stderr, "**** Server unexpectedly disconnected\n");
                    scRet = SEC_E_INTERNAL_ERROR;
                    break;
                }
                fprintf(stderr, "%d bytes of handshake data received\n", cbData);
                if(fVerbose) { PrintHexDump(cbData, IoBuffer + cbIoBuffer); fprintf(stderr, "\n"); }
                cbIoBuffer += cbData;
            }
            else
              fDoRead = TRUE;
        }

        // Set up the input buffers. Buffer 0 is used to pass in data
        // received from the server. Schannel will consume some or all
        // of this. Leftover data (if any) will be placed in buffer 1 and
        // given a buffer type of SECBUFFER_EXTRA.
        InBuffers[0].pvBuffer   = IoBuffer;
        InBuffers[0].cbBuffer   = cbIoBuffer;
        InBuffers[0].BufferType = SECBUFFER_TOKEN;

        InBuffers[1].pvBuffer   = NULL;
        InBuffers[1].cbBuffer   = 0;
        InBuffers[1].BufferType = SECBUFFER_EMPTY;

        InBuffer.cBuffers       = 2;
        InBuffer.pBuffers       = InBuffers;
        InBuffer.ulVersion      = SECBUFFER_VERSION;


        // Set up the output buffers. These are initialized to NULL
        // so as to make it less likely we'll attempt to free random
        // garbage later.
        OutBuffers[0].pvBuffer  = NULL;
        OutBuffers[0].BufferType= SECBUFFER_TOKEN;
        OutBuffers[0].cbBuffer  = 0;

        OutBuffer.cBuffers      = 1;
        OutBuffer.pBuffers      = OutBuffers;
        OutBuffer.ulVersion     = SECBUFFER_VERSION;


        // Call InitializeSecurityContext.
		scRet = InitializeSecurityContextA(	&(sslContextRef->hCred),
			&(sslContextRef->hCtxt),
			(SEC_CHAR*)(serverName),
			dwSSPIFlags,
			0,
			SECURITY_NATIVE_DREP,
			&InBuffer,
			0,
			&(sslContextRef->hCtxt),
			&OutBuffer,
			&dwSSPIOutFlags,
			NULL);


        // If InitializeSecurityContext was successful (or if the error was
        // one of the special extended ones), send the contends of the output
        // buffer to the server.
        if(scRet == SEC_E_OK                ||
           scRet == SEC_I_CONTINUE_NEEDED   ||
           FAILED(scRet) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR))
        {
			//if (FAILED(scRet)) { fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet); }
			fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet);

            if(OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != NULL)
            {
                cbData = send(socketfd, (const char *)OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0 );
                if(cbData == SOCKET_ERROR || cbData == 0)
                {
                    fprintf(stderr,  "**** Error %d sending data to server (2)\n",  WSAGetLastError() );
                    FreeContextBuffer(OutBuffers[0].pvBuffer);
                    DeleteSecurityContext(&(sslContextRef->hCtxt));
                    return SEC_E_INTERNAL_ERROR;
                }
                fprintf(stderr, "%d bytes of handshake data sent\n", cbData);
                if(fVerbose) { PrintHexDump(cbData, (PBYTE)OutBuffers[0].pvBuffer); fprintf(stderr, "\n"); }

                // Free output buffer.
                FreeContextBuffer(OutBuffers[0].pvBuffer);
                OutBuffers[0].pvBuffer = NULL;
            }
        }

        // If InitializeSecurityContext returned SEC_E_INCOMPLETE_MESSAGE,
        // then we need to read more data from the server and try again.
		if(scRet == SEC_E_INCOMPLETE_MESSAGE) { fprintf(stderr, "SEC_E_INCOMPLETE_MESSAGE\n"); continue; }

        // If InitializeSecurityContext returned SEC_E_OK, then the
        // handshake completed successfully.
        if(scRet == SEC_E_OK)
        {
            // If the "extra" buffer contains data, this is encrypted application
            // protocol layer stuff. It needs to be saved. The application layer
            // will later decrypt it with DecryptMessage.
            fprintf(stderr, "Handshake was successful\n");

            if(InBuffers[1].BufferType == SECBUFFER_EXTRA)
            {
                sslContextRef->ExtraData.pvBuffer = LocalAlloc( LMEM_FIXED, InBuffers[1].cbBuffer );
                if(sslContextRef->ExtraData.pvBuffer == NULL) { fprintf(stderr, "**** Out of memory (2)\n"); return SEC_E_INTERNAL_ERROR; }

                MoveMemory( sslContextRef->ExtraData.pvBuffer,
                            IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer),
                            InBuffers[1].cbBuffer );

                sslContextRef->ExtraData.cbBuffer   = InBuffers[1].cbBuffer;
                sslContextRef->ExtraData.BufferType = SECBUFFER_TOKEN;

                fprintf(stderr,  "%d bytes of app data was bundled with handshake data\n", sslContextRef->ExtraData.cbBuffer );
            }
            else
            {
                sslContextRef->ExtraData.pvBuffer   = NULL;
                sslContextRef->ExtraData.cbBuffer   = 0;
                sslContextRef->ExtraData.BufferType = SECBUFFER_EMPTY;
            }
            break; // Bail out to quit
        }



        // Check for fatal error.
        if(FAILED(scRet)) { fprintf(stderr, "**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet); break; }

        // If InitializeSecurityContext returned SEC_I_INCOMPLETE_CREDENTIALS,
        // then the server just requested client authentication.
        if(scRet == SEC_I_INCOMPLETE_CREDENTIALS)
        {
            // Busted. The server has requested client authentication and
            // the credential we supplied didn't contain a client certificate.
            // This function will read the list of trusted certificate
            // authorities ("issuers") that was received from the server
            // and attempt to find a suitable client certificate that
            // was issued by one of these. If this function is successful,
            // then we will connect using the new certificate. Otherwise,
            // we will attempt to connect anonymously (using our current credentials).
            GetNewClientCredentials(&(sslContextRef->hCred), &(sslContextRef->hCtxt));

            // Go around again.
            fDoRead = FALSE;
            scRet = SEC_I_CONTINUE_NEEDED;
            continue;
        }

        // Copy any leftover data from the "extra" buffer, and go around again.
        if ( InBuffers[1].BufferType == SECBUFFER_EXTRA )
        {
            MoveMemory( IoBuffer, IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer );
            cbIoBuffer = InBuffers[1].cbBuffer;
        }
        else
          cbIoBuffer = 0;
    }

	LocalFree(IoBuffer);
    // Delete the security context in the case of a fatal error.
	if(FAILED(scRet)) 
	{ 
		fprintf(stderr, "ReqlSSLhandshake::Deleting Securitiy Context\n");
		DeleteSecurityContext(&(sslContextRef->hCtxt)); return scRet;
	}
	
#elif defined(__APPLE__)

#endif
	return CTSuccess;
}



/***
 *  CTVerifyServerCertificate
 *
 *
 ***/
int CTVerifyServerCertificate(CTSSLContextRef sslContextRef, char * pszServerName)
{
#ifdef CTRANSPORT_WOLFSSL
	/*
	//Load client certificates into WOLFSSL_CTX
	if (wolfSSL_CTX_load_verify_locations(sslContextRef->conf, certRef, NULL) != WOLFSSL_SUCCESS)
	{
		fprintf(stderr, stderr, "ERROR: failed to load %s, please check the file.\n", certRef);
		assert(1 == 0);
	}
	*/
	return CTSuccess;
#elif defined( CTRANSPORT_USE_MBED_TLS )
	uint32_t flags;
	//In real life, we probably want to bail out when ret != 0
	if ((flags = mbedtls_ssl_get_verify_result(&(sslContextRef->ctx))) != 0)
	{
		char vrfy_buf[512];
		mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

		//Reql_fprintf(stderr, "ReqlVerifyServerCertificate::mbedtls_ssl_get_verify_result failed\n");
		Reql_fprintf(stderr, "ReqlVerifyServerCertificate::mbedtls_ssl_get_verify_result failed %s", vrfy_buf);
	}
	else
		Reql_fprintf(stderr, "%s", "ReqlVerifyServerCertificate::success\n");
	return (int)flags;
#elif defined(_WIN32)

	//int i, systemAlgIndex;
	SECURITY_STATUS scRet;
	SecPkgCred_CipherStrengths cipherStrengths = { 0 };
	SecPkgCred_SupportedAlgs supportedAlgs = { 0 };
	SecPkgCred_SupportedProtocols supportedProtocols = { 0 };
	PCCERT_CONTEXT pRemoteCertContext = NULL;
	//PSecurityFunctionTable pSSPI = NULL;

	/*
	// Retrieve a data pointer for the current thread's security function table
	pSSPI = (PSecurityFunctionTable)TlsGetValue(dwTlsSecurityInterface);
	if ((pSSPI == 0) && (GetLastError() != ERROR_SUCCESS))
	{
	  fprintf(stderr, "\nReqlSSLDecryptMessage::TlsGetValue dwTlsSecurityInterface error.  Using global Security Interface...\n");
	  pSSPI = g_pSSPI;
	}
	*/

	//Query Credential Attributes
	scRet = QueryCredentialsAttributes(&(sslContextRef->hCred), SECPKG_ATTR_CIPHER_STRENGTHS, &cipherStrengths);
	//fprintf(stderr, "Cipher Strengths = %d, %d\n", (int)cipherStrengths.dwMinimumCipherStrength, (int)cipherStrengths.dwMaximumCipherStrength);

	if (scRet != SEC_E_OK)
	{
		fprintf(stderr, "Error 0x%x querying Cipher STrengs\n", scRet); return scRet;
	} //

	scRet = QueryCredentialsAttributes(&(sslContextRef->hCred), SECPKG_ATTR_SUPPORTED_ALGS, &supportedAlgs);

	if (scRet != SEC_E_OK)
	{
		fprintf(stderr, "Error 0x%x querying Supported Algs\n", scRet); return scRet;
	} //

/*
fprintf(stderr, "Supported Algs = \n");
assert(supportedAlgs.palgSupportedAlgs);
for( i=0; i<(int)supportedAlgs.cSupportedAlgs; i++)
{
	ALG_ID alg = supportedAlgs.palgSupportedAlgs[i];
	fprintf(stderr, "%u,", (unsigned)alg);

	for( systemAlgIndex = 0; systemAlgIndex<47; systemAlgIndex++)
	{
		if( allAlgs[systemAlgIndex] == (ALG_ID)supportedAlgs.palgSupportedAlgs[i] )
			fprintf(stderr, "Alg AtIndex = %d\n", systemAlgIndex);

	}

}
fprintf(stderr, "\n");
*/
	scRet = QueryCredentialsAttributes(&(sslContextRef->hCred), SECPKG_ATTR_SUPPORTED_PROTOCOLS, &supportedProtocols);

	if (scRet != SEC_E_OK)
	{
		fprintf(stderr, "Error 0x%x querying Protocols\n", scRet); return scRet;
	} //

	fprintf(stderr, "**** Protocols 0x%x (%ld) returned by AcquireCredentialsHandle\n", supportedProtocols.grbitProtocol);

	printProtocolsForDWORD(supportedProtocols.grbitProtocol);


	//fprintf(stderr, "pszServerName = %s\n", pszServerName);
	 // Authenticate server's credentials. Get server's certificate.
	scRet = QueryContextAttributes(&(sslContextRef->hCtxt), SECPKG_ATTR_REMOTE_CERT_CONTEXT, (LPVOID)&pRemoteCertContext);
	if (scRet != SEC_E_OK)
	{
		fprintf(stderr, "Error 0x%x querying remote certificate\n", scRet); return scRet;
	} //

//fprintf(stderr, "----- Server Credentials Authenticated \n");
	assert(pRemoteCertContext);
	// Display server certificate chain.
	DisplayCertChain(pRemoteCertContext, FALSE); //

	fprintf(stderr, "----- Certificate Chain Displayed \n");

	// Attempt to validate server certificate.
	scRet = VerifyServerCertificate(pRemoteCertContext, pszServerName, 0);
	if (scRet) { fprintf(stderr, "**** Error 0x%x authenticating server credentials!\n", scRet); return scRet; }
	// The server certificate did not validate correctly. At this point, we cannot tell
	// if we are connecting to the correct server, or if we are connecting to a
	// "man in the middle" attack server - Best to just abort the connection.

	fprintf(stderr, "----- Server Certificate Verified\n");

	// Free the server certificate context.
	CertFreeCertificateContext(pRemoteCertContext);
	pRemoteCertContext = NULL; //
	fprintf(stderr, "----- Server certificate context released \n");

	// Display connection info.
	DisplayConnectionInfo(&sslContextRef->hCtxt); //
	fprintf(stderr, "----- Secure Connection Info\n");

	// Read stream encryption properties.
	scRet = QueryContextAttributes(&(sslContextRef->hCtxt), SECPKG_ATTR_STREAM_SIZES, &(sslContextRef->Sizes));
	if (scRet != SEC_E_OK)
	{
		fprintf(stderr, "**** Error 0x%x reading SECPKG_ATTR_STREAM_SIZES\n", scRet); return scRet;
	}

	//    SecPkgContext_StreamSizes Sizes;            // unsigned long cbBuffer;    // Size of the buffer, in bytes
	//SECURITY_STATUS                        scRet;            // unsigned long BufferType;  // Type of the buffer (below)        
	//PBYTE                                            pbIoBuffer; // void    SEC_FAR * pvBuffer;   // Pointer to the buffer
	//DWORD                                            cbIoBufferLength, cbData;

	// Create ssl header and trailer buffers
	sslContextRef->hBuffer = (char*)malloc(sslContextRef->Sizes.cbHeader);
	sslContextRef->tBuffer = (char*)malloc(sslContextRef->Sizes.cbTrailer);

	return scRet;
#endif
}



/***
 *	CTSSLRead
 ***/
CTSSLStatus CTSSLRead( CTSocket socketfd, CTSSLContextRef sslContextRef, void * msg, unsigned long * msgLength )

// calls recv() - blocking socket read
// http://msdn.microsoft.com/en-us/library/ms740121(VS.85).aspx

// The encrypted message is decrypted in place, overwriting the original contents of its buffer.
// http://msdn.microsoft.com/en-us/library/aa375211(VS.85).aspx

{

#ifdef CTRANSPORT_WOLFSSL
	assert(1 == 0);
#elif defined( CTRANSPORT_USE_MBED_TLS )
	struct timeval m_timeInterval;
	FD_SET ReadSet;
	CTSSLStatus scRet = 0;
	//NOTE:  CTransport uses blocking sockets but asynchronous operation
	//This is not a problem when using IOCP on WIN32, because IOCP + WSARecv takes care of reading from the socket before notifying us that data is available to do encryption...
	//However, because MBEDTLS only provides decryption through a routine that also reads from the socket, we must prevent IOCP from reading from the socket and just have it notify us when data is available
	//But then, we still have the issue of not knowing when we have finished reading a full message from the socket without blocking calls, so in that case, when a msgLenght of zero is input to this function
	//We use to determine whether there is data available for reading or none at all
	
	
	
	if( *msgLength == 0 )
	{
		FD_ZERO(&ReadSet);
		FD_SET(socketfd, &ReadSet);

		m_timeInterval.tv_sec = 0;
		m_timeInterval.tv_usec = 30; //30 Microseconds for Polling

		scRet = select((int)socketfd + 1, &ReadSet, NULL, NULL, &m_timeInterval);

		if (scRet == SOCKET_ERROR)
		{
			//std::cout << "ERROR" << std::endl;
			//break;
			return scRet;
		}

		if (scRet == 0)
		{
			return 0;
			//std::cout << "No Data to Receive" << std::endl;
			//return CTSocketWouldBlock;
		}

		//DWORD numBytesAvailable
		scRet = ioctlsocket(socketfd, FIONREAD, msgLength);
	
		fprintf(stderr, "**** mbedReadFromSocket::ictlsocket status = %d\n", (int)scRet);
		fprintf(stderr, "**** mbedReadFromSocket::numBytesAvailable = %d\n", (int)*msgLength);

		//select said there was data but ioctlsocket says there is no data
		if( *msgLength == 0)
			return 0;

	}

	scRet = mbedtls_ssl_read(&(sslContextRef->ctx), (unsigned char *)msg, (size_t)*msgLength);
	
	//message length output is zero unless mbedtls_ssl_read succeeded 
	*msgLength = 0;


	if( scRet == 0 )
	{
		//if mbedtls_ssl_read returned 0 then the socket disconnected
#ifdef _DEBUG
		Reql_fprintf(stderr, "%s", "ReqlSSLRead::mbedtls_ssl_read disconnect!\n");
#endif
		return CTSocketDisconnect;
	}
	else if (scRet == MBEDTLS_ERR_SSL_WANT_READ) 
	{
#ifdef _DEBUG
		Reql_fprintf(stderr, "%s", "ReqlSSLRead::mbedtls_ssl_read MBED_ERR_TLS_WANT_READ\n");
#endif
		//MBEDTLS_ERR_SSL_WANT_READ/WRITE is the same ass WOULDBLOCK? confirm
		return CTSocketWouldBlock;
	}
#ifdef _DEBUG
	else if( scRet < 0 )
	{
		Reql_fprintf(stderr, "%s", "ReqlSSLRead::mbedtls_ssl_read error (%d)\n", scRet);
	}
#endif
	else
	{
		//on success mbedtls_ssl_read return provides the number of bytes
		//but we want to return success and put 
		Reql_fprintf(stderr, "ReqlSSLRead::mbedtls_ssl_read %d bytes\n", scRet);
		*msgLength = scRet;
		scRet = CTSuccess;
		//Reql_fprintf(stderr, "\n%s\n", (char*)msg);
	}
	return scRet;
#elif defined(_WIN32)

  //SecBuffer                ExtraBuffer;
  //SecBuffer        *pExtraBuffer;
  CTSSLStatus    scRet;				// unsigned long cbBuffer;    // Size of the buffer, in bytes
 
  unsigned long              cbIoBuffer, cbData, cbIoBufferLength;
  
	// Read data from server until done.
    cbIoBuffer = 0;
	cbIoBufferLength = (unsigned long)*msgLength;
    scRet = 0;

    while(TRUE) // Read some data.
    {
        if( cbIoBuffer == 0 || scRet == CTSSLIncompleteMessage ) // get the data
        {
            cbData = recv(socketfd, (char*)msg + cbIoBuffer, cbIoBufferLength - cbIoBuffer, 0);
            if(cbData == SOCKET_ERROR)
            {
                fprintf(stderr, "**** Error %d reading data from server\n", WSAGetLastError());
                scRet = SEC_E_INTERNAL_ERROR;
                break;
            }
            else if(cbData == 0) // Server disconnected.
            {
                if(cbIoBuffer)
                {
                    fprintf(stderr, "**** Server unexpectedly disconnected\n");
                    scRet = SEC_E_INTERNAL_ERROR;
                    return scRet;
                }
                else
                  break; // All Done
            }
            else // success
            {
                fprintf(stderr, "%d bytes of (encrypted) application data received\n\n", cbData);
                if(fVerbose) { PrintHexDump(cbData, (PBYTE)msg + cbIoBuffer); fprintf(stderr, "\n"); }
                cbIoBuffer += cbData;
            }
        }

		scRet = CTSSLDecryptMessage(sslContextRef, msg, &cbIoBuffer);
		if( scRet == CTSSLContextExpired ) break; // Server signaled end of session
		if( scRet == CTSuccess ) break;

		//go around again if needed?
		cbIoBuffer = 0;

        // The server wants to perform another handshake sequence.
        if(scRet == CTSSLRenegotiate)
        {
            fprintf(stderr, "Server requested renegotiate!\n");
            //scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
			scRet = CTSSLHandshake(socketfd, sslContextRef, NULL, NULL);
            if(scRet != SEC_E_OK) return scRet;

            if(sslContextRef->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
            {
                MoveMemory(msg, sslContextRef->ExtraData.pvBuffer, sslContextRef->ExtraData.cbBuffer);
                cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
            }
        }
    } // Loop till CRLF is found at the end of the data

	*msgLength = (size_t)cbIoBuffer;
    return scRet;//(size_t)cbIoBuffer;

#endif
}


/***
 *	CTSSLWrite
 ***/
int CTSSLWrite( CTSocket socketfd, CTSSLContextRef sslContextRef, void * msg, unsigned long * msgLength )// * phContext, PBYTE pbIoBuffer, SecPkgContext_StreamSizes Sizes )
// http://msdn.microsoft.com/en-us/library/aa375378(VS.85).aspx
// The encrypted message is encrypted in place, overwriting the original contents of its buffer.
{
	int errOrBytesWritten = 0;
#ifdef CTRANSPORT_WOLFSSL

	//Set transient to inform our custom wolfssl callback not to send in a handshake context
	//CTSSLEncryptTransient transient = { 0, socketfd };
	//wolfSSL_SetIOWriteCtx(sslContextRef->ctx, &transient);

	CTSSLEncryptTransient* eTransient = wolfSSL_GetIOWriteCtx(sslContextRef->ctx);
	assert(eTransient);
	
	eTransient->wolf_socket_fd = socketfd;
	//eTransient->ct_socket_fd = 0;
	assert(eTransient->wolf_socket_fd != 0);
	//assert(eTransient->ct_socket_fd == 0);

	// Send the message to the server
	//if ( (errOrBytesWritten = wolfSSL_BIO_write(sslContextRef->bio, msg, *msgLength)) != *msgLength)
	if ((errOrBytesWritten = wolfSSL_write(sslContextRef->ctx, msg, *msgLength)) != *msgLength)
	{
		wolfSSL_get_error(sslContextRef->ctx, errOrBytesWritten);
		fprintf(stderr, "CTSSLWrite ERROR: wolfSSL_write failed to write with error: %d\n", (int)errOrBytesWritten);
		assert(1==0);
		errOrBytesWritten = -1;
	}
	*msgLength = errOrBytesWritten;
#elif defined( CTRANSPORT_USE_MBED_TLS )
	assert(&(sslContextRef->ctx) != NULL && sslContextRef->ctx.conf != NULL);

	errOrBytesWritten = mbedtls_ssl_write(&(sslContextRef->ctx), (const unsigned char *)msg, (size_t)*msgLength);
	if (errOrBytesWritten < 0) {
		if (errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_READ &&
			errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_WRITE) {
			Reql_fprintf(stderr, "ReqlSSLWrite::mbedtls_ssl_write error (%d)", errOrBytesWritten);
			//onError(_tcpsocket, -1);
		}
	}
	fprintf(stderr, "ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);
#elif defined(_WIN32)
    //int errOrBytesWritten = 0;
	//returns status, overwrites msgLength with encrypted bytes written
	errOrBytesWritten = CTSSLEncryptMessage(sslContextRef, msg, msgLength);
	if( errOrBytesWritten >-1 )
	{
	    // Send the encrypted data to the server.
		errOrBytesWritten += send( socketfd, (char*)sslContextRef->hBuffer, sslContextRef->Sizes.cbHeader/*Buffers[0].cbBuffer*/, 0);
		errOrBytesWritten += send( socketfd, (char*)msg, *msgLength, 0);
		errOrBytesWritten += send( socketfd, (char*)sslContextRef->tBuffer, sslContextRef->Sizes.cbTrailer/*Buffers[2].cbBuffer*/, 0);
		//fprintf(stderr, "%d bytes of encrypted data sent\n\n", errOrBytesWritten);
		//if(fVerbose) { PrintHexDump(errOrBytesWritten, (PBYTE)msg); fprintf(stderr, "\n"); }
		
	}
	*msgLength = errOrBytesWritten;
#elif defined(__APPLE__)
	//TO DO:  Move Secure Tranport SSLWrite call here
#endif
	return (int)errOrBytesWritten; // send( Socket, pbIoBuffer,    Sizes.cbHeader + strlen(pbMessage) + Sizes.cbTrailer,  0 );
}

