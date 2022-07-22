#pragma once

#ifndef CXQUEUE_H
#define CXQUEUE_H

using namespace CoreTransport;

#define CX_MAX_INFLIGHT_CONNECT_PACKETS 1L
#define CX_MAX_INFLIGHT_DECRYPT_PACKETS 1L
#define CX_MAX_INFLIGHT_ENCRYPT_PACKETS 1L

//Define some windows var mappings for unix style platform compatibility
//(but really if this doesn't completely avoid ifdef below why bother?)
#ifndef LPVOID
#define LPVOID void*
#endif

#ifndef __stdcall
#define __stdcall 
#endif

//there does not seem to be any way to avoid a win32 iocp queue + thread requiring callback to return unsigned long
//vs pthread_create requiring to the callback to return void*
#ifdef _WIN32
CXTRANSPORT_API CXTRANSPORT_INLINE unsigned long __stdcall CX_Dequeue_Resolve_Connect_Handshake(LPVOID lpParameter);
CXTRANSPORT_API CXTRANSPORT_INLINE unsigned long __stdcall CX_Dequeue_Recv_Decrypt(LPVOID lpParameter);
CXTRANSPORT_API CXTRANSPORT_INLINE unsigned long __stdcall CX_Dequeue_Encrypt_Send(LPVOID lpParameter);
#else
CXTRANSPORT_API CXTRANSPORT_INLINE void* __stdcall CX_Dequeue_Resolve_Connect_Handshake(LPVOID lpParameter);
CXTRANSPORT_API CXTRANSPORT_INLINE void* __stdcall CX_Dequeue_Recv_Decrypt(LPVOID lpParameter);
CXTRANSPORT_API CXTRANSPORT_INLINE void* __stdcall CX_Dequeue_Encrypt_Send(LPVOID lpParameter);
#endif

#endif //CXQUEUE_H