//
//  NSReQL.h
//  Mastry
//
//  NSReQL exposes NSReQLSession, NSReQLConnection, and NSReQLQuery Obj-C APIs
//  through a global object interface to closely match RethinkDB NodeJS Client Driver Syntax
//
//  Created by Joe Moulton on 6/5/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#import <Foundation/Foundation.h>
//#import "NSReQLSession.h"
//#import "NSReQLMessageChannel.h"


NS_ASSUME_NONNULL_BEGIN

//NSString * _Nonnull NSREQL_CHANGE_TYPE_INITIAL = @"initial";
//NSString * _Nonnull NSREQL_CHANGE_TYPE_CHANGE = @"change";
//NSString * _Nonnull NSREQL_CHANGE_TYPE_REMOVE = @"remove";


@interface NSTReQL : NSObject

/***
 *  NSReQL connectToService:withCallback:
 *
 *  Create an NSReQLConnection using [NSReQLSession sharedInstance] connectToService:withCallback:
 ***/
+(NSTConnectFunc)connect;


/***
 *  NSReQL.db(NSString* name);
 *
 *  Create a database query as an NSReQLQuery object
 ***/
+(NSTReQLStringQueryFunc)db;


//+(NSReQLIntermediateArrayQueryFunc)desc;
//+(NSReQLObjectQueryFunc)row;

/***
 *  NSReQL.do(NSArray* queries);
 *
 *  Create a database query as an NSReQLQuery object
 ***/
//+(NSReQLArrayQueryFunc)doQuery;
//+(NSReQLArrayQueryFunc)expr;
//+(NSReQLQueryStopFunc)stop;





@end

NS_ASSUME_NONNULL_END
