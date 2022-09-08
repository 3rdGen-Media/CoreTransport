//
//  NSRQLSession.h
//  ReØMQL
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#import <Foundation/Foundation.h>

#define NSTReQLSharedSession [NSTReQLSession sharedSession]

NS_ASSUME_NONNULL_BEGIN

//Dedicated GCD concurrent queues for creating NSReQLConnections
//static const char * kReqlConnectionQueue = "com.3rdgen.ReØMQL.ConnectionQueue";
//static const char * kReqlQueryQueue      = "com.3rdgen.ReØMQL.QueryQueue";
//static const char * kReqlResponseQueue      = "com.3rdgen.ReØMQL.ResponseQueue";
//static const char * kReqlChangeQueue     = "com.3rdgen.ReØMQL.ChangeQueue";

//An NSReQLSession is an Object that manages NSReQLConnection(s)
@interface NSTReQLSession : NSObject

/***
 *  NSReQLSession connectToService:withCallback:
 *
 *  @Description:
 *      A shared singlton that should be utilized when using NSReQLSession to manage NSReQLConnections
 *
 *  @Usage
 *       NSReQLSession * s = [NSReQLSesssion sharedInstance];
 *
 *  [sync] return:
 *      id -- An Obj-C id corresponding to the NSReQLConnection object memory
 ***/
+(NSTReQLSession*)sharedSession;

/***
 *  NSReQLSession connectToService:withCallback:
 *
 *  @Description:
 *      Starts ReØMQL Asynchronous Connection Coroutine(s) in parallel on its internal GCD connection queue
 *
 *  @Usage:
 *       Call this from anywhere and it will asynchronously run the provided NSReQLConnectionClosure on completion.
 *       NSReQLConnection(s) asynchronously returned by and should be closed with a corresponding NSReQLSession routine
 *
 *  [async] return:
 *       NSReQLConnectionClosure
 ***/
-(NSTConnectFunc)connect;


//void addPendingConnection(ReqlService* service, std::pair<CXConnectionClosureFunc, void*> serviceCallbackContext);
//void removeConnection(ReqlService *service);

//std::map<ReqlService*, std::pair<CXConnectionClosureFunc, void*>>        _pendingConnections;
//std::map<ReqlService*, CXConnection*>                                    _connections;

//-(NSMutableDictionary*)pendingConnections;
//-(NSMutableDictionary*)connections;


//these have to be public so they can be called from the static CTransport API callback
//but they should be updated to be handled for outside calls from the client, which should be allowed...
-(NSPair*)pendingConnectionForKey:(CTTarget*)targetKey;
-(void)removePendingConnection:(CTTarget*)targetKey;
-(void)addConnection:(NSTConnection*)connection withTargetKey:(CTTarget*)target;

//id db(char * name);

@end

NS_ASSUME_NONNULL_END
