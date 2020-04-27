#include "../CXTransport.h"

using namespace CoreTransport;

CXConnection::CXConnection(CTConnection *conn, CTThreadQueue rxQueue, CTThreadQueue txQueue)
{
	//copy the from CTConnection CTC memory to CXConnection managed memory (because it will go out of scope)
	memcpy(&_conn, conn, sizeof(CTConnection));

	//Reserve/allocate memory for the connection's query and response buffers 
	//reserveConnectionMemory();

	//create a dispatch source / handler for the CTConnection's socket
	if (!rxQueue)
		_rxQueue = _conn.socketContext.rxQueue;
	else 
		_rxQueue = rxQueue;
	
	if(!txQueue)
		_txQueue = _conn.socketContext.txQueue;
	else
		_txQueue = txQueue;

	startConnectionThreadQueues(_rxQueue, _txQueue);
	
}

CXConnection::~CXConnection()
{
	printf("CXConnection::Destructor()\n");
	close();
}

void CXConnection::close()
{
	CTCloseConnection(&_conn);
}

CTThreadQueue CXConnection::queryQueue()
{
	return _txQueue;
}

CTThreadQueue CXConnection::responseQueue()
{
	return _rxQueue;
}

void CXConnection::distributeResponseWithCursorForToken(uint64_t requestToken)
{
		std::shared_ptr<CXCursor> cxCursor = getRequestCursorForKey(requestToken);
		auto callback = getRequestCallbackForKey(requestToken);

		if( callback )
		{
			//TO DO:  The header and or body needs to be parsed for the error to be populated
			//appropriately based on the various connection protocols
			
			//reopen the mapping for the client, the CXCursor will close the file and mapping after the callback
			//the callback could potentially return a value that tells us whether to close the file or not
			CTCursorMapFileR(&(cxCursor->_cursor));
			callback( NULL, cxCursor);
		}
		else
		{
			printf("CXConnection::distributeResponseWithCursorForToken::No Callback!");
			printQueries();
		}

		//We are done with the cursor
		//Do the manual cleanup we need to do
		//and let the rest get cleaned up when the sharedptr to the CXCursor goes out of scope
		CTCursorCloseFile(&(cxCursor->_cursor));

}


/*****************************************************************************/
static void PrintText(DWORD length, PBYTE buffer) // handle unprintable charaters
{
	int i; //

	printf("\n"); // "length = %d bytes \n", length);
	for (i = 0; i < (int)length; i++)
	{
		if (buffer[i] == 10 || buffer[i] == 13)
			printf("%c", (char)buffer[i]);
		else if (buffer[i] < 32 || buffer[i] > 126 || buffer[i] == '%')
			continue;//printf("%c", '.');
		else
			printf("%c", (char)buffer[i]);
	}
	printf("\n\n");
}

static unsigned long __stdcall CXDecryptResponseCallback(LPVOID lpParameter)
{
	int i;
	ULONG CompletionKey;

	OVERLAPPED_ENTRY ovl_entry[CX_MAX_INFLIGHT_QUERIES];
	ULONG nPaquetsDequeued;
	
	CTOverlappedResponse * overlappedResponse = NULL;

	unsigned long NumBytesRecv = 0;
	unsigned long NumBytesRemaining = 0;
	char * extraBuffer= NULL;
	
	unsigned long PrevBytesRecvd = 0;

	CTSSLStatus scRet = 0;
	CTClientError reqlError = CTSuccess;

	CXConnection * cxConn = (CXConnection*)lpParameter;
	HANDLE hCompletionPort = cxConn->responseQueue();// (HANDLE)lpParameter;

	unsigned long cBufferSize = ct_system_allocation_granularity(); 

	//unsigned long headerLength;
	//unsigned long contentLength;
#ifdef _DEBUG
	printf("CXConnection::DecryptResponseCallback Begin\n");
#endif

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CX_MAX_INFLIGHT_QUERIES, &nPaquetsDequeued, INFINITE, TRUE))
	{
#ifdef _DEBUG
		printf("CXConnection::CTDecryptResponseCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
#endif
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
				printf("CXConnection::CXDecryptResponseCallback::overlappedResponse is NULL!\n");
				assert(1==0);
			}
#endif

			//1 If specifically requested zero bytes using WSARecv, then IOCP is notifying us that it has bytes available in the socket buffer (MBEDTLS)
			//2 If we requested anything more than zero bytes using WSARecv, then IOCP is telling us that it was unable to read any bytes at all from the socket buffer (SCHANNEL)
			if (NumBytesRecv == 0) //
			{
#ifdef CTRANSPORT_USE_MBED_TLS				
				
				unsigned long TotalBytesToDecrypt = cBufferSize;
				NumBytesRecv = cBufferSize;
				int status = 0;//cBufferSize;
								
				CTCursor * cursor = (CTCursor*)overlappedResponse->cursor;

				//read cBufferSize bytes until there are no bytes left on the socket or no bytes left in the message
				while ( TotalBytesToDecrypt > 0 ) 
				{
					NumBytesRecv = 0;
					//Issue a blocking call to read and decrypt from the socket using CTransport internal SSL platform provider
					printf("CXConnection::CXDecryptResponseCallback::Starting ReqlSSLRead blocking socket read + decrypt...\n");
					status  = CTSSLRead(cursor->conn->socket, cursor->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);

					if( status == CTSuccess )
					{
						std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(overlappedResponse->queryToken);
						if (NumBytesRecv > 0)
						{
#ifdef _DEBUG
							printf("CXConnection::CXDecryptResponseCallback::CTSSLRead NumBytesRecv  = %d\n", (int)NumBytesRecv );
#endif
							//Do housekeeping before logic
							TotalBytesToDecrypt -= NumBytesRecv;//-= numBytesRecvd;


							//check for header
							if( cursor->headerLength == 0 )
							{
		
								//parse header, 
								//assume we have at least the whole header + some of the body at this point

								//put a null character at the end of our decrypted bytes
								char * endDecryptedBytes = overlappedResponse->wsaBuf.buf + NumBytesRecv;
								//endDecryptedBytes = '\0';
						
								//search for end of protocol header (and set cxCursor headerLength property
								char * endOfHeader = cxCursor->ProcessResponseHeader(overlappedResponse->wsaBuf.buf, NumBytesRecv);

								memcpy( cursor->requestBuffer, overlappedResponse->wsaBuf.buf, cursor->headerLength);
								if( endOfHeader == endDecryptedBytes )
								{
									//we only received the header, if we wish only to store the body of the message in the mapped file
									//copy the header to the cursor's request buffer as it is no longer in use
									NumBytesRecv -= cursor->headerLength;
								}
								else
								{
									//we received the header and some of the body
									//copy body to start of buffer to overwrite header 
									memcpy(overlappedResponse->wsaBuf.buf, endOfHeader, endDecryptedBytes - endOfHeader );
									NumBytesRecv -= cursor->headerLength;
									//overlappedResponse->wsaBuf.buf += endDecryptedBytes-endOfHeader;
								}
							}
							/*
							else
							{
								//memcpy( (char*)overlappedResponse->buf, (char*)overlappedResponse->buf, NumBytesRecv );
								passBytesDecrypted  += NumBytesRecv;
								overlappedResponse->buf += NumBytesRecv;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;
								//passBytesDecrypted = 0;

							
								if( NumBytesRemaining == 0 )
								{
									FlushViewOfFile(cxCursor->_cursor.file.buffer , overlappedResponse->buf - cxCursor->_cursor.file.buffer );
									UnmapViewOfFile(cxCursor->_cursor.file.buffer );
									CloseHandle(cxCursor->_cursor.file.mFile );


									DWORD fileSize = overlappedResponse->buf - cxCursor->_cursor.file.buffer ;
									DWORD dwMaximumSizeHigh = (unsigned long long)fileSize >> 32;
									DWORD dwMaximumSizeLow = fileSize & 0xffffffffu;
									LONG offsetHigh = (LONG)dwMaximumSizeHigh;
									LONG offsetLow = (LONG)dwMaximumSizeLow;
									cxCursor->_cursor.file.size = fileSize;

									printf("Closing file with size = %d bytes\n", fileSize);
									DWORD dwCurrentFilePosition = SetFilePointer(cxCursor->_cursor.file.hFile, offsetLow, &offsetHigh, FILE_BEGIN); // provides offset from current position
									SetEndOfFile(cxCursor->_cursor.file.hFile);

									cxConn->distributeResponseWithCursorForToken(overlappedResponse->queryToken);
								}
							}
							*/

							//advance buffer for subsequent reading
							overlappedResponse->wsaBuf.buf += NumBytesRecv;

							//PrintText(NumBytesRecv, (char*)overlappedResponse);
							printf("CXConnection::CTSSLRead Buffer = \n\n%s\n\n", (char*)overlappedResponse->wsaBuf.buf);
							//cxConn->distributeMessageWithCursor(  (char*)overlappedResponse, NumBytesRecv);

							
						}
						else
						{
							printf("CXConnection::CXDecryptResponseCallback::Message End\n");
							CTCursorCloseMappingWithSize(cursor, overlappedResponse->wsaBuf.buf - cursor->file.buffer); 
							cxConn->distributeResponseWithCursorForToken(cursor->queryToken);
							break;
						}
					}
					else if( status == CTSocketWouldBlock )
					{
						PrevBytesRecvd = 0;
						NumBytesRemaining = 0;
						overlappedResponse->len = cBufferSize;
						if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->wsaBuf.buf, PrevBytesRecvd, &NumBytesRemaining)) != CTSocketIOPending)
						{
							//the async operation returned immediately
							if (scRet == CTSuccess)
							{
								printf("CXConnection::CXDecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
								//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
							}
							else //failure
							{
								printf("CXConnection::CXDecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
								//break;
							}

						}
						else
						{
							printf("CXConnection::DecryptResponseCallback::CTAsyncRecv2 CTSocketIOPending\n");

						}

						continue;
				
					}
					else
					{
						printf("CXReQLConnection::CXDecryptResponseCallback::CTSSLRead failed with error (%d)\n", (int)status);
						break;
					}

				}


#else
				printf("CXConnection::CXDecryptResponseCallback::SCHANNEL IOCP::NumBytesRecv = ZERO\n");
#endif


			}
			else //IOCP already read the data from the socket into the buffer we provided to Overlapped, we have to decrypt it
			{
				CTCursor * cursor = (CTCursor*)overlappedResponse->cursor;
				NumBytesRecv += overlappedResponse->wsaBuf.buf - overlappedResponse->buf;
#ifdef _DEBUG
				assert(cursor);
				assert(cursor->conn);
				assert(overlappedResponse->buf);
				assert(overlappedResponse->wsaBuf.buf);
				printf("CXConnection::DecryptResponseCallback B::NumBytesRecv = %d\n", (int)NumBytesRecv);
#endif
				//Issue a blocking call to decrypt the message (again, only if using non-zero read path, which is exclusively used with MBEDTLS at current)
#ifndef CTRANSPORT_USE_MBED_TLS

				PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
				NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks
								
				//Attempt to Decrypt buffer at least once
				scRet = CTSSLDecryptMessage2(cursor->conn->sslContext, overlappedResponse->buf, &NumBytesRecv, &extraBuffer, &NumBytesRemaining);
				printf("CXConnection::DecryptResponseCallback B::Decrypted data: %d bytes recvd, %d bytes remaining, scRet = 0x%x\n", NumBytesRecv, NumBytesRemaining, scRet); 

				if( scRet == SEC_E_INCOMPLETE_MESSAGE )
				{
					printf("CXConnection::DecryptResponseCallback::SEC_E_INCOMPLETE_MESSAGE %d bytes recvd, %d bytes remaining\n", PrevBytesRecvd, NumBytesRemaining); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));
					scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, PrevBytesRecvd, &NumBytesRemaining);
					//free(overlappedResponse);
					continue;
				}
				else if( scRet == SEC_E_OK )
				{
					//PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));
					//system("pause");
					
					
					std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(overlappedResponse->queryToken);
				
					if( cxCursor->_cursor.headerLength == 0 )
					{
		
						//parse header, 
						//assume we have at least the whole header + some of the body at this point

						//put a null character at the end of our decrypted bytes
						char * endDecryptedBytes = overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader + NumBytesRecv;
						//endDecryptedBytes = '\0';
						
						//search for end of protocol header (and set cxCursor headerLength property
						char * endOfHeader = cxCursor->ProcessResponseHeader(overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv);

						memcpy( cursor->requestBuffer, overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader, cxCursor->_cursor.headerLength);
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
						
						//free(overlappedResponse);
						//continue;
					}
					else
					{
					
						//push past cbHeader and copy decrypte message bytes
						memcpy( (char*)overlappedResponse->buf, (char*)overlappedResponse->buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv );
						overlappedResponse->buf += NumBytesRecv;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;
							
						if( NumBytesRemaining == 0 )
						{
							scRet = ioctlsocket(cursor->conn->socket, FIONREAD, &NumBytesRemaining);
#ifdef _DEBUG
							printf("**** CXDecryptResponseCallback::ictlsocket C status = %d\n", (int)scRet);
							printf("**** CXDecryptResponseCallback::ictlsocket C = %d\n", (int)NumBytesRemaining);
#endif
							//select said there was data but ioctlsocket says there is no data
							if( NumBytesRemaining == 0)
							{
								std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(overlappedResponse->queryToken);

								CTCursorCloseMappingWithSize(&(cxCursor->_cursor), overlappedResponse->buf - cxCursor->_cursor.file.buffer); 
								cxConn->distributeResponseWithCursorForToken(overlappedResponse->queryToken);
							}
							else
							{
								NumBytesRemaining = cBufferSize;

								if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
								{

									//the async operation returned immediately
									if (scRet == CTSuccess)
									{
										printf("CXConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
										//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
									}
									else //failure
									{
										printf("CXConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
										break;
									}
								}
							}

							break;
						}


					}
					
				
					
				}
					
				PrevBytesRecvd = NumBytesRecv;  //store the previous amount of decrypted bytes

				//while their is extra data to decrypt keep calling decrypt
				while( NumBytesRemaining > 0 && scRet == SEC_E_OK )
				{
					//move the extra buffer to the front of our operating buffer
					//memcpy((char*)overlappedResponse->buf, extraBuffer, NumBytesRemaining);

					overlappedResponse->wsaBuf.buf = extraBuffer;

					PrevBytesRecvd = NumBytesRecv;		//store the previous amount of decrypted bytes
					NumBytesRecv = NumBytesRemaining;	//set up for the next decryption call
					scRet = CTSSLDecryptMessage2(cursor->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv, &extraBuffer, &NumBytesRemaining);
#ifdef _DEBUG
					printf("CXConnection::DecryptResponseCallback C::Decrypted data: %d bytes recvd, %d bytes remaining, scRet = 0x%x\n", NumBytesRecv, NumBytesRemaining, scRet); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));
#endif
					if( scRet == SEC_E_OK )
					{
						//push past cbHeader and copy decrypte message bytes
						memcpy( (char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv );
	
						//break;
						overlappedResponse->buf += NumBytesRecv ;//+ overlappedResponse->conn->sslContext->Sizes.cbHeader;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;
							
						if( NumBytesRemaining == 0 )
						{
							scRet = ioctlsocket(cursor->conn->socket, FIONREAD, &NumBytesRemaining);
#ifdef _DEBUG
							printf("**** CXDecryptResponseCallback::ictlsocket C status = %d\n", (int)scRet);
							printf("**** CXDecryptResponseCallback::ictlsocket C = %d\n", (int)NumBytesRemaining);
#endif
							//select said there was data but ioctlsocket says there is no data
							if( NumBytesRemaining == 0)
							{
								std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(overlappedResponse->queryToken);

								CTCursorCloseMappingWithSize(&(cxCursor->_cursor), overlappedResponse->buf - cxCursor->_cursor.file.buffer); 
								cxConn->distributeResponseWithCursorForToken(overlappedResponse->queryToken);
							}
							else
							{
								NumBytesRemaining = cBufferSize;

								if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
								{

									//the async operation returned immediately
									if (scRet == CTSuccess)
									{
										printf("CXConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
										//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
									}
									else //failure
									{
										printf("CXConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
										break;
									}
								}
							}

							break;
						}
					}
				}

				//break;

				if( scRet == SEC_E_INCOMPLETE_MESSAGE )
				{

					//overlappedResponse->wsaBuf.buf = overlappedResponse->buf + NumBytesRemaining;
					//memcpy(cxConn->_mFilePtr, overlappedResponse->wsaBuf.buf, NumBytesRemaining);
					
					//overlappedResponse->buf = overlappedResponse->wsaBuf.buf;
					printf("CXConnection::DecryptResponseCallback::SEC_E_INCOMPLETE_MESSAGE %d bytes recvd, %d bytes remaining\n", PrevBytesRecvd, NumBytesRemaining); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));
					
					//unsigned long tmp = PrevBytesRecvd;
					//PrevBytesRecvd = overlappedResponse->wsaBuf.buf - cxConn->_mFilePtr;//NumBytesRemaining;
					

					//assert(extraBuffer);

					memcpy( (char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf, NumBytesRemaining);
						
					/*
					DWORD dwMaximumSizeHigh = (unsigned long long)PrevBytesRecvd >> 32;
					DWORD dwMaximumSizeLow = PrevBytesRecvd & 0xffffffffu;
					if( !(overlappedResponse->buf = (char *)MapViewOfFileEx(cxConn->_mFile, FILE_MAP_ALL_ACCESS, dwMaximumSizeHigh, dwMaximumSizeLow, cxConn->_mFileSize - PrevBytesRecvd, NULL)) )
					{

						printf("cr_file_map_to_buffer failed:  OS Virtual Mapping failed 3 ");
					}
					*/
									
					PrevBytesRecvd = NumBytesRemaining;// + overlappedResponse->wsaBuf.buf - cxConn->_mFilePtr;
					NumBytesRemaining = cBufferSize;// + overlappedResponse->wsaBuf.buf - cxConn->_mFilePtr;// - NumBytesRemaining;
					//overlappedResponse->buf =  overlappedResponse->wsaBuf.buf = cxConn->_mFilePtr + cBufferSize;

					if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, PrevBytesRecvd, &NumBytesRemaining)) != CTSocketIOPending)
					{

						//the async operation returned immediately
						if (scRet == CTSuccess)
						{
							printf("CXConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
							//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
						}
						else //failure
						{
							printf("CXConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
							break;
						}
					}
				}

#endif

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

	//if( overlappedConn )
	//	free(overlappedConn);
	//overlappedConn = NULL;

	printf("\nCTDecryptResponseCallback::End\n");
	ExitThread(0);
	return 0;
}


static unsigned long __stdcall CXEncryptQueryCallback(LPVOID lpParameter)
{
	int i;
	ULONG_PTR CompletionKey;	//when to use completion key?
	uint64_t queryToken = 0;
	OVERLAPPED_ENTRY ovl_entry[CX_MAX_INFLIGHT_QUERIES];
	ULONG nPaquetsDequeued;

	CTOverlappedResponse * overlappedRequest = NULL;

	unsigned long NumBytesSent = 0;
	unsigned long recvMsgLength = 0;

	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;

	CXConnection * cxConn = (CXConnection*)lpParameter;
	HANDLE hCompletionPort = cxConn->queryQueue();// (HANDLE)lpParameter;

	printf("CXConnection::EncryptQueryCallback Begin\n");

			unsigned long cBufferSize = ct_system_allocation_granularity(); 


	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CX_MAX_INFLIGHT_QUERIES, &nPaquetsDequeued, INFINITE, TRUE))
	{
		printf("CXConnection::EncryptQueryCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			CompletionKey = (ULONG_PTR)ovl_entry[i].lpCompletionKey;
			NumBytesSent = ovl_entry[i].dwNumberOfBytesTransferred;
			overlappedRequest = (CTOverlappedResponse*)ovl_entry[i].lpOverlapped;

			if (!overlappedRequest)
			{
				printf("CXConnection::EncryptQueryCallback::GetQueuedCompletionStatus continue\n");
				continue;
			}

			if (NumBytesSent == 0)
			{
				printf("CXConnection::EncryptQueryCallback::Server Disconnected!!!\n");
			}
			else
			{
				//Get the cursor/connection
				//Get a the CTCursor from the overlappedRequest struct, retrieve the overlapped response struct memory, and store reference to the CTCursor in the about-to-be-queued overlappedResponse
				CTCursor * cursor = (CTCursor*)(overlappedRequest->cursor);
				std::shared_ptr<CXCursor> cxCursor = cxConn->getRequestCursorForKey(cursor->queryToken);
				
#ifdef _DEBUG
				assert(cursor);
				assert(cursor->conn);
#endif
				NumBytesSent = overlappedRequest->len;
				printf("CXConnection::EncryptQueryCallback::NumBytesSent = %d\n", (int)NumBytesSent);
				printf("CXConnection::EncryptQueryCallback::msg = %s\n", overlappedRequest->buf + sizeof(ReqlQueryMessageHeader));

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
				printf("CXConnection::EncryptQueryCallback::queryTOken = %llu\n", queryToken);

				//Issue a blocking read for debugging purposes
				//CTRecv(overlappedQuery->conn, (void*)&(overlappedQuery->conn->response_buffers[ (queryToken%REQL_MAX_INFLIGHT_QUERIES) * REQL_RESPONSE_BUFF_SIZE]), &recvMsgLength);
	

				//populate the overlapped response as needed before passing to CTCursorAsyncRecv
				CTOverlappedResponse * overlappedResponse = &(cursor->overlappedResponse);
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
						printf("CXConnection::EncryptQueryCallback::CTAsyncRecv returned immediate with %lu bytes\n", recvMsgLength);
					}
					else //failure
					{
						printf("CXConnection::EncryptQueryCallback::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
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

	printf("\nCXConnectin::EncryptQueryCallback::End\n");
	ExitThread(0);
	return 0;

}

std::shared_ptr<CXCursor> CXConnection::createRequestCursor(uint64_t queryToken)
{
	std::shared_ptr<CXCursor> cxCursor( new CXCursor(&_conn) );
	addRequestCursorForKey(cxCursor, queryToken);
	return cxCursor;
}
	

CTDispatchSource CXConnection::startConnectionThreadQueues(CTThreadQueue rxQueue, CTThreadQueue txQueue)
{
#ifdef _WIN32
	//set up the win32 iocp handler for the connection's socket here
	int i;
	HANDLE hThread;

	//1  Create a single worker thread, respectively, for the CTConnection's (Socket) Tx and Rx Queues
	//   We associate our associated processing callbacks for each by associating the callback with the thread(s) we associate with the queues
	//   Note:  On Win32, "Queues" are actually IO Completion Ports
	for (i = 0; i < 1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool
		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CXDecryptResponseCallback, this, 0, NULL);
		CloseHandle(hThread);

		hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CXEncryptQueryCallback, this, 0, NULL);
		CloseHandle(hThread);
	}


#elif defined(__APPLE__)
	//set up the disptach_source_t of type DISPATCH_SOURCE_TYPE_READ using gcd to issue a callback callback/closure for the connection's socket here

#endif
}
