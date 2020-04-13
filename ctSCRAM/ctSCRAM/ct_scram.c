//
//  ct_scram.c
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

#include "ct_scram.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "assert.h"


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

HCRYPTPROV   ct_scramCryptProv;

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
CT_SCRAM_API CT_SCRAM_INLINE void * cr_base64_to_utf8(const char *inputBuffer,size_t length,size_t *outputLength)
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
            unsigned char decode = base64DecodeLookup[inputBuffer[i++]];
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
CT_SCRAM_API CT_SCRAM_INLINE char *cr_utf8_to_base64(const void *buffer,size_t length,bool separateLines,size_t *outputLength)
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
void ct_scram_init_rng_algorithm()
{
	DWORD cbData;
	CHAR pszName[1000];
	//---------------------------------------------------------------
	// Open Default CSP (Crypto Service Provider)
	// Get a handle to the default PROV_RSA_FULL provider.
	if(CryptAcquireContext(
		&ct_scramCryptProv,		// handle to the CSP
		NULL,				// container name 
		NULL,				// use the default provider
		PROV_RSA_FULL,	// provider type
		0 ))					// flag values
	{
		printf(TEXT("ct_scram_init::CryptAcquireContext succeeded.\n"));
	}  
	else
	{
		if (GetLastError() == NTE_BAD_KEYSET)
		{
			printf("NTE_BAD_KEYSET\n");
			// No default container was found. Attempt to create it.
			if(CryptAcquireContext(
				&ct_scramCryptProv, 
				NULL, 
				NULL, 
				PROV_RNG, 
				CRYPT_NEWKEYSET)) 
			{
				printf(TEXT("ct_scram_init::CryptAcquireContext succeeded.\n"));
			}
			else
			{
				printf(TEXT("Could not create the default key container.\n"));
			}
		}
		else
		{
			printf(TEXT("A general error running CryptAcquireContext."));
		}
	}

	//---------------------------------------------------------------
	// Read the name of the CSP.
	cbData = 1000;
	if(CryptGetProvParam(
		ct_scramCryptProv, 
		PP_NAME, 
		(BYTE*)pszName, 
		&cbData, 
		0))
	{
		//printf(TEXT("CryptGetProvParam succeeded.\n"));
		printf("Provider name: %s\n", pszName);
	}
	else
	{
		printf(TEXT("Error reading CSP name.\n"));
	}

	//---------------------------------------------------------------
	// Read the name of the key container.
	cbData = 1000;
	if(CryptGetProvParam(
		ct_scramCryptProv, 
		PP_CONTAINER, 
		(BYTE*)pszName, 
		&cbData, 
		0))
	{
		//printf(TEXT("CryptGetProvParam succeeded.\n"));
		printf("Key Container name: %s\n", pszName);
	}
	else
	{
		printf(TEXT("Error reading key container name.\n"));
	}

}
*/

void ct_scram_init_hmac_algorithm()
{
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
        printf("ct_scram_hmac_init::**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", g_bcryptStatus);

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
        printf("ct_scram_hmac_init::**** Error 0x%x returned by BCryptGetProperty\n", g_bcryptStatus);
        return;
    }

    //allocate the hash object on the heap
    g_pbHMACHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, g_cbHMACHashObject);
    if (NULL == g_pbHMACHashObject)
    {
        printf("ct_scram_hmac_init::**** memory allocation failed\n");
        return;
    }
}


void ct_scram_init_hash_algorithm()
{
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
        printf("ct_scram_hash_init::**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", g_bcryptStatus);

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
        printf("ct_scram_hash_init::**** Error 0x%x returned by BCryptGetProperty\n", g_bcryptStatus);
        return;
    }

    //allocate the hash object on the heap
    g_pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, g_cbHashObject);
    if (NULL == g_pbHashObject)
    {
        printf("ct_scram_hash_init::**** memory allocation failed\n");
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
        printf("ct_scram_hmac::**** Error 0x%x returned by BCryptCreateHash\n", g_bcryptStatus);
        return;
    }
}


CT_SCRAM_API CT_SCRAM_INLINE void ct_scram_init()
{
	//ct_scram_init_rng_algorithm();
	ct_scram_init_hmac_algorithm();
	ct_scram_init_hash_algorithm();
}

void ct_scram_cleanup()
{
	//TO DO:  Crypt API Cleanup

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

}

CT_SCRAM_API CT_SCRAM_INLINE OSStatus ct_scram_gen_rand_bytes(char * byteBuffer, size_t numBytes)
{
	OSStatus status;
	size_t byteIndex=0;
    for (byteIndex=0; byteIndex<numBytes;)
    {
#ifdef __APPLE__
        status = SecRandomCopyBytes(kSecRandomDefault, 1, byteBuffer + byteIndex);
#else
		//status = !CryptGenRandom(ct_scramCryptProv, 1, (BYTE*)(byteBuffer + byteIndex));
		status = BCryptGenRandom(NULL, (BYTE*)(byteBuffer + byteIndex), 1, BCRYPT_USE_SYSTEM_PREFERRED_RNG); // Flags                  
#endif
		if (status != 0) { // Always test the status.
            printf("ct_scram_gen_rand_bytes failed with error:  %d\n", status);
            return status;
        }
        //Only fill buffer with valid alphanumeric UTF-8 bytes
        if((byteBuffer[byteIndex] > 47 && byteBuffer[byteIndex] < 58)
           || (byteBuffer[byteIndex] > 64 && byteBuffer[byteIndex] < 91)
           || (byteBuffer[byteIndex] > 96 && byteBuffer[byteIndex] < 123 ))
            byteIndex++;
    }
	return noErr;
}


//extern unsigned char *CC_SHA256(const void *data, CC_LONG len, unsigned char *md)
CT_SCRAM_API CT_SCRAM_INLINE void ct_scram_hash(const char * data, size_t dataLength, char * hashBuffer)
{
#ifdef _WIN32
	 //hash some data
    if (!NT_SUCCESS(g_bcryptStatus = BCryptHashData(
        g_bcryptHash,
        (PBYTE)data,
        (ULONG)dataLength,
        0)))
    {
        printf("ct_scram_hmac::**** Error 0x%x returned by BCryptHashData\n", g_bcryptStatus);
        return;
    }

    //close the hash
    if (!NT_SUCCESS(g_bcryptStatus = BCryptFinishHash(
        g_bcryptHash,
        (PUCHAR)hashBuffer,
        CC_SHA256_DIGEST_LENGTH,
        0)))
    {
        printf("ct_scram_hmac::**** Error 0x%x returned by BCryptFinishHash\n", g_bcryptStatus);
        return;
    }
#elif defined(__APPLE__)
    //the return value is the 3rd input ptr
    CC_SHA256((const void*)data, (CC_LONG)dataLength, (unsigned char*)hashBuffer);
#endif
}

//HMAC SHA 256 on Darwin;  Input Must be UTF8
//hexBuffer output must be allocated 64 bits or larger
CT_SCRAM_API CT_SCRAM_INLINE void ct_scram_hmac(const char * secretKey, size_t secretKeyLen, const char * data, size_t dataLen, char * hmacBuffer)
{
#ifdef _WIN32
	DWORD                   cbData = 0,cbHash = 0,cbHashObject = 0;
	//PBYTE   pbHash;

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
        printf("ct_scram_hmac::**** Error 0x%x returned by BCryptGetProperty\n", g_bcryptStatus);
        return;
    }

    //allocate the hash buffer on the heap
    pbHash = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHash);
    if (NULL == pbHash)
    {
        printf("ct_scram_hmac::**** memory allocation failed\n");
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
        printf("ct_scram_hmac::**** Error 0x%x returned by BCryptCreateHash\n", g_bcryptStatus);
        return;
    }

    //hash some data
    if (!NT_SUCCESS(g_bcryptStatus = BCryptHashData(
        g_bcryptHMACHash,
        (PBYTE)data,
        (ULONG)dataLen,
        0)))
    {
        printf("ct_scram_hmac::**** Error 0x%x returned by BCryptHashData\n", g_bcryptStatus);
        return;
    }

    //close the hash
    if (!NT_SUCCESS(g_bcryptStatus = BCryptFinishHash(
        g_bcryptHMACHash,
        (PUCHAR)hmacBuffer,
        CC_SHA256_DIGEST_LENGTH,
        0)))
    {
        printf("ct_scram_hmac::**** Error 0x%x returned by BCryptFinishHash\n", g_bcryptStatus);
        return;
    }

	/*
    printf("The hash is:  ");
    for (DWORD i = 0; i < cbHash; i++)
    {
        printf("%2.2X-", pbHash[i]);
    }
	*/
#elif defined(__APPLE__)
    CCHmac(kCCHmacAlgSHA256, secretKey, secretKeyLen, data, dataLen, hmacBuffer);
#endif
}

//SCRAM Hi Algorithm Step
CT_SCRAM_API CT_SCRAM_INLINE void ct_scram_salt_password(char * pw, size_t pwLen, char * salt, size_t saltLen, int ni, char * saltedPassword)
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
    ct_scram_hmac(pw, pwLen, saltMod, saltModLen, hmacBuffer[0]);
//#endif    
    
    //copy the hmac hex string to the xorbuffer
    memcpy(saltedPassword, hmacBuffer, CC_SHA256_DIGEST_LENGTH);
    
    //For each iteration do another sha256 keyed hash (using the password as key)
    //and xor the result in an accumulation buffer
    for(i = 1; i < ni; i++)
    {
        ct_scram_hmac( pw, pwLen, hmacBuffer[(i-1)%2], CC_SHA256_DIGEST_LENGTH, hmacBuffer[i%2] );
        //each byte of the previous output must be XOR'd with that of the previous output
        for(charIndex=0; charIndex<CC_SHA256_DIGEST_LENGTH; charIndex++)
            saltedPassword[charIndex] = (char)(saltedPassword[charIndex] ^ hmacBuffer[i%2][charIndex]);
    }

	free(saltMod);
}


//a hex function for convenience
CT_SCRAM_API CT_SCRAM_INLINE void ct_scram_hex(unsigned char * in, size_t insz, char * out, size_t outsz)
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


