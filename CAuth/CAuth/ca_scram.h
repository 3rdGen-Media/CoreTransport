//
//  ca_scram.h
//  ReØMQL
//
//  This is meant to be a x-platform SCRAM interface in C,
//  but currently only supports HMAC SHA-256 via Darwin Secure Transport API on OSX/iOS platforms.
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#ifndef ca_scram_h
#define ca_scram_h



//WIN32 PLATFORM DEPENDENCIES
#if defined(_WIN32)         //MICROSOFT WINDOWS PLATFORMS
//Std C Header Includes
//For Win32 it is imperative that stdio gets included
//before platform headers, or printf with throw a "redefinition; different linkage error"
#include <stdio.h>


#define NOMINMAX
#define NTDDI_VERSION NTDDI_WIN7
#define _WIN32_WINNT _WIN32_WINNT_WIN7 

// standard definitions
#define STRICT                                                  // enable strict type-checking of Windows handles
#define WIN32_LEAN_AND_MEAN                                     // allow the exclusion of uncommon features
#define WINVER                                          _WIN32_WINNT_WIN7  // allow the use of Windows XP specific features
#define _WIN32_WINNT                                    _WIN32_WINNT_WIN7  // allow the use of Windows XP specific features
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES         1       // use the new secure functions in the CRT
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT   1       // use the new secure functions in the CRT


#include <windows.h>            // fundamental Windows header file
#define GLDECL WINAPI

#define _USE_MATH_DEFINES	//holy crap!!! must define this on ms windows to get M_PI definition!
#include <math.h>

#define THREADPROC WINAPI   //What does this do again?

#elif defined(__FreeBSD__)
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/random.h>
#include <crypto/cryptodev.h>
#include <errno.h>

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OSStatus
typedef signed int OSStatus;
static const OSStatus noErr = 0;
#endif


/* Don't define bool, true, and false in C++, except as a GNU extension. */
#ifndef __cplusplus

#ifdef _Bool
#define bool _Bool
#elif !defined(bool)
#define bool int
#endif

#define true 1
#define false 0

#elif defined(__GNUC__) && !defined(__STRICA_ANSI__)
/* Define _Bool, bool, false, true as a GNU extension. */
#define _Bool bool
#define bool  bool
#define false false
#define true  true

#endif


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/

/* If we are we on Views, we want a single define for it.
 */
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
 #define _WIN32
#endif /* _WIN32 */


#if defined(CA_SCRAM_DLL) && defined(_CA_SCRAM_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both CA_SCRAM_DLL and _CA_SCRAM_BUILD_DLL defined"
#endif

/* CA_SCRAM_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_CA_SCRAM_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define CA_SCRAM_API __declspec(dllexport)
#elif defined(_WIN32) && defined(CA_SCRAM_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define CA_SCRAM_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_CA_SCRAM_BUILD_DLL)
 /* We are building CA_SCRAM as a shared / dynamic library */
 #define CA_SCRAM_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define CA_SCRAM_API extern
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CA_SCRAM_INLINE
#ifdef _WIN32
#define CA_SCRAM_INLINE __inline
#else
#define CA_SCRAM_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CA_SCRAM_DECLSPEC
#ifdef _WIN32
#define CA_SCRAM_DECLSPEC __declspec
#else
#define CA_SCRAM_DECLSPEC
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef CA_SCRAM_ALIGN//(X)
#ifdef _WIN32
#define CA_SCRAM_ALIGN(X) (align(X))
#else
#define CA_SCRAM_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

#ifndef CC_SHA256_DIGEST_LENGTH
#define CC_SHA256_DIGEST_LENGTH 32
#endif

#ifndef CC_SHA256_BLOCK_BYTES
#define CC_SHA256_BLOCK_BYTES 64
#endif


#ifndef MAX
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifdef __APPLE__
#include <CommonCrypto/CommonCrypto.h>
#include <Security/SecRandom.h>             //Random Number Generation
#endif

//Base 64 <-> UTF8 helper functions
CA_SCRAM_API CA_SCRAM_INLINE void * cr_base64_to_utf8(const char *inputBuffer,size_t length,size_t *outputLength);
CA_SCRAM_API CA_SCRAM_INLINE char * cr_utf8_to_base64(const void *buffer,size_t length,bool separateLines,size_t *outputLength);
    
//Init
CA_SCRAM_API CA_SCRAM_INLINE void ca_scram_init();
CA_SCRAM_API CA_SCRAM_INLINE void ca_scram_cleanup();

//Generate UTF-8 Nonce
CA_SCRAM_API CA_SCRAM_INLINE OSStatus ca_scram_gen_rand_bytes(char * byteBuffer, size_t numBytes);
//SHA 256 HASH
CA_SCRAM_API CA_SCRAM_INLINE void ca_scram_hash(const char * data, size_t dataLength, char * hashBuffer);
//HMAC SHA 256
CA_SCRAM_API CA_SCRAM_INLINE void ca_scram_hmac(const char * secretKey, size_t secretKeyLen, const char * data, size_t dataLen, char * hmacBuffer);
//Iterative SHA 256 password salting
CA_SCRAM_API CA_SCRAM_INLINE void ca_scram_salt_password(char * pw, size_t pwLen, char * salt, size_t saltLen, int ni, char * saltedPassword);

CA_SCRAM_API CA_SCRAM_INLINE void ca_scram_hex(unsigned char * in, size_t insz, char * out, size_t outsz);

#ifdef __cplusplus
}
#endif

#endif /* ca_scram_h */
