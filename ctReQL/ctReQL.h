#ifndef CTREQL_H
#define CTREQL_H

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


#if defined(REQL_DLL) && defined(_REQL_BUILD_DLL)
 /* CRGC_DLL must be defined by applications that are linking against the DLL
  * version of the CRGC library.  _CRGC_BUILD_DLL is defined by the CRGC
  * configuration header when compiling the DLL version of the library.
  */
 #error "You may not have both REQL_DLL and _REQL_BUILD_DLL defined"
#endif

/* REQL_API is used to declare public API functions for export
 * from the DLL / shared library / dynamic library.
 */
#if defined(_WIN32) && defined(_REQL_BUILD_DLL)
 /* We are building crMath as a Win32 DLL */
 #define REQL_API __declspec(dllexport)
#elif defined(_WIN32) && defined(REQL_DLL)
 /* We are calling crMath as a Win32 DLL */
 #define REQL_API __declspec(dllimport)
#elif defined(__GNUC__) && defined(_REQL_BUILD_DLL)
 /* We are building REQL as a shared / dynamic library */
 #define REQL_API __attribute__((visibility("default")))
#else
 /* We are building or calling crMath as a static library */
 #define REQL_API extern
#endif


#ifndef REQL_PACK_ATTRIBUTE
#ifdef _WIN32
#define REQL_PACK_ATTRIBUTE 
#else
#define REQL_PACK_ATTRIBUTE __attribute__ ((packed))
#endif
#endif

//align functions are diffent on windows vs iOS, Linux, etc.
#ifndef REQL_ALIGN//(X)
#ifdef _WIN32
#define REQL_ALIGN(X) (align(X))
#else
#define REQL_ALIGN(X) __attribute__ ((aligned(X)))
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef REQL_INLINE
#ifdef _WIN32
#define REQL_INLINE
#else
#define REQL_INLINE
#endif
#endif

//inline doesn't exist in C89, __inline is MSVC specific
#ifndef REQL_DECLSPEC
#ifdef _WIN32
#define REQL_DECLSPEC __declspec
#else
#define REQL_DECLSPEC
#endif
#endif


//#define Reql_printf(f_, ...) printf((f_), __VA_ARGS__)

//#define REQL_USE_MBED_TLS

#include "ctReQL/ReqlFile.h"
#include "ctReQL/ReqlSSL.h"
#include "ctReQL/ReqlAPI.h"

#ifdef __cplusplus
}
#endif

#endif