#ifndef CTQUEUE_H
#define CTQUEUE_H

//CoreTransport CXURL and ReQL C++ APIs uses CTransport (CTConnection) API internally
//#include <CoreTransport/CTransport.h>

//This defines the maximum allowable asynchronous packets to process on the IOCP tx/rx queue(s) ... per connection?
#define CT_MAX_INFLIGHT_CONNECT_PACKETS 1L
#define CT_MAX_INFLIGHT_DECRYPT_PACKETS 1L
#define CT_MAX_INFLIGHT_ENCRYPT_PACKETS 1L

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
CTRANSPORT_API CTRANSPORT_INLINE unsigned long __stdcall CT_Dequeue_Resolve_Connect_Handshake(LPVOID lpParameter);
CTRANSPORT_API CTRANSPORT_INLINE unsigned long __stdcall CT_Dequeue_Recv_Decrypt(LPVOID lpParameter);
CTRANSPORT_API CTRANSPORT_INLINE unsigned long __stdcall CT_Dequeue_Encrypt_Send(LPVOID lpParameter);
#else
CTRANSPORT_API CTRANSPORT_INLINE void* __stdcall CT_Dequeue_Resolve_Connect_Handshake(LPVOID lpParameter);
CTRANSPORT_API CTRANSPORT_INLINE void* __stdcall CT_Dequeue_Recv_Decrypt(LPVOID lpParameter);
CTRANSPORT_API CTRANSPORT_INLINE void* __stdcall CT_Dequeue_Encrypt_Send(LPVOID lpParameter);
#endif
#endif //CTQUEUE_H