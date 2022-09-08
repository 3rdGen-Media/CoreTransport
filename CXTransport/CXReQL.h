#pragma once
#ifndef CXREQL_H
#define CXREQL_H


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/

/* If we are we on Views, we want a single define for it.
 */
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
 #define _WIN32
#endif /* _WIN32 */


#if defined(CXREQL_DLL) && defined(_CXREQL_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both CXREQL_DLL and _CXREQL_BUILD_DLL defined"
#endif

/* CXREQL_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_CXREQL_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define CXREQL_API __declspec(dllexport)
#elif defined(_WIN32) && defined(CXREQL_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define CXREQL_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_CXREQL_BUILD_DLL)
 /* We are building REQL as a shared / dynamic library */
 #define CXREQL_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define CXREQL_API //extern
#endif


#ifndef CXREQL_PACK_ATTRIBUTE
#ifdef _WIN32
#define CXREQL_PACK_ATTRIBUTE 
#else
#define CXREQL_PACK_ATTRIBUTE __attribute__ ((packed))
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef CXREQL_ALIGN//(X)
#ifdef _WIN32
#define CXREQL_ALIGN(X) (align(X))
#else
#define CXREQL_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CXREQL_INLINE
#ifdef _WIN32
#define CXREQL_INLINE
#else
#define CXREQL_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef CXREQL_DECLSPEC
#ifdef _WIN32
#define CXREQL_DECLSPEC __declspec
#else
#define CXREQL_DECLSPEC
#endif
#endif

//#define CT_fprintf(stderr, f_, ...) fprintf(stderr, (f_), __VA_ARGS__)

#include "CXTransport.h"
#include "CXReQL/CXReQLCursor.h"
#include "CXReQL/CXReQLQuery.h"
#include "CXReQL/CXReQLSession.h"
#include "CXReQL/CXReQLInterface.h"

//#include <CoreTransport/CTransport.h>
//#include "CXReQL/CXReQLService.h"
//#include "CXReQL/CXReQLConnection.h"
//#include "CXReQL/CXREQLRequest.h"
//#include "CXReQL/CXReQLCursor.h"
//#include "CXReQL/CXReQLSession.h"
//#include "CXReQL/CXReQLInterface.h"


#endif
