//
//  NSReQLQuery.h
//  Mastry
//
//  Created by Joe Moulton on 6/5/19.
//  Copyright © 2019 VRTVentures LLC. All rights reserved.
//

#import <Foundation/Foundation.h>
//#import "NSReQLConnection.h"
//#import "NSReQLCursor.h"
//#import "NSReQLGeometry.h"

NS_ASSUME_NONNULL_BEGIN

/*
*  REQL COMMANDS
*
*  ReQL commands are represented as a list of two or three elements.
*
*       [<command>, [<arguments>], {<options>}]
*       <command> is the integer representing the command, from ql2.proto
*       <arguments> is a list of all arguments. Each argument is itself a query (a command list, or data).
*       <options> are the command’s optional arguments. This element may be left out if the command has no optional arguments given.
*/

@interface NSTReQLQuery : NSObject

//#include <stdlib.h>
//#include <stdio.h>

/*
#define NARGS(...) NARGS_(__VA_ARGS__, 5, 4, 3, 2, 1, 0)
#define NARGS_(_5, _4, _3, _2, _1, N, ...) N

#define CONC(A, B) CONC_(A, B)
#define CONC_(A, B) A##B

#define NSReQLDeleteQueryFunc(...) CONC(Console, NARGS(__VA_ARGS__))(__VA_ARGS__)


typedef NSArray*    _Nonnull  (*NSReQLIntermediateArrayQueryFunc) (NSObject * __nullable arg);
*/
//A function pointer prototype for defining Reql Query Creation Methods
typedef NSTReQLQuery* _Nonnull (*NSTReQLStringQueryFunc)  (NSString*      name);


typedef NSTReQLQuery* _Nonnull (*NSTReQLArrayQueryFunc)   (NSArray*       array);

typedef NSTReQLQuery* _Nonnull (*NSTReQLInsertQueryFunc)  (NSArray*       arrayOfDicts);
typedef NSTReQLQuery* _Nonnull (*NSTReQLFilterQueryFunc)  (NSObject*  __nullable predicateObj);

//typedef NSTReQLQuery* _Nonnull (*NSReQLGeoQueryFunc)  (NSReQLGeometry * geometry, NSString*  geospatialIndex);

typedef NSTReQLQuery* _Nonnull (*NSTReQLOptionsQueryFunc) (NSDictionary*  __nullable options);
typedef NSTReQLQuery* _Nonnull (*NSTReQLObjectQueryFunc)  (NSObject*      __nullable options);

typedef NSTReQLQuery* _Nonnull (*NSTReQLUIntQueryFunc)  (uint64_t limit);

typedef NSTReQLQuery* _Nonnull (*NSTReQLVoidQueryFunc)    (void);


//A function pointer prototype for defining client callback to retrieve the result of an NSReQLQueryRunFunc
//typedef void (^__nullable NSReQLQueryClosure)(ReqlError* __nullable error, NSReQLCursor* cursor);

//A function pointer for defining Reql Query Run Methods

typedef uint64_t (*NSTReQLQueryRunFunc)(NSTConnection * conn, NSDictionary* __nullable options, NSTRequestClosure closure);
//typedef void (*NSReQLQueryContinueFunc)(NSTReQLCursor * cursor, NSDictionary* __nullable options);
//typedef void (*NSReQLQueryStopFunc)(NSTReQLConnection * conn, uint64_t queryToken);

@property (nonatomic, readonly) NSArray * queryArray;

//Constructor
//-(id)initWithType:(ReqlTermType)queryType andName:(NSString*)name;
-(id)initWithCommand:(ReqlTermType)command Arguments:(NSObject* __nullable)args Options:(NSObject* __nullable)options;

//Convenience Constructors
+(id)queryWithCommand:(ReqlTermType)command Arguments:(NSObject* __nullable)args Options:(NSObject* __nullable)options;



//@property (nonatomic, retain) NSTReQLQuery * __nullable parent;
//@property (nonatomic, retain) NSTReQLQuery * __nullable child;

-(uint64_t)RunQueryWithCursorOnQueue:(NSTReQLCursor*)nstReQLCursor ForQueryToken:(uint64_t)queryToken;


//Convenience Initializers
+(NSTReQLStringQueryFunc)db;

/*
+(NSReQLIntermediateArrayQueryFunc)desc;
+(NSReQLArrayQueryFunc)doQuery;
+(NSReQLArrayQueryFunc)expr;
+(NSReQLQueryStopFunc)stop;
*/

-(NSTReQLStringQueryFunc)table;
-(NSTReQLObjectQueryFunc)get;

/*
-(NSReQLStringQueryFunc)pluck;
-(NSReQLInsertQueryFunc)insert;
-(NSReQLFilterQueryFunc)merge;
-(NSReQLFilterQueryFunc)filter;
-(NSReQLStringQueryFunc)getField;
-(NSReQLStringQueryFunc)sum;
-(NSReQLFilterQueryFunc)update;

-(NSReQLVoidQueryFunc)count;


-(NSReQLStringQueryFunc)getField;
-(NSReQLStringQueryFunc)coerceTo;
-(NSReQLObjectQueryFunc)append;

+(NSReQLObjectQueryFunc)row;
-(NSReQLStringQueryFunc)match;


-(NSReQLGeoQueryFunc)getIntersecting;

-(NSReQLObjectQueryFunc)orderBy;
-(NSReQLUIntQueryFunc)limit;

-(NSReQLOptionsQueryFunc)delete;
-(NSReQLOptionsQueryFunc)changes;

-(NSReQLObjectQueryFunc)row;            //REQL_BRACKET


//-(NSReQLQuery*)delete(...);

 */

//Run Query
-(NSTReQLQueryRunFunc)run;
//-(NSReQLQueryContinueFunc)continue;
//-(NSReQLQueryStopFunc)stop;

@end

NS_ASSUME_NONNULL_END
