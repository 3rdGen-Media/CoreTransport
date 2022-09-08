//
//  NSTReQLQuery.m
//  Mastry
//
//  Created by Joe Moulton on 6/5/19.
//  Copyright © 2019 VRTVentures LLC. All rights reserved.
//

#include "../NSTReQL.h"

//#import "NSTReQLQuery.h"
//#import "NSTReQLSession.h"

@interface NSTReQLQuery()
{
    //ReqlQuery _query;
    NSArray * _queryArray;

    //ReqlTermType _type;
    //NSString * _string;

    
    //NSObject * _argument;
    //NSObject * _options;
    
    //NSTReQLQuery * _childQuery;

    const char *      _filepath;

}

@property (nonatomic) ReqlTermType command;
//@property (nonatomic, retain) NSString * string;
@property (nonatomic, retain) NSObject * arguments;
@property (nonatomic, retain) NSObject * options;

@property (nonatomic, retain) NSArray * queryArray;

@property (nonatomic) const char* filepath;

@end



@implementation NSTReQLQuery

@synthesize queryArray = _queryArray;
@synthesize filepath = _filepath;

//Private NSTReQLQuery TLS (Thread Local Storage) API
static void NSTReQLQueryContextInit_(struct ReqlQueryContext *ctx) {
    printf("NSTReQLQueryContextInit\n");
    /*
    dill_assert(ctx->initialized == 0);
    ctx->initialized = 1;
    int rc = dill_ctx_now_init(&ctx->now);
    dill_assert(rc == 0);
    rc = dill_ctx_cr_init(&ctx->cr);
    dill_assert(rc == 0);
    rc = dill_ctx_handle_init(&ctx->handle);
    dill_assert(rc == 0);
    rc = dill_ctx_stack_init(&ctx->stack);
    dill_assert(rc == 0);
    rc = dill_ctx_pollset_init(&ctx->pollset);
    dill_assert(rc == 0);
    rc = dill_ctx_fd_init(&ctx->fd);
    dill_assert(rc == 0);
    */
}

static void NSTReQLQueryContextTerminate_(struct ReqlQueryContext *ctx) {
    
    printf("NSTReQLQueryContextTerminate\n");
    /*
    dill_assert(ctx->initialized == 1);
    dill_ctx_fd_term(&ctx->fd);
    dill_ctx_pollset_term(&ctx->pollset);
    dill_ctx_stack_term(&ctx->stack);
    dill_ctx_handle_term(&ctx->handle);
    dill_ctx_cr_term(&ctx->cr);
    dill_ctx_now_term(&ctx->now);
    ctx->initialized = 0;
    */
}

static int reql_ismain() {
    return pthread_main_np();
}
static pthread_key_t NSTReQLContextKey;
static pthread_once_t reql_keyonce = PTHREAD_ONCE_INIT;
static void *reql_main = NULL;

static void NSTReQLQueryContextTerminate(void *ptr) {
    struct ReqlQueryContext *ctx = ptr;
    NSTReQLQueryContextTerminate_(ctx);
    ///if( ctx ) free(ctx);
    if(reql_ismain()) reql_main = NULL;
}

static void NSTReQLQueryContextAtExit(void) {
    if(reql_main) NSTReQLQueryContextTerminate(reql_main);
}

static void NSTReQLQueryContextMakeKey(void) {
    int rc = pthread_key_create(&NSTReQLContextKey, NSTReQLQueryContextTerminate);
    assert(!rc);
    rc = pthread_setspecific(NSTReQLContextKey, NULL);
    assert(rc == 0);

}

void NSTReQLQuerySetContext(NSTReQLQuery * query) {
    
    //create context if needed
    int rc = pthread_once(&reql_keyonce, NSTReQLQueryContextMakeKey);
    assert(rc == 0);
    
    //check to make sure there isn't a current query
    //NSTReQLQuery * existingQuery = (__bridge NSTReQLQuery *)(pthread_getspecific(NSTReQLContextKey));
    //assert(!existingQuery);
    
    //set the query context
    rc = pthread_setspecific(NSTReQLContextKey, (__bridge const void * _Nullable)(query));
    assert(rc == 0);
    
    //ReqlQueryContext * threadCtx = ReqlQueryGetContext();
    //memcpy(threadCtx, ctx, sizeof(ReqlQueryContext));
}

void NSTReQLQueryClearContext(ReqlQueryContext* query) {
    int rc = pthread_setspecific(NSTReQLContextKey, NULL);
    assert(rc == 0);
}


NSTReQLQuery * NSTReQLQueryGetContext(void) {
    int rc = pthread_once(&reql_keyonce, NSTReQLQueryContextMakeKey);
    assert(rc == 0);
    NSTReQLQuery *query = (__bridge NSTReQLQuery *)(pthread_getspecific(NSTReQLContextKey));
    /*
    if(query) return query;
    query = calloc(1, sizeof(struct ReqlQueryContext));
    assert(ctx);
    ReqlQueryContextInit_(ctx);
    if(reql_ismain()) {
        reql_main = ctx;
        rc = atexit(ReqlQueryContextAtExit);
        assert(rc == 0);
    }
    rc = pthread_setspecific(NSTReQLContextKey, ctx);
    assert(rc == 0);
    */
    return query;
}



//@synthesize type = _type;
//@synthesize string = _string;
//@synthesize argument = _argument;
//@synthesize options = _options;

-(void)dealloc
{
    NSLog(@"NSTReqlQuery::dealloc");
}


//Copy Constructor
/*
-(id)initWithReqlQuery:(ReqlQuery)query
{
    if( (self = [super init]) )
    {
        _query = query;
    }
    return self;
}
*/

-(id)initWithCommand:(ReqlTermType)command Arguments:(NSObject* __nullable)args Options:(NSObject* __nullable)options
{
    if( (self = [super init]) )
    {
        self.command = command;
        self.arguments = args;
        self.options = options;
        /*
        _queryArray = [NSMutableArray new];
            [_queryArray addObject:@(command)];
        if( args )
            [_queryArray addObject:args];
        if( options )
            [_queryArray addObject:options];
        */

        if( command == REQL_EXPR )
        {
            assert([args isKindOfClass:[NSArray class]]);
            _queryArray = (NSArray*)args;
        }
        else
        {
            if( options )
            {
                assert(1==0);
                _queryArray = @[@(command), args, options];
            }
            else if( args )
                _queryArray = @[@(command),args];
            else
                _queryArray = @[@(command)];
        }

        //self.parent = self.child = nil;
    }
    return self;
    
}


+(id)queryWithCommand:(ReqlTermType)command Arguments:(NSObject* __nullable)args Options:(NSObject* __nullable)options
{
    return [[NSTReQLQuery alloc] initWithCommand:command Arguments:args Options:options];
}


NSTReQLQuery* _Nonnull NSTReQLDatabaseQuery(NSString * name)
{
    NSTReQLQuery * query = [NSTReQLQuery queryWithCommand:REQL_DB Arguments:@[name] Options:nil];
    NSTReQLQuerySetContext(query);
    return query;
}

NSTReQLQuery* _Nonnull NSTReQLTableQuery(NSString * name)
{
    NSTReQLQuery * query = NSTReQLQueryGetContext();
    assert(query);
    query.queryArray = @[@(REQL_TABLE), @[query.queryArray, name]];
    return query;
}


NSTReQLQuery* _Nonnull NSTReQLGetQuery(NSObject * arg)
{
    NSTReQLQuery * query = NSTReQLQueryGetContext();
    assert(query);

    if( arg )
    {
        if( [arg isKindOfClass:[NSString class]] )
        {
           NSString * name = (NSString*)arg;
           //if( name )
               query.queryArray = @[@(REQL_GET), @[query.queryArray, name]];
           //else
           //    query.queryArray = @[@(REQL_FILTER), @[query.queryArray, @{}]];
        }
        else if( [arg isKindOfClass:[NSArray class]] )
        {
            NSArray * getArgArray = (NSArray*)arg;
            //if( getArgArray )
                //query.queryArray = @[@(REQL_GET), @[@(REQL_MAKE_ARRAY), getArgArray] ];
            query.queryArray = @[@(REQL_GET), @[query.queryArray, getArgArray]];
        }
    }
       return query;//[NSTReQLQuery queryWithCommand:REQL_FILTER Arguments:predicateObj Options:nil];
    //return [NSTReQLQuery queryWithCommand:REQL_GET Arguments:name Options:nil];

}


uint64_t NSTReQLQueryRun(NSTConnection * _Nonnull conn, NSDictionary * __nullable options, NSTRequestClosure callback)
{

    //reserver the next query token on the connection
    __block uint64_t expectedQueryToken = (conn.connection->queryCount);
    
    
    //get the current query object we need to build json request for
    __block NSTReQLQuery * query = NSTReQLQueryGetContext();
    assert(query);
    
    if( options )
        query.queryArray = @[@(1), query.queryArray, options];
    else
        query.queryArray = @[@(1), query.queryArray];

    
    if( !query.filepath )
    {
#ifdef _WIN32
        char filepath[1024] = "C:\\3rdGen\\CoreTransport\\bin\\x64\\ReQL\0";
#else
        //copy documents dir to asset paths
        char filepath[1024] = "\0";

        //get path to documents dir
        const char *home = getenv("HOME");
        strcat(filepath, home);
        strcat(filepath, "/Documents/ReQL");
#endif
        //_itoa((int)expectedQueryToken, filepath + strlen(filepath), 10);
        snprintf(filepath + strlen(filepath), strlen(filepath), "%d", (int)expectedQueryToken);

        strcat(filepath, ".txt");

        query.filepath = filepath;// "ReQLQuery.txt\0";//ct_file_name((char*)requestPath);
        fprintf(stderr, "NSTReQLQUery::NSTReQLQueryRun filepath = %s\n", query.filepath);
    }

    //create a cursor (for the query response, but also it holds our query buffer)
    NSTReQLCursor * nstReQLCursor = [NSTReQLCursor newCursor:conn filePath:query.filepath];
    //add response callback to queue NSReqlConnection queue
    [conn addRequestCursor:nstReQLCursor forKey:expectedQueryToken];
    [conn addRequestCallback:callback forKey:expectedQueryToken];
    return [query RunQueryWithCursorOnQueue:nstReQLCursor ForQueryToken:expectedQueryToken];

    //delete after so we don't hold up the async send with a deallocation
    //delete oldQueryArray;
    
    //return expectedQueryToken;
    
    
    
}


-(uint64_t)RunQueryWithCursorOnQueue:(NSTReQLCursor*)nstReQLCursor ForQueryToken:(uint64_t)queryToken
{
    //a string/stream for serializing the json query to char*
    unsigned long queryMessageLength;
    size_t queryStrLength;

    //serialize the query array to string

    char * queryBuffer = nstReQLCursor.cursor->requestBuffer;//&(conn->query_buffers[(queryToken % CX_REQL_MAX_INFLIGHT_QUERIES) * CX_REQL_QUERY_STATIC_BUFF_SIZE]);
    char * queryMsgBuffer = queryBuffer + sizeof(ReqlQueryMessageHeader);
    
    //Serialize json to buffer (queryMsgBuffer)
    
    //Launch thread on a dedicated serial queue that will
    //1)  Build the json string
    //2)  Send the json string over the network

    NSError *err = nil;
    NSString *jsonString = nil;
    //NSReQLQuery * nsQuery = nil;
    NSData *jsonData = nil;
    void * queryData = nil;
    
    jsonData = [NSJSONSerialization dataWithJSONObject:(NSArray*)self.queryArray options:0 error:&err];
    if( err || !jsonData ) { NSLog(@"NSTReQLQuery::RunQueryWithCursorOnQueue NSJSONSerialization Error:  %@", err.localizedDescription); assert(1==0); }
    jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
    queryData = (void*)jsonString.UTF8String;
    
    const char * _Nullable jsonQueryStr = jsonString.UTF8String;
    assert(jsonQueryStr);
    
    queryStrLength = strlen(jsonQueryStr);

    //*** copy the nsstring c-string to our cursor's buffer for sending over the network
    memcpy(queryMsgBuffer, jsonQueryStr, queryStrLength);
    queryMsgBuffer += queryStrLength;//strlen(jsonQueryStr);
    //queryStrLength = queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

    //now we know the size of the query string we can define the header
    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)queryBuffer;// queryBuffer;// { queryToken, (int32_t)queryStrLength };
    queryHeader->token = queryToken;
    queryHeader->length = (int32_t)queryStrLength;

#ifdef _DEBUG
    fprintf(stderr, "NSTReQLQuery::RunQueryWithCursorOnQueue jsonQueryStream (%d) = %.*s\n", (int)queryStrLength, (int)queryStrLength, queryBuffer + sizeof(ReqlQueryMessageHeader));
#endif
    queryMessageLength = sizeof(ReqlQueryMessageHeader) + queryStrLength;

    //Send
    //ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
    //CXReQLSendWithQueue(conn, (void*)queryBuffer, &queryMessageLength);
    //return CTSendOnQueue2(cxCursor->_cursor.conn, (char **)&queryBuffer, queryMessageLength, queryToken, &(cxCursor->_cursor.overlappedResponse));//, &overlappedResponse);
    nstReQLCursor.cursor->queryToken = queryToken;
    return CTCursorSendOnQueue((nstReQLCursor.cursor), (char**)&queryBuffer, queryMessageLength);
    
 
}

+(NSTReQLStringQueryFunc)db
{
    return NSTReQLDatabaseQuery;
}


-(NSTReQLStringQueryFunc)table
{
    return NSTReQLTableQuery;
}

-(NSTReQLObjectQueryFunc)get
{
    return NSTReQLGetQuery;
}


-(NSTReQLQueryRunFunc)run
{
    return NSTReQLQueryRun;
}

@end
