# CoreTransport

CoreTransport is a no-compromise cross-platform pure C library (with wrapper APIs in various languages) for establishing and consuming from persistent TCP socket client connections secured with SSL/TLS.  CoreTransport aims to implement the following non-standard features for all supported platforms:

<table width="100%">
  <tr width ="100%">
    <td align="center" width="49%"><strong>Motivating Feature</strong></td>
    <td align="center" width="17%"><strong>Win32, Xbox</strong></td>
    <td align="center" width="17%"><strong>Darwin</strong></td>
    <td align="center" width="17%"><strong>FreeBSD</strong></td>
  </tr>
  <tr width ="100%">
    <td width="49%">Structured concurrency to promote simultaneous asynchronous non-blocking socket connections from a single thread</td>
    <td width="17%"><s>Coroutines,</s><br/>IO Completion</td>
    <td width="17%">Coroutines,<br/><s>IO Completion</s></td>
    <td width="17%">Coroutines,<br/>IO Completion</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Async non-blocking DNS resolve</td>
    <td width="17%">IPv4,&nbsp;&nbsp;IPv6</td>
    <td width="17%">IPv4,&nbsp;&nbsp;IPv6</td>
    <td width="17%">IPv4,&nbsp;&nbsp;IPv6</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Async non-blocking TLS negotiation via platform provided encryption with fallback to 3rd party embedded support</td>
    <td width="17%">SCHANNEL,&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<br/>WolfSSL</td>
    <td width="17%">SecureTransport,<br/>WolfSSL</td>
    <td width="17%"><s>Kernel TLS,</s>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<br/>WolfSSL</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Lock-Free, Wait-Free Tx/Rx scheduling & completion operation dequeuing with fewest userspace context switches</td>
    <td width="17%">IOCP,<br/>PeekMessage</td>
    <td width="17%">kqueue,<br/>Cursor pipe</td>
    <td width="17%">AIO,<br/>Cursor pipe</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Memory management that supports in-place processing and optional response caching</td>
    <td width="17%">Memory Pools,<br/>Mapped Files</td>
    <td width="17%">Memory Pools,<br/>Mapped Files</td>
    <td width="17%">Memory Pools,<br/>Mapped Files</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Closures to delegate response buffers back to caller</td>
    <td width="17%">Clang Blocks,<br/>C++ Lambdas</td>
    <td width="17%">Clang Blocks,<br/>C++ Lambdas</td>
    <td width="17%">Clang Blocks,<br/>C++ Lambdas</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Conditional chaining of requests/queries from the same and other connections</td>
    <td width="17%">Completion<br/>Closures</td>
    <td width="17%">Completion<br/>Closures</td>
    <td width="17%">Completion<br/>Closures</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Streaming downloads for consumption by an accelerated graphics or real-time hardware pipeline</td>
    <td width="17%">Progress<br/>Closures&nbsp;&nbsp;</td>
    <td width="17%">Progress<br/>Closures&nbsp;&nbsp;</td>
    <td width="17%">Progress<br/>Closures&nbsp;&nbsp;</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Tunneling via proxy connections</td>
    <td width="17%">HTTP,&nbsp;&nbsp;SOCKS5</td>
    <td width="17%">HTTP,&nbsp;&nbsp;SOCKS5</td>
    <td width="17%">HTTP,&nbsp;&nbsp;SOCKS5</td>
  </tr>
  <tr width ="100%">
    <td width="49%">Numa Siloing</td>
    <td width="17%">❌</td>
    <td width="17%">❌</td>
    <td width="17%">❌</td>
  </tr>	
</table>

<!---
| <div style="width:49%">Motivating Feature</div> | <div style="width:17%">Win32, Xbox</div> | <div style="width:17%">Darwin</div> | <div style="width:17%">FreeBSD</div> |
| --------------------------------------------------- | ----------------- | ----------------- | ----------------- |
| Structured concurrency to promote simultaneous asynchronous non-blocking socket connections from a single thread | IOCP | Coroutines | Coroutines |
| Lock-Free, Wait-Free Tx/Rx scheduling & completion operation dequeuing with fewest userspace context switches | IOCP,<br/>PeekMessage | kqueue,<br/>Cursor pipe | kqueue,<br/>Cursor pipe |
| Closures to delegate response buffers back to caller | Clang Blocks,<br/>C++ Lambdas | Clang Blocks,<br/>C++ Lambdas | Clang Blocks,<br/>C++ Lambdas |
| Memory management that supports in-place processing and optional response caching | Memory Pools, Mapped Files | Memory Pools, Mapped Files | Memory Pools, Mapped Files |
| Asynchronous TLS negotiation via platform provided encryption with fallback to 3rd party embedded support | SCHANNEL,&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<br/>WolfSSL | SecureTransport,<br/>MBEDTLS | In&nbsp;Progress&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; |
| Conditional chaining of requests/queries from the same and other connections | Completion<br/>Closures | Completion<br/>Closures | Completion<br/>Closures |
| Streaming downloads for consumption by an accelerated graphics pipeline or real-time hardware pipeline | Progress<br/>Closures | Progress<br/>Closures | Progress<br/>Closures |
| Numa Siloing | &cross; | &cross; | &cross; |
| Tunneling via proxy connections | HTTP, SOCKS5 | To Do | In Progress |	
--->

*CoreTransport is the modular Network Transport Layer that operates in parallel with 3rdGen's Accelerated Graphics Layer, Core Render.  Together, CoreTransport and CoreRender's C libraries embody the foundational layer of 3rdGen's proprietary simulation engine and cross-platform application framework, Cobalt Rhenium.* 

## Usage

The general pattern for establishing and consuming from client connections using CTransport and its wrapper interface libraries is the same:

1.  Create a connection + cursor pool 
2.  Create queues for async + non-blocking socket operations (CTKernelQueue)
3.  Create a pool of threads to dequeue socket operations (CTThread)
4.  Define your target (CTTarget)
5.  Create a socket connection + perform SSL Handshake (CTConnection)
6.  Make a network request and asynchronously receive the response (CTCursor)
7.  Clean up the connection

## CTransport API
```
   #include <CoreTransport/CTransport.h>
```

####  Create Connection + Cursor Pool
```
   CTCreateConnectionPool(&(HAPPYEYEBALLS_CONNECTION_POOL[0]), HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS);
   CTCreateCursorPool(&(_httpCursor[0]), CT_MAX_INFLIGHT_CURSORS);
```

####  Create Socket Queues
```	
   CTKernelQueue cq = CTKernelQueueCreate();
   CTKernelQueue tq = CTKernelQueueCreate();
   CTKernelQueue rq = CTKernelQueueCreate();
```

####  Create Thread Pool 
```
   CTThread cxThread = CTThreadCreate(&cq, CT_Dequeue_Connect);
   CTThread txThread = CTThreadCreate(&tq, CT_Dequeue_Recv_Decrypt);
   CTThread rxThread = CTThreadCreate(&rq, CT_Dequeue_Encrypt_Send);
```

####  Define your target
```
   CTTarget httpTarget =   {0};
   httpTarget.host =       "learnopengl.com";
   httpTarget.port =       443;
   httpTarget.ssl.ca =     NULL;          //CTransport will look for the certificate in the platform CA trust store
   httpTarget.ssl.method = CTSSL_TLS_1_2; //Optionally specify if TLS encryption is desired and what version
   httpTarget.cq =  	   cq; //If no cx queue is specified [DNS + Connect + TLS Handshake] will occur on the current thread
   httpTarget.tq =  	   tq; //If no tx queue is specified CTransport internal queues will be assigned to the connection for send
   httpTarget.rq =  	   rq; //If no rx queue is specified CTransport internal queues will be assigned to the connection for recv 
```

####  Connect with Closure/Callback
```
   CTConnection _httpConn;
   CTConnectionClosure _httpConnectionClosure = ^int(CTError * err, CTConnection * conn)   
   {
	//Parse error status
	assert(err->id == CTSuccess);

	//Copy CTConnection struct memory from coroutine stack to application memory 
	//(because connection will go out of scope when this funciton exits)
	_httpConn = *conn;
	
	return err->id;
   };	
   CTransport.connect(&httpTarget, _httpConnectionClosure);
```

####  Make a network Request using a Cursor (CTCursor)
```
   //the purpose of this callback is to return the end of the header to CTransport so it can continue processing
   char * httpHeaderLengthCallback(struct CTCursor * cursor, char * buffer, unsigned long length )
   {
	char* endOfHeader = strstr(buffer, "\r\n\r\n");
	if (!endOfHeader) 
	{
	   pCursor->headerLength = 0;
	   pCursor->contentLength = 0;
	   return NULL;
	}
	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//The client *must* set the cursor's contentLength property to aid CoreTransport 
	//in knowing when to stop reading from the socket (CTransport layer is protocol agnostic) 
	cursor->contentLength = ...

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
   }

   CTCursorCompletionClosure httpResponseClosure = ^void(CTError * err, CTCursor* cursor)
   {
	printf("httpResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	printf("httpResponseCallback body:    \n\n%.*s\n\n", cursor->file.size, cursor->file.buffer);
	
	//Close the cursor's file buffer mapping if one was opened
	CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
	CTCursorCloseFile(cursor);
   };

   //Define a cursor which handles the buffers for sending a request and receiving an associated response
   //Each cursor request gets sent with a unique query token id
   CTCursor _httpCursor;
   
   //Optionally use a memory mapped file for cursor response caching if desired
   CTCursorCreateResponseBuffers(&_httpCursor, filepath);

   //define callbacks for client to process header and receive response
   _httpCursor.headerLengthCallback = httpHeaderLengthCallback;
   _httpCursor.responseCallback = httpResponseClosure;

   //define an HTTP GET request
   char * queryBuffer;
   unsigned long queryStrLength;
   char GET_REQUEST[1024] = "GET /img/textures/wood.png HTTP/1.1\r\nHost: learnopengl.com\r\nUser-Agent: CoreTransport\r\nAccept: 	*/*\r\n\r\n\0";
   queryStrLength = strlen(GET_REQUEST);
   queryBuffer = cursor->requestBuffer;//cursor->file.buffer;
   memcpy(queryBuffer, GET_REQUEST, queryStrLength);
   queryBuffer[queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminator
   
   //put the connection on the cursor before sending it off to CT land
   _httpCursor.conn = &_httpConn; 

   //send the cursor's request buffer using CTransport API
   CTCursorSendRequestOnQueue( &_httpCursor, _httpConn.queryCount++);	
```
####  Clean up the connection
```
   CTCloseConnection(&_httpConn);
```

##  CXTransport API
```
   #include <CoreTransport/CXURL.h>
   using namespace CoreTransport;
```

####  Create Connection + Cursor Pool (same as CTransport)
```
   CTCreateConnectionPool(&(HAPPYEYEBALLS_CONNECTION_POOL[0]), HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS);
   CTCreateCursorPool(&(_httpCursor[0]), CT_MAX_INFLIGHT_CURSORS);
```

####  Create Socket Queues (same as CTransport)
```	
   CTKernelQueue cq = CTKernelQueueCreate();
   CTKernelQueue tq = CTKernelQueueCreate();
   CTKernelQueue rq = CTKernelQueueCreate();
```

####  Create Thread Pool 
```
   CTThread cxThread = CTThreadCreate(&cq, CX_Dequeue_Connect);
   CTThread txThread = CTThreadCreate(&tq, CX_Dequeue_Recv_Decrypt);
   CTThread rxThread = CTThreadCreate(&rq, CX_Dequeue_Encrypt_Send);
```

####  Define your target (same as CTransport)
```
   CTTarget httpTarget =   {0};
   httpTarget.host =       "learnopengl.com";
   httpTarget.port =       443;
   httpTarget.ssl.ca =     NULL;          //CTransport will look for the certificate in the platform CA trust store
   httpTarget.ssl.method = CTSSL_TLS_1_2; //Optionally specify if TLS encryption is desired and what version
   httpTarget.cq =  	   cq; //If no cx queue is specified [DNS + Connect + TLS Handshake] will occur on the current thread
   httpTarget.tq =  	   tq; //If no tx queue is specified CTransport internal queues will be assigned to the connection for send
   httpTarget.rq =  	   rq; //If no rx queue is specified CTransport internal queues will be assigned to the connection for recv
```

####  Connect with Closure
```
   CXConnection * _httpCXConn;
   //Define Lambda for the connection callback
   auto _cxURLconnectionClosure = [&](CTError* err, CXConnection* conn)
   {
	if (err->id == CTSuccess && conn) { _httpCXConn = conn; }
	else {} //process errors	
	return err->id;
   };

   //Use CXTransport CXReQL C++ API to connect to our RethinkDB service
   CXURL.connect(&httpTarget, _cxURLconnectionClosure);
```

####  Make a network Request using a Cursor (CXCursor)
```
   //Create a CXTransport API C++ CXURLRequest
   std::shared_ptr<CXURLRequest> getRequest = CXURL.GET("/img/textures/wood.png");
	
   //Add some HTTP headers to the CXURLRequest
   getRequest->setValueForHTTPHeaderField("Accept:", "*/*");
	
   //Define the response callback to return response buffers using CXTransport CXCursor object returned via a Lambda Closure
   auto requestCallback = [&] (CTError * error, std::shared_ptr<CXCursor> &cxCursor)
   { 
	printf("Lambda callback response header:  %.*s\n", cxCursor->_cursor.headerLength, cxCursor->_cursor.requestBuffer);  
   };

   //Pass the CXConnection and the lambda to populate the request buffer and asynchronously send it on the CXConnection
   //The lambda will be executed so the code calling the request can interact with the asynchronous response buffers
   getRequest->send(_httpCXConn, requestCallback);
```

####  Clean up the connection
```
   delete _httpCXConn;
```

