//
//  NSRQLConnection.m
//  ReØMQL
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#include "../NSTransport.h"

/*
#import "NSReQLConnection.h"
#import "NSReQLCursor.h"
#import "NSReQLQuery.h"
#include <sys/mman.h>
#import "NSReQLSession.h"
*/

#define NSREQL_CONN_BUFFER_SIZE 4096*4

NSErrorDomain       const NSReQLErrorDomain        = @"kNSReQLErrorDomain";
NSErrorUserInfoKey  const NSReQLErrorUserInfoKey   = @"kNSReQLErrorUserInfoKey";

//FOUNDATION_EXPORT NSErrorDomain const NSReQLErrorDomain;             // NSString, a complete sentence (or more) describing ideally both what failed and why it failed.
//FOUNDATION_EXPORT NSErrorUserInfoKey const NSReQLErrorUserInfoKey;             // NSString, a complete sentence (or more) describing ideally both what failed and why it failed.


@interface NSPair()
{
    __unsafe_unretained id _first;
    __unsafe_unretained id _second;
}

@end

@implementation NSPair
@synthesize first = _first;
@synthesize second = _second;
-(id)initWithFirst:(id)first andSecond:(id)second
{
    self = [super init];
    if(self)
    {
        self.first = first;
        self.second = second;
    }
    return self;
}

+(NSPair*)first:(id)first second:(id)second
{
    return [[NSPair alloc] initWithFirst:first andSecond:second];
}

@end

/*
@interface NSReQLService()
{
    ReqlService * _service;
    
}

@end

@implementation NSReQLService

-(id)initWithService:(ReqlService*)service
{
    if( service && (self = [super init]) )
    {
        _service = service;
    }
    return self;
}

+(NSReQLService*)newService:(ReqlService*)service
{
    return [[NSReQLService alloc] initWithService:service];
}

@end
*/

@interface NSTConnection()
{
    CTConnection            _conn;
    CTKernelQueueType       _cxQueue, _rxQueue, _txQueue;
    //dispatch_queue_t      _queue;
    //dispatch_source_t     _source;
    NSMutableDictionary*    _cursors;
    NSMutableDictionary*    _queries;
    //volatile BOOL _cancel;
}


@end


@implementation NSTConnection

@synthesize cursors = _cursors;
@synthesize queries = _queries;

/*
//Connection/Reconnection Notifications
+(NSString*)ConnectionUpdateNotification
{
    return [NSString stringWithUTF8String:kReqlConnectionNotification];
}

+(NSString*)ReconnectUpdateNotification
{
    return [NSString stringWithUTF8String:kReqlReconnectNotification];
}

//Login/Logout Notifications
+(NSString*)LoginUpdateNotification
{
    return [NSString stringWithUTF8String:kReqlLoginNotification];
}

+(NSString*)LogoutUpdateNotification
{
    return [NSString stringWithUTF8String:kReqlLogoutNotification];    
}
*/


-(CTConnection*)connection
{
    return &_conn;
}



//Private Copy constructor

-(id)initWithCTConnection:(CTConnection*)conn// andDispatchQueue:(dispatch_queue_t __nullable)queue
{
    //Only allow creation of NSReQLConnection if ReØMQL connection memory is valid
    if( conn && (self == [super init]) )
    {
        //copy the from CTConnection CTC memory to NSTConnection managed memory (because it will go out of scope)
        //followup note:  this is no longer the case with connection pool feature + the nonblocking path
        // but is still the case for the blocking connection path (though it doesn't have to be)
        memcpy(&_conn, conn, sizeof(CTConnection));

        //Reserve/allocate memory for the connection's query and response buffers
        //reserveConnectionMemory();

        /*
        //create a dispatch source / handler for the CTConnection's socket
        if (!rxQueue)
            _rxQueue = _conn.socketContext.rxQueue;
        else
            _rxQueue = rxQueue;
        
        if(!txQueue)
            _txQueue = _conn.socketContext.txQueue;
        else
            _txQueue = txQueue;
         */
        
        //IMPORTANT:  Point CTransport connection at it's CXTransport wrapper object for use by CXRoutines
        _conn.object_wrapper = (__bridge void *)(self);

        _cursors = [NSMutableDictionary new];
        _queries = [NSMutableDictionary new];
        //startConnectionThreadQueues(_rxQueue, _txQueue);
    }
    return self;
}

-(void)dealloc
{
    NSLog(@"NSTConnection::dealloc\n");
    
    //[self.queries removeAllObjects];
    //self.queries = nil;
    
    _cursors = nil;
    _queries = nil;
    
    //TO DO: Close connection
}


//Public wrapper to private constructor
+(NSTConnection*)newConnection:(CTConnection*)conn// withQueue:(dispatch_queue_t __nullable)queue
{
    return [[NSTConnection alloc] initWithCTConnection:conn];// andDispatchQueue:queue];
}

-(void)addRequestCallback:(NSTRequestClosure)callback  forKey:(uint64_t)requestToken
{
    [_queries setObject:callback forKey:[NSNumber numberWithUnsignedLongLong:requestToken]];
}

-(void)removeRequestCallbackForKey:(uint64_t)requestToken
{
    [_queries removeObjectForKey:[NSNumber numberWithUnsignedLongLong:requestToken]];
}

-(NSTRequestClosure)getRequestCallbackForKey:(uint64_t)requestToken
{
    return [_queries objectForKey:[NSNumber numberWithUnsignedLongLong:requestToken]];
}

-(void)addRequestCursor:(NSTCursor*)cursor forKey:(uint64_t)requestToken
{
    //assert(_cursors.find(requestToken) == _cursors.end());
    [_cursors setObject:cursor forKey:[NSNumber numberWithUnsignedLongLong:requestToken]];
}

-(void)removeRequestCursorForKey:(uint64_t)requestToken
{
    [_cursors removeObjectForKey:[NSNumber numberWithUnsignedLongLong:requestToken]];
}

-(NSTCursor*)getRequestCursorForKey:(uint64_t)requestToken
{
    return _cursors.count > 0 ? [_cursors objectForKey:[NSNumber numberWithUnsignedLongLong:requestToken]] : NULL;
}

-(void)distributeResponseWithCursorForToken:(uint64_t)requestToken
{
    //std::shared_ptr<CXCursor> cxCursor = getRequestCursorForKey(requestToken);
    //auto callback = getRequestCallbackForKey(requestToken);
    NSTCursor * nstCursor = [self getRequestCursorForKey:requestToken];
    NSTRequestClosure callback = [self getRequestCallbackForKey:requestToken];
    
    [self removeRequestCallbackForKey:requestToken];
    [self removeRequestCursorForKey:requestToken];

    if( callback )
    {
        nstCursor.callback = callback;
        callback( NULL, nstCursor);
        //removeRequestCallbackForKey(requestToken);  this used to be here, but if we want to be able to delete the connection from the callback...

        fprintf(stderr, "NSTConnection::Removed Cursor (%llu)\n", requestToken);

    }
    else
    {
        fprintf(stderr, "NSTConnection::distributeResponseWithCursorForToken::No Callback!");
        assert(1 == 0);
        //printQueries();
    }

    //We are done with the cursor
    //Do the manual cleanup we need to do
    //and let the rest get cleaned up when the sharedptr to the CXCursor goes out of scope
    //removeRequestCursorForKey(requestToken); //and this used to be here,  but if we want to be able to delete the connection from the callback...

    
}


@end
