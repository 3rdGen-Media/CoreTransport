#ifndef CTRANSPORT_H
#define CTRANSPORT_H

#ifdef __cplusplus 
extern "C" {
#endif


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/

/* If we are we on Views, we want a single define for it.
 */
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
 #define _WIN32
#endif /* _WIN32 */


#if defined(CTRANSPORT_DLL) && defined(_CTRANSPORT_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both CTRANSPORT_DLL and _CTRANSPORT_BUILD_DLL defined"
#endif

/* CTRANSPORT_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_CTRANSPORT_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define CTRANSPORT_API __declspec(dllexport)
#elif defined(_WIN32) && defined(CTRANSPORT_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define CTRANSPORT_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_CTRANSPORT_BUILD_DLL)
 /* We are building CTRANSPORT as a shared / dynamic library */
 #define CTRANSPORT_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define CTRANSPORT_API extern
#endif


#ifndef CTRANSPORT_PACK_ATTRIBUTE
#ifdef _WIN32
#define CTRANSPORT_PACK_ATTRIBUTE 
#else
#define CTRANSPORT_PACK_ATTRIBUTE __attribute__ ((packed))
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef CTRANSPORT_ALIGN//(X)
#ifdef _WIN32
#define CTRANSPORT_ALIGN(X) __declspec(align(X))//(align(X))
#else
#define CTRANSPORT_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CTRANSPORT_INLINE
#ifdef _WIN32
#define CTRANSPORT_INLINE
#else
#define CTRANSPORT_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CTRANSPORT_DECLSPEC
#ifdef _WIN32
#define CTRANSPORT_DECLSPEC __declspec
#else
#define CTRANSPORT_DECLSPEC
#endif
#endif


//#define Reql_printf(f_, ...) printf((f_), __VA_ARGS__)

//shared outgoing request and incoming response header buffer size
#define CT_REQUEST_BUFFER_SIZE 65536L

//switch to use native encryption vs mbedtls static lib
//#define CTRANSPORT_USE_MBED_TLS

//Core Headers/"Classes" Needed To Create a Secure Socket Connection

#ifdef _WIN32
#include "CTransport/CTSystem.h"
#include "CTransport/CTEndian.h"
#include "CTransport/CTFile.h"
#include "CTransport/CTCoroutine.h"
#include "CTransport/CTError.h"
#include "CTransport/CTDNS.h"
#include "CTransport/CTSocket.h"
#include "CTransport/CTSSL.h"
#include "CTransport/CTURL.h"
#include "CTransport/CTConnection.h"
#include "CTransport/CTQueue.h"

//Exposed API and Protocols extensions
#include "CTransport/CTReQL.h"
#include "CTransport/CTransportAPI.h"

#else

#include "CTransport/CTSystem.h"
#include "CTransport/CTEndian.h"
#include "CTransport/CTFile.h"
#include "CTransport/CTCoroutine.h"
#include "CTransport/CTError.h"
#include "CTransport/CTDNS.h"
#include "CTransport/CTSocket.h"
#include "CTransport/CTSSL.h"
#include "CTransport/CTURL.h"
#include "CTransport/CTConnection.h"
#include "CTransport/CTQueue.h"

//Exposed API and Protocols extensions
//#include "CTransport/CTReQL.h"
#include "CTransport/CTransportAPI.h"

#endif

#ifdef __cplusplus
}
#endif

#endif