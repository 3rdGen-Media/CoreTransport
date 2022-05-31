#ifndef CTQUEUE_H
#define CTQUEUE_H

//CoreTransport CXURL and ReQL C++ APIs uses CTransport (CTConnection) API internally
//#include <CoreTransport/CTransport.h>

//This defines the maximum allowable asynchronous packets to process on the IOCP tx/rx queue(s) ... per connection?
#define CT_MAX_INFLIGHT_CONNECT_PACKETS 1L
#define CT_MAX_INFLIGHT_DECRYPT_PACKETS 1L
#define CT_MAX_INFLIGHT_ENCRYPT_PACKETS 1L

CTRANSPORT_API CTRANSPORT_INLINE unsigned long __stdcall CT_Dequeue_Connect(LPVOID lpParameter);
CTRANSPORT_API CTRANSPORT_INLINE unsigned long __stdcall CT_Dequeue_Recv_Decrypt(LPVOID lpParameter);
CTRANSPORT_API CTRANSPORT_INLINE unsigned long __stdcall CT_Dequeue_Encrypt_Send(LPVOID lpParameter);


#endif