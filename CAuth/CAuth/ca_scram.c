//
//  ca_scram.c
//  ReØMQL
//
//  This is meant to be a x-platform SCRAM interface in C,
//  but currently only supports HMAC SHA-256 via Darwin Secure Transport API on OSX/iOS platforms.
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#include "stdafx.h"
#endif

#include "ca_scram.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assert.h"

#ifndef __APPLE__
#include <malloc.h>
#endif

#ifdef _WIN32

//Common Win 32
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

//CRYPT API
#define SECURITY_WIN32
//#define IO_BUFFER_SIZE  0x10000
//#define DLL_NAME TEXT("Secur32.dll")
//#define NT4_DLL_NAME TEXT("Security.dll")

//Crypt API Globals
//#include <winsock.h>
#include <WinCrypt.h>
//#include <wintrust.h>
//#include <schannel.h>
#include <security.h>
#include <sspi.h>

HCRYPTPROV   ca_scramCryptProv;

//CNG/BCrypt API Globals
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib") 
#define NT_SUCCESS(Status)          (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001L)

#ifndef BCRYPT_HASH_REUSABLE_FLAG
#define BCRYPT_HASH_REUSABLE_FLAG               0x00000020
#endif

//Algorithm + Hash Handles
 BCRYPT_ALG_HANDLE       g_bcryptHashAlg = NULL;
 BCRYPT_HASH_HANDLE      g_bcryptHash = NULL;

 BCRYPT_ALG_HANDLE       g_bcryptHMACAlg = NULL;
 BCRYPT_HASH_HANDLE      g_bcryptHMACHash = NULL;
 NTSTATUS                g_bcryptStatus = STATUS_UNSUCCESSFUL;

 //Hash Object Pointer for dynamic allocation
 PBYTE                   g_pbHashObject = NULL;
 PBYTE                   g_pbHash = NULL;
 DWORD					 g_cbHashObject = 0;

 PBYTE                   g_pbHMACHashObject = NULL;
 PBYTE                   g_pbHMACHash = NULL;
 DWORD					 g_cbHMACHashObject = 0;


#elif defined(__APPLE__)
#include <MacTypes.h>

#elif defined(__FreeBSD__)

struct cryptodev_ctx 
{
	int      cfd;
	struct   session2_op sess;
	uint16_t alignmask;
};

int                  g_cryptoDevice     = -1;
struct cryptodev_ctx g_cryptoHash       = {0};
struct cryptodev_ctx g_cryptoHMAC       = {0};

#endif


/*
SaltedPassword  := Hi(Normalize(password), salt, i)
ClientKey       := HMAC(SaltedPassword, "Client Key")
StoredKey       := H(ClientKey)
AuthMessage     := client-first-message-bare + "," +
server-first-message + "," +
client-final-message-without-proof
ClientSignature := HMAC(StoredKey, AuthMessage)
ClientProof     := ClientKey XOR ClientSignature
ServerKey       := HMAC(SaltedPassword, "Server Key")
ServerSignature := HMAC(ServerKey, AuthMessage)
*/

//
// Mapping from 6 bit pattern to ASCII character.
//
static unsigned char base64EncodeLookup[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//
// Definition for "masked-out" areas of the base64DecodeLookup mapping
//
#define xx 65

//
// Mapping from ASCII character to 6 bit pattern.
//
static unsigned char base64DecodeLookup[256] =
{
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, 62, xx, xx, xx, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, xx, xx, xx, xx, xx, xx,
    xx,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, xx, xx, xx, xx, xx,
    xx, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
    xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx, xx,
};

//
// Fundamental sizes of the binary and base64 encode/decode units in bytes
//
#define BINARY_UNIT_SIZE 3
#define BASE64_UNIT_SIZE 4


//
// cr_base64_to_utf8
//
// Decodes the base64 ASCII string in the inputBuffer to a newly malloced
// output buffer.
//
//  inputBuffer - the source ASCII string for the decode
//    length - the length of the string or -1 (to specify strlen should be used)
//    outputLength - if not-NULL, on output will contain the decoded length
//
// returns the decoded buffer. Must be free'd by caller. Length is given by
//    outputLength.
//
void * cr_base64_to_utf8(const char *inputBuffer,size_t length,size_t *outputLength)
{
	unsigned char *outputBuffer=NULL;
	size_t outputBufferSize = 0;
    size_t i = 0;
    size_t j = 0;
    if (length == -1)
    {
        length = strlen(inputBuffer);
    }
    outputBufferSize = ((length+BASE64_UNIT_SIZE-1) / BASE64_UNIT_SIZE) * BINARY_UNIT_SIZE;
    outputBuffer = (unsigned char *)malloc(outputBufferSize);
	memset(outputBuffer, 0, outputBufferSize);
	while (i < length)
    {
        //
        // Accumulate 4 valid characters (ignore everything else)
        //
        unsigned char accumulated[BASE64_UNIT_SIZE];
        size_t accumulateIndex = 0;
        while (i < length)
        {
            unsigned char decode = base64DecodeLookup[(unsigned)inputBuffer[i++]];
            if (decode != xx)
            {
                accumulated[accumulateIndex] = decode;
                accumulateIndex++;
                
                if (accumulateIndex == BASE64_UNIT_SIZE)
                {
                    break;
                }
            }
        }
        
        //
        // Store the 6 bits from each of the 4 characters as 3 bytes
        //
        // (Uses improved bounds checking suggested by Alexandre Colucci)
        //
        if(accumulateIndex >= 2)
            outputBuffer[j] = (accumulated[0] << 2) | (accumulated[1] >> 4);
        if(accumulateIndex >= 3)
            outputBuffer[j + 1] = (accumulated[1] << 4) | (accumulated[2] >> 2);
        if(accumulateIndex >= 4)
            outputBuffer[j + 2] = (accumulated[2] << 6) | accumulated[3];
        j += accumulateIndex - 1;
    }
    
    if (outputLength != NULL)
    {
        *outputLength = j;
    }
    return outputBuffer;
}


//
// cr_utf8_to_base64
//
// Encodes the arbitrary data in the inputBuffer as base64 into a newly malloced
// output buffer.
//
//  inputBuffer - the source data for the encode
//    length - the length of the input in bytes
//  separateLines - if zero, no CR/LF characters will be added. Otherwise
//        a CR/LF pair will be added every 64 encoded chars.
//    outputLength - if not-NULL, on output will contain the encoded length
//        (not including terminating 0 char)
//
// returns the encoded buffer. Must be free'd by caller. Length is given by
//    outputLength.
//
char *cr_utf8_to_base64(const void *buffer,size_t length, int separateLines,size_t *outputLength)
{
	char *outputBuffer;
	size_t lineLength;
	size_t lineEnd;
	size_t i = 0;
    size_t j = 0;
    const unsigned char *inputBuffer = (const unsigned char *)buffer;
    
#define MAX_NUM_PADDING_CHARS 2
#define OUTPUT_LINE_LENGTH 64
#define INPUT_LINE_LENGTH ((OUTPUT_LINE_LENGTH / BASE64_UNIT_SIZE) * BINARY_UNIT_SIZE)
#define CR_LF_SIZE 2
    

    //
    // Byte accurate calculation of final buffer size
    //
    size_t outputBufferSize =
    ((length / BINARY_UNIT_SIZE)
     + ((length % BINARY_UNIT_SIZE) ? 1 : 0))
    * BASE64_UNIT_SIZE;
    if (separateLines)
    {
        outputBufferSize +=
        (outputBufferSize / OUTPUT_LINE_LENGTH) * CR_LF_SIZE;
    }
    
    //
    // Include space for a terminating zero
    //
    outputBufferSize += 1;
    
    //
    // Allocate the output buffer
    //
    outputBuffer = (char *)malloc(outputBufferSize);
    if (!outputBuffer)
    {
        return NULL;
    }
    

    lineLength = separateLines ? INPUT_LINE_LENGTH : length;
    lineEnd = lineLength;
    
    while (true)
    {
        if (lineEnd > length)
        {
            lineEnd = length;
        }
        
        for (; i + BINARY_UNIT_SIZE - 1 < lineEnd; i += BINARY_UNIT_SIZE)
        {
            //
            // Inner loop: turn 48 bytes into 64 base64 characters
            //
            outputBuffer[j++] = base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2];
            outputBuffer[j++] = base64EncodeLookup[((inputBuffer[i] & 0x03) << 4)
                                                   | ((inputBuffer[i + 1] & 0xF0) >> 4)];
            outputBuffer[j++] = base64EncodeLookup[((inputBuffer[i + 1] & 0x0F) << 2)
                                                   | ((inputBuffer[i + 2] & 0xC0) >> 6)];
            outputBuffer[j++] = base64EncodeLookup[inputBuffer[i + 2] & 0x3F];
        }
        
        if (lineEnd == length)
        {
            break;
        }
        
        //
        // Add the newline
        //
        outputBuffer[j++] = '\r';
        outputBuffer[j++] = '\n';
        lineEnd += lineLength;
    }
    
    if (i + 1 < length)
    {
        //
        // Handle the single '=' case
        //
        outputBuffer[j++] = base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2];
        outputBuffer[j++] = base64EncodeLookup[((inputBuffer[i] & 0x03) << 4)
                                               | ((inputBuffer[i + 1] & 0xF0) >> 4)];
        outputBuffer[j++] = base64EncodeLookup[(inputBuffer[i + 1] & 0x0F) << 2];
        outputBuffer[j++] =    '=';
    }
    else if (i < length)
    {
        //
        // Handle the double '=' case
        //
        outputBuffer[j++] = base64EncodeLookup[(inputBuffer[i] & 0xFC) >> 2];
        outputBuffer[j++] = base64EncodeLookup[(inputBuffer[i] & 0x03) << 4];
        outputBuffer[j++] = '=';
        outputBuffer[j++] = '=';
    }
    outputBuffer[j] = 0;
    
    //
    // Set the output length and return the buffer
    //
    if (outputLength)
    {
        *outputLength = j;
    }
    return outputBuffer;
}

/*
void ca_scram_init_rng_algorithm()
{
	DWORD cbData;
	CHAR pszName[1000];
	//---------------------------------------------------------------
	// Open Default CSP (Crypto Service Provider)
	// Get a handle to the default PROV_RSA_FULL provider.
	if(CryptAcquireContext(
		&ca_scramCryptProv,		// handle to the CSP
		NULL,				// container name 
		NULL,				// use the default provider
		PROV_RSA_FULL,	// provider type
		0 ))					// flag values
	{
		fprintf(stderr, TEXT("ca_scram_init::CryptAcquireContext succeeded.\n"));
	}  
	else
	{
		if (GetLastError() == NTE_BAD_KEYSET)
		{
			fprintf(stderr, "NTE_BAD_KEYSET\n");
			// No default container was found. Attempt to create it.
			if(CryptAcquireContext(
				&ca_scramCryptProv, 
				NULL, 
				NULL, 
				PROV_RNG, 
				CRYPT_NEWKEYSET)) 
			{
				fprintf(stderr, TEXT("ca_scram_init::CryptAcquireContext succeeded.\n"));
			}
			else
			{
				fprintf(stderr, TEXT("Could not create the default key container.\n"));
			}
		}
		else
		{
			fprintf(stderr, TEXT("A general error running CryptAcquireContext."));
		}
	}

	//---------------------------------------------------------------
	// Read the name of the CSP.
	cbData = 1000;
	if(CryptGetProvParam(
		ca_scramCryptProv, 
		PP_NAME, 
		(BYTE*)pszName, 
		&cbData, 
		0))
	{
		//fprintf(stderr, TEXT("CryptGetProvParam succeeded.\n"));
		fprintf(stderr, "Provider name: %s\n", pszName);
	}
	else
	{
		fprintf(stderr, TEXT("Error reading CSP name.\n"));
	}

	//---------------------------------------------------------------
	// Read the name of the key container.
	cbData = 1000;
	if(CryptGetProvParam(
		ca_scramCryptProv, 
		PP_CONTAINER, 
		(BYTE*)pszName, 
		&cbData, 
		0))
	{
		//fprintf(stderr, TEXT("CryptGetProvParam succeeded.\n"));
		fprintf(stderr, "Key Container name: %s\n", pszName);
	}
	else
	{
		fprintf(stderr, TEXT("Error reading key container name.\n"));
	}

}
*/

#ifdef __FreeBSD__ //cryptodev SHA256 HASH + HMAC API

int cryptodev_sha256_ctx_init(struct cryptodev_ctx* ctx, int cfd, const char *key, unsigned int key_size)
{
#ifdef CIOCGSESSION2
    struct crypt_find_op fop = {0};
#elif defined(CIOCGSESSINFO)
    struct session_info_op siop = {0};
#endif

	memset(ctx, 0, sizeof(*ctx));
	ctx->cfd = cfd;

	if (key == NULL)
		ctx->sess.mac = CRYPTO_SHA2_256;
	else 
	{		
		//struct session_op {
		//       uint32_t	cipher;	   /* e.g. CRYPTO_AES_CBC */
		//       uint32_t	mac;	   /* e.g. CRYPTO_SHA2_256_HMAC	*/
		//       uint32_t	keylen;	   /* cipher key */
		//       const void *key;
		//       int mackeylen;	   /* mac key */
		//       const void *mackey;
		//       uint32_t	ses;	   /* returns: ses # */
		//};

		//ctx->sess.cipher = CRYPTO_AES_CBC;
		//ctx->sess.keylen = key_size;
		//ctx->sess.key = (void*)key;

		ctx->sess.mac = CRYPTO_SHA2_256_HMAC;
		ctx->sess.mackeylen = key_size;
		ctx->sess.mackey = (void*)key;
	}

#ifdef CIOCGSESSION2
	if (ioctl(ctx->cfd, CIOCGSESSION2, &ctx->sess)) {
		perror("ioctl(CIOCGSESSION2) 1");
		return -1;
	}
#else
	if (ioctl(ctx->cfd, CIOCGSESSION, &ctx->sess)) {
		perror("ioctl(CIOCGSESSION) 1");
		return -1;
	}
#endif

#ifdef CIOCGSESSION2
	fop.crid = ctx->sess.crid;
	if (ioctl(ctx->cfd, CIOCFINDDEV , &fop)) {
		perror("ioctl(CIOCFINDDEV)");
		return -1;
	}

	//printf("CIOCGSESSION2 Driver Name: %.*s\n", (int)strlen(fop.name), fop.name);

#elif defined(CIOCGSESSINFO)
	siop.ses = ctx->sess.ses;
	if (ioctl(ctx->cfd, CIOCGSESSINFO, &siop)) {
		perror("ioctl(CIOCGSESSINFO)");
		return -1;
	}
	
	/*printf("Alignmask is %x\n", (unsigned int)siop.alignmask);*/
	ctx->alignmask = siop.alignmask;
#endif

	return 0;
}

void cryptodev_sha256_ctx_deinit(struct cryptodev_ctx* ctx) 
{
	if (ioctl(ctx->cfd, CIOCFSESSION, &ctx->sess.ses)) {
		perror("ioctl(CIOCFSESSION)");
	}
}

int cryptodev_sha256_hash(struct cryptodev_ctx* ctx, const void* text, size_t size, void* digest)
{
	struct crypt_op cryp;
	void* p;
	
	/* check text and ciphertext alignment */
	if (ctx->alignmask) {
		p = (void*)(((unsigned long)text + ctx->alignmask) & ~ctx->alignmask);
		if (text != p) {
			fprintf(stderr, "text is not aligned\n");
			return -1;
		}
	}

	memset(&cryp, 0, sizeof(cryp));

	/* Encrypt data.in to data.encrypted */
	cryp.ses = ctx->sess.ses;
	cryp.len = size;
	cryp.src = (void*)text;
	cryp.mac = digest;
	if (ioctl(ctx->cfd, CIOCCRYPT, &cryp)) {
		perror("ioctl(CIOCCRYPT)");
		return -1;
	}

	return 0;
}
#endif

void ca_scram_init_hmac_algorithm(void)
{

#if defined(_WIN32)
	DWORD cbData = 0;
	//Init BCrypt Algorithm Handle
	//Beginning with Windows 8 and Windows Server 2012, you can create a reusable hashing object for scenarios 
	//that require you to compute multiple hashes or HMACs in rapid succession. Do this by specifying the BCRYPT_HASH_REUSABLE_FLAG 
	//when calling the BCryptOpenAlgorithmProvider function. All Microsoft hash algorithm providers support this flag. A hashing object 
	//created by using this flag can be reused immediately after calling BCryptFinishHash just as if it had been freshly created by calling BCryptCreateHash. 
	if (!NT_SUCCESS(g_bcryptStatus = BCryptOpenAlgorithmProvider(
        &g_bcryptHMACAlg,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        BCRYPT_ALG_HANDLE_HMAC_FLAG | BCRYPT_HASH_REUSABLE_FLAG)))
    {
        fprintf(stderr, "ca_scram_hmac_init::**** Error 0x%lx returned by BCryptOpenAlgorithmProvider\n", g_bcryptStatus);

    }

	//calculate the size of the buffer to hold the hash object
    if (!NT_SUCCESS(g_bcryptStatus = BCryptGetProperty(
        g_bcryptHMACAlg,
        BCRYPT_OBJECT_LENGTH,
        (PBYTE)&g_cbHMACHashObject,
        sizeof(DWORD),
        &cbData,
        0)))
    {
        fprintf(stderr, "ca_scram_hmac_init::**** Error 0x%lx returned by BCryptGetProperty\n", g_bcryptStatus);
        return;
    }

    //allocate the hash object on the heap
    g_pbHMACHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, g_cbHMACHashObject);
    if (NULL == g_pbHMACHashObject)
    {
        fprintf(stderr, "ca_scram_hmac_init::**** memory allocation failed\n");
        return;
    }
#elif defined (__FreeBSD__)
    cryptodev_sha256_ctx_init(&g_cryptoHash, g_cryptoDevice, NULL, 0);
#endif
}


void ca_scram_init_hash_algorithm(void)
{
#if defined(_WIN32)
	DWORD cbData = 0;
	//Init BCrypt Algorithm Handle
	//Beginning with Windows 8 and Windows Server 2012, you can create a reusable hashing object for scenarios 
	//that require you to compute multiple hashes or HMACs in rapid succession. Do this by specifying the BCRYPT_HASH_REUSABLE_FLAG 
	//when calling the BCryptOpenAlgorithmProvider function. All Microsoft hash algorithm providers support this flag. A hashing object 
	//created by using this flag can be reused immediately after calling BCryptFinishHash just as if it had been freshly created by calling BCryptCreateHash. 
	if (!NT_SUCCESS(g_bcryptStatus = BCryptOpenAlgorithmProvider(
        &g_bcryptHashAlg,
        BCRYPT_SHA256_ALGORITHM,
        NULL,
        BCRYPT_HASH_REUSABLE_FLAG)))
    {
        fprintf(stderr, "ca_scram_hash_init::**** Error 0x%lx returned by BCryptOpenAlgorithmProvider\n", g_bcryptStatus);

    }

	//calculate the size of the buffer to hold the hash object
    if (!NT_SUCCESS(g_bcryptStatus = BCryptGetProperty(
        g_bcryptHashAlg,
        BCRYPT_OBJECT_LENGTH,
        (PBYTE)&g_cbHashObject,
        sizeof(DWORD),
        &cbData,
        0)))
    {
        fprintf(stderr, "ca_scram_hash_init::**** Error 0x%lx returned by BCryptGetProperty\n", g_bcryptStatus);
        return;
    }

    //allocate the hash object on the heap
    g_pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, g_cbHashObject);
    if (NULL == g_pbHashObject)
    {
        fprintf(stderr, "ca_scram_hash_init::**** memory allocation failed\n");
        return;
    }

	//create a resuable hash
    if (!NT_SUCCESS(g_bcryptStatus = BCryptCreateHash(
        g_bcryptHashAlg,
        &g_bcryptHash,
        g_pbHashObject,
        g_cbHashObject,
		NULL,
        0,//sizeof(key)-1,
        BCRYPT_HASH_REUSABLE_FLAG )))
    {
        fprintf(stderr, "ca_scram_hmac::**** Error 0x%lx returned by BCryptCreateHash\n", g_bcryptStatus);
        return;
    }
#endif
}

void ca_scram_init_device(void)
{
#ifdef __FreeBSD__
    /* Open the crypto device */
	g_cryptoDevice = open("/dev/crypto", O_RDWR, 0);
	if (g_cryptoDevice < 0) {
		perror("ca_scram_init_device::open(/dev/crypto)");
		return;
	}
#endif
}

void ca_scram_init(void)
{
    ca_scram_init_device();
	//ca_scram_init_rng_algorithm();
	ca_scram_init_hmac_algorithm();
	ca_scram_init_hash_algorithm();
}

void ca_scram_cleanup(void)
{
	//TO DO:  Crypt API Cleanup
#if defined(_WIN32)
	//BCrypt SHA256 Hash clenaup
    if (g_bcryptHashAlg)
        BCryptCloseAlgorithmProvider(g_bcryptHashAlg, 0);
    if (g_bcryptHash)
        BCryptDestroyHash(g_bcryptHash);
    if (g_pbHashObject)
        HeapFree(GetProcessHeap(), 0, g_pbHashObject);
    if (g_pbHash)
        HeapFree(GetProcessHeap(), 0, g_pbHash);

	//BCrypt HMAC SHA256 Hash clenaup
    if (g_bcryptHMACAlg)
        BCryptCloseAlgorithmProvider(g_bcryptHMACAlg, 0);
    if (g_bcryptHMACHash)
        BCryptDestroyHash(g_bcryptHMACHash);
    if (g_pbHMACHashObject)
        HeapFree(GetProcessHeap(), 0, g_pbHMACHashObject);
    if (g_pbHMACHash)
        HeapFree(GetProcessHeap(), 0, g_pbHMACHash);
#elif defined( __FreeBSD__ )

    //clean up /dev/crypto sha256 global hash context
	cryptodev_sha256_ctx_deinit(&g_cryptoHash);

    //close /dev/crypto
	if (close(g_cryptoDevice) == -1) 
    {
		perror("close(g_cryptoDevice)");
		return;
	}

#endif
}

OSStatus ca_scram_gen_rand_bytes(char * byteBuffer, size_t numBytes)
{
	OSStatus status;
    size_t byteIndex = 0;

    //char *drbg = "drbg_nopr_sha256"; /* Hash DRBG with SHA-256, no PR */
    //uint8_t *seed = NULL;
    //uint32_t seedlen = 0;
    
    //Linux Kernel Crypto API example
    /*
    struct crypto_rng *rng = NULL;
    rng = crypto_alloc_rng(drbg, 0, 0);
    if (IS_ERR(rng)) {
        pr_debug("could not allocate RNG handle for %s\n", drbg);
        return PTR_ERR(rng);
    }
    */

    //libkcapi RNG example
    /*
    struct kcapi_handle *rng = NULL;
    status = kcapi_rng_init(&rng, drbg, 0);
	if (status) {
        printf("va_scram_gen_rand_bytes::Allocation of cipher %s failed\n", "va_rng_cipher");
        return status;
    }

	// invocation of seeding is mandatory even when seed is NuLL
	status = kcapi_rng_seed(rng, seed, seedlen);
	if (0 > status) {
		printf("va_scram_gen_rand_bytes::Failure to seed RNG %d\n", (int)status);
		kcapi_rng_destroy(rng);
		return 1;
	}
    */

    for (byteIndex = 0; byteIndex < numBytes;)
    {
//#ifdef VAUTH_OPENSSL
//        status = !RAND_bytes(byteBuffer + byteIndex, 1); //RAND_bytes results in 10 memory leaks for OpenSSL 1.1.1f!!!
#if defined(__APPLE__)
        status = SecRandomCopyBytes(kSecRandomDefault, 1, byteBuffer + byteIndex);
#elif defined(_WIN32) 
        //status = !CryptGenRandom(va_scramCryptProv, 1, (BYTE*)(byteBuffer + byteIndex));
#ifndef BCRYPT_USE_SYSTEM_PREFERRED_RNG
#define BCRYPT_USE_SYSTEM_PREFERRED_RNG     0x00000002
#endif
        status = BCryptGenRandom(NULL, (BYTE*)(byteBuffer + byteIndex), 1, BCRYPT_USE_SYSTEM_PREFERRED_RNG); // Flags                  
#else
        //TO DO:  more work to be done here using HAVE_RANDOM preprocessor define
        status = getrandom(byteBuffer + byteIndex, 1, 0) != 1;               //call getrandom c call
        //status = syscall(SYS_getrandom, byteBuffer+bytesIndex, numBytes);  //call getrandom as system call
        //status = crypto_rng_get_bytes(rng, byteBuffer+byteIndex, 1) != 1;  //Linux Kernel Crypto API
        //statuse= kcapi_rng_generate(rng, byteBuffer+byteIndex, 1) != 1;    //LibKCAPI
#endif
        if (status != 0) { // Always test the status.
            printf("ca_scram_gen_rand_bytes failed with error:  %d\n", 0);
            //crypto_free_rng(rng);
            //kcapi_rng_destroy(rng);
            return status;
        }
        //Only fill buffer with valid alphanumeric UTF-8 bytes
        if ((byteBuffer[byteIndex] > 47 && byteBuffer[byteIndex] < 58)
            || (byteBuffer[byteIndex] > 64 && byteBuffer[byteIndex] < 91)
            || (byteBuffer[byteIndex] > 96 && byteBuffer[byteIndex] < 123))
            byteIndex++;
    }
    //crypto_free_rng(rng);     //Linux Kernel Crypto API RNG Destroy
    //kcapi_rng_destroy(rng);   //LibKCAPI RNG Destroy
    return 0;
}


//extern unsigned char *CC_SHA256(const void *data, CC_LONG len, unsigned char *md)
void ca_scram_hash(const char * data, size_t dataLength, char * hashBuffer)
{
#ifdef _WIN32
	 //hash some data
    if (!NT_SUCCESS(g_bcryptStatus = BCryptHashData(
        g_bcryptHash,
        (PBYTE)data,
        (ULONG)dataLength,
        0)))
    {
        fprintf(stderr, "ca_scram_hmac::**** Error 0x%lx returned by BCryptHashData\n", g_bcryptStatus);
        return;
    }

    //close the hash
    if (!NT_SUCCESS(g_bcryptStatus = BCryptFinishHash(
        g_bcryptHash,
        (PUCHAR)hashBuffer,
        CC_SHA256_DIGEST_LENGTH,
        0)))
    {
        fprintf(stderr, "ca_scram_hmac::**** Error 0x%lx returned by BCryptFinishHash\n", g_bcryptStatus);
        return;
    }
#elif defined(__APPLE__)
    //the return value is the 3rd input ptr
    CC_SHA256((const void*)data, (CC_LONG)dataLength, (unsigned char*)hashBuffer);
#elif defined(__FreeBSD__)

    //int cfd = -1, i;
	//struct cryptodev_ctx ctx = {0};
	
	/* Open the crypto device */
    /*
	cfd = open("/dev/crypto", O_RDWR, 0);
	if (cfd < 0) {
		perror("ca_scram_hash::open(/dev/crypto)");
		return;
	}
    */

	/* Clone file descriptor */
	//if (ioctl(fd, CRIOGET, &cfd)) {
	//	perror("ioctl(CRIOGET)");
	//	return 1;
	//}

	/* Set close-on-exec (not really neede here) */
	/*
    if (fcntl(g_cryptoDevice, F_SETFD, 1) == -1) {
		perror("ca_scram_hash::fcntl(F_SETFD)");
		return;
	}
    */

	//cryptodev_sha256_ctx_init(&ctx, g_cryptoDevice, NULL, 0);
	cryptodev_sha256_hash(&g_cryptoHash, data, dataLength, hashBuffer);
	//cryptodev_sha256_ctx_deinit(&ctx);

    /*
	printf("ca_scram_hash::hashBuffer: ");
	for (i = 0; i < 20; i++) {
		printf("%02x:", digest[i]);
	}
	printf("\n");
	
	if (memcmp(digest, expected, 20) != 0) {
		fprintf(stderr, "SHA1 hashing failed\n");
		return 1;
	}
    */
    
	/* Close cloned descriptor */
    /*
	if (close(cfd)) {
		perror("close(cfd)");
		return;
	}
    */
#endif
}

//HMAC SHA 256 on Darwin;  Input Must be UTF8
//hexBuffer output must be allocated 64 bits or larger
void ca_scram_hmac(const char * secretKey, size_t secretKeyLen, const char * data, size_t dataLen, char * hmacBuffer)
{
#ifdef _WIN32

    //PBYTE   pbHash;
    //DWORD   cbData = 0,cbHash = 0, cbHashObject = 0;

	//Note the length of the hash will always be 32 for SHIA256
	//The buffer will be supplied as input to this function
	/*
    //calculate the length of the hash
    if (!NT_SUCCESS(g_bcryptStatus = BCryptGetProperty(
        g_bcryptAlg,
        BCRYPT_HASH_LENGTH,
        (PBYTE)&cbHash,
        sizeof(DWORD),
        &cbData,
        0)))
    {
        fprintf(stderr, "ca_scram_hmac::**** Error 0x%x returned by BCryptGetProperty\n", g_bcryptStatus);
        return;
    }

    //allocate the hash buffer on the heap
    pbHash = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHash);
    if (NULL == pbHash)
    {
        fprintf(stderr, "ca_scram_hmac::**** memory allocation failed\n");
        return;
    }
	*/
    //create a resuable hmac hash
    if (!NT_SUCCESS(g_bcryptStatus = BCryptCreateHash(
        g_bcryptHMACAlg,
        &g_bcryptHMACHash,
        g_pbHMACHashObject,
        g_cbHMACHashObject,
        (PBYTE)secretKey,
        (ULONG)secretKeyLen,//sizeof(key)-1,
        BCRYPT_HASH_REUSABLE_FLAG )))
    {
        fprintf(stderr, "ca_scram_hmac::**** Error 0x%lx returned by BCryptCreateHash\n", g_bcryptStatus);
        return;
    }

    //hash some data
    if (!NT_SUCCESS(g_bcryptStatus = BCryptHashData(
        g_bcryptHMACHash,
        (PBYTE)data,
        (ULONG)dataLen,
        0)))
    {
        fprintf(stderr, "ca_scram_hmac::**** Error 0x%lx returned by BCryptHashData\n", g_bcryptStatus);
        return;
    }

    //close the hash
    if (!NT_SUCCESS(g_bcryptStatus = BCryptFinishHash(
        g_bcryptHMACHash,
        (PUCHAR)hmacBuffer,
        CC_SHA256_DIGEST_LENGTH,
        0)))
    {
        fprintf(stderr, "ca_scram_hmac::**** Error 0x%lx returned by BCryptFinishHash\n", g_bcryptStatus);
        return;
    }

	/*
    fprintf(stderr, "The hash is:  ");
    for (DWORD i = 0; i < cbHash; i++)
    {
        fprintf(stderr, "%2.2X-", pbHash[i]);
    }
	*/
#elif defined(__APPLE__)
    CCHmac(kCCHmacAlgSHA256, secretKey, secretKeyLen, data, dataLen, hmacBuffer);
#elif defined(__FreeBSD__) //cryptodev kernel api
    
    int i;
    //int cfd = -1;
	struct cryptodev_ctx ctx = {0};

    /* Open the crypto device */
    /*
	cfd = open("/dev/crypto", O_RDWR, 0);
	if (cfd < 0) {
		perror("ca_scram_hmac::open(/dev/crypto)");
		return;
	}
    */

	/* Clone file descriptor */
	//if (ioctl(fd, CRIOGET, &cfd)) {
	//	perror("ioctl(CRIOGET)");
	//	return 1;
	//}
    
    //cfd = dup(g_cryptoDevice);
    //assert(cfd != -1);
	
	/* Set close-on-exec (not really neede here) */
    /*
	if (fcntl(g_cryptoDevice, F_SETFD, 1) == -1) {
		perror("ca_scram_hmac::fcntl(F_SETFD)");
		return;
	}
    */
    
	cryptodev_sha256_ctx_init(&ctx, g_cryptoDevice, secretKey, secretKeyLen);
	cryptodev_sha256_hash(&ctx, data, dataLen, hmacBuffer);
	cryptodev_sha256_ctx_deinit(&ctx);

    /*
	printf("ca_scram_hmac::hmacBuffer: ");
	for (i = 0; i < 20; i++) {
		printf("%02x:", digest[i]);
	}
	printf("\n");
	
	if (memcmp(digest, expected, 20) != 0) {
		fprintf(stderr, "SHA1 hashing failed\n");
		return 1;
	}
    */

	/* Close cloned descriptor */
    /*
	if (close(cfd)) {
		perror("close(cfd)");
		return;
	}
    */
#endif
}

//SCRAM Hi Algorithm Step
void ca_scram_salt_password(char * pw, size_t pwLen, char * salt, size_t saltLen, int ni, char * saltedPassword)
{
	int i, charIndex;
    char hmacBuffer[2][CC_SHA256_DIGEST_LENGTH];    //a buffer to hold hmac at each iteration
    
    //Allocate memory for and populate modified salt string
    char one = 1;
    size_t saltModLen = saltLen + 4;//sizeof(int32_t);
    char * saltMod = (char*)malloc( saltModLen );
    //int32_t one = 1;
    //int32_t saltAddendum = OSSwapHostToBigInt32(one);
    memcpy(saltMod,salt,saltLen);    //copy the salt
    saltMod[saltLen]    = 0;
    saltMod[saltLen+1]  = 0;
    saltMod[saltLen+2]  = 0;
    saltMod[saltLen+3]  = one;
 
	memset(hmacBuffer[0], 0, CC_SHA256_DIGEST_LENGTH);
	memset(hmacBuffer[1], 0, CC_SHA256_DIGEST_LENGTH);

//#ifdef __APPLE__
    //From 1 to i...or 0 to i-1
    //Calculate a SHA-256 keyed HMAC for the salt, using the password as the secret key
    ca_scram_hmac(pw, pwLen, saltMod, saltModLen, hmacBuffer[0]);
//#endif    
    
    //copy the hmac hex string to the xorbuffer
    memcpy(saltedPassword, hmacBuffer, CC_SHA256_DIGEST_LENGTH);
    
    //For each iteration do another sha256 keyed hash (using the password as key)
    //and xor the result in an accumulation buffer
    for(i = 1; i < ni; i++)
    {
        ca_scram_hmac( pw, pwLen, hmacBuffer[(i-1)%2], CC_SHA256_DIGEST_LENGTH, hmacBuffer[i%2] );

        //each byte of the previous output must be XOR'd with that of the previous output
        for(charIndex=0; charIndex<CC_SHA256_DIGEST_LENGTH; charIndex++)
        {
            saltedPassword[charIndex] = (char)(saltedPassword[charIndex] ^ hmacBuffer[i%2][charIndex]);
            //fprintf(stderr, "%d\n", charIndex);
        }
        //fprintf(stderr, "iteration:%d\n", i);

    }

	free(saltMod);
}


//a hex function for convenience
void ca_scram_hex(unsigned char * in, size_t insz, char * out, size_t outsz)
{
    unsigned char * pin = in;
    const char * hex = "0123456789ABCDEF";
    char * pout = out;
    for(; pin < in+insz; pout +=2, pin++){
        pout[0] = hex[(*pin>>4) & 0xF];
        pout[1] = hex[ *pin     & 0xF];
        //pout[2] = ':';
        if (pout + 2 - out > outsz){
            /* Better to truncate output string than overflow buffer */
            /* it would be still better to either return a status */
            /* or ensure the target buffer is large enough and it never happen */
            break;
        }
    }
    if( (unsigned char*)pout - pin < outsz)
        pout[0] = 0;
}


