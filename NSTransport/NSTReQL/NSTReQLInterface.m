//
//  NSReQL.m
//  Mastry
//
//  Created by Joe Moulton on 6/5/19.
//  Copyright © 2019 VRTVentures LLC. All rights reserved.
//

#include "../NSTReQL.h"

@implementation NSTReQL

+(NSTConnectFunc)connect
{

    //CXReQLSession* sharedSession = sharedSession->getInstance();
    //return sharedSession->connect(target, callback);
    return NSTReQLSession.sharedSession.connect;
    //return NSReQLSession.connect;
}

/*
+(NSReQLCloseConnectFunc)closeConnections
{
    NSLog(@"NSReQL closeConnections");

    return NSReQLSession.closeConnections;
}


+(NSReQLConnectionCacheFunc)connection
{
    return NSReQLSession.connection;
}
*/
+(NSTReQLStringQueryFunc)db
{
    return NSTReQLQuery.db;
}

/*
+(NSReQLIntermediateArrayQueryFunc)desc
{
    return NSReQLQuery.desc;
}

+(NSReQLArrayQueryFunc)doQuery
{
    
    return NSReQLQuery.doQuery;
}

+(NSReQLArrayQueryFunc)expr
{
    return NSReQLQuery.expr;
}

+(NSReQLObjectQueryFunc)row
{
    return NSReQLQuery.row;
}

+(NSReQLQueryStopFunc)stop
{
    return NSReQLQuery.stop;
}
*/

@end
