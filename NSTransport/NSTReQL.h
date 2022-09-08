#pragma once
#ifndef NSREQL_H
#define NSREQL_H


/*************************************************************************
 * Compiler- and platform-specific preprocessor work
 *************************************************************************/

/* If we are we on Views, we want a single define for it.
 */
#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__MINGW32__))
 #define _WIN32
#endif /* _WIN32 */


#if defined(NSREQL_DLL) && defined(_NSREQL_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both NSREQL_DLL and _NSREQL_BUILD_DLL defined"
#endif

/* NSREQL_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_NSREQL_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define NSREQL_API __declspec(dllexport)
#elif defined(_WIN32) && defined(NSREQL_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define NSREQL_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_NSREQL_BUILD_DLL)
 /* We are building REQL as a shared / dynamic library */
 #define NSREQL_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define NSREQL_API //extern
#endif


#ifndef NSREQL_PACK_ATTRIBUTE
#ifdef _WIN32
#define NSREQL_PACK_ATTRIBUTE
#else
#define NSREQL_PACK_ATTRIBUTE __attribute__ ((packed))
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef NSREQL_ALIGN//(X)
#ifdef _WIN32
#define NSREQL_ALIGN(X) (align(X))
#else
#define NSREQL_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef NSREQL_INLINE
#ifdef _WIN32
#define NSREQL_INLINE
#else
#define NSREQL_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef NSREQL_DECLSPEC
#ifdef _WIN32
#define NSREQL_DECLSPEC __declspec
#else
#define NSREQL_DECLSPEC
#endif
#endif

//#define CT_fprintf(stderr, f_, ...) fprintf(stderr, (f_), __VA_ARGS__)

#include "NSTransport.h"
#include "NSTReQL/NSTReQLCursor.h"
#include "NSTReQL/NSTReQLQuery.h"
#include "NSTReQL/NSTReQLSession.h"
#include "NSTReQL/NSTReQLInterface.h"

//#include <CoreTransport/CTransport.h>
//#include "CXReQL/CXReQLService.h"
//#include "CXReQL/CXReQLConnection.h"
//#include "CXReQL/NSREQLRequest.h"
//#include "CXReQL/CXReQLCursor.h"
//#include "CXReQL/CXReQLSession.h"
//#include "CXReQL/CXReQLInterface.h"


#endif
