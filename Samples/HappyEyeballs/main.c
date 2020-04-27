
/***
**  
*	Core Transport -- HappyEyeballs
*
*	Description:  
*	
*
*	@author: Joe Moulton III
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

//Core Transport ReQL C API
//#include <CoreTransport/CTransport.h>

//ctReQL can be optionally built with native SSL encryption
//or against MBEDTLS
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
static const char *				http_server = "learnopengl.com";//"mineralism.s3-us-west-2.amazonaws.com";
static const unsigned short		http_port = 443;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char * rdb_server = "RETHINKDB_SERVER";
static const unsigned short rdb_port = 18773;
static const char * rdb_user = "RETHINKDB_USERNAME";
static const char * rdb_pass = "RETHINKDB_PASSWORD";

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

//This defines the maximum allowable asynchronous packets to process on the IOCP tx/rx queue(s)
#define CT_MAX_INFLIGHT_QUERIES 8L

static unsigned long __stdcall CTDecryptResponseCallback(LPVOID lpParameter)
{
	int i;
	ULONG CompletionKey;

	OVERLAPPED_ENTRY ovl_entry[CT_MAX_INFLIGHT_QUERIES];
	ULONG nPaquetsDequeued;
	
	//CTQueryMessageHeader * header = NULL;
	CTOverlappedResponse * overlappedResponse = NULL;

	unsigned long NumBytesRecv = 0;

	unsigned long NumBytesRemaining = 0;
	char * extraBuffer= NULL;
	
	unsigned long PrevBytesRecvd = 0;
	unsigned long TotalBytesToDecrypt = 0;
	//unsigned long NumBytesRecvMore = 0;

	CTSSLStatus scRet = 0;
	CTClientError reqlError = CTSuccess;

	HANDLE hCompletionPort = (HANDLE)lpParameter;

	unsigned long cBufferSize = ct_system_allocation_granularity(); 

	//unsigned long headerLength;
	//unsigned long contentLength;
	CTCursor * cursor = NULL;

	printf("CTConnection::DecryptResponseCallback Begin\n");

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CT_MAX_INFLIGHT_QUERIES, &nPaquetsDequeued, INFINITE, TRUE))
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

#ifdef _DEBUG
			if (!overlappedResponse)
			{
				printf("CTConnection::CXDecryptResponseCallback::overlappedResponse is NULL!\n");
				assert(1==0);
			}
#endif

			//1 If specifically requested zero bytes using WSARecv, then IOCP is notifying us that it has bytes available in the socket buffer (MBEDTLS)
			//2 If we requested anything more than zero bytes using WSARecv, then IOCP is telling us that it was unable to read any bytes at all from the socket buffer (SCHANNEL)
			if (NumBytesRecv == 0) //
			{
#ifdef CTRANSPORT_USE_MBED_TLS				
				
				TotalBytesToDecrypt = cBufferSize;
				NumBytesRecv = cBufferSize;
				
				//Get the cursor/connection
				cursor = (CTCursor*)overlappedResponse->cursor;
				
				//read cBufferSize bytes until there are no bytes left on the socket or no bytes left in the message
				while ( TotalBytesToDecrypt > 0 ) 
				{
					NumBytesRecv = 0;
					//Issue a blocking call to read and decrypt from the socket using CTransport internal SSL platform provider
					printf("CTConnection::CXDecryptResponseCallback::Starting ReqlSSLRead blocking socket read + decrypt...\n");
					scRet  = CTSSLRead(cursor->conn->socket, cursor->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);

					if( scRet == CTSuccess )
					{
						//std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(overlappedResponse->queryToken);
						if (NumBytesRecv > 0)
						{
#ifdef _DEBUG
							printf("CTConnection::CXDecryptResponseCallback::CTSSLRead NumBytesRecv  = %d\n", (int)NumBytesRecv );
#endif
							//Do housekeeping before logic
							TotalBytesToDecrypt -= NumBytesRecv;//-= numBytesRecvd;

							//check for header
							if( cursor->headerLength == 0 )
							{
								//parse header, 

								//put a null character at the end of our decrypted bytes
								char * endDecryptedBytes = overlappedResponse->wsaBuf.buf + NumBytesRecv;
								//endDecryptedBytes = '\0';
						
								//search for end of protocol header (and set cxCursor headerLength property
								char * endOfHeader = cursor->headerLengthCallback((void*)cursor, overlappedResponse->wsaBuf.buf, NumBytesRecv);

								//Example of parsing HTTP header
								//char * endOfHeader = strstr(overlappedResponse->wsaBuf.buf, "\r\n\r\n");
								//if( !endOfHeader )
								//	assert(1==0);
								//endOfHeader += sizeof("\r\n\r\n") - 1;	//set ptr to end of http header

								//calculate size of header
								cursor->headerLength = endOfHeader - (overlappedResponse->wsaBuf.buf);

								memcpy( cursor->requestBuffer, overlappedResponse->wsaBuf.buf, cursor->headerLength);
								if( endOfHeader == endDecryptedBytes )
								{
									//we only received the header, if we wish only to store the body of the message in the mapped file
									//copy the header to the cursor's request buffer as it is no longer in use
									//NumBytesRecv -= cursor->headerLength;
								}
								else
								{
									//we received the header and some of the body
									//copy body to start of buffer to overwrite header 
									memcpy(overlappedResponse->wsaBuf.buf, endOfHeader, endDecryptedBytes - endOfHeader );
									//NumBytesRecv -= cursor->headerLength;
									//overlappedResponse->wsaBuf.buf += endDecryptedBytes-endOfHeader;
								}

								NumBytesRecv -= cursor->headerLength;
							}

							//advance buffer for subsequent reading
							//printf("CTConnection::CTSSLRead Buffer = \n\n%s\n\n", (char*)overlappedResponse->wsaBuf.buf);
							overlappedResponse->wsaBuf.buf += NumBytesRecv;							
						}
						else
						{
							//We think we reached the end of the network response
#ifdef _DEBUG
							printf("CTConnection::CXDecryptResponseCallback::Message End\n");
#endif
							CTCursorCloseMappingWithSize(cursor, overlappedResponse->wsaBuf.buf - cursor->file.buffer);
#ifdef _DEBUG
							assert(cursor->responseCallback);
#endif
							cursor->responseCallback(NULL, cursor);
							break;
						}
					}
					else if( scRet == CTSocketWouldBlock )
					{
						PrevBytesRecvd = 0;
						NumBytesRemaining = 0;
						overlappedResponse->len = cBufferSize;
						if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->wsaBuf.buf, PrevBytesRecvd, &NumBytesRemaining)) != CTSocketIOPending)
						{
							//the async operation returned immediately
							if (scRet == CTSuccess)
							{
								printf("CTConnection::CXDecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
								//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
							}
							else //failure
							{
								printf("CTConnection::CXDecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
								//break;
							}

						}
						else
						{
							printf("CTConnection::DecryptResponseCallback::CTAsyncRecv2 CTSocketIOPending\n");

						}

						continue;
				
					}
					else
					{
						printf("CTConnection::CXDecryptResponseCallback::CTSSLRead failed with error (%d)\n", (int)scRet);
						break;
					}

				}


#else
				printf("CTConnection::CXDecryptResponseCallback::SCHANNEL IOCP::NumBytesRecv = ZERO\n");
#endif


			}
			else //IOCP already read the data from the socket into the buffer we provided to Overlapped, we have to decrypt it
			{
				//Get the cursor/connection
				cursor = (CTCursor*)overlappedResponse->cursor;
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
#ifndef CTRANSPORT_USE_MBED_TLS

				PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
				NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks
				
				//Attempt to Decrypt buffer at least once
				scRet = CTSSLDecryptMessage2(cursor->conn->sslContext, overlappedResponse->buf, &NumBytesRecv, &extraBuffer, &NumBytesRemaining);
#ifdef _DEBUG
				printf("CTConnection::DecryptResponseCallback B::Decrypted data: %d bytes recvd, %d bytes remaining, scRet = 0x%x\n", NumBytesRecv, NumBytesRemaining, scRet); 
#endif
				if( scRet == SEC_E_INCOMPLETE_MESSAGE )
				{
#ifdef _DEBUG
					printf("CXConnection::DecryptResponseCallback::SEC_E_INCOMPLETE_MESSAGE %d bytes recvd, %d bytes remaining\n", PrevBytesRecvd, NumBytesRemaining); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));
#endif
					scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, PrevBytesRecvd, &NumBytesRemaining);
					continue;
				}
				else if( scRet == SEC_E_OK )
				{
					if( cursor->headerLength == 0 )
					{		
						//parse header, 

						//put a null character at the end of our decrypted bytes
						char * endDecryptedBytes = overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader + NumBytesRecv;
						//endDecryptedBytes = '\0';
						
						//search for end of protocol header (and set cxCursor headerLength property
						char * endOfHeader = cursor->headerLengthCallback(cursor, overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv);

						//char * endOfHeader = strstr(overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader, "\r\n\r\n");
						//if( !endOfHeader )
						//	assert(1==0);
						//endOfHeader += sizeof("\r\n\r\n") - 1;	//set ptr to end of http header

						//calculate size of header
						cursor->headerLength = endOfHeader - (overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader);

						memcpy( cursor->requestBuffer, overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader, cursor->headerLength);
						if( endOfHeader == endDecryptedBytes )
						{
							//we only received the header, if we wish only to store the body of the message in the mapped file
							//copy the header to the cursor's request buffer as it is no longer in use
							//NumBytesRecv = 0;
							if( NumBytesRemaining == 0)
							{
								NumBytesRemaining = cBufferSize;
								scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining);
								continue;
							}
						}
						else
						{
							//we received the header and some of the body

							//copy body to start of buffer to overwrite header 
							memcpy(overlappedResponse->buf, endOfHeader, endDecryptedBytes - endOfHeader );
							overlappedResponse->buf += endDecryptedBytes- endOfHeader;
						}
						
					}
					else
					{
					
						//push past cbHeader and copy decrypte message bytes
						memcpy( (char*)overlappedResponse->buf, (char*)overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv );
						overlappedResponse->buf += NumBytesRecv;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;
					}

					if( NumBytesRemaining == 0 )
					{
							
						scRet = ioctlsocket(cursor->conn->socket, FIONREAD, &NumBytesRemaining);

						printf("**** CTConnection::ictlsocket C status = %d\n", (int)scRet);
						printf("**** CTConnection::ictlsocket C = %d\n", (int)NumBytesRemaining);

						//select said there was data but ioctlsocket says there is no data
						if( NumBytesRemaining == 0)
						{
							CTCursorCloseMappingWithSize(cursor, overlappedResponse->buf - cursor->file.buffer); 
#ifdef _DEBUG
							assert(cursor->responseCallback);
#endif
							cursor->responseCallback(NULL, cursor);
						}
						else
						{
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

						break;
					}
					
				
					
				}
					
				PrevBytesRecvd = NumBytesRecv;  //store the previous amount of decrypted bytes

				//while their is extra data to decrypt keep calling decrypt
				while( NumBytesRemaining > 0 && scRet == SEC_E_OK )
				{
					//move the extra buffer to the front of our operating buffer
					overlappedResponse->wsaBuf.buf = extraBuffer;

					PrevBytesRecvd = NumBytesRecv;		//store the previous amount of decrypted bytes
					NumBytesRecv = NumBytesRemaining;	//set up for the next decryption call
					scRet = CTSSLDecryptMessage2(cursor->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv, &extraBuffer, &NumBytesRemaining);
					printf("CTConnection::DecryptResponseCallback C::Decrypted data: %d bytes recvd, %d bytes remaining, scRet = 0x%x\n", NumBytesRecv, NumBytesRemaining, scRet); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));

					if( scRet == SEC_E_OK )
					{
						//push past cbHeader and copy decrypte message bytes
						memcpy( (char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv );
						overlappedResponse->buf += NumBytesRecv ;//+ overlappedResponse->conn->sslContext->Sizes.cbHeader;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;
														
						if( NumBytesRemaining == 0 )
						{
							scRet = ioctlsocket(cursor->conn->socket, FIONREAD, &NumBytesRemaining);

							printf("**** CTConnection::DecryptResponseCallback::ictlsocket C status = %d\n", (int)scRet);
							printf("**** CTConnection::DecryptResponseCallback::ictlsocket C = %d\n", (int)NumBytesRemaining);

							//select said there was data but ioctlsocket says there is no data
							if( NumBytesRemaining == 0)
							{
								CTCursorCloseMappingWithSize(cursor, overlappedResponse->buf - cursor->file.buffer); 
#ifdef _DEBUG
								assert(cursor->responseCallback);
#endif
								cursor->responseCallback(NULL, cursor);
							}
							else
							{
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

							break;
						}
					}
				}

				if( scRet == SEC_E_INCOMPLETE_MESSAGE )
				{
					printf("CTConnection::DecryptResponseCallback::SEC_E_INCOMPLETE_MESSAGE %d bytes recvd, %d bytes remaining\n", PrevBytesRecvd, NumBytesRemaining); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));
					
					memcpy( (char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf, NumBytesRemaining);
									
					PrevBytesRecvd = NumBytesRemaining;// + overlappedResponse->wsaBuf.buf - cxConn->_mFilePtr;
					NumBytesRemaining = cBufferSize;// + overlappedResponse->wsaBuf.buf - cxConn->_mFilePtr;// - NumBytesRemaining;
					
					if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, PrevBytesRecvd, &NumBytesRemaining)) != CTSocketIOPending)
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
#endif
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
			}
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
	OVERLAPPED_ENTRY ovl_entry[CT_MAX_INFLIGHT_QUERIES];
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
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CT_MAX_INFLIGHT_QUERIES, &nPaquetsDequeued, INFINITE, TRUE))
	{
		printf("CTConnection::EncryptQueryCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
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

				overlappedResponse = &(cursor->overlappedResponse);
				overlappedResponse->cursor = (void*)cursor;
				overlappedResponse->len = cBufferSize;
				//Use WSASockets + IOCP to asynchronously do one of the following (depending on the usage of our underlying ssl context):
				//a)	notify us when data is available for reading from the socket via the rxQueue/thread (ie MBEDTLS path)
				//b)	read the encrypted response message from the socket into the response buffer and then notify us on the rxQueue/thread (ie SCHANNEL path)
				//if ((ctError = CTAsyncRecv(overlappedRequest->conn, (void*)&(overlappedRequest->conn->response_buffers[(queryToken%CX_MAX_INFLIGHT_QUERIES) * cBufferSize]), 0, &recvMsgLength)) != CTSocketIOPending)
				if ((ctError = CTCursorAsyncRecv(&overlappedResponse, (void*)(cursor->file.buffer), 0, &recvMsgLength)) != CTSocketIOPending)
				{
					//the async operation returned immediately
					if (ctError == CTSuccess)
					{
						printf("CTConnection::EncryptQueryCallback::CTAsyncRecv returned immediate with %lu bytes\n", recvMsgLength);
					}
					else //failure
					{
						printf("CTConnection::EncryptQueryCallback::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
					}
				}

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

void sendHTTPRequest(CTCursor * cursor)
{
	int requestIndex;

	//send buffer ptr/length
	char * queryBuffer;
	unsigned long queryStrLength;

	//send request
	char GET_REQUEST[1024] = "GET /img/textures/wood.png HTTP/1.1\r\nHost: learnopengl.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";

	//printf("_conn.sslContextRef->Sizes.cbHeader = %d\n", _conn.sslContext->Sizes.cbHeader);
	//printf("_conn.sslContextRef->Sizes.cbTrailer = %d\n", _conn.sslContext->Sizes.cbTrailer);

	for( requestIndex = 0; requestIndex< 1; requestIndex++)
	{
		queryStrLength = strlen(GET_REQUEST);
		queryBuffer = cursor->requestBuffer;//cursor->file.buffer;
		memcpy(queryBuffer, GET_REQUEST, queryStrLength);
		queryBuffer[queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminato
		printf("\n%s\n", queryBuffer);
		CTCursorSendRequestOnQueue( cursor, _httpConn.queryCount++);
		//CTSendOnQueue(&_httpConn, (char **)&queryBuffer, queryStrLength, _httpConn.queryCount++);
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



CTCursor _httpCursor;
CTCursor _reqlCursor;

char * httpHeaderLengthCallback(struct CTCursor * pCursor, char * buffer, unsigned long length )
{
	//The client can retrieve the client context from the cursor here if desired
	//or manipulate the cursors response file/file buffer if desired
	//otherwise, if the client does not desire to set the contentLength, the cursor does not need to be touched
	//CTCursor * cursor = (CTCursor*)pCursor;
	
	//the purpose of this function is to return the end 
	char * endOfHeader = strstr(buffer, "\r\n\r\n");
#ifdef _DEBUG
	if( !endOfHeader )
		assert(1==0);
#endif
	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//The client can optionally set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	//Note: The contentLength property will be overwritten with the actual content length 
	//read from the network when CoreTransport is finished reading from the socket
	//cursor->contentLength = ...

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}


char * reqlHeaderLengthCallback(struct CTCursor *cursor, char * buffer, unsigned long length )
{
	//The client can retrieve the client context from the cursor here if desired
	//or manipulate the cursors response file/file buffer if desired
	//otherwise, if the client does not desire to set the contentLength, the cursor does not need to be touched
	//CTCursor * cursor = (CTCursor*)pCursor;
	
	//the purpose of this function is to return the end 
	char * endOfHeader = buffer + sizeof(ReqlQueryMessageHeader);

	//The client can optionally set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	//Note: The contentLength property will be overwritten with the actual content length 
	//read from the network when CoreTransport is finished reading from the socket
	cursor->contentLength = ((ReqlQueryMessageHeader*)buffer)->length;
	assert( ((ReqlQueryMessageHeader*)buffer)->token == cursor->queryToken );

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
}

void httpResponseCallback(CTError * err, CTCursor *cursor)
{
	CTCursorMapFileR(cursor);
	printf("httpResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	CTCursorCloseFile(cursor);
}


void reqlResponseCallback(CTError * err, CTCursor *cursor)
{
	//CTCursor * cursor = (CTCursor*)pCursor;
	CTCursorMapFileR(cursor);
	printf("reqlCompletionCallback body:  \n\n%.*s\n\n", cursor->file.size, cursor->file.buffer);
	CTCursorCloseFile(cursor);
}


void createCursorResponseBuffers(CTCursor * cursor, const char * filepath)
{

#ifdef _WIN32
	//recv
	LPVOID queryPtr, responsePtr;
	unsigned long ctBufferSize = ct_system_allocation_granularity(); //+ sizeof(CTOverlappedResponse); 
	
	if( !filepath )
		filepath = "map.jpg\0";
	cursor->file.path = (char*)filepath;

	//file path to open
	CTCursorCreateMapFileW(cursor, cursor->file.path, ct_system_allocation_granularity()*256L);

	cursor->overlappedResponse.buf = cursor->file.buffer;
	cursor->overlappedResponse.len = ctBufferSize;
	cursor->overlappedResponse.wsaBuf.buf = cursor->file.buffer;
	cursor->overlappedResponse.wsaBuf.len = ctBufferSize;
	
#endif

#ifdef _DEBUG
	assert(cursor->file.buffer);
#endif

#ifndef _WIN32
    if (madvise(alembic_archive.filebuffer, (size_t)alembic_archive.size, MADV_SEQUENTIAL | MADV_WILLNEED ) == -1) {
        printf("\nread madvise failed\n");
    }
#endif


}
	


int _httpConnectionClosure(CTError *err, CTConnection * conn)
{

	//file path to open
	char *filepath = "wood.jpg\0";
	
	//thread
	HANDLE hThread;
	DWORD i;

	//TO DO:  Parse error status
	assert(err->id == CTSuccess);

	//Copy CTConnection object memory from coroutine stack
	//to appplication memory (or the connection will go out of scope when this funciton exits)
	//FYI, do not use the pointer memory after this!!!
	 _httpConn = *conn;
	
	 //allocate mapped file buffer on cursor for http request response
     createCursorResponseBuffers(&_httpCursor, filepath);
	 _httpCursor.headerLengthCallback = httpHeaderLengthCallback;
	 _httpCursor.responseCallback = httpResponseCallback;

	//1  Create a single worker thread, respectively, for the CTConnection's (Socket) Tx and Rx Queues
	//   We associate our associated processing callbacks for each by associating the callback with the thread(s) we associate with the queues
	//   Note:  On Win32, "Queues" are actually IO Completion Ports
	for( i = 0; i <1; ++i)
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

unsigned int __stdcall win32MainEventLoop()
{
	char indexStr[256];
	BOOL bRet = true;
	MSG msg;
	memset(&msg, 0, sizeof(MSG));

	//start the hwnd message loop, which is event driven based on platform messages regarding interaction for input and the active window
	//while (bRet)
	//{
		//pass NULL to GetMessage listen to both thread messages and window messages associated with this thread
		//if the msg is WM_QUIT, the return value of GetMessage will be zero
	while (1)//(bRet = GetMessage(&msg, NULL, 0, 0)))
	{
		bRet = GetMessage(&msg, NULL, 0, 0);
		//if (bRet == -1)
		//{
			// handle the error and possibly exit
		//}

		//if a PostQuitMessage is posted from our wndProc callback,
		//we will handle it here and break out of the view event loop and not forward back to the wndproc callback
		if (bRet == 0 || msg.message == WM_QUIT)
		{
			
		}

		//pass on the messages to the __main_event_queue for processing
		//TranslateMessage(&msg);
		DispatchMessage(&msg);
		memset(&msg, 0, sizeof(MSG));
	}

	fprintf(stdout, "\nEnd win32MainEventLoop\n");

	//if we have allocated the view ourselves we can expect the graphics context and platform window have been deallocated
	//and we can destroy the memory associatedk with the view structure
	//TO DO:  clean up view struct memory
	//ReleaseDC(glWindow.hwnd, view->hdc);	//release the device context backing the hwnd platform window
	
	//when drawing we must explitly release the DC handle
	//fyi, hwnd and hdc were assigned to the glWindowContext in CreateWnd
	//ReleaseDC(glWindow.hwnd, glWindow.hdc);

	//Since our glWindow and gpgpuWindow windows are managed by separate
	//instances of WndProc callback on separate threads,
	//but they are both closed at the same time
	//there is a race condition between when the windows get destroyed
	//and thus break out of their above GetMessage pump loops

	//we will let the gpgpuWindow control thread set the _gpgpuIdleEvent
	//to always be signaled when it is finished closing its thread
	//and have this glWindow control thread wait until that event is signaled
	//as open to ensure both get closed before the application closes
	//WaitForSingleObject(_gpgpuIdleEvent, INFINITE);

	// let's play nice and return any message sent by windows
    return (int)msg.wParam;
}

int main(int argc, char **argv) 
{
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
	//For MBEDTLS and SecureTransport Cert Verification, the certificate can be passed 
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
	CTransport.connect(&reqlService, _reqlConnectionClosure);

	sendHTTPRequest(&_httpCursor);
	sendReqlQuery(&_reqlCursor);

	//Keep the app running using platform defined run loop
	//FYI, more work needs to be done to break out of the event loop appropriately
	return win32MainEventLoop();
	//Keep the app alive long enough to receive network responses
	//Sleep(25000);

	//Clean up socket connections
	CTCloseConnection(&_httpConn);
	CTCloseConnection(&_reqlConn);
	
	return 0;
}
