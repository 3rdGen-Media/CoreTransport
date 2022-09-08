#ifndef CTSYSTEM_H
#define CTSYSTEM_H


#ifdef __cplusplus
extern "C" {
#endif
    

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>           //Core Foundation
#elif defined(__FREEBSD__)

#endif

//#ifndef __BLOCKS__
//#error must be compiled with -fblocks option enabled
//#endif
#include <stdio.h>
#include <Block.h>

#ifndef CBlockInit
#define CBlockInit() 
#endif

#ifdef __cplusplus
}
#endif


#endif //CTSYSTEM_H
