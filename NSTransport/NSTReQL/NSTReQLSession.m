//
//  NSRQLSession.m
//  ReØMQL
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#include "../NSTReQL.h"

CFRunLoopSourceContext NS_REQL_RUN_LOOP_SRC_CTX = {0};
const CFOptionFlags NS_REQL_RUN_LOOP_WAITING = kCFRunLoopBeforeWaiting | kCFRunLoopAfterWaiting;
const CFOptionFlags NS_REQL_RUN_LOOP_RUNNING  = kCFRunLoopAllActivities;

@interface NSTReQLSession()
{
    //An array to store pending/open NSReQLConnections
    NSMutableDictionary * _connections;      //maintain an internal connection cache as a hash
    NSMutableDictionary * _pendingConnections;      //maintain an internal connection cache as a hash
 
}

@property (nonatomic, retain) NSMutableDictionary * connections;
@property (nonatomic, retain) NSMutableDictionary * pendingConnections;

@end

@implementation NSTReQLSession

@synthesize connections        = _connections;
@synthesize pendingConnections = _pendingConnections;


-(void)dealloc
{
    NSLog(@"NSTReQLSession::dealloc");
    
    //TO DO: close all open connections
    
    //Destroy pendingConnectionsArray
    [self.pendingConnections removeAllObjects];
    self.pendingConnections = NULL;
    
    //Destroy connections array
    [self.connections removeAllObjects];
    self.connections = nil;
}

+(NSTReQLSession *)sharedSession
{
    static NSTReQLSession *sharedSession = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedSession = [[NSTReQLSession alloc] init];
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
    NSLog(@"NSTReQLSession::getConnectionForKey(%@) = \n\n%@", connectionKey, self.connections);
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
CTConnectionClosure _NSTReQLSessionConnectionClosure = ^int(CTError * err, CTConnection * conn) {
#else
static int _NSTReQLSessionConnectionCallback(CTError * err, CTConnection * conn) {
#endif
    //conn should always exist, so it can contain the input CTTarget pointer
    //which contains the client context
    assert(conn);

    CTConnectionClosure _NSTReQLSessionHandshakeCallback = ^ int(CTError * err, ReqlConnection * conn)
    //static int _CXReQLSessionHandshakeCallback(CTError* err, ReqlConnection* conn)
    {
        //conn should always exist, so it can contain the input CTTarget pointer
        //which contains the client context
        assert(conn);

        //Return variables
        //CXConnectionClosureFunc callback;
        //CXConnection* cxConnection = NULL;
        NSTConnectionClosure callback;
        NSTConnection * nstConnection = NULL;
        
        //Retrieve CXReQLSession caller object from ReQLService input client context property
        NSTReQLSession* nstReQLSession = (__bridge NSTReQLSession*)(conn->target->ctx);

        //Parse any errors
        if (err->id != CTSuccess)
        {
            //The handshake failed
            //parse the Error and bail out
            fprintf(stderr, "_NSTReQLSessionHandshakeCallback::CTReQLAsyncLHandshake failed!\n");
            err->id = CTSASLHandshakeError;
            assert(1 == 0);

        }
        else //Handle successfull connection!!!
        {
            //For WIN32 each CTConnection gets its own pair of tx/rx iocp queues internal to the ReQL C API (but the client must assign thread(s) to these)
            //For Darwin + GCD, these queue handles are currently owned by NSReQLSession and still need to be moved into ReQLConnection->socketContext
            //*Note:  On platforms where the SSL Context is not thread safe, such as Darwin w/ SecureTransport, we need to manually specify the same [GCD] thread queue for send and receive
            //because the SSL Context API (SecureTransport) is not thread safe when reading and writing simultaneously

            //Wrap the CTConnection in a C++ CXConnection Object
            //A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
            //cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);
            nstConnection = [NSTConnection newConnection:conn];
            
            //  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
            if (nstConnection) [nstReQLSession addConnection:nstConnection withTargetKey:conn->target];
            //  Add to error connections for retry?
        }

        //retrieve the client connection callback and target ctx (replace target context on target)
        //std::pair<CXConnectionClosureFunc, void*> targetCallbackContext = cxReQLSession->pendingConnectionForKey(conn->target);
        NSPair * targetCallbackContext = [nstReQLSession pendingConnectionForKey:conn->target];
        callback = targetCallbackContext.first;
        conn->target->ctx = (__bridge void *)(targetCallbackContext.second);

        //  Always remove pending connections
        //cxReQLSession->removePendingConnection(conn->target);
        [nstReQLSession removePendingConnection:conn->target];
        
        //issue the client callback
        callback(err, nstConnection);

        return err->id;

    };

    int status = CTSuccess;

    //Return variables
    //CXConnectionClosureFunc callback;
    //CXConnection* cxConnection = NULL;
    NSTConnectionClosure callback;
    NSTConnection * nstConnection = NULL;
    
    //Retrieve CXReQLSession caller object from ReQLService input client context property
    NSTReQLSession* nsReQLSession = (__bridge NSTReQLSession*)(conn->target->ctx);

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
        
        
        //Do the CTransport C-Style ReQL Handshake
        // Complete the RethinkDB 2.3+ connection: Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
        if ((1))
        {
            if ((status = CTReQLAsyncHandshake(conn, conn->target, _NSTReQLSessionHandshakeCallback)) != CTSuccess)
                assert(1 == 0);

            return err->id;
        }
        else if (/* DISABLES CODE */ ((status = CTReQLHandshake(conn, conn->target)) == CTSuccess))
        {
            //For WIN32 each CTConnection gets its own pair of tx/rx iocp queues internal to the ReQL C API (but the client must assign thread(s) to these)
            //For Darwin + GCD, these queue handles are currently owned by NSReQLSession and still need to be moved into ReQLConnection->socketContext
            //*Note:  On platforms where the SSL Context is not thread safe, such as Darwin w/ SecureTransport, we need to manually specify the same [GCD] thread queue for send and receive
            //because the SSL Context API (SecureTransport) is not thread safe when reading and writing simultaneously

            //Wrap the CTConnection in a C++ CXConnection Object
            //A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
            //cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);

            //  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
            //if (cxConnection) cxReQLSession->addConnection(conn->target, cxConnection);
            
            //  Add to error connections for retry?

        }
        else
        {
            fprintf(stderr, "_NSTReQLSessionConnectionClosure::CTReQLSASLHandshake failed!\n");
            //CTCloseConnection(&_reqlConn);
            //CTSSLCleanup();
            err->id = CTSASLHandshakeError;
            assert(1 == 0);
        }
        

        //Wrap the CTConnection in a C++ CXConnection Object
        //A CXConnection gets created with a CTConnection (socket + ssl context) and a dedicated thread queue for reading from the connection's socket
        //cxConnection = new CXConnection(conn, conn->socketContext.rxQueue, conn->socketContext.txQueue);
        nstConnection = [NSTConnection newConnection:conn];

        //  Store CXConnection in internal NXReQLSession connections array/map/hash or whatever
        //if (cxConnection) cxReQLSession->addConnection(conn->target, cxConnection);
        if (nstConnection) [nsReQLSession addConnection:nstConnection withTargetKey:conn->target];
    }

    //retrieve the client connection callback and target ctx (replace target context on target)
    //std::pair<CXConnectionClosureFunc, void*> targetCallbackContext = cxReQLSession->pendingConnectionForKey(conn->target);
    NSPair * targetCallbackContext = [nsReQLSession pendingConnectionForKey:conn->target];
    callback = targetCallbackContext.first;
    conn->target->ctx = (__bridge void *)(targetCallbackContext.second);

    //  Always remove pending connections
    //cxReQLSession->removePendingConnection(conn->target);
    [nsReQLSession removePendingConnection:conn->target];
    
    //issue the client callback
    callback(err, nstConnection);

    return err->id;
};



int NSTReQLSessionConnect(CTTarget* target, NSTConnectionClosure closure)
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
    target->ctx = (__bridge void *)NSTReQLSharedSession;
    
    //wrap ReQLService c struct ptr in CXReQLService object
    //CXReQLService * cxService = new CXReQLService(target);

    //Add target to map of pending connections
    //std::function< void(CTError* err, CXConnection* conn) > callbackFunc = callback;
    
    [NSTReQLSharedSession addPendingConnection:[NSPair first:closure second:[NSValue valueWithPointer:clientCtx]] withTargetKey:target];

    //Issue the connection using CTransport C API
    return CTransport.connect(target, _NSTReQLSessionConnectionClosure);
}

NSTConnection* NSReQLSessionConnection(NSString * connectionKey)
{
    return [NSTReQLSharedSession getConnectionForKey:connectionKey];
}

-(NSTConnectFunc)connect
{
    return NSTReQLSessionConnect;
}



@end
