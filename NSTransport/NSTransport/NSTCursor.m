//
//  NSReQLCursor.m
//  MastryAdmin
//
//  Created by Joe Moulton on 6/7/19.
//  Copyright © 2019 VRTVentures LLC. All rights reserved.
//

#define class extern_class
#define delete extern_delete
#include "../NSTransport.h"
#undef delete
#undef class

@interface NSTCursor()
{
    //__weak id               _delegate;
    __weak NSTConnection*   _nstConnection;
    CTCursor*               _cursor;
    NSTRequestClosure       _callback;
}

@end

@implementation NSTCursor

//@synthesize nstConnection   = _cursor;
//@synthesize delegate        = _delegate;
@synthesize cursor          = _cursor;
@synthesize callback        = _callback;

-(id)initWithConnection:(NSTConnection*)conn
{
    if( (self = [super init]) )
    {
        //_delegate = self;
        _nstConnection = conn;
        //This will set CTCursor header lenght to 0 for us
        _cursor = CTGetNextPoolCursor();
        memset((void*)_cursor, 0, sizeof(CTCursor));
        _cursor->conn = _nstConnection.connection;
        //_cursor.headerLength = 0;
        //TO DO:  create a random string name for the file based on the request
        [self createResponseBuffers:"map.jpg\0"];
    }
    return self;
}

-(id)initWithConnection:(NSTConnection*)conn andFilePath:(const char*)filepath
{
    if( (self = [super init]) )
    {
        //_delegate = self;
        _nstConnection = conn;
        //This will set CTCursor header lenght to 0 for us
        _cursor = CTGetNextPoolCursor();
        assert(_cursor);
        memset((void*)_cursor, 0, sizeof(CTCursor));
        _cursor->conn = _nstConnection.connection;
        //_cursor.headerLength = 0;
        [self createResponseBuffers:filepath];
    }
    return self;
}

/*
+(NSTCursor*)newCursor:(NSTConnection*)conn
{
    return [[NSTCursor alloc] initWithConnection:conn];
}

+(NSTCursor*)newCursor:(NSTConnection*)conn filePath:(const char*)filepath
{
    return [[NSTCursor alloc] initWithConnection:conn andFilePath:filepath];
}
*/

-(NSTConnection*)connection
{
    return _nstConnection;
}

-(void)createResponseBuffers:(const char *)filepath
{
    if( !filepath )
        filepath = "map.jpg\0";
    _cursor->file.path = (char*)filepath;
    
    //file path to open
    CTCursorCreateMapFileW(_cursor, _cursor->file.path, ct_system_allocation_granularity() * 256UL);

    memset(_cursor->file.buffer, 0, ct_system_allocation_granularity() * 256UL);

    //_cursor->file.buffer = _cursor->requestBuffer;

#ifdef _DEBUG
    assert(_cursor->file.buffer);
#endif
}



-(char*)ProcessResponseHeader:(char*)buffer Length:(unsigned long)bufferLength Cursor:(CTCursor*)cursor
{
    fprintf(stderr, "NSTCursor::ProcessResponseHeader should be overidden by superclass !!!\n\n");
    assert(1==0);
    return NULL;
    
    //assert(_delegate);
    //return [_delegate ProcessResponseHeader:buffer Length:bufferLength Cursor:cursor];
    //return NULL;
}


-(void)dealloc
{
    NSLog(@"NSTCursor::dealloc");// for token (%d)", (int)_cursor.header->token);
}
/*
-(ReqlResponseType)responseType
{
    return ReqlCursorGetResponseType(&_cursor);
}

-(uint64_t)token
{
    return (&_cursor)->header->token;
}

-(NSDictionary*)dict
{
    if( !_dict )
    {
        NSError* e = nil;
        _dict = [NSJSONSerialization JSONObjectWithData:_data options: NSJSONReadingMutableContainers error:&e];
        if( e )  //Failure to Parse JSON Response
        {
            NSLog(@"REQL JSON Response Parsing Error: %@", e.localizedDescription);
            printf("_cursor.buffer = %.*s", _cursor.length - sizeof(ReqlQueryMessageHeader), _cursor.buffer + sizeof(ReqlQueryMessageHeader));
        }
    }
    return _dict;
}

-(NSArray*)toArray
{
    //NSLog(@"Cursor.dict = \n\n%@", self.dict);
    return [self.dict objectForKey:@"r"];
}

-(void)continue:(NSDictionary*)options
{
    NSError *err = nil;
    NSString *jsonString = nil;
    NSData *jsonData = nil;
    void * queryData = nil;
    
    if( options ) jsonData = [NSJSONSerialization dataWithJSONObject:options options:0 error:&err];
    if( err ) { NSLog(@"NSReQLChangesQuery NSJSONSerialization Error:  %@", err.localizedDescription); return; }
    
    if( jsonData )
    {
        jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
        queryData = (void*)jsonString.UTF8String;
    }
    ReqlCursorContinue(&_cursor, queryData);
}
*/


@end
