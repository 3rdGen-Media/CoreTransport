#include "../CTransport.h"


unsigned long __stdcall CT_Dequeue_Connect(LPVOID lpParameter)
{
	int i;
	OSStatus status;
	ULONG_PTR CompletionKey;	//when to use completion key?
	uint64_t queryToken = 0;
	OVERLAPPED_ENTRY ovl_entry[CT_MAX_INFLIGHT_CONNECT_PACKETS];
	ULONG nPaquetsDequeued;

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;

	CTCursor* cursor = NULL;
	CTOverlappedTarget* overlappedTarget = NULL;
	//CTOverlappedResponse* overlappedResponse = NULL;

	unsigned long NumBytesTransferred = 0;
	unsigned long recvMsgLength = 0;

	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;

	//we passed the completion port handle as the lp parameter
	HANDLE hCompletionPort = (HANDLE)lpParameter;
	CTConnection* conn = NULL;
	unsigned long cBufferSize = ct_system_allocation_granularity();

	CTCursor* closeCursor = NULL;
	CTError err = { 0,0,NULL };

	fprintf(stderr, "CTConnectThread Begin\n");

	//TO DO:  Optimize number of packets dequeued at once
	while (1)
	{
		BOOL result = GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CT_MAX_INFLIGHT_CONNECT_PACKETS, &nPaquetsDequeued, INFINITE, TRUE);
		//fprintf(stderr, "CTConnection::EncryptQueryCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			CompletionKey = (ULONG_PTR)ovl_entry[i].lpCompletionKey;
			NumBytesTransferred = ovl_entry[i].dwNumberOfBytesTransferred;
			overlappedTarget = (CTOverlappedTarget*)ovl_entry[i].lpOverlapped;

			if (!overlappedTarget)
			{
				fprintf(stderr, "CTConnection::CTConnectThread::GetQueuedCompletionStatus continue\n");
				continue;
			}

			fprintf(stderr, "CTConnection::CTConnectThread::Overlapped Target Packet Dequeued!!!\n");

			if (overlappedTarget->stage == CT_OVERLAPPED_SCHEDULE)
			{
				//This will resolve the host and kick off the async connection
				//TO DO: make the resolve async
				//overlappedTarget->target->ctx = overlappedTarget; // store the overlapped for subsquent async connection execution
				CTTargetResolveHost(overlappedTarget->target, overlappedTarget->target->callback);
				fprintf(stderr, "CTConnection::CTConnectThread::End CTTargetResolveHost\n");

			}
			else if (overlappedTarget->stage == CT_OVERLAPPED_RECV_FROM)
			{
				//assert(1 == 0);
				cursor = overlappedTarget->cursor;
				cursor->target = overlappedTarget->target;
				//cursor->target->ctx = overlappedTarget;
				overlappedTarget->cursor->overlappedResponse.buf += NumBytesTransferred;
				if (cursor->headerLength == 0)
				{
					//search for end of protocol header (and set cxCursor headerLength property
					char* endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesTransferred);

					//calculate size of header
					if (endOfHeader)
					{
						cursor->headerLength = endOfHeader - (cursor->file.buffer);

						memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

						fprintf(stderr, "CTDecryptResponseCallbacK::Header = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

						//copy body to start of buffer to overwrite header 
						memcpy(cursor->file.buffer, endOfHeader, NumBytesTransferred - cursor->headerLength);
						//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

						overlappedTarget->cursor->overlappedResponse.buf -= cursor->headerLength;
					}
				}
				assert(cursor->contentLength > 0);

				if (cursor->contentLength <= overlappedTarget->cursor->overlappedResponse.buf - cursor->file.buffer)
				{
					closeCursor = cursor;
					//increment the connection response count
							//closeCursor->conn->rxCursorQueueCount--;
					//closeCursor->conn->responseCount++;

#ifdef _DEBUG
					assert(closeCursor->responseCallback);
#endif
					//issue the response to the client caller
					closeCursor->responseCallback(&err, closeCursor);
					closeCursor = NULL;	//set to NULL for posterity

				}

				continue;
			}
			else if (overlappedTarget->stage == CT_OVERLAPPED_EXECUTE)
			{
				//assert(result == TRUE);
				//This will get the result of the async connection and perform ssl hanshake

				fprintf(stderr, "ConnectEx complete error = %d\n\n", WSAGetLastError());
				//struct CTConnection conn = { 0 };
				//get a pointer to a new connection object memory
				struct CTError error = { (CTErrorClass)0,0,0 };    //Reql API client functions will generally return ints as errors

				//Check for socket error to ensure that the connection succeeded
				if ((error.id = CTSocketGetError(overlappedTarget->target->socket)) != 0)
				{
					// connection failed; error code is in 'result'
					fprintf(stderr, "CTConnection::CTConnectThread::socket async connect failed with result: %d", error.id);
					CTSocketClose(overlappedTarget->target->socket);
					error.id = CTSocketConnectError;
					goto CONN_CALLBACK;
				}

				/* Make the socket more well-behaved.
			   int rc = setsockopt(overlappedTarget->target->socket, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
			   if (rc != 0) {
				   fprintf(stderr, "SO_UPDATE_CONNECT_CONTEXT failed: %d\n", WSAGetLastError());
				   return 1;
			   }
			   */

				conn = CTGetNextPoolConnection();
				memset(conn, 0, sizeof(CTConnection));

				conn->socket = overlappedTarget->target->socket;
				conn->socketContext.socket = overlappedTarget->target->socket;
				conn->socketContext.host = overlappedTarget->target->url.host;






				//For Loading
				IO_STATUS_BLOCK iostatus;
				pNtSetInformationFile NtSetInformationFile = NULL;

				//For Executing
				FILE_COMPLETION_INFORMATION  socketCompletionInfo = { NULL, NULL };// { overlappedTarget->target->cxQueue, dwCompletionKey };
				ULONG fcpLen = sizeof(FILE_COMPLETION_INFORMATION);

				//Load library if needed...
				//	ntdll = LoadLibrary("ntdll.dll");
				//	if (ntdll == NULL) {
				//		return 0;
				//	}

				//LoadNtSetInformation function ptr from ntdll.lib
				NtSetInformationFile = (pNtSetInformationFile)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtSetInformationFile");
				if (NtSetInformationFile == NULL) {
					return 0;
				}

				//remove the existing completion port attache to socket handle used for connection
				if (NtSetInformationFile(conn->socket, &iostatus, &socketCompletionInfo, sizeof(FILE_COMPLETION_INFORMATION), FileReplaceCompletionInformation) < 0) {
					return 0;
				}

				//Copy tx, rx completion port queue from target to connection and attach them to the socket handle for async writing and async reading, respectively
				//We assume that since we are executing on a cxQueue placed on the target by the user, they will have also specified their own txQueue, rxQueue as well
				conn->socketContext.txQueue = overlappedTarget->target->txQueue;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
				conn->socketContext.rxQueue = overlappedTarget->target->rxQueue;// CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
				CreateIoCompletionPort((HANDLE)conn->socket, conn->socketContext.rxQueue, dwCompletionKey, 1);


				//There is some short order work that would be good to parallelize regarding loading the cert and saving it to keychain
				//However, the SSLHandshake loop has already been optimized with yields for every asynchronous call

#ifdef DEBUG
				fprintf(stderr, "Before CTSSLRoutine Host = %s\n", conn.socketContext.host);
				fprintf(stderr, "Before CTSSLRoutine Host = %s\n", overlappedTarget->target->host);
#endif

				//put the connection callback on the service so we can ride it through the async handshake

				conn->target = overlappedTarget->target;

				if (conn->target->proxy.host)
				{
					//Initate the Async SSL Handshake scheduling from this thread (ie cxQueue) to rxQueue and txQueue threads
					if ((status = CTProxyHandshake(conn)) != 0)
					{
						fprintf(stderr, "CTConnection::CTConnectThread::CTSSLRoutine failed with error: %d\n", (int)status);
						error.id = CTSSLHandshakeError;
						goto CONN_CALLBACK;
						//return status;
					}

					//if we are running the handshake asynchronously on a queue, don't return connection to client yet
					if (conn->socketContext.txQueue)
						continue;
				}
				else if (conn->target->ssl.method > CTSSL_NONE)
				{
					//Initate the Async SSL Handshake scheduling from this thread (ie cxQueue) to rxQueue and txQueue threads
					if ((status = CTSSLRoutine(conn, conn->socketContext.host, overlappedTarget->target->ssl.ca)) != 0)
					{
						fprintf(stderr, "CTConnection::CTConnectThread::CTSSLRoutine failed with error: %d\n", (int)status);
						error.id = CTSSLHandshakeError;
						goto CONN_CALLBACK;
						//return status;
					}

					//if we are running the handshake asynchronously on a queue, don't return connection to client yet
					if (conn->socketContext.txQueue)
						continue;
				}

				/*
				int iResult;

				u_long iMode = 1;

				//-------------------------
				// Set the socket I/O mode: In this case FIONBIO
				// enables or disables the blocking mode for the
				// socket based on the numerical value of iMode.
				// If iMode = 0, blocking is enabled;
				// If iMode != 0, non-blocking mode is enabled.

				iResult = ioctlsocket(conn.socket, FIONBIO, &iMode);
				if (iResult != NO_ERROR)
					fprintf(stderr, "ioctlsocket failed with error: %ld\n", iResult);
				*/

				//The connection completed successfully, allocate memory to return to the client
				error.id = CTSuccess;

			CONN_CALLBACK:
				//fprintf(stderr, "before callback!\n");
				conn->responseCount = 0;  //we incremented this for the handshake, it is critical to reset this for the client before returning the connection
				conn->queryCount = 0;
				overlappedTarget->target->callback(&error, conn);
				//fprintf(stderr, "After callback!\n");

				/*
				//Remove observer that yields to coroutines
				CFRunLoopRemoveObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
				//Remove dummy source that keeps run loop alive
				CFRunLoopRemoveSource(runLoop, runLoopSource, kCFRunLoopDefaultMode);

				//release runloop object memory
				CFRelease(runLoopObserver);
				CFRelease(runLoopSource);
				*/

				//return the error id
				return error.id;

			}
			else
			{
				fprintf(stderr, "CTConnection::CTConnectThread::UNHANDLED_STAGE (%d)\n", overlappedTarget->stage);
				fflush(stderr);
			}
			/*
			if (NumBytesSent == 0)
			{
				fprintf(stderr, "CTConnection::CTConnectThread::Server Disconnected!!!\n");
			}
			else
			{

				CTTargetResolveHost(overlappedTarget->target, overlappedTarget->callback);

			}
			*/
		}

	}

	fprintf(stderr, "\nCTConnection::CTConnectThread::End\n");
	ExitThread(0);
	return 0;

}


static unsigned long __stdcall CT_Dequeue_Recv_Decrypt(LPVOID lpParameter)
{
	int i;
	ULONG CompletionKey;

	OVERLAPPED_ENTRY ovl_entry[CT_MAX_INFLIGHT_DECRYPT_PACKETS];
	ULONG nPaquetsDequeued;

	//CTQueryMessageHeader * header = NULL;
	CTOverlappedResponse* overlappedResponse = NULL;

	unsigned long NumBytesRecv = 0;

	unsigned long NumBytesRemaining = 0;
	char* extraBuffer = NULL;

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
	CTCursor* cursor = NULL;
	CTCursor* nextCursor = NULL;
	CTCursor* closeCursor = NULL;

	MSG nextCursorMsg = { 0 };
	CTError err = { 0,0,NULL };

	unsigned long currentThreadID = GetCurrentThreadId();

	fprintf(stderr, "CTConnection::DecryptResponseCallback Begin\n");

	//Create a Win32 Platform Message Queue for this thread by calling PeekMessage 
	//(there should not be any messages currently in the queue because it hasn't been created until now)
	PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER, PM_REMOVE);

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CT_MAX_INFLIGHT_DECRYPT_PACKETS, &nPaquetsDequeued, INFINITE, TRUE))
	{
		//fprintf(stderr, "CTConnection::CTDecryptResponseCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			//When to use CompletionKey?
			//CompletionKey = *(ovl_entry[i].lpCompletionKey);

			//Retrieve the number of bytes provided from the socket buffer by IOCP
			NumBytesRecv = ovl_entry[i].dwNumberOfBytesTransferred;//% (cBufferSize - sizeof(CTOverlappedResponse));

			//Get the overlapped struct associated with the asychronous IOCP call
			overlappedResponse = (CTOverlappedResponse*)ovl_entry[i].lpOverlapped;

			//Get the cursor/connection that has been pinned on the overlapped
			cursor = (CTCursor*)overlappedResponse->cursor;

#ifdef _DEBUG
			if (!overlappedResponse)
			{
				fprintf(stderr, "CTConnection::CXDecryptResponseCallback::overlappedResponse is NULL!\n");
				assert(1 == 0);
			}
#endif
			//Cursor requests are always queued onto the connection's decrypt thread (ie this one) to be scheduled
			//so every overlapped sent to this decrypt thread is handled here first.
			//There are two possible scenarios:
			//	1)  There are currently previous cursor requests still being processed and the cursor request is scheduled
			//		for pickup later from this thread's message queue
			//	2)  The are currently no previous cursor requests still being processed and so we may initiate an async socket recv
			//		for the cursor's response
			if (overlappedResponse->stage > -1 && overlappedResponse->stage < 3)
			{
				fprintf(stderr, "CTConnection::CTDecryptResponseCallback::Scheduling Cursor (%d) via ...", cursor->queryToken);

				//set the overlapped message type to indicate that scheduling for the current
				//overlapped/cursor has already been processed
				overlappedResponse->stage += 4;// CT_OVERLAPPED_EXECUTE;
				if (cursor->conn->responseCount > 0)//< cursor->queryToken)
				{
					fprintf(stderr, "PostThreadMessage\n");
					if (PostThreadMessage(currentThreadID, WM_USER + cursor->queryToken, 0, (LPARAM)cursor) < 1)
					{
						//cursor->conn->rxCursorQueueCount++;
						fprintf(stderr, "CTDecryptResponseCallback::PostThreadMessage failed with error: %d", GetLastError());
					}
				}
				else
				{
					fprintf(stderr, "CTCursorAsyncRecv\n");
					if ((ctError = CTCursorAsyncRecv(&overlappedResponse, (void*)(cursor->file.buffer), 0, &NumBytesRecv)) != CTSocketIOPending)
					{
						//the async operation returned immediately
						if (ctError == CTSuccess)
						{
							//cursor->conn->rxCursorQueueCount++;
							fprintf(stderr, "CTConnection::CTDecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRecv);
						}
						else //failure
						{
							fprintf(stderr, "CTConnection::CTDecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
						}
					}
					//else cursor->conn->rxCursorQueueCount++;
				}

				cursor->conn->responseCount++;
				continue;
			}


			//1 If specifically requested zero bytes using WSARecv, then IOCP is notifying us that it has bytes available in the socket buffer (MBEDTLS)
			//2 If we requested anything more than zero bytes using WSARecv, then IOCP is telling us that it was unable to read any bytes at all from the socket buffer (SCHANNEL)
			//if (NumBytesRecv == 0) //
			//{
			//	fprintf(stderr, "CTConnection::CXDecryptResponseCallback::SCHANNEL IOCP::NumBytesRecv = ZERO\n");
			//}
			//else //IOCP already read the data from the socket into the buffer we provided to Overlapped, we have to decrypt it
			//{
				//Get the cursor/connection
				//cursor = (CTCursor*)overlappedResponse->cursor;
			NumBytesRecv += overlappedResponse->wsaBuf.buf - overlappedResponse->buf;
			fprintf(stderr, "Cursor Response Count (%d) NumBytesRecv (%d)\n\n", cursor->conn->responseCount, NumBytesRecv);

			PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
			NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks

			extraBuffer = overlappedResponse->buf;
			NumBytesRemaining = NumBytesRecv;
			scRet = SEC_E_OK;

#ifdef _DEBUG
			assert(cursor);
			assert(cursor->conn);
			assert(cursor->headerLengthCallback);
			assert(overlappedResponse->buf);
			assert(overlappedResponse->wsaBuf.buf);
			//fprintf(stderr, "CTConnection::DecryptResponseCallback B::NumBytesRecv = %d\n", (int)NumBytesRecv);
#endif

			//while their is extra data to decrypt keep calling decrypt

			if (overlappedResponse->stage == CT_OVERLAPPED_RECV)
			{
				overlappedResponse->buf += NumBytesRecv;

				while (NumBytesRemaining > 0 && scRet == SEC_E_OK)
				{

					if (cursor->headerLength == 0)
					{
						//search for end of protocol header (and set cxCursor headerLength property
						char* endOfHeader = cursor->headerLengthCallback(cursor, cursor->file.buffer, NumBytesRecv);

						//calculate size of header
						if (endOfHeader)
						{
							cursor->headerLength = endOfHeader - (cursor->file.buffer);

							fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Header (%llu) = \n\n%.*s\n\n", ((ReqlQueryMessageHeader*)(cursor->file.buffer))->token, cursor->headerLength, cursor->file.buffer);

							if (cursor->headerLength > 0)
								memcpy(cursor->requestBuffer, cursor->file.buffer, cursor->headerLength);

							//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Header (%llu) = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

							//copy body to start of buffer to overwrite header 
							if (endOfHeader != cursor->file.buffer)
							{
								if (NumBytesRecv == cursor->headerLength)
									memset(cursor->file.buffer, 0, cursor->headerLength);
								else
								{
									memcpy(cursor->file.buffer, endOfHeader, NumBytesRecv - cursor->headerLength);
									fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body (%d) = \n\n%.*s\n\n", cursor->contentLength, cursor->contentLength, cursor->file.buffer);
								}
							}
							overlappedResponse->buf -= cursor->headerLength;
							NumBytesRemaining -= cursor->headerLength;
						}
					}
					//assert(cursor->contentLength > 0);

					if (cursor->contentLength > 0 && cursor->contentLength <= overlappedResponse->buf - cursor->file.buffer)
					{
						extraBufferOffset = 0;
						closeCursor = cursor;
						NumBytesRemaining = 0;// (cursor->contentLength + cursor->headerLength);// -NumBytesRecv;

						memset(&nextCursorMsg, 0, sizeof(MSG));
						nextCursor = NULL;
						if (PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER + cursor->conn->queryCount - 1, PM_REMOVE)) //(nextCursor)
						{
							nextCursor = (CTCursor*)(nextCursorMsg.lParam);
#ifdef _DEBUG
							//assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
							assert(nextCursor);
#endif									
							if (cursor->contentLength < overlappedResponse->buf - cursor->file.buffer)
							{
								fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::vagina");

								cursor->conn->response_overlap_buffer_length = (size_t)overlappedResponse->buf - (size_t)(cursor->file.buffer + cursor->contentLength);
								assert(cursor->conn->response_overlap_buffer_length > 0);

								if (nextCursor && cursor->conn->response_overlap_buffer_length > 0)
								{
									NumBytesRemaining = cursor->conn->response_overlap_buffer_length;

									//copy overlapping cursor's response from previous cursor's buffer to next cursor's buffer
									memcpy(nextCursor->file.buffer, cursor->file.buffer + cursor->contentLength, cursor->conn->response_overlap_buffer_length);

									//read the new cursor's header
									if (nextCursor->headerLength == 0)
									{
										//search for end of protocol header (and set cxCursor headerLength property
										assert(cursor->file.buffer != nextCursor->file.buffer);
										//assert(((ReqlQueryMessageHeader*)nextCursor->file.buffer)->token == nextCursor->queryToken);

										char* endOfHeader = nextCursor->headerLengthCallback(nextCursor, nextCursor->file.buffer, cursor->conn->response_overlap_buffer_length);

										//calculate size of header
										if (endOfHeader)
										{
											nextCursor->headerLength = endOfHeader - (nextCursor->file.buffer);

											if (nextCursor->headerLength > 0)
											{

												if (cursor->conn->response_overlap_buffer_length < nextCursor->headerLength)
												{
													//leave the incomplete header in place and go around again
													nextCursor->headerLength = 0;
													//NumBytesRemaining = 32768;
												}
												else
												{

													memcpy(nextCursor->requestBuffer, nextCursor->file.buffer, nextCursor->headerLength);

													fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV nextCursor->Header (%llu) = \n\n%.*s\n\n", ((ReqlQueryMessageHeader*)(nextCursor->file.buffer))->token, nextCursor->headerLength, nextCursor->requestBuffer);

													//copy body to start of buffer to overwrite header 
													if (cursor->conn->response_overlap_buffer_length > nextCursor->headerLength && endOfHeader != nextCursor->file.buffer)
													{
														memcpy(nextCursor->file.buffer, endOfHeader, cursor->conn->response_overlap_buffer_length - nextCursor->headerLength);
														fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV nextCursor->Body (%d) = \n\n%.*s\n\n", cursor->conn->response_overlap_buffer_length, nextCursor->contentLength, nextCursor->file.buffer);

													}
													//else if( cursor->conn->response_overlap_buffer_length == nextCursor->headerLength)
													//	NumBytesRemaining = 
													//overlappedResponse->buf += endDecryptedBytes - endOfHeader;

													//fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV Body = \n\n%.*s\n\n", nextCursor->contentLength, nextCursor->file.buffer);


													cursor->conn->response_overlap_buffer_length -= nextCursor->headerLength;
													NumBytesRemaining = cursor->conn->response_overlap_buffer_length;

													//NumBytesRemaining = 32768;//


												}
											}
										}

										//extraBufferOffset = cursor->conn->response_overlap_buffer_length;
										//NumBytesRemaining -= cursor->headerLength;
										nextCursor->overlappedResponse.buf = nextCursor->file.buffer + cursor->conn->response_overlap_buffer_length;
										//if (nextCursor->queryToken == 99) assert(1 == 0);


									}

								}
							}
							//else
							//	nextCursor = NULL;

							if (nextCursor && NumBytesRemaining == 0)
							{
								fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::Penis\n");

								assert(nextCursor);
								NumBytesRecv = cBufferSize;;
								overlappedResponse = &(nextCursor->overlappedResponse);
								if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->file.buffer  /* + cursor->conn->response_overlap_buffer_length - nextCursor->headerLength*/), 0, &NumBytesRecv)) != CTSocketIOPending)
								{
									//the async operation returned immediately
									if (scRet == CTSuccess)
									{
										fprintf(stderr, "CTConnection::CT_OVERLAPPED_RECV::CTAsyncRecv (new cursor) returned immediate with %lu bytes\n", NumBytesRecv);

									}
									else //failure
									{
										fprintf(stderr, "CTConnection::CT_OVERLAPPED_RECV::CTAsyncRecv (new cursor) failed with CTDriverError = %d\n", scRet);
									}
								}
								nextCursor = NULL;
								//NumBytesRemaining = 0;
								//break;
							}

							else if( nextCursor )
							{
								fprintf(stderr, "CT_Deque_Recv_Decrypt::CT_OVERLAPPED_RECV::Scrotum\n");

#ifdef _DEBUG
								assert(nextCursor);
								assert(nextCursor->file.buffer);
								//assert(extraBuffer);
#endif
							//since extra buffer may be pointing to current cursor buffer memory we have to copy it to the next
							//cursor BEFORE closing the current cursor's mapping
							//memcpy(nextCursor->file.buffer + extraBufferOffset, extraBuffer, NumBytesRemaining);
							//extraBuffer = nextCursor->file.buffer + extraBufferOffset;


								overlappedResponse = &(nextCursor->overlappedResponse);
								//overlappedResponse->buf = nextCursor->file.buffer + nextCursor->headerLength;// cursor->conn->response_overlap_buffer_length;// +extraBufferOffset;

								cursor = nextCursor;
								nextCursor = NULL;

							}

						}

						//increment the connection response count
						//closeCursor->conn->rxCursorQueueCount--;
						closeCursor->conn->responseCount--;

#ifdef _DEBUG
						assert(closeCursor->responseCallback);
#endif
						//issue the response to the client caller
						closeCursor->responseCallback(&err, closeCursor);
						//if (closeCursor->queryToken == 99) assert(1 == 0);
						closeCursor = NULL;	//set to NULL for posterity

					}
					else
					{
						NumBytesRecv = NumBytesRemaining;
						NumBytesRemaining = cBufferSize - NumBytesRecv;
						if ((scRet = CTCursorAsyncRecv(&overlappedResponse, cursor->file.buffer, NumBytesRecv, &NumBytesRemaining)) != CTSocketIOPending)
						{
							//the async operation returned immediately
							if (scRet == CTSuccess)
							{
	 							fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
								//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
								//continue;
							}
							else //failure
							{
								fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
								assert(1 == 0);
							}
						}
						//NumBytesRemaining = 0;
						break;
					}
					continue;
				}
			}

			else if (overlappedResponse->stage == CT_OVERLAPPED_EXECUTE)//CT_OVERLAPPED_RECV_DECRYPT)
			{

				//Issue a blocking call to decrypt the message (again, only if using non-zero read path, which is exclusively used with MBEDTLS at current)

				/*
				PrevBytesRecvd = NumBytesRecv;	 //store the recvd bytes value from the overlapped
				NumBytesRemaining = cBufferSize; //we can never decrypt more bytes than the size of our buffer, and likely schannel will still separate that into chunks

				extraBuffer = overlappedResponse->buf;
				NumBytesRemaining = NumBytesRecv;
				scRet = SEC_E_OK;
				*/

				//while their is extra data to decrypt keep calling decrypt
				while (NumBytesRemaining > 0 && scRet == SEC_E_OK)
				{
					//move the extra buffer to the front of our operating buffer
					overlappedResponse->wsaBuf.buf = extraBuffer;
					extraBuffer = NULL;
					PrevBytesRecvd = NumBytesRecv;		//store the previous amount of decrypted bytes
					NumBytesRecv = NumBytesRemaining;	//set up for the next decryption 

					scRet = CTSSLDecryptMessage2(cursor->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv, &extraBuffer, &NumBytesRemaining);
					//fprintf(stderr, "CTConnection::DecryptResponseCallback C::Decrypted data: %d bytes recvd, %d bytes remaining, scRet = 0x%x\n", NumBytesRecv, NumBytesRemaining, scRet); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));

					if (scRet == SEC_E_OK)
					{
						//push past cbHeader and copy decrypted message bytes
						memcpy((char*)overlappedResponse->buf, (char*)overlappedResponse->wsaBuf.buf + cursor->conn->sslContext->Sizes.cbHeader, NumBytesRecv);
						overlappedResponse->buf += NumBytesRecv;//+ overlappedResponse->conn->sslContext->Sizes.cbHeader;// + overlappedResponse->conn->sslContext->Sizes.cbTrailer;

						//if( NumBytesRemaining == 0 )
						//{
							//scRet = ioctlsocket(cursor->conn->socket, FIONREAD, &NumBytesRemaining);

							//fprintf(stderr, "**** CTConnection::DecryptResponseCallback::ictlsocket C status = %d\n", (int)scRet);
							//fprintf(stderr, "**** CTConnection::DecryptResponseCallback::ictlsocket C = %d\n", (int)NumBytesRemaining);

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

								//fprintf(stderr, "CTDecryptResponseCallbacK::Header = \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);

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
							if (PeekMessage(&nextCursorMsg, NULL, WM_USER, WM_USER + cursor->conn->queryCount - 1, PM_REMOVE)) //(nextCursor)
							{
								nextCursor = (CTCursor*)(nextCursorMsg.lParam);
#ifdef _DEBUG
								//assert(nextCursorMsg.message == WM_USER + (UINT)cursor->queryToken + 1);
								assert(nextCursor);
#endif									
								if (cursor->contentLength < overlappedResponse->buf - cursor->file.buffer)
								{
									fprintf(stderr, "vagina");

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
								if (NumBytesRemaining == 0)
								{
									fprintf(stderr, "penis");

									NumBytesRecv = cBufferSize;;
									overlappedResponse = &(nextCursor->overlappedResponse);
									if ((scRet = CTCursorAsyncRecv(&(overlappedResponse), (void*)(nextCursor->file.buffer + cursor->conn->response_overlap_buffer_length - nextCursor->headerLength), 0, &NumBytesRecv)) != CTSocketIOPending)
									{
										//the async operation returned immediately
										if (scRet == CTSuccess)
										{
											fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv (new cursor) returned immediate with %lu bytes\n", NumBytesRecv);

										}
										else //failure
										{
											fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv (new cursor) failed with CTDriverError = %d\n", scRet);
										}
									}
									nextCursor = NULL;
									//break;
								}
								else
								{
									fprintf(stderr, "scrotum");

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
							closeCursor->conn->responseCount--;

#ifdef _DEBUG
							assert(closeCursor->responseCallback);
#endif
							//issue the response to the client caller
							closeCursor->responseCallback(NULL, closeCursor);
							closeCursor = NULL;	//set to NULL for posterity
						}
						else if (NumBytesRemaining == 0)
						{
							fprintf(stderr, "wanker");

							NumBytesRemaining = cBufferSize;
							if ((scRet = CTCursorAsyncRecv(&overlappedResponse, overlappedResponse->buf, 0, &NumBytesRemaining)) != CTSocketIOPending)
							{
								//the async operation returned immediately
								if (scRet == CTSuccess)
								{
									fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
									//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
								}
								else //failure
								{
									fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
									break;
								}
							}
							break;
						}

					}
				}

				if (scRet == SEC_E_INCOMPLETE_MESSAGE)
				{
					//fprintf(stderr, "CTConnection::DecryptResponseCallback C::SEC_E_INCOMPLETE_MESSAGE %d bytes recvd, %d bytes remaining\n", PrevBytesRecvd, NumBytesRemaining); //PrintText(NumBytesRecv, (PBYTE)(overlappedResponse->wsaBuf.buf + overlappedResponse->conn->sslContext->Sizes.cbHeader));

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
							//fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
							//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
						}
						else //failure
						{
							fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
							break;
						}
					}
				}

			}
			//fprintf(stderr, "\nCTDecryptResponseCallback::END\n\n");

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
						fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv returned immediate with %lu bytes\n", NumBytesRemaining);
						//scRet = CTSSLDecryptMessage(overlappedResponse->conn->sslContext, overlappedResponse->wsaBuf.buf, &NumBytesRecv);
					}
					else //failure
					{
						fprintf(stderr, "CTConnection::DecryptResponseCallback::CTAsyncRecv failed with CTDriverError = %d\n", scRet);
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
				fprintf(stderr, "CTDecryptResponseCallback::Server requested renegotiate!\n");
				//scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
				scRet = CTSSLHandshake(overlappedResponse->conn->socket, overlappedResponse->conn->sslContext, NULL);
				if(scRet != SEC_E_OK) return scRet;

				if(overlappedResponse->conn->sslContext->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
				{
					fprintf(stderr, "\nCTDecryptResponseCallback::Unhandled Extra Data\n");
					//MoveMemory(overlappedConn->conn->response_buf, overlappedConn->conn->sslContextRef->ExtraData.pvBuffer, overlappedConn->conn->sslContextRef->ExtraData.cbBuffer);
					//cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
				}
			}
			*/
			//}
		}

	}

	assert(1 == 0);
	fprintf(stderr, "\nCTDecryptResponseCallback::End\n");
	ExitThread(0);
	return 0;
}


static unsigned long __stdcall CT_Dequeue_Encrypt_Send(LPVOID lpParameter)
{
	int i;
	ULONG_PTR CompletionKey;	//when to use completion key?
	uint64_t queryToken = 0;
	OVERLAPPED_ENTRY ovl_entry[CT_MAX_INFLIGHT_ENCRYPT_PACKETS];
	ULONG nPaquetsDequeued;

	CTCursor* cursor = NULL;
	CTOverlappedResponse* overlappedRequest = NULL;
	CTOverlappedResponse* overlappedResponse = NULL;

	unsigned long NumBytesSent = 0;
	unsigned long recvMsgLength = 0;

	CTSSLStatus scRet = 0;
	CTClientError ctError = CTSuccess;
	CTError err = { 0,0,NULL };

	//we passed the completion port handle as the lp parameter
	HANDLE hCompletionPort = (HANDLE)lpParameter;

	unsigned long cBufferSize = ct_system_allocation_granularity();

	fprintf(stderr, "CTConnection::EncryptQueryCallback Begin\n");

	//TO DO:  Optimize number of packets dequeued at once
	while (GetQueuedCompletionStatusEx(hCompletionPort, ovl_entry, CT_MAX_INFLIGHT_ENCRYPT_PACKETS, &nPaquetsDequeued, INFINITE, TRUE))
	{
		//fprintf(stderr, "CTConnection::EncryptQueryCallback::nPaquetsDequeued = %d\n", (int)nPaquetsDequeued);
		for (i = 0; i < nPaquetsDequeued; i++)
		{
			CompletionKey = (ULONG_PTR)ovl_entry[i].lpCompletionKey;
			NumBytesSent = ovl_entry[i].dwNumberOfBytesTransferred;
			overlappedRequest = (CTOverlappedResponse*)ovl_entry[i].lpOverlapped;

			int schedule_stage = CT_OVERLAPPED_SCHEDULE;
			if (!overlappedRequest)
			{
				fprintf(stderr, "CTConnection::EncryptQueryCallback::GetQueuedCompletionStatus continue\n");
				continue;
			}

			if (NumBytesSent == 0)
			{
				fprintf(stderr, "CTConnection::EncryptQueryCallback::Server Disconnected!!!\n");
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
				//fprintf(stderr, "CTConnection::EncryptQueryCallback::NumBytesSent = %d\n", (int)NumBytesSent);
				//fprintf(stderr, "CTConnection::EncryptQueryCallback::msg = %s\n", overlappedRequest->buf + sizeof(ReqlQueryMessageHeader));

				//Use the ReQLC API to synchronously
				//a)	encrypt the query message in the query buffer
				//b)	send the encrypted query data over the socket
				if (overlappedRequest->stage != CT_OVERLAPPED_SEND)
					scRet = CTSSLWrite(cursor->conn->socket, cursor->conn->sslContext, overlappedRequest->buf, &NumBytesSent);
				else
				{
					//TO DO:  create a send callback cursor attachment to remove post send TLS handshake logic here
					//assert(1 == 0);
					int cbData = send(cursor->conn->socket, (char*)overlappedRequest->buf, overlappedRequest->len, 0);

					if (cbData == SOCKET_ERROR || cbData == 0)
					{
						fprintf(stderr, "**** CTConnection::EncryptQueryCallback::CT_OVERLAPPED_SCHEDULE_SEND Error %d sending data to server (%d)\n", WSAGetLastError(), cbData);
					}

					//we haven't populated the cursor's overlappedResponse for recv'ing yet,
					//so use it here to pass the overlappedRequest->buf to the queryCallback (for instance, bc SCHANNEL needs to free handshake memory)
					cursor->overlappedResponse.buf = overlappedRequest->buf;
					err.id = cbData;
					if (cursor->queryCallback)
						cursor->queryCallback(&err, cursor);

					fprintf(stderr, "%d bytes of handshake data sent\n", cbData);
					cursor->overlappedResponse.buf = NULL;
					overlappedRequest->buf = NULL; //should this stay here or in the queryCallback?
					schedule_stage = CT_OVERLAPPED_SCHEDULE_RECV;

				}



#ifdef CTRANSPORT_USE_MBED_TLS
				//Issuing a zero buffer read will tell iocp to let us know when there is data available for reading from the socket
				//so we can issue our own blocking read
				recvMsgLength = 0;
#else
				//However, the zero buffer read doesn't seem to scale very well if Schannel is encrypting/decrypting for us 
				recvMsgLength = cBufferSize;
#endif
				queryToken = cursor->queryToken;
				//fprintf(stderr, "CTConnection::EncryptQueryCallback::queryTOken = %llu\n", queryToken);

				//Issue a blocking read for debugging purposes
				//CTRecv(overlappedQuery->conn, (void*)&(overlappedQuery->conn->response_buffers[ (queryToken%REQL_MAX_INFLIGHT_QUERIES) * REQL_RESPONSE_BUFF_SIZE]), &recvMsgLength);

				//put the overlapped response in the connections rxCursorQueue
				//cursor->conn->rxCursorQueue[cursor->conn->rxCursorQueueIndex] = cursor;
				//cursor->conn->rxCursorQueueIndex = (cursor->conn->rxCursorQueueIndex + 1) % cursor->conn->rxCursorQueueSize;
				//g_cursorQueueCount++;
				overlappedResponse = &(cursor->overlappedResponse);
				overlappedResponse->cursor = (void*)cursor;
				overlappedResponse->len = cBufferSize;
				overlappedResponse->stage = schedule_stage;// CT_OVERLAPPED_SCHEDULE;


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

							fprintf(stderr, "CTConnection::EncryptQueryCallback::CTAsyncRecv returned immediate with %lu bytes\n", recvMsgLength);
						}
						else //failure
						{
							fprintf(stderr, "CTConnection::EncryptQueryCallback::CTAsyncRecv failed with CTDriverError = %d\n", ctError);
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
						fprintf(stderr, "CTEncryptQueryCallback::PostThreadMessage failed with error: %d", GetLastError());
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
					fprintf(stderr, "CTEncryptQueryCallback::Server requested renegotiate!\n");
					//scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
					scRet = CTSSLHandshake(overlappedQuery->conn->socket, overlappedQuery->conn->sslContext, NULL);
					if(scRet != SEC_E_OK) return scRet;

					if(overlappedQuery->conn->sslContext->ExtraData.pvBuffer) // Move any "extra" data to the input buffer.
					{
						fprintf(stderr, "\nCTEncryptQueryCallback::Unhandled Extra Data\n");
						//MoveMemory(overlappedConn->conn->response_buf, overlappedConn->conn->sslContextRef->ExtraData.pvBuffer, overlappedConn->conn->sslContextRef->ExtraData.cbBuffer);
						//cbIoBuffer = sslContextRef->ExtraData.cbBuffer;
					}
				}
				*/
			}
		}

	}

	fprintf(stderr, "\nCTConnection::EncryptQueryCallback::End\n");
	ExitThread(0);
	return 0;

}
