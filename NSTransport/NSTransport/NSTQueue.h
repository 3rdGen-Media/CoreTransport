//
//  NSTQueue.h
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

//using namespace CoreTransport;

#define NST_MAX_INFLIGHT_CONNECT_PACKETS 1L
#define NST_MAX_INFLIGHT_DECRYPT_PACKETS 1L
#define NST_MAX_INFLIGHT_ENCRYPT_PACKETS 1L

//Define some windows var mappings for unix style platform compatibility
//(but really if this doesn't completely avoid ifdef below why bother?)
#ifndef LPVOID
#define LPVOID void*
#endif

#ifndef __stdcall
#define __stdcall
#endif

NSTRANSPORT_API NSTRANSPORT_INLINE void* __stdcall NST_Dequeue_Resolve_Connect_Handshake(LPVOID lpParameter);
NSTRANSPORT_API NSTRANSPORT_INLINE void* __stdcall NST_Dequeue_Recv_Decrypt(LPVOID lpParameter);
NSTRANSPORT_API NSTRANSPORT_INLINE void* __stdcall NST_Dequeue_Encrypt_Send(LPVOID lpParameter);
