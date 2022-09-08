//
//  NSRQLSession.m
//  ReØMQL
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#include "../NSTURL.h"

@interface NSTURLSession()
{
    //An array to store pending/open NSReQLConnections
    NSMutableDictionary * _connections;         //maintain an internal connection cache as a hash
    NSMutableDictionary * _pendingConnections;  //maintain an internal connection cache as a hash
}

@property (nonatomic, retain) NSMutableDictionary * connections;
@property (nonatomic, retain) NSMutableDictionary * pendingConnections;

@end

@implementation NSTURLSession

@synthesize connections        = _connections;
@synthesize pendingConnections = _pendingConnections;

-(void)dealloc
{
    NSLog(@"NSTURLSession::dealloc");
    
    //TO DO: close all open connections
    
    //Destroy pendingConnectionsArray
    [self.pendingConnections removeAllObjects];
    self.pendingConnections = NULL;
    
    //Destroy connections array
    [self.connections removeAllObjects];
    self.connections = nil;
}

+(NSTURLSession *)sharedSession
{
    static NSTURLSession *sharedSession = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedSession = [[NSTURLSession alloc] init];
    });
    return sharedSession;
}

-(id)init
{
    self = [super init];
    if( self )
    {
        _pendingConnections = [NSMutableDictionary new];
        _connections = [NSMutableDictionary new];
    }
    return self;
}


-(NSMutableDictionary*)connections
{
    return _connections;
}

-(NSTConnection*)getConnectionForKey:(NSString*)connectionKey
{
    NSLog(@"NSTURLSession::getConnectionForKey(%@) = \n\n%@", connectionKey, self.connections);
    assert(connectionKey);
    if( self.connections && self.connections.count > 0)
    {
        return [self.connections objectForKey:connectionKey];
    }
    
    return nil;
    //return (self.connections && self.connections.count > 0) ? [self.connections objectForKey:connectionKey] : nil;
}

-(void)addPendingConnection:(NSPair*)pendingConnection withTargetKey:(CTTarget*)targetKey
{
    [_pendingConnections setObject:pendingConnection forKey:[NSValue valueWithPointer:targetKey]];
}

-(NSPair*)pendingConnectionForKey:(CTTarget*)targetKey
{
    return [_pendingConnections objectForKey:[NSValue valueWithPointer:targetKey]];
}

-(void)removePendingConnection:(CTTarget*)targetKey
{
    [_pendingConnections removeObjectForKey:[NSValue valueWithPointer:targetKey]];
}

-(void)addConnection:(NSTConnection*)connection withTargetKey:(CTTarget*)targetKey
{
    [_connections setObject:connection forKey:[NSValue valueWithPointer:targetKey]];
}
-(void)removeConnection:(CTTarget*)targetKey
{
    [_connections removeObjectForKey:[NSValue valueWithPointer:targetKey]];
}


#ifdef __clang__
CTConnectionClosure _NSTURLSessionConnectionClosure = ^int(CTError * err, CTConnection * conn) {
#else
static int _NSTURLSessionConnectionCallback(CTError * err, CTConnection * conn) {
#endif
    //conn should always exist, so it can contain the input CTTarget pointer
    //which contains the client context
    assert(conn);

    //Return variables
    //CXConnectionClosureFunc callback;
    //CXConnection* cxConnection = NULL;
    NSTConnectionClosure callback;
    NSTConnection * nstConnection = NULL;
    
    //Retrieve CXReQLSession caller object from ReQLService input client context property
    NSTURLSession* nstURLSession = (__bridge NSTURLSession*)(conn->target->ctx);

    //Parse any errors
    if (err->id != CTSuccess)
    {
        //The connection failed
        //parse the Error and bail out
        assert(1 == 0);

    }
    else //Handle successfull connection!!!
    {
        
        if (!conn->socketContext.txQueue)
        {
            assert(1 == 0);
        }

        assert(conn->socketContext.rxQueue);
        assert(conn->socketContext.txQueue);

        //Wrap the CTConnection in a C++ CXConnection Object
        //A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
        //cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);
        nstConnection = [NSTConnection newConnection:conn];

        //  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
        //if (cxConnection) cxReQLSession->addConnection(conn->target, cxConnection);
        if (nstConnection) [nstURLSession addConnection:nstConnection withTargetKey:conn->target];
    }

    //retrieve the client connection callback and target ctx (replace target context on target)
    //std::pair<CXConnectionClosureFunc, void*> targetCallbackContext = cxReQLSession->pendingConnectionForKey(conn->target);
    NSPair * targetCallbackContext = [nstURLSession pendingConnectionForKey:conn->target];
    callback = targetCallbackContext.first;
    conn->target->ctx = (__bridge void *)(targetCallbackContext.second);

    //  Always remove pending connections
    //cxReQLSession->removePendingConnection(conn->target);
    [nstURLSession removePendingConnection:conn->target];
    
    //issue the client callback
    callback(err, nstConnection);

    return err->id;
};



int NSTURLSessionConnect(CTTarget* target, NSTConnectionClosure closure)
{
    //Create a connection key for caching the connection and add to hash as a pending connection
    //__block NSString * connectionKey = [NSString stringWithFormat:@"%s:%d", service->host, service->port];
    //NSReQLService * nsService = [NSReQLService newService:service];
    //[NSReQLSharedSession addPendingConnection:nsService forKey:connectionKey];
    
    //convert port short to c string
    //char port[6];
    //snprintf(port, 6, "%d", (int)target->url.port);

    //__block NSString * connectionKey = [NSString stringWithFormat:@"%s:%hu", target->url.host, target->url.port];

    //hijack the input target ctx variable
    void* clientCtx = target->ctx;
    target->ctx = (__bridge void *)NSTURLSharedSession;
    
    //wrap ReQLService c struct ptr in CXReQLService object
    //CXReQLService * cxService = new CXReQLService(target);

    //Add target to map of pending connections
    //std::function< void(CTError* err, CXConnection* conn) > callbackFunc = callback;
    
    [NSTURLSharedSession addPendingConnection:[NSPair first:closure second:[NSValue valueWithPointer:clientCtx]] withTargetKey:target];

    //Issue the connection using CTransport C API
    return CTransport.connect(target, _NSTURLSessionConnectionClosure);
}

NSTConnection* NSTURLSessionConnection(NSString * connectionKey)
{
    return [NSTURLSharedSession getConnectionForKey:connectionKey];
}

-(NSTConnectFunc)connect
{
    return NSTURLSessionConnect;
}

/*
-(int)connectWithTarget:(CTTarget*)target andClosure:(NSTConnectionClosure)closure
{
    //return NSReQLSessionConnect;
}
*/


@end
