#pragma once
#ifndef CXTRANSPORT_H
#define CXTRANSPORT_H


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/

/* If we are we on Views, we want a single define for it.
 */
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
 #define _WIN32
#endif /* _WIN32 */


#if defined(CXTRANSPORT_DLL) && defined(_CXTRANSPORT_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both CXTRANSPORT_DLL and _CXTRANSPORT_BUILD_DLL defined"
#endif

/* CXTRANSPORT_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_CXTRANSPORT_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define CXTRANSPORT_API __declspec(dllexport)
#elif defined(_WIN32) && defined(CXTRANSPORT_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define CXTRANSPORT_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_CXTRANSPORT_BUILD_DLL)
 /* We are building REQL as a shared / dynamic library */
 #define CXTRANSPORT_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define CXTRANSPORT_API //extern
#endif


#ifndef CXTRANSPORT_PACK_ATTRIBUTE
#ifdef _WIN32
#define CXTRANSPORT_PACK_ATTRIBUTE 
#else
#define CXTRANSPORT_PACK_ATTRIBUTE __attribute__ ((packed))
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef CXTRANSPORT_ALIGN//(X)
#ifdef _WIN32
#define CXTRANSPORT_ALIGN(X) (align(X))
#else
#define CXTRANSPORT_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CXTRANSPORT_INLINE
#ifdef _WIN32
#define CXTRANSPORT_INLINE
#else
#define CXTRANSPORT_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CXTRANSPORT_DECLSPEC
#ifdef _WIN32
#define CXTRANSPORT_DECLSPEC __declspec
#else
#define CXTRANSPORT_DECLSPEC
#endif
#endif

//#define CT_printf(f_, ...) printf((f_), __VA_ARGS__)

//statically tune the outgoing request/query buffer size
#define CX_REQUEST_BUFFER_SIZE 65536L

//The response buffer uses memory mapped file that is a multiple of the sytem granularity
#define CX_GRANULARITY_SCALAR  256L	 

#include <CoreTransport/CTransport.h>
#include "CXTransport/CXConnection.h"
#include "CXTransport/CXCursor.h"
//#include "CXReQL/CXReQLService.h"
//#include "CXReQL/CXReQLConnection.h"
//#include "CXReQL/CXURLRequest.h"
//#include "CXReQL/CXReQLCursor.h"
//#include "CXReQL/CXReQLSession.h"
//#include "CXReQL/CXReQLInterface.h"


#endif