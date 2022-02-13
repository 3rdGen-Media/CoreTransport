
/***
**  
*	Core Transport -- HappyEyeballs
*
*	Description:  A test application demonstrating usage of CoreTransport C API
*	
*
*	@author: Joe Moulton
*	Copyright 2020 Abstract Embedded LLC DBA 3rdGen Multimedia
**
***/

//cr_display_sync is part of Core Render crPlatform library
#ifdef _DEBUG
#include <vld.h>                //Visual Leak Detector
#endif

#include "stdio.h"
#include "stdlib.h"
#include <process.h>            // threading routines for the CRT

//CoreTransport can be optionally built with native SSL encryption OR against MBEDTLS
#ifndef CTRANSPORT_USE_MBED_TLS
#pragma comment(lib, "crypt32.lib")
//#pragma comment(lib, "user32.lib")
#pragma comment(lib, "secur32.lib")
#else
// mbded tls
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#endif

//CoreTransport CXURL and ReQL C++ APIs uses CTransport (CTConnection) API internally
#include <CoreTransport/CTransport.h>

/***
 * Start CTransport (CTConnection) C API Example
 ***/

//Define a CTransport API CTTarget C style struct to initiate an HTTPS connection with a CTransport API CTConnection
static const char* http_server = "learnopengl.com";//"mineralism.s3 - us - west - 2.amazonaws.com";
static const unsigned short	http_port = 443;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char *			rdb_server = "RETHINKDB_SERVER";
static const unsigned short rdb_port = 18773;
static const char *			rdb_user = "RETHINKDB_USERNAME";
static const char *			rdb_pass = "RETHINKDB_PASSWORD";

//Define SSL Certificate for ReqlService construction:
//	a)  For xplatform MBEDTLS support: The SSL certificate can be defined in a buffer OR a file AS a .cer in PEM format
//	b)  For Win32 SCHANNEL support:	   The SSL certificate must be pre-installed in a trusted root store on the system using MS .cer format
//  c)  For Darwin SecureTransport:	   The SSL certificate must be defined in a .der file with ... format

//const char * certFile = "../Keys/MSComposeSSLCertificate.cer";
const char * certStr = "RETHINKDB_CERT_PATH_OR_STR\0";

//Predefine ReQL query to use with CTransport C Style API (because message support is not provided at the CTransport/C level)
char * paintingTableQuery = "[1,[15,[[14,[\"MastryDB\"]],\"Paintings\"]]]\0";

//Define pointers and/or structs to keep reference to our secure CTransort HTTPS and RethinkDB connections, respectively
CTConnection _httpConn;
CTConnection _reqlConn;

//This defines the maximum allowable asynchronous packets to process on the IOCP tx/rx queue(s) ... per connection?
#define CT_MAX_INFLIGHT_DECRYPT_PACKETS 2L
#define CT_MAX_INFLIGHT_ENCRYPT_PACKETS 1L

static unsigned long __stdcall CTDecryptResponseCallback(LPVOID lpParameter)
{
	int i;
	ULONG CompletionKey;

	OVERLAPPED_ENTRY ovl_entry[CT_MAX_INFLIGHT_DECRYPT_PACKETS];
	ULONG nPaquetsDequeued;
	
	//CTQueryMessageHeader * header = NULL;
	CTOverlappedResponse * overlappedResponse = NULL;

	unsigned long NumBytesRecv = 0;

	unsigned long NumBytesRemaining = 0;
	char * extraBuffer= NULL;
	
	unsigned long PrevBytesRecvd = 0;
	unsigned long TotalBytesToDecrypt = 0;
	//unsigned long NumBytesRecvMore = 0;
	unsigned long extraBufferOffset = 0;
	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;

	HANDLE hCompletionPort = (HANDLE)lpParameter;

	unsigned long cBufferSize = ct_system_allocation_granularity(); 

	//unsigned long headerLength;
	//unsigned long contentLength;
	CTCursor * cursor = NULL;
	CTCursor * nextCursor = NULL;
	CTCursor*  closeCursor = NULL;

	MSG nextCursorMsg = { 0 };
	unsigned long currentThreadID = GetCurrentThreadId();

	printf("CTConnection::DecryptResponseCallback Begin\n");

	//Create a Win32 Platform Message Queue for this thread by calling PeekMessage 
	//(there should not be any messages currently in the queue because it hasn't been created until now)
	PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER, PM_REMOVE);

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CT_MAX_INFLIGHT_DECRYPT_PACKETS, &nPaquetsDequeued, INFINITE, TRUE))
	{
		printf("CTConnection::CTDecryptResponseCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			//When to use CompletionKey?
			//CompletionKey = *(ovl_entry[i].lpCompletionKey);

			//Retrieve the number of bytes provided from the socket buffer by IOCP
			NumBytesRecv = ovl_entry[i].dwNumberOfBytesTransferred ;//% (cBufferSize - sizeof(CTOverlappedResponse));
			
			//Get the overlapped struct associated with the asychronous IOCP call
			overlappedResponse = (CTOverlappedResponse*)ovl_entry[i].lpOverlapped;

			//Get the cursor/connection that has been pinned on the overlapped
			cursor = (CTCursor*)overlappedResponse->cursor;

#ifdef _DEBUG
			if (!overlappedResponse)
			{
				printf("CTConnection::CXDecryptResponseCallback::overlappedResponse is NULL!\n");
				assert(1==0);
			}
#endif
			//Cursor requests are always queued onto the connection's decrypt thread (ie this one) to be scheduled
			//so every overlapped sent to this decrypt thread is handled here first.
			//There are two possible scenarios:
			//	1)  There are currently previous cursor requests still being processed and the cursor request is scheduled
			//		for pickup later from this thread's message queue
			//	2)  The are currently no previous cursor requests still being processed and so we may initiate an async socket recv
			//		for the cursor's response
			if (overlappedResponse->type == CT_OVERLAPPED_SCHEDULE)
			{
				printf("CTConnection::CTDecryptResponseCallback::Scheduling Cursor via ...");

				//set the overlapped message type to indicate that scheduling for the current
				//overlapped/cursor has already been processed
				overlappedResponse->type = CT_OVERLAPPED_EXECUTE;
				if( cursor->conn->responseCount < cursor->queryToken )//(cursor->conn->rxCursorQueueCount < 1)
				{
					printf("PostThreadMessage\n");
					if (PostThreadMessage(currentThreadID, WM_USER + cursor->queryToken, 0, (LPARAM)cursor) < 1)
					{
						//cursor->conn->rxCursorQueueCount++;
						printf("CTDecryptResponseCallback::PostThreadMessage failed with error: %d", GetLastError());
					}
				}
				else
				{
					printf("CTCursorAsyncRecv\n");
					if ((ctError = CTCursorAsyncRecv(&overlappedResponse, (void*)(cursor->file.buffer), 0, &NumBytesRecv)) != CTSocketIOPending)
					{
						//the async operation returned immediately
						if (ctError == CTSuccess)
						{
							//cursor->conn->rxCursorQueueCount++;
							printf("CTConnection::CTDecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRecv);
						}
						else //failure
						{
							printf("CTConnection::CTDecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
						}
					}
					//else cursor->conn->rxCursorQueueCount++;
				}

				continue;
			}

			//1 If specifically requested zero bytes using WSARecv, then IOCP is notifying us that it has bytes available in the socket buffer (MBEDTLS)
			//2 If we requested anything more than zero bytes using WSARecv, then IOCP is telling us that it was unable to read any bytes at all from the socket buffer (SCHANNEL)
			//if (NumBytesRecv == 0) //
			//{
			//	printf("CTConnection::CXDecryptResponseCallback::SCHANNEL IOCP::NumBytesRecv = ZERO\n");
			//}
			//else //IOCP already read the data from the socket into the buffer we provided to Overlapped, we have to decrypt it
			//{
				//Get the cursor/connection
				//cursor = (CTCursor*)overlappedResponse->cursor;
				NumBytesRecv += overlappedResponse->wsaBuf.buf - overlappedResponse->buf;

#ifdef _DEBUG
				assert(cursor);
				assert(cursor->conn);
				assert(cursor->headerLengthCallback);
				assert(overlappedResponse->buf);
				assert(overlappedResponse->wsaBuf.buf);
				printf("CTConnection::DecryptResponseCallback B::NumBytesRecv = %d\n", (int)NumBytesRecv);
#endif
		
				//Issue a blocking call to decrypt the message (again, only if using non-zero read path, which is exclusively used with MBEDTLS at current)

				PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
				NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks
				
				extraBuffer = overlappedResponse->buf;
				NumBytesRemaining = NumBytesRecv;
				scRet = SEC_E_OK;

				//while their is extra data to decrypt keep calling decrypt
				while( NumBytesRemaining > 0 && scRet == SEC_E_OK )
				{
					//move the extra buffer to the front of our operating buffer
					overlappedResponse->wsaBuf.buf = extraBuffer;
					extraBuffer = NULL;
					PrevBytesRecvd = NumBytesRecv;		//store the previous amount of decrypted bytes
					NumBytesRecv = NumBytesRemaining;	//set up for the next decryption 
					
					scRet = CTSSLDecryptMessage2(cursor->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv, &extraBuffer, &NumBytesRemaining);
					printf("CTConnection::DecryptResponseCallback C::Decrypted data: %d bytes recvd, %d bytes remaining, scRet = 0x%x\n", NumBytesRecv, NumBytesRemaining, scRet); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));

					if( scRet == SEC_E_OK )
					{
						//push past cbHeader and copy decrypted message bytes
						memcpy( (char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv );
						overlappedResponse->buf += NumBytesRecv ;//+ overlappedResponse->conn->sslContext->Sizes.cbHeader;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;
														
						//if( NumBytesRemaining == 0 )
						//{
							//scRet = ioctlsocket(cursor->conn->socket, FIONREAD, &NumBytesRemaining);

							//printf("**** CTConnection::DecryptResponseCallback::ictlsocket C status = %d\n", (int)scRet);
							//printf("**** CTConnection::DecryptResponseCallback::ictlsocket C = %d\n", (int)NumBytesRemaining);

							//select said there was data but ioctlsocket says there is no data
							//if( NumBytesRemaining == 0)
							
							if (cursor->headerLength == 0)
							{
								//search for end of protocol header (and set cxCursor headerLength property
								char* endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesRecv);

								//calculate size of header
								if (endOfHeader)
								{
									cursor->headerLength = endOfHeader - (cursor->file.buffer);
									
									memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

									printf("CTDecryptResponseCallbacK::Header = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

									//copy body to start of buffer to overwrite header 
									memcpy(cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength);
									//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

									overlappedResponse->buf -= cursor->headerLength;
								}
							}
							assert(cursor->contentLength > 0);

							if (cursor->contentLength <= overlappedResponse->buf - cursor->file.buffer)
							{
								extraBufferOffset = 0;
								closeCursor = cursor;	//store a ptr to the current cursor so we can close it after replacing with next cursor
								memset(&nextCursorMsg, 0, sizeof(MSG));
								if (PeekMessage(&nextCursorMsg, NULL, WM_USER + cursor->queryToken + 1, WM_USER + cursor->queryToken + 1, PM_REMOVE)) //(nextCursor)
								{
									nextCursor = (CTCursor*)(nextCursorMsg.lParam);
#ifdef _DEBUG
									assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
									assert(nextCursor);
#endif									
									if (cursor->contentLength < overlappedResponse->buf - cursor->file.buffer)
									{									
										printf("vagina");
										
										cursor->conn->response_overlap_buffer_length = (size_t)overlappedResponse->buf - (size_t)(cursor->file.buffer + cursor->contentLength);
										assert(cursor->conn->response_overlap_buffer_length > 0);

										//NumBytesRemaining = cursor->conn->response_overlap_buffer_length;
										if (nextCursor && cursor->conn->response_overlap_buffer_length > 0)
										{
											//copy overlapping cursor's response from previous cursor's buffer to next cursor's buffer
											memcpy(nextCursor->file.buffer, cursor->file.buffer + cursor->contentLength, cursor->conn->response_overlap_buffer_length);
											
											//read the new cursor's header
											if (nextCursor->headerLength == 0)
											{
												//search for end of protocol header (and set cxCursor headerLength property
												assert(cursor->file.buffer != nextCursor->file.buffer);
												char* endOfHeader = nextCursor->headerLengthCallback(nextCursor, nextCursor->file.buffer, cursor->conn->response_overlap_buffer_length);

												//calculate size of header
												nextCursor->headerLength = endOfHeader - (nextCursor->file.buffer);

												//copy header to cursor's request buffer
												memcpy(nextCursor->requestBuffer, nextCursor->file.buffer, nextCursor->headerLength);

												//copy body to start of buffer to overwrite header
												assert(nextCursor->file.buffer);
												memcpy(nextCursor->file.buffer, endOfHeader, cursor->conn->response_overlap_buffer_length - nextCursor->headerLength);

												cursor->conn->response_overlap_buffer_length -= nextCursor->headerLength;
												extraBufferOffset = cursor->conn->response_overlap_buffer_length;

												/*
												if (nextCursor->contentLength <= cursor->conn->response_overlap_buffer_length)
												{
													//if (nextCursor->contentLength < cursor->conn->response_overlap_buffer_length)


													cursor->conn->response_overlap_buffer_length -= nextCursor->contentLength;

													cursor = nextCursor;
													nextCursor = NULL;
													if (PeekMessage(&nextCursorMsg, NULL, WM_USER + cursor->queryToken + 1, WM_USER + cursor->queryToken + 1, PM_REMOVE)) //(nextCursor)
													{
														assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
														nextCursor = (CTCursor*)(nextCursorMsg.lParam);
														assert(nextCursor);
													}
													//if (nextCursor->contentLength < cursor->conn->response_overlap_buffer_length - nextCursor->headerLength)
													//{
													//	cursor = nextCursor;
													//
													//}

													//decrement the cursor queue count to finish ours
													cursor->conn->rxCursorQueueCount--;

													CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
#ifdef _DEBUG
													assert(cursor->responseCallback);
#endif
													cursor->responseCallback(NULL, cursor);

													continue;

												}

												*/
											}
											
										}
									}
									if( NumBytesRemaining ==0 )
									{
										printf("penis");

										NumBytesRecv = cBufferSize;;
										overlappedResponse = &(nextCursor->overlappedResponse);
										if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->file.buffer + cursor->conn->response_overlap_buffer_length - nextCursor->headerLength), 0, &NumBytesRecv)) != CTSocketIOPending)
										{
											//the async operation returned immediately
											if (scRet == CTSuccess)
											{
												printf("CTConnection::DecryptResponseCallback::CTAsyncRecv (new cursor) returned immediate with %lu bytes\n", NumBytesRecv);

											}
											else //failure
											{
												printf("CTConnection::DecryptResponseCallback::CTAsyncRecv (new cursor) failed with CTDriverError = %d\n", scRet);
											}
										}
										nextCursor = NULL;
										//break;
									}
									else
									{
										printf("scrotum");

#ifdef _DEBUG
										assert(nextCursor);
										assert(nextCursor->file.buffer);
										assert(extraBuffer);
#endif
										//since extra buffer may be pointing to current cursor buffer memory we have to copy it to the next
										//cursor BEFORE closing the current cursor's mapping
										memcpy(nextCursor->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
										extraBuffer = nextCursor->file.buffer + extraBufferOffset;										

										overlappedResponse = &(nextCursor->overlappedResponse);
										overlappedResponse->buf = nextCursor->file.buffer + extraBufferOffset;

										cursor = nextCursor;
										nextCursor = NULL;
									}
									
								}

								//increment the connection response count
								//closeCursor->conn->rxCursorQueueCount--;
								closeCursor->conn->responseCount++;

#ifdef _DEBUG
								assert(closeCursor->responseCallback);
#endif
								//issue the response to the client caller
								closeCursor->responseCallback(NULL, closeCursor);
								closeCursor = NULL;	//set to NULL for posterity
							}
							else if (NumBytesRemaining == 0)
							{
								printf("wanker");

								NumBytesRemaining = cBufferSize;
								if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
								{
									//the async operation returned immediately
									if (scRet == CTSuccess)
									{
										printf("CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
										//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
									}
									else //failure
									{
										printf("CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
										break;
									}
								}
								break;
							}

					}
				}

				if( scRet == SEC_E_INCOMPLETE_MESSAGE )
				{
					printf("CTConnection::DecryptResponseCallback C::SEC_E_INCOMPLETE_MESSAGE %d bytes recvd, %d bytes remaining\n", PrevBytesRecvd, NumBytesRemaining); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));
				
					//
					//	Handle INCOMPLETE ENCRYPTED MESSAGE AS FOLLOWS:
					// 
					//	1)  copy the remaining encrypted bytes to the start of our buffer so we can 
					//		append to them when receiving from socket on the next async iocp iteration
					//  
					//	2)  use NumBytesRecv var as an offset input parameter CTCursorAsyncRecv
					//
					//	3)  use NumByteRemaining var as input to CTCursorAsyncRecv to request num bytes to recv from socket
					//		(PrevBytesRecvd will be subtracted from NumBytesRemaining internal to CTAsyncRecv

					//For SCHANNEL:  CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer
					//For WolfSSL:   CTSSLDecryptMessage2 will point extraBuffer at the input msg buffer + the size of the ssl header wolfssl already consumed
					memcpy((char*)overlappedResponse->buf, extraBuffer, NumBytesRemaining);
					NumBytesRemaining = cBufferSize;   //request exactly the amount to complete the message NumBytesRemaining 
													   //populated by CTSSLDecryptMessage2 above or request more if desired

					if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)
					{
						//the async operation returned immediately
						if (scRet == CTSuccess)
						{
							printf("CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
							//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
						}
						else //failure
						{
							printf("CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
							break;
						}
					}
				}
				
				printf("\nCTDecryptResponseCallback::END\n\n");

				/*
				else if (PeekMessage(&nextCursorMsg, NULL, WM_USER + cursor->queryToken + 1, WM_USER + cursor->conn->queryCount, PM_REMOVE)) //(nextCursor)
				{
					//assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
					nextCursor = (CTCursor*)(nextCursorMsg.lParam);
					assert(nextCursor);

					overlappedResponse = &(nextCursor->overlappedResponse);
					overlappedResponse->buf = nextCursor->file.buffer;

					NumBytesRemaining = cBufferSize;

					if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
					{

						//the async operation returned immediately
						if (scRet == CTSuccess)
						{
							printf("CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
							//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
						}
						else //failure
						{
							printf("CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
							break;
						}
					}

				}

				cursor->conn->rxCursorQueueCount=0;
				*/

				//TO DO:: check return value of CTSSLDecryptMessage
				//return scRet;
				//if( scRet == CTSSLContextExpired ) break; // Server signaled end of session
				//if( scRet == CTSuccess ) break;

				//TO DO:  notify the caller of the query using the C++ equivalent of blocks

				//TO DO:  Enable SSL negotation.  Determine when to appropriately do so...here?
				// The server wants to perform another handshake sequence.
				/*
				if(scRet == CTSSLRenegotiate)
				{
					printf("CTDecryptResponseCallback::Server requested renegotiate!\n");
					//scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
					scRet = CTSSLHandshake(overlappedResponse->conn->socket, overlappedResponse->conn->sslContext, NULL);
					if(scRet != SEC_E_OK) return scRet;

					if(overlappedResponse->conn->sslContext->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
					{
						printf("\nCTDecryptResponseCallback::Unhandled Extra Data\n");
						//MoveMemory(overlappedConn->conn->response_buf, overlappedConn->conn->sslContextRef->ExtraData.pvBuffer, overlappedConn->conn->sslContextRef->ExtraData.cbBuffer);
						//cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
					}
				}
				*/
			//}
		}

	}

	printf("\nCTDecryptResponseCallback::End\n");
	ExitThread(0);
	return 0;
}


static unsigned long __stdcall CTEncryptQueryCallback(LPVOID lpParameter)
{
	int i;
	ULONG_PTR CompletionKey;	//when to use completion key?
	uint64_t queryToken = 0;
	OVERLAPPED_ENTRY ovl_entry[CT_MAX_INFLIGHT_ENCRYPT_PACKETS];
	ULONG nPaquetsDequeued;

	CTCursor * cursor = NULL;
	CTOverlappedResponse * overlappedRequest = NULL;
	CTOverlappedResponse * overlappedResponse = NULL;

	unsigned long NumBytesSent = 0;
	unsigned long recvMsgLength = 0;

	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;
	
	//we passed the completion port handle as the lp parameter
	HANDLE hCompletionPort = (HANDLE)lpParameter;

	unsigned long cBufferSize = ct_system_allocation_granularity(); 

	printf("CTConnection::EncryptQueryCallback Begin\n");

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CT_MAX_INFLIGHT_ENCRYPT_PACKETS, &nPaquetsDequeued, INFINITE, TRUE))
	{
		//printf("CTConnection::EncryptQueryCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			CompletionKey = (ULONG_PTR)ovl_entry[i].lpCompletionKey;
			NumBytesSent = ovl_entry[i].dwNumberOfBytesTransferred;
			overlappedRequest = (CTOverlappedResponse*)ovl_entry[i].lpOverlapped;

			if (!overlappedRequest)
			{
				printf("CTConnection::EncryptQueryCallback::GetQueuedCompletionStatus continue\n");
				continue;
			}

			if (NumBytesSent == 0)
			{
				printf("CTConnection::EncryptQueryCallback::Server Disconnected!!!\n");
			}
			else
			{
				//Get the cursor/connection
				//Get a the CTCursor from the overlappedRequest struct, retrieve the overlapped response struct memory, and store reference to the CTCursor in the about-to-be-queued overlappedResponse
				cursor = (CTCursor*)(overlappedRequest->cursor);
				
#ifdef _DEBUG
				assert(cursor);
				assert(cursor->conn);
#endif
				NumBytesSent = overlappedRequest->len;
				printf("CTConnection::EncryptQueryCallback::NumBytesSent = %d\n", (int)NumBytesSent);
				printf("CTConnection::EncryptQueryCallback::msg = %s\n", overlappedRequest->buf + sizeof(ReqlQueryMessageHeader));

				//Use the ReQLC API to synchronously
				//a)	encrypt the query message in the query buffer
				//b)	send the encrypted query data over the socket
				scRet = CTSSLWrite(cursor->conn->socket, cursor->conn->sslContext, overlappedRequest->buf, &NumBytesSent);

#ifdef CTRANSPORT_USE_MBED_TLS
				//Issuing a zero buffer read will tell iocp to let us know when there is data available for reading from the socket
				//so we can issue our own blocking read
				recvMsgLength = 0;
#else
				//However, the zero buffer read doesn't seem to scale very well if Schannel is encrypting/decrypting for us 
				recvMsgLength = cBufferSize;
#endif
				queryToken = cursor->queryToken;
				printf("CTConnection::EncryptQueryCallback::queryTOken = %llu\n", queryToken);

				//Issue a blocking read for debugging purposes
				//CTRecv(overlappedQuery->conn, (void*)&(overlappedQuery->conn->response_buffers[ (queryToken%REQL_MAX_INFLIGHT_QUERIES) * REQL_RESPONSE_BUFF_SIZE]), &recvMsgLength);

				//put the overlapped response in the connections rxCursorQueue
				//cursor->conn->rxCursorQueue[cursor->conn->rxCursorQueueIndex] = cursor;
				//cursor->conn->rxCursorQueueIndex = (cursor->conn->rxCursorQueueIndex + 1) % cursor->conn->rxCursorQueueSize;
				//g_cursorQueueCount++;
				overlappedResponse = &(cursor->overlappedResponse);
				overlappedResponse->cursor = (void*)cursor;
				overlappedResponse->len = cBufferSize;
				overlappedResponse->type == CT_OVERLAPPED_SCHEDULE;


				//Use WSASockets + IOCP to asynchronously do one of the following (depending on the usage of our underlying ssl context):
				//a)	notify us when data is available for reading from the socket via the rxQueue/thread (ie MBEDTLS path)
				//b)	read the encrypted response message from the socket into the response buffer and then notify us on the rxQueue/thread (ie SCHANNEL path)
				//if ((ctError = CTAsyncRecv(overlappedRequest->conn, (void*)&(overlappedRequest->conn->response_buffers[(queryToken%CX_MAX_INFLIGHT_QUERIES) * cBufferSize]), 0, &recvMsgLength)) != CTSocketIOPending)
				/*
				if ( cursor->conn->rxCursorQueueCount < 1)
				{

					cursor->conn->nextCursor = NULL;
					if ((ctError = CTCursorAsyncRecv(&overlappedResponse, (void*)(cursor->file.buffer), 0, &recvMsgLength)) != CTSocketIOPending)
					{
						//the async operation returned immediately
						if (ctError == CTSuccess)
						{
							cursor->conn->rxCursorQueueCount++;

							printf("CTConnection::EncryptQueryCallback::CTAsyncRecv returned immediate with %lu bytes\n", recvMsgLength);
						}
						else //failure
						{
							printf("CTConnection::EncryptQueryCallback::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
						}
					}
					else
					{
						cursor->conn->rxCursorQueueCount++;
					}
				}
				else
				{
					if (PostThreadMessage(g_decryptThreadID, WM_USER + cursor->queryToken, 0, (LPARAM)cursor) < 1)
					{
						cursor->conn->rxCursorQueueCount++;

						//int err = GetLastError();
						printf("CTEncryptQueryCallback::PostThreadMessage failed with error: %d", GetLastError());
					}
					//cursor->conn->nextCursor = cursor;
				}
				*/

				CTCursorRecvOnQueue(&overlappedResponse, (void*)(cursor->file.buffer), 0, &recvMsgLength);
				//Manual encryption is is left here commented out as an example
				//scRet = CTSSLEncryptMessage(overlappedConn->conn->sslContext, overlappedConn->conn->response_buf, &NumBytesRecv);
				//if( scRet == CTSSLContextExpired ) break; // Server signaled end of session
				//if( scRet == CTSuccess ) break;

				//TO DO:  Enable renogotiate, but how would the send thread/queue know if the server wants to perform another handshake?
				// The server wants to perform another handshake sequence.
				/*
				if(scRet == CTSSLRenegotiate)
				{
					printf("CTEncryptQueryCallback::Server requested renegotiate!\n");
					//scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
					scRet = CTSSLHandshake(overlappedQuery->conn->socket, overlappedQuery->conn->sslContext, NULL);
					if(scRet != SEC_E_OK) return scRet;

					if(overlappedQuery->conn->sslContext->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
					{
						printf("\nCTEncryptQueryCallback::Unhandled Extra Data\n");
						//MoveMemory(overlappedConn->conn->response_buf, overlappedConn->conn->sslContextRef->ExtraData.pvBuffer, overlappedConn->conn->sslContextRef->ExtraData.cbBuffer);
						//cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
					}
				}
				*/
			}
		}

	}

	printf("\nCTConnection::EncryptQueryCallback::End\n");
	ExitThread(0);
	return 0;

}


void createCursorResponseBuffers(CTCursor* cursor, const char* filepath)
{
#ifdef _WIN32
	//recv
	LPVOID queryPtr, responsePtr;
	unsigned long ctBufferSize = ct_system_allocation_granularity(); //+ sizeof(CTOverlappedResponse); 

	if (!filepath)
		filepath = "map.jpg\0";
	cursor->file.path = (char*)filepath;

	//file path to open
	CTCursorCreateMapFileW(cursor, cursor->file.path, ct_system_allocation_granularity() * 256L);

	cursor->overlappedResponse.buf = cursor->file.buffer;
	cursor->overlappedResponse.len = ctBufferSize;
	cursor->overlappedResponse.wsaBuf.buf = cursor->file.buffer;
	cursor->overlappedResponse.wsaBuf.len = ctBufferSize;
#endif

#ifdef _DEBUG
	assert(cursor->file.buffer);
#endif
}

uint64_t CTCursorRunQueryOnQueue(CTCursor * cursor, uint64_t queryToken)	
{
	unsigned long queryHeaderLength, queryMessageLength;
    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)cursor->requestBuffer;
    
	//unsigned long requestHeaderLength, requestLength;

	//get the buffer for sending with async iocp
	//char * queryBuffer = cursor->requestBuffer;
	char* queryBuffer = cursor->requestBuffer + sizeof(ReqlQueryMessageHeader);
	char * baseBuffer = cursor->requestBuffer;
	
	//calulcate the request length
	queryMessageLength = strlen(queryBuffer);//requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

	//Populate the network message buffer with a header and the serialized query JSON string
    queryHeader->token = queryToken;//(conn->queryCount)++;//conn->token;
    queryHeader->length = queryMessageLength;

	//print the request for debuggin
	//printf("CXURLSendRequestWithTokenOnQueue::request (%d) = %.*s\n", (int)queryMessageLength, (int)queryMessageLength, baseBuffer);
	printf("CTCursorRunQueryOnQueue::queryBuffer (%d) = %.*s", (int)queryMessageLength, (int)queryMessageLength, cursor->requestBuffer + sizeof(ReqlQueryMessageHeader));
    queryMessageLength += sizeof(ReqlQueryMessageHeader);// + queryMessageLength;
	
	//cursor->overlappedResponse.cursor = cursor;
	cursor->conn = &_reqlConn;
	cursor->queryToken = queryToken;

	//Send the HTTP(S) request
	return CTCursorSendOnQueue(cursor, (char **)&baseBuffer, queryMessageLength);//, &overlappedResponse);
}

uint64_t CTCursorSendRequestOnQueue(CTCursor * cursor, uint64_t requestToken)	
{
	//a string/stream for serializing the json query to char*	
	unsigned long requestHeaderLength, requestLength;

	//get the buffer for sending with async iocp
	char * requestBuffer = cursor->requestBuffer;
	char * baseBuffer = requestBuffer;
	
	//calulcate the request length
	requestLength = strlen(requestBuffer);//requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

	//print the request for debuggin
	printf("CXURLSendRequestWithTokenOnQueue::request (%d) = %.*s\n", (int)requestLength, (int)requestLength, baseBuffer);

	//cursor->overlappedResponse.cursor = cursor;
	cursor->conn = &_httpConn;
	cursor->queryToken = requestToken;

	//Send the HTTP(S) request
	return CTCursorSendOnQueue(cursor, (char **)&baseBuffer, requestLength);//, &overlappedResponse);
}


#define CT_MAX_INFLIGHT_CURSORS 1024
CTCursor _httpCursor[CT_MAX_INFLIGHT_CURSORS];
CTCursor _reqlCursor;

char* httpHeaderLengthCallback(struct CTCursor* pCursor, char* buffer, unsigned long length)
{
	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = strstr(buffer, "\r\n\r\n");
#ifdef _DEBUG
	if (!endOfHeader)
		assert(1 == 0);
#endif
	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	pCursor->contentLength = 1641845;

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}


char* reqlHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
	//the purpose of this function is to return the end of the header in the 
	//connection's tcp stream to CoreTransport decrypt/recv thread
	char* endOfHeader = buffer + sizeof(ReqlQueryMessageHeader);

	//The client *must* set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	cursor->contentLength = ((ReqlQueryMessageHeader*)buffer)->length;
	assert(((ReqlQueryMessageHeader*)buffer)->token == cursor->queryToken);

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}

void httpResponseCallback(CTError* err, CTCursor* cursor)
{
	CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	//CTCursorMapFileR(cursor);
	printf("httpResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	CTCursorCloseFile(cursor);
}


void reqlResponseCallback(CTError* err, CTCursor* cursor)
{
	CTCursorMapFileR(cursor);
	printf("reqlCompletionCallback body:  \n\n%.*s\n\n", cursor->file.size, cursor->file.buffer);
	CTCursorCloseFile(cursor);
}

static int httpRequestCount = 0;
char responseBuffers [CT_MAX_INFLIGHT_CURSORS][65536];

void sendHTTPRequest(CTCursor* cursor)
{
	int requestIndex;

	//send buffer ptr/length
	char* queryBuffer;
	unsigned long queryStrLength;

	//send request
	char GET_REQUEST[1024] = "GET /img/textures/wood.png HTTP/1.1\r\nHost: learnopengl.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";
	//char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";

	//file path to open
	char filepath[1024] = "C:\\3rdGen\\CoreTransport\\Samples\\HappyEyeballs\\https\0";

	memset(cursor, 0, sizeof(CTCursor));

	_itoa(httpRequestCount, filepath + strlen(filepath), 10);

	strcat(filepath, ".png");
	httpRequestCount++;

	//CoreTransport provides support for each cursor to read responses into memory mapped files
	//however, if this is not desired, simply cursor's file.buffer slot to your own memory
	createCursorResponseBuffers(cursor, filepath);
	//cursor->file.buffer = &(responseBuffers[httpRequestCount - 1][0]);

	assert(cursor->file.buffer);
	//if (!cursor->file.buffer)
	//	return;

	cursor->headerLengthCallback = httpHeaderLengthCallback;
	cursor->responseCallback = httpResponseCallback;

	//printf("_conn.sslContextRef->Sizes.cbHeader = %d\n", _conn.sslContext->Sizes.cbHeader);
	//printf("_conn.sslContextRef->Sizes.cbTrailer = %d\n", _conn.sslContext->Sizes.cbTrailer);

	for (requestIndex = 0; requestIndex < 1; requestIndex++)
	{
		queryStrLength = strlen(GET_REQUEST);
		queryBuffer = cursor->requestBuffer;//cursor->file.buffer;
		memcpy(queryBuffer, GET_REQUEST, queryStrLength);
		queryBuffer[queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminato
		printf("\n%s\n", queryBuffer);
		CTCursorSendRequestOnQueue(cursor, _httpConn.queryCount++);
	}

}

void sendReqlQuery(CTCursor * cursor)
{
	int queryIndex;

	//send buffer ptr/length
	char * queryBuffer;
	unsigned long queryStrLength;

	//send query
	for( queryIndex = 0; queryIndex < 1; queryIndex++ )
	{
		queryStrLength = (unsigned long)strlen(paintingTableQuery);
		queryBuffer = cursor->requestBuffer;
		memcpy(queryBuffer + sizeof(ReqlQueryMessageHeader), paintingTableQuery, queryStrLength);
		queryBuffer[sizeof(ReqlQueryMessageHeader) + queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminator
		printf("queryBuffer = %s\n", queryBuffer+sizeof(ReqlQueryMessageHeader));
		CTCursorRunQueryOnQueue(cursor, _reqlConn.queryCount++);
		//CTReQLRunQueryOnQueue(&_reqlConn, (const char**)&queryBuffer, queryStrLength, _reqlConn.queryCount++);
	}

}

int _httpConnectionClosure(CTError* err, CTConnection* conn)
{
	//thread
	HANDLE hThread;
	DWORD i;

	//TO DO:  Parse error status
	assert(err->id == CTSuccess);

	//Copy CTConnection object memory from coroutine stack
	//to appplication memory (or the connection will go out of scope when this funciton exits)
	//FYI, do not use the pointer memory after this!!!
	_httpConn = *conn;
	_httpConn.response_overlap_buffer_length = 0;
	
	//allocate mapped file buffer on cursor for http request response
	//createCursorResponseBuffers(&_httpCursor, filepath);
	//_httpCursor.headerLengthCallback = httpHeaderLengthCallback;
	//_httpCursor.responseCallback = httpResponseCallback;

   //1  Create a single worker thread, respectively, for the CTConnection's (Socket) Tx and Rx Queues
   //   We associate our associated processing callbacks for each by associating the callback with the thread(s) we associate with the queues
   //   Note:  On Win32, "Queues" are actually IO Completion Ports
	for (i = 0; i < 1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CTDecryptResponseCallback, _httpConn.socketContext.rxQueue, 0, NULL);
		CloseHandle(hThread);

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CTEncryptQueryCallback, _httpConn.socketContext.txQueue, 0, NULL);
		CloseHandle(hThread);
	}

	printf("HTTP Connection Success\n");
	return err->id;
}

int _reqlConnectionClosure(CTError *err, CTConnection * conn)
{
	//file path to open
	char *filepath = "ReQLQuery.txt\0";

	//thread
	HANDLE hThread;
	DWORD i;

	//send
	char * queryBuffer;
	unsigned long queryStrLength;
	
	//status
	int status;

	//TO DO:  Parse error status
	assert(err->id == ReqlSuccess);

	//Copy CTConnection object memory from coroutine stack
	//to appplication memory (or the connection will go out of scope when this funciton exits)
	//FYI, do not use the pointer memory after this!!!
	 _reqlConn = *conn;

	//  Now Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
    if( (status = CTReQLSASLHandshake(&_reqlConn, _reqlConn.target)) != CTSuccess )
    {
        printf("CTReQLSASLHandshake failed!\n");
		CTCloseConnection(&_reqlConn);
		CTSSLCleanup();
		err->id = CTSASLHandshakeError;
		return err->id;
	}

	 createCursorResponseBuffers(&_reqlCursor, filepath);
	 _reqlCursor.headerLengthCallback = reqlHeaderLengthCallback;	
	 _reqlCursor.responseCallback = reqlResponseCallback;

	//1  Create a single worker thread, respectively, for the CTConnection's (Socket) Tx and Rx Queues
	//   We associate our associated processing callbacks for each by associating the callback with the thread(s) we associate with the queues
	//   Note:  On Win32, "Queues" are actually IO Completion Ports
	for( i = 0; i <1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CTDecryptResponseCallback, _reqlConn.socketContext.rxQueue, 0, NULL);
		CloseHandle(hThread);

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CTEncryptQueryCallback, _reqlConn.socketContext.txQueue, 0, NULL);
		CloseHandle(hThread);
	}

	printf("REQL Connection Success\n");
	return err->id;
}

//Define crossplatform keyboard event loop handler
bool getconchar(KEY_EVENT_RECORD* krec)
{
#ifdef _WIN32
	DWORD cc;
	INPUT_RECORD irec;
	KEY_EVENT_RECORD* eventRecPtr;

	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

	if (h == NULL)
	{
		return false; // console not found
	}

	for (;;)
	{
		//On Windows, we read console input and then ask if it's key down
		ReadConsoleInput(h, &irec, 1, &cc);
		eventRecPtr = (KEY_EVENT_RECORD*)&(irec.Event);
		if (irec.EventType == KEY_EVENT && eventRecPtr->bKeyDown)//&& ! ((KEY_EVENT_RECORD&)irec.Event).wRepeatCount )
		{
			*krec = *eventRecPtr;
			return true;
		}
	}
#else
	//On Linux we detect a keyboard hit using select and then read the console input
	if (kbhit())
	{
		int r;
		unsigned char c;

		//printf("\nkbhit!\n");

		if ((r = read(0, &c, sizeof(c))) == 1) {
			krec.uChar.AsciiChar = c;
			return true;
		}
	}
	//else	
	//	printf("\n!kbhit\n");


#endif
	return false; //future ????
}

void SystemKeyboardEventLoop()
{
#ifndef _WIN32	
	//user linux termios to set terminal mode from line read to char read
	set_conio_terminal_mode();
#endif
	printf("\nStarting SystemKeyboardEventLoop...\n");

	KEY_EVENT_RECORD key;
	for (;;)
	{
		memset(&key, 0, sizeof(KEY_EVENT_RECORD));
		getconchar(&key);
		if (key.uChar.AsciiChar == 's')
			sendHTTPRequest(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);
		else if (key.uChar.AsciiChar == 'q')
			break;
	}
}

int main(int argc, char **argv) 
{
	int i;
	char * caPath = (char*)certStr;//"C:\\Development\\git\\CoreTransport\\bin\\Keys\\ComposeSSLCertficate.der";// .. / Keys / ComposeSSLCertificate.der";// MSComposeSSLCertificate.cer";
	
	//These are only relevant for custom DNS resolution, which has not been implemented yet for WIN32 platforms
	//char * resolvConfPath = "../Keys/resolv.conf";
	//char * nsswitchConfPath = "../Keys/nsswitch.conf";

	CTTarget httpTarget = {0};
	CTTarget reqlService = {0};
	
	//Define https connection target
	//We aren't doing any authentication via http
	//SCHANNEL can find the certficate in the system CA keychain for us, MBEDTLS cannot
	httpTarget.host = (char*)http_server;
	httpTarget.port = http_port;
	httpTarget.ssl.ca = NULL;//(char*)caPath;
	//httpTarget.dns.resconf = (char*)resolvConfPath;
	//httpTarget.dns.nssconf = (char*)nsswitchConfPath;

	//Define RethinkDB service connection target
	//We are doing SCRAM authentication over TLS required by RethinkDB
	//For SCHANNEL SSL Cert Verification, the certficate needs to be installed in the system CA trusted authorities store
	//For WolfSSL, MBEDTLS and SecureTransport Cert Verification, the path to the certificate can be passed 
	reqlService.host = (char*)rdb_server;
	reqlService.port = rdb_port;
	reqlService.user = (char*)rdb_user;
	reqlService.password = (char*)rdb_pass;
	reqlService.ssl.ca = (char*)caPath;
	//reqlService.dns.resconf = (char*)resolvConfPath;
	//reqlService.dns.nssconf = (char*)nsswitchConfPath;

	//Demonstrate CTransport CT C API Connection (No JSON support)
	//On Darwin platforms, the Core Transport connection routine operates asynchronously on a single thread to by integrating libdill coroutines with a thread's CFRunLoop operation (so the connection can be started from the main thread before cocoa app starts)
	//On WIN32, we only have synchronous/blocking connection support, but this can be easily thrown onto a background thread at startup (even in a UWP app?)
	CTransport.connect(&httpTarget, _httpConnectionClosure);
	//CTransport.connect(&reqlService, _reqlConnectionClosure);

	//for(i=0;i<10000;i++)	
	//	sendHTTPRequest(&_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);

	SystemKeyboardEventLoop();
	
	//Clean up socket connections
	CTCloseConnection(&_httpConn);
	//CTCloseConnection(&_reqlConn);
	ct_scram_cleanup();
	CTSSLCleanup();
	return 0;
}
