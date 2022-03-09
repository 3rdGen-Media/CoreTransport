#pragma once

#ifndef CXQUEUE_H
#define CXQUEUE_H

using namespace CoreTransport;

#define CX_MAX_INFLIGHT_CONNECT_PACKETS 1L
#define CX_MAX_INFLIGHT_DECRYPT_PACKETS 2L
#define CX_MAX_INFLIGHT_ENCRYPT_PACKETS 1L

CXTRANSPORT_API CXTRANSPORT_INLINE unsigned long __stdcall CX_Dequeue_Connect(LPVOID lpParameter);
CXTRANSPORT_API CXTRANSPORT_INLINE unsigned long __stdcall CX_Dequeue_Recv_Decrypt(LPVOID lpParameter);
CXTRANSPORT_API CXTRANSPORT_INLINE unsigned long __stdcall CX_Dequeue_Encrypt_Send(LPVOID lpParameter);

#endif //CXQUEUE_H