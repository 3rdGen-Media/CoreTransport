//
//  NSURLCursor.m
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

#include "../NSTURL.h"

@interface NSTURLCursor()
{
}

@end

@implementation NSTURLCursor : NSTCursor

/*
-(id)initWithConnection:(NSTConnection*)conn
{
    if( (self = [super initWithConnection:conn]) )
    {
        //_delegate = self;
    }
    return self;
}

-(id)initWithConnection:(NSTConnection*)conn andFilePath:(const char*)filepath
{
    if( (self = [super initWithConnection:conn andFilePath:filepath]) )
    {
        //_delegate = self;
    }
    return self;
}
*/


+(NSTURLCursor*)newCursor:(NSTConnection*)conn
{
    return [[NSTURLCursor alloc] initWithConnection:conn];
}

+(NSTURLCursor*)newCursor:(NSTConnection*)conn filePath:(const char*)filepath
{
    return [[NSTURLCursor alloc] initWithConnection:conn andFilePath:filepath];
}

-(char*)ProcessResponseHeader:(char*)buffer Length:(unsigned long)bufferLength Cursor:(CTCursor*)cursor
{
    assert(self.cursor == cursor);
    char* endOfHeader = strstr(buffer, "\r\n\r\n");

    if (!endOfHeader)
    {
        cursor->headerLength = 0;
        cursor->contentLength = 0;
        return NULL;
    }

    //set ptr to end of http header
    endOfHeader += sizeof("\r\n\r\n") - 1;

    //calculate size of header
    //_cursor->headerLength = endOfHeader - buffer; //why am I doing this?

    //The client *must* set the cursor's contentLength property
    //to aid CoreTransport in knowing when to stop reading from the socket
    //_cursor->contentLength = 0;// 1641845;
    char* contentLengthPtr = strstr(buffer, "Content-Length: ");

    if (contentLengthPtr)
    {
        char* contentLengthStart = contentLengthPtr + 16;
        char* contentLengthEnd = strstr(contentLengthStart, "\r\n");
        assert(contentLengthEnd);

        char cLength[32] = "\0";
        memcpy(cLength, contentLengthStart, contentLengthEnd - contentLengthStart);
        cLength[contentLengthEnd - contentLengthStart] = '\0';

        cursor->contentLength = (unsigned long)atoi((const char*)cLength);
    }
    else assert(1 == 0);
    
    return endOfHeader;
}

@end
