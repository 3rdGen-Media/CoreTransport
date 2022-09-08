#pragma once
#ifndef NSTRANSPORT_H
#define NSTRANSPORT_H


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/

/* If we are we on Views, we want a single define for it.
 */
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
 #define _WIN32
#endif /* _WIN32 */


#if defined(NSTRANSPORT_DLL) && defined(_NSTRANSPORT_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both NSTRANSPORT_DLL and _NSTRANSPORT_BUILD_DLL defined"
#endif

/* NSTRANSPORT_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_NSTRANSPORT_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define NSTRANSPORT_API __declspec(dllexport)
#elif defined(_WIN32) && defined(NSTRANSPORT_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define NSTRANSPORT_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_NSTRANSPORT_BUILD_DLL)
 /* We are building REQL as a shared / dynamic library */
 #define NSTRANSPORT_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define NSTRANSPORT_API //extern
#endif


#ifndef NSTRANSPORT_PACK_ATTRIBUTE
#ifdef _WIN32
#define NSTRANSPORT_PACK_ATTRIBUTE
#else
#define NSTRANSPORT_PACK_ATTRIBUTE __attribute__ ((packed))
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef NSTRANSPORT_ALIGN//(X)
#ifdef _WIN32
#define NSTRANSPORT_ALIGN(X) (align(X))
#else
#define NSTRANSPORT_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef NSTRANSPORT_INLINE
#ifdef _WIN32
#define NSTRANSPORT_INLINE
#else
#define NSTRANSPORT_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef NSTRANSPORT_DECLSPEC
#ifdef _WIN32
#define NSTRANSPORT_DECLSPEC __declspec
#else
#define NSTRANSPORT_DECLSPEC
#endif
#endif

//#define CT_fprintf(stderr, f_, ...) fprintf(stderr, (f_), __VA_ARGS__)

//statically tune the outgoing request/query buffer size
#define NS_REQUEST_BUFFER_SIZE 65536L

//The response buffer uses memory mapped file that is a multiple of the sytem granularity
#define NS_GRANULARITY_SCALAR  256L

#include <CoreTransport/CTransport.h>
#include "NSTransport/NSTConnection.h"
#include "NSTransport/NSTCursor.h"
#include "NSTransport/NSTQueue.h"
//#include "CXReQL/CXReQLService.h"
//#include "CXReQL/CXReQLConnection.h"
//#include "CXReQL/CXURLRequest.h"
//#include "CXReQL/CXReQLCursor.h"
//#include "CXReQL/CXReQLSession.h"
//#include "CXReQL/CXReQLInterface.h"


#endif
