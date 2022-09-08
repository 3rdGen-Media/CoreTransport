//
//  NSRQLConnection.h
//  ReØMQL
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//


#import <Foundation/Foundation.h>

#define class extern_class
//#import "NSCursor.h"
#undef class


/*
static const char * _Nonnull kReqlConnectionNotification =  "com.3rdgen.CoReQL.ConnectionNotification";
static const char * _Nonnull kReqlReconnectNotification =  "com.3rdgen.CoReQL.ReconnectNotification";
static const char * _Nonnull kReqlLoginNotification =       "com.3rdgen.CoReQL.LoginNotification";
static const char * _Nonnull kReqlLogoutNotification =       "com.3rdgen.CoReQL.LogoutNotification";
*/


//#define CX_PAGE_SIZE                4096L //only use this to hard code or test changes in page size
#define   NS_MAX_INFLIGHT_QUERIES    8L

//#define CX_MAX_INFLIGHT_CONNECT_PACKETS 1L
//#define CX_MAX_INFLIGHT_DECRYPT_PACKETS 2L
//#define CX_MAX_INFLIGHT_ENCRYPT_PACKETS 1L

#define NS_QUERY_BUFF_PAGES          8L    //the number of system pages that will be used to calculate the size of a single query buffer
#define NS_REQUEST_BUFF_PAGE         NS_QUERY_BUFF_PAGES
#define NS_RESPONSE_BUFF_PAGES       8L    //the number of system pages that will be used to calculate the size of a single response buffer

//Note:  If you're buffer size is anywher close to MAX UNSIGNED LONG, you're network requests are too large...

//#define CX_QUERY_BUFF_SIZE        CX_PAGE_SIZE * 256L
//#define CX_RESPONSE_BUFF_SIZE     CX_PAGE_SIZE * 256L
static unsigned long                NS_QUERY_BUFF_SIZE     = 0;    //Query buffer size will be calculated dynamically based on the above macros
static unsigned long                NS_RESPONSE_BUFF_SIZE  = 0;    //Response buffer size will be calculated dynamically based on the above macros

NS_ASSUME_NONNULL_BEGIN

@interface NSPair : NSObject
{
    //id      _first;
    //id      _second;
}
@property (nonatomic, assign) id first;
@property (nonatomic, assign) id second;


+(NSPair*)first:(id)first second:(id)second;

@end
NS_ASSUME_NONNULL_END


NS_ASSUME_NONNULL_BEGIN

//FOUNDATION_EXPORT NSErrorDomain const NSReQLErrorDomain;             // NSString, a complete sentence (or more) describing ideally both what failed and why it failed.
//FOUNDATION_EXPORT NSErrorUserInfoKey const NSReQLErrorUserInfoKey;   // NSString, a complete sentence (or more) describing ideally both what failed and why it failed.

//@interface NSReQLService : NSObject
//+(NSReQLService*)newService:(ReqlService*)service;
//@end

@class NSTCursor;
@class NSTConnection;

typedef int     (^NSTConnectionClosure)      (CTError  * error,   NSTConnection* connection);
typedef int     (*NSTConnectFunc)            (CTTarget * target,  NSTConnectionClosure closure);

typedef void    (^NSTRequestClosure)         (CTError * __nullable error, NSTCursor* cursor);


//An NSReQLConnection creates and maintains a TCP + TLS BSD Socket Connection
//For reading from and writing to a Remote RehinkDB Service using the ReØMQL C API
@interface NSTConnection : NSObject


//typedef void    (*NSReQLCloseConnectFunc)   (void);
//typedef  NSReQLConnection* _Nullable (*NSReQLConnectionCacheFunc)   (NSString * connectionKey);

@property (nonatomic, retain) NSMutableDictionary * cursors;
@property (nonatomic, retain) NSMutableDictionary * queries;

//Get NSTConnection internal CTConnection C struct
-(CTConnection*)connection;

//Convenience Initializer Wraps Copy Constructor:
//Create NSReQLConnection object from a ReqlConnection C-API object
+(NSTConnection*)newConnection:(CTConnection*)conn;// withQueue:(dispatch_queue_t __nullable)queue;



/*
CTKernelQueueType queryQueue();
CTKernelQueueType responseQueue();

//template<typename CXRequestClosure>
void addRequestCallbackForKey(std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)> const callback, uint64_t requestToken) {_queries.insert(std::make_pair(requestToken, callback));}
void removeRequestCallbackForKey(uint64_t requestToken) {_queries.erase(requestToken);}
std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)> getRequestCallbackForKey(uint64_t requestToken) { return _queries.at(requestToken); }

void addRequestCursorForKey(std::shared_ptr<CXCursor> cursor, uint64_t requestToken) { assert(_cursors.find(requestToken) == _cursors.end());  _cursors.insert(std::make_pair(requestToken, cursor)); }
void removeRequestCursorForKey(uint64_t requestToken) {_cursors.erase(requestToken);}
*/
 
-(void)addRequestCallback:(NSTRequestClosure)callback  forKey:(uint64_t)requestToken;
-(void)removeRequestCallbackForKey:(uint64_t)requestToken;
-(NSTRequestClosure)getRequestCallbackForKey:(uint64_t)requestToken;// { return _queries.at(requestToken); }

-(void)addRequestCursor:(NSTCursor*)cursor forKey:(uint64_t)requestToken;// { assert(_cursors.find(requestToken) == _cursors.end());  _cursors.insert(std::make_pair(requestToken, cursor));
-(void)removeRequestCursorForKey:(uint64_t)requestToken;

-(NSTCursor*)getRequestCursorForKey:(uint64_t)requestToken;
-(void)distributeResponseWithCursorForToken:(uint64_t)requestToken;
//void close();


//void SwapCursors(uint64_t key1, uint64_t key2) { std::swap(_cursors[key1], _cursors[key2]); }


/*
+(NSString *)ConnectionUpdateNotification;
+(NSString *)ReconnectUpdateNotification;
+(NSString*)LoginUpdateNotification;
+(NSString*)LogoutUpdateNotification;
*/


@end


NS_ASSUME_NONNULL_END
