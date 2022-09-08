#pragma once
#ifndef NSURL_H
#define NSURL_H


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/

/* If we are we on Views, we want a single define for it.
 */
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
 #define _WIN32
#endif /* _WIN32 */


#if defined(NSURL_DLL) && defined(_NSURL_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both NSURL_DLL and _NSURL_BUILD_DLL defined"
#endif

/* NSURL_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_NSURL_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define NSURL_API __declspec(dllexport)
#elif defined(_WIN32) && defined(NSURL_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define NSURL_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_NSURL_BUILD_DLL)
 /* We are building REQL as a shared / dynamic library */
 #define NSURL_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define NSURL_API //extern
#endif


#ifndef NSURL_PACK_ATTRIBUTE
#ifdef _WIN32
#define NSURL_PACK_ATTRIBUTE
#else
#define NSURL_PACK_ATTRIBUTE __attribute__ ((packed))
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef NSURL_ALIGN//(X)
#ifdef _WIN32
#define NSURL_ALIGN(X) (align(X))
#else
#define NSURL_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef NSURL_INLINE
#ifdef _WIN32
#define NSURL_INLINE
#else
#define NSURL_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef NSURL_DECLSPEC
#ifdef _WIN32
#define NSURL_DECLSPEC __declspec
#else
#define NSURL_DECLSPEC
#endif
#endif

//#define CT_fprintf(stderr, f_, ...) fprintf(stderr, (f_), __VA_ARGS__)

#include "NSTransport.h"
#include "NSTURL/NSTURLCursor.h"
#include "NSTURL/NSTURLRequest.h"
#include "NSTURL/NSTURLSession.h"
#include "NSTURL/NSTURLInterface.h"
//#include <CoreTransport/CTransport.h>
//#include "NSReQL/NSReQLService.h"
//#include "NSReQL/NSReQLConnection.h"
//#include "NSReQL/NSURLRequest.h"
//#include "NSReQL/NSReQLCursor.h"
//#include "NSReQL/NSReQLSession.h"
//#include "NSReQL/NSReQLInterface.h"


#endif
