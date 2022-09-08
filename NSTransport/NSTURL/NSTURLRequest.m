//
//  NSURLRequest.m
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

#include "../NSTURL.h"

@interface NSTURLRequest()
{
    URLRequestType            _command;
    char *                    _requestPath;
    NSMutableDictionary*      _headers;
    char *                    _body;
    unsigned long             _contentLength;

    //CTOverlappedResponse    _overlappedResponse;

    const char *              _filepath;
}

@end

@implementation NSTURLRequest

-(id)initWithCommand:(URLRequestType)command requestPath:(const char*)requestPath headers:(NSDictionary*)headers body:(char*)body contentLength:(unsigned long)contentLength responsePath:(const char* _Nullable)responsePath
{
    
    self = [super init];
    if( self )
    {
        _command = command;
        _requestPath = (char*)requestPath;

        //Is this a deep copy?
        if(headers) _headers = [NSMutableDictionary dictionaryWithDictionary:headers];
        else        _headers = [NSMutableDictionary new];
        
        _body = body;
        _contentLength = contentLength;

        if (responsePath) _filepath = responsePath;
        else              _filepath = ct_file_name((char*)requestPath);
    }
    return self;
}

+(NSTURLRequest*)newRequest:(URLRequestType)command requestPath:(const char*)requestPath headers:(NSDictionary* _Nullable)headers body:(char* _Nullable)body contentLength:(unsigned long)contentLength
{
    return [[NSTURLRequest alloc] initWithCommand:command requestPath:requestPath headers:headers body:body contentLength:contentLength responsePath:NULL];
}

+(NSTURLRequest*)newRequest:(URLRequestType)command requestPath:(const char*)requestPath headers:(NSDictionary* _Nullable)headers body:(char* _Nullable)body contentLength:(unsigned long)contentLength responsePath:(const char*)responsePath
{
    return [[NSTURLRequest alloc] initWithCommand:command requestPath:requestPath headers:headers body:body contentLength:contentLength responsePath:responsePath];
}

-(void)setValue:(NSString*)value forHTTPHeaderField:(NSString*)field
{
    [_headers setObject:value forKey:field];
}
-(void)setContentValue:(char*)value Length:(unsigned long)contentLength
{
    _body = value;
    _contentLength = contentLength;
}


-(uint64_t)sendOnConnection:(NSTConnection*)conn withCallback:(NSTRequestClosure)callback
{
    uint64_t expectedQueryToken = conn.connection->requestCount;
    
    //create a CXURL cursor (for the response, but also it holds our request buffer)
    //std::shared_ptr<CXURLCursor> cxURLCursor = conn->createRequestCursor(expectedQueryToken);
    //std::shared_ptr<CXURLCursor> cxURLCursor( new CXURLCursor(conn, _filepath) );
    //conn->addRequestCursorForKey(std::static_pointer_cast<CXCursor>(cxURLCursor), expectedQueryToken);
    //conn->addRequestCallbackForKey(callback, expectedQueryToken);
    //return SendRequestWithCursorOnQueue(cxURLCursor, expectedQueryToken);

    NSTURLCursor * nstURLCursor = [NSTURLCursor newCursor:conn filePath:_filepath];
    [conn addRequestCursor:nstURLCursor forKey:expectedQueryToken];
    [conn addRequestCallback:callback forKey:expectedQueryToken];
    return [self SendRequestWithCursorOnQueue:nstURLCursor ForQueryToken:expectedQueryToken];
    
    
}


-(uint64_t)SendRequestWithCursorOnQueue:(NSTURLCursor*)nstCursor ForQueryToken:(uint64_t)requestToken//, void * options)//, ReqlQueryClosure callback)
{

    //a string/stream for serializing the json query to char*
    unsigned long requestLength;
    //int32_t requestLength;

    
    //get the buffer for sending with async iocp

    //unsigned long cBufferSize = ct_system_allocation_granularity(); //+ sizeof(CTOverlappedResponse);
    char * requestBuffer = nstCursor.cursor->requestBuffer;//&(cxCursor->_cursor.conn->request_buffers[(requestToken % CX_MAX_INFLIGHT_QUERIES) * cBufferSize ]);
    char * baseBuffer = requestBuffer;

    //serialize the http headers and body to a string
    if( _command == URL_GET)
    {
        memcpy( requestBuffer, "GET ", 4);
        requestBuffer+=4;

    }
    else if (_command == URL_POST)
    {
        memcpy(requestBuffer, "POST ", 5);
        requestBuffer += 5;
    }
    else if (_command == URL_CONNECT)
    {
        memcpy(requestBuffer, "CONNECT ", 8);
        requestBuffer += 8;
    }
    else if (_command == URL_OPTIONS)
    {
        memcpy(requestBuffer, "OPTIONS ", 8);
        requestBuffer += 8;
    }

    if( _requestPath )
    {
        memcpy( requestBuffer, _requestPath, strlen( _requestPath ));
        requestBuffer+= strlen(_requestPath);
    }else
    {
        memcpy( requestBuffer, "/", 1);
        requestBuffer+=1;
    }


    memcpy( requestBuffer, " HTTP/1.1\r\n", strlen( " HTTP/1.1\r\n" ));
    requestBuffer += strlen( " HTTP/1.1\r\n" );
    //Add Host and User Agent Headers
    memcpy( requestBuffer, "Host: ", strlen("Host: "));
    requestBuffer+=strlen("Host: ");

    /*
    std::map<char*, char*>::iterator hostPos = _headers.find("Host");
    if (hostPos == _headers.end()) {
        memcpy(requestBuffer, cxCursor->_cursor->conn->target->url.host, strlen(cxCursor->_cursor->conn->target->url.host));
        requestBuffer += strlen(cxCursor->_cursor->conn->target->url.host);
    }
    else {
        memcpy(requestBuffer, hostPos->second, strlen(hostPos->second));
        requestBuffer += strlen(hostPos->second);
    }
    */
    NSString * hostVal = [_headers objectForKey:@"Host"];
    if( hostVal )
    {
        memcpy(requestBuffer, hostVal.UTF8String, strlen(hostVal.UTF8String));
        requestBuffer += strlen(hostVal.UTF8String);
    }
    else
    {
        memcpy(requestBuffer, nstCursor.cursor->conn->target->url.host, strlen(nstCursor.cursor->conn->target->url.host));
        requestBuffer += strlen(nstCursor.cursor->conn->target->url.host);
    }
    
    memcpy( requestBuffer, "\r\nUser-Agent: CXTransport\r\n", strlen("\r\nUser-Agent: CXTransport\r\n"));
    requestBuffer += strlen("\r\nUser-Agent: CXTransport\r\n");

    //add user headers
    NSLog(@"NSTURLRequest::SendRequestWithTokenOnQueue Parsing HTTP Headers...\n");
    
    for(NSString* fieldKey in _headers.allKeys)
    {
        NSString *fieldVal = [_headers objectForKey:fieldKey];
        NSLog(@"%@: %@\n", fieldKey, fieldVal);
        
        if( fieldVal == hostVal ) continue;
        
        memcpy( requestBuffer, fieldKey.UTF8String, strlen(fieldKey.UTF8String));
        requestBuffer+=strlen(fieldKey.UTF8String);
        memcpy( requestBuffer, ": ", strlen(": "));
        requestBuffer += strlen(": ");
        memcpy( requestBuffer, fieldVal.UTF8String, strlen(fieldVal.UTF8String));
        requestBuffer+= strlen(fieldVal.UTF8String);
        memcpy( requestBuffer, "\r\n", strlen("\r\n"));
        requestBuffer+= strlen("\r\n");
    }

    if (_body) //set content length header
    {
        memcpy(requestBuffer, "Content-Length: ", strlen("Content-Length: "));
        requestBuffer += strlen("Content-Length: ");

        char contentLengthStr[6];
        //_itoa((int)_contentLength, contentLengthStr, 10);
        snprintf(contentLengthStr, 6, "%d", (int)_contentLength);

        memcpy(requestBuffer, contentLengthStr, strlen(contentLengthStr));
        requestBuffer += strlen(contentLengthStr);

        memcpy(requestBuffer, "\r\n", strlen("\r\n"));
        requestBuffer += strlen("\r\n");

    }

    //now add the final return carriage to the HTTP header
    memcpy( requestBuffer, "\r\n\0", strlen("\r\n\0"));
    requestBuffer+= strlen("\r\n\0");

    if (_body) //now add the body
    {
        memcpy(requestBuffer, _body, _contentLength);
        requestBuffer += _contentLength;
    }

    //calulcate the request length
    requestLength = requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

    //print the request for debuggin
    fprintf(stderr, "CXURLSendRequestWithTokenOnQueue::request (%d) = %.*s\n", (int)requestLength, (int)requestLength, baseBuffer);

    //Send the HTTP(S) request
    //ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
    //CXURLSendWithQueue(conn, (void*)requestBuffer, &requestLength);
    //CTOverlappedResponse * overlappedResponse = &_overlappedResponse;
    //return CTSendOnQueue2(cxCursor->_cursor.conn, (char **)&baseBuffer, requestLength, requestToken, &(cxCursor->_cursor.overlappedResponse));//, &overlappedResponse);
    
    //CXCursor internal CTCursor conn was already set when CXCursor was created with constructor
    //cxCursor->_cursor->conn = &_httpConn;
    nstCursor.cursor->queryToken = requestToken;
    return CTCursorSendOnQueue((nstCursor.cursor), (char**)&baseBuffer, requestLength);
}


@end
