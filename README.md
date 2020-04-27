# CoreTransport

CoreTransport is a no-compromise cross-platform pure C library (with wrapper APIs in various languages) for establishing and consuming from persistent TCP client connections secured with SSL/TLS.  CoreTransport aims to espouse the following non-standardized principles:

* Structured concurrency to allow for multiple concurrent asynchronous blocking/non-blocking socket connections from a single thread
* Dedicated Tx/Rx thread queues for each socket connection implemented as closest-to-kernel option for maximum concurrent     requests/responses.
* Memory management that supports in-place processing and response caching where possible
* Use closures to delegate response buffers back to the caller when possible
* Support conditional chaining of requests/queries from the same and other connections
* Support streaming downloads for consumption by an accelerated graphics pipeline

*CoreTransport is the modular network counterpart to the CoreRender framework.  Together, CoreTransport and CoreRender's modular C libraries embody the foundational layer of 3rdGen's proprietary render engine and cross-platform application framework, Cobalt Rhenium.* 

## Usage

The general process for establishing and consuming connections CTransport and its wrapper interface libraries is the same:

1.  Define your target (CTTarget)
2.  Create a socket connection + perform SSL Handshake (CTConnection)
3.  Make a network request and asynchrously receive the response (CTCursor)
4.  Clean up the connection

## CTransport API

####  Define your target
```
   CTTarget httpTarget = {0};
   httpTarget.host =     "learnopengl.com";
   httpTarget.port =     443;
   httpTarget.ssl.ca =   NULL;  //CTransport will look for the certificate in the platform CA trust store
```

####  Connect with Closure/Callback
```
   CTConnection _httpConnection;
   //This is not a closure -- just a pure c callback for when we don't have support for closures
   int _httpConnectionClosure(CTError *err, CTConnection * conn)
   {
   	//Parse error status
    	assert(err->id == CTSuccess);

	    //Copy CTConnection object memory from coroutine stack to application memory 
    	//(or the connection ptr will go out of scope when this function exits)
    	_httpConn = *conn;

	return err->id;
   }	
  
   CTransport.connect(&httpTarget, _httpConnectionClosure);
```

####  Make a network Request using a Cursor (CTCursor)
```
   //the purpose of this callback is to return the end of the header to CTransport so it can continue processing
   char * httpHeaderLengthCallback(struct CTCursor * cursor, char * buffer, unsigned long length )
   {
   	char * endOfHeader = strstr(buffer, "\r\n\r\n");
	//set ptr to end of http header
	endOfHeader += sizeof("\r\n\r\n") - 1;

	//The client can optionally set the cursor's contentLength property
	//to aid CoreTransport in knowing when to stop reading from the socket
	//cursor->contentLength = ...

	//The cursor headerLength is calculated as follows after this function returns
	//cursor->headerLength = endOfHeader - buffer;
	return endOfHeader;
   }

   void httpResponseCallback(CTError * err, CTCursor *cursor)
   {
	CTCursorMapFileR(cursor);
	printf("httpResponseCallback header:  \n\n%.*s\n\n", cursor->headerLength, cursor->requestBuffer);
	//printf("httpResponseCallback body:    \n\n%.*s\n\n", cursor->file.size, cursor->file.buffer);
	CTCursorCloseFile(cursor);
   }

   //Define a cursor which handles the buffers for sending a request and receiving an associated response
   //Each cursor request gets sent with a unique query token id
   CTCursor _httpCursor;
   CTCursorCreateResponseBuffers(&_httpCursor, filepath);

   //define callbacks for client to process header and receive response
   _httpCursor.headerLengthCallback = httpHeaderLengthCallback;
   _httpCursor.responseCallback = httpResponseCallback;

   //define an HTTP GET request
   char * queryBuffer;
   unsigned long queryStrLength;
   char GET_REQUEST[1024] = "GET /img/textures/wood.png HTTP/1.1\r\nHost: learnopengl.com\r\nUser-Agent: CoreTransport\r\nAccept: 	*/*\r\n\r\n\0";
   queryStrLength = strlen(GET_REQUEST);
   queryBuffer = cursor->requestBuffer;//cursor->file.buffer;
   memcpy(queryBuffer, GET_REQUEST, queryStrLength);
   queryBuffer[queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminator

   //send the cursor's request buffer using CTransport API
   CTCursorSendRequestOnQueue( &_httpCursor, _httpConn.queryCount++);	
```
####  Clean up the connection
```
   CTCloseConnection(&_httpConn);
```

##  CXTransport API

####  Define your target (same as CTransport)
```
   CTTarget httpTarget = {0};
   httpTarget.host =     "learnopengl.com";
   httpTarget.port =     443;
   httpTarget.ssl.ca =   NULL;  //CTransport will look for the certificate in the platform CA trust store
```

####  Connect with Closure
```
   CXConnection * _httpCXConn;
   //CXTransport implements closures as C++ Lambdas
   int _cxURLConnectionClosure(CTError *err, CXConnection * conn)
   {
	if( err->id == CTSuccess && conn ){ _httpCXConn = conn; }
	else {} //process errors
	return err->id;
   }

   //Use CXTransport CXURL C++ API to connect to our HTTPS target server
   CXURL.connect(&httpTarget, _cxURLConnectionClosure);
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
   delete _httpConnection;
```

