#pragma once
#ifndef CXURL_H
#define CXURL_H


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/

/* If we are we on Views, we want a single define for it.
 */
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
 #define _WIN32
#endif /* _WIN32 */


#if defined(CXURL_DLL) && defined(_CXURL_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both CXURL_DLL and _CXURL_BUILD_DLL defined"
#endif

/* CXURL_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_CXURL_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define CXURL_API __declspec(dllexport)
#elif defined(_WIN32) && defined(CXURL_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define CXURL_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_CXURL_BUILD_DLL)
 /* We are building REQL as a shared / dynamic library */
 #define CXURL_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define CXURL_API //extern
#endif


#ifndef CXURL_PACK_ATTRIBUTE
#ifdef _WIN32
#define CXURL_PACK_ATTRIBUTE 
#else
#define CXURL_PACK_ATTRIBUTE __attribute__ ((packed))
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef CXURL_ALIGN//(X)
#ifdef _WIN32
#define CXURL_ALIGN(X) (align(X))
#else
#define CXURL_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CXURL_INLINE
#ifdef _WIN32
#define CXURL_INLINE
#else
#define CXURL_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CXURL_DECLSPEC
#ifdef _WIN32
#define CXURL_DECLSPEC __declspec
#else
#define CXURL_DECLSPEC
#endif
#endif

//#define CT_printf(f_, ...) printf((f_), __VA_ARGS__)

#include "CXTransport.h"
#include "CXURL/CXURLCursor.h"
#include "CXURL/CXURLRequest.h"
#include "CXURL/CXURLSession.h"
#include "CXURL/CXURLInterface.h"
//#include <CoreTransport/CTransport.h>
//#include "CXReQL/CXReQLService.h"
//#include "CXReQL/CXReQLConnection.h"
//#include "CXReQL/CXURLRequest.h"
//#include "CXReQL/CXReQLCursor.h"
//#include "CXReQL/CXReQLSession.h"
//#include "CXReQL/CXReQLInterface.h"


#endif