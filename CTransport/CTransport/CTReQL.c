#include "../CTransport.h"

//Pthread TLS context
struct ReqlQueryContext *ReqlQueryGetContext(void) {

#ifndef _WIN32
    int rc = pthread_once(&reql_keyonce, ReqlQueryContextMakeKey);
    assert(rc == 0);
    struct ReqlQueryContext *ctx = pthread_getspecific(reql_key);
    if(ctx) return ctx;
    ctx = calloc(1, sizeof(struct ReqlQueryContext));
    assert(ctx);
    ReqlQueryContextInit_(ctx);
    if(reql_ismain()) {
        reql_main = ctx;
        rc = atexit(ReqlQueryContextAtExit);
        assert(rc == 0);
    }
    rc = pthread_setspecific(reql_key, ctx);
    assert(rc == 0);
    return ctx;
#else
	return NULL;
#endif
}


const char* responseKey = "\"t\"";

ReqlResponseType ReqlCursorGetResponseType(ReqlCursor * cursor)
{
    //don't bother converting json to nsobject just parse c-style
    char * tmp;
	size_t responseTypeLength = 0;
	char* responseTypeValue = strstr((char*)(cursor->buffer) + sizeof(ReqlQueryMessageHeader), responseKey);
    if( responseTypeValue )
    {
        responseTypeValue += strlen(responseKey) + 1;
        //if( *successVal == ':') successVal += 1;
        //if( *authValue == '"')
        //{
        //authValue+=1;
        tmp = (char*)responseTypeValue;
        while( *(tmp) != ',' ) tmp++;
        responseTypeLength = tmp - responseTypeValue;
        //}
    }
    return (ReqlResponseType)atoi(responseTypeValue);
}


uint64_t ReqlContinueQueryCtx( ReqlQueryContext* ctx, ReqlConnection * conn, uint64_t queryToken, void * options )
{
    //printf("ReqlContinueQuery\n");
    
	int32_t queryHeaderLength, queryStrLength;
    ReqlQueryMessageHeader queryHeader;
	char * queryStr;
	char queryBuf[2][4096];
	 int queryIndex = 0;
	ReqlError * errPtr = NULL;
    ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    //Get the ReqlQuery stack stored via Posix Thread Local Storage memory
    //ReqlQueryContext * ctx = ReqlQueryGetContext();
    //printf("Num Queries = %d\n", ctx->numQueries);
    //assert( ctx->numQueries > 0 );
    
    //const char * empty_options = "{}";
    //Serialize the ReqlQueryStack to the JSON expected by REQL
    ReqlQuery continueQuery = {(ReqlTermType)2, "{}", NULL};
    if( options )
        continueQuery.data = options;
    //ctx->stack[ctx->numQueries++] = runQuery;
    
    //Assume the first is always a REQL database query
    //int queryIndex = 0;

    if( ctx->numQueries > 0 )
    {
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[\"%s\"]]", ctx->stack[0].type, (char*)ctx->stack[0].data );
        queryIndex++;
        
        //Serialize all subsequent queries in a for  loop
        //TO DO:  consider unrolling?
        for(;queryIndex<ctx->numQueries; queryIndex++)
        {
            if( !( ctx->stack[queryIndex].type == REQL_TABLE ))//|| ctx->stack[queryIndex].type == REQL_ORDER_BY) )
            {
                //This is an annoying branch:
                //  Some Queries expect 1 argument while other queries expect 2 arguments
                //  While any queries can have no arguments if such data is empty
                if( !ctx->stack[queryIndex].data )  //no arg case
                    sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[%s]]", ctx->stack[queryIndex].type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader));
                else if(  ctx->stack[queryIndex].type == REQL_CHANGES || ctx->stack[queryIndex].type == REQL_ORDER_BY) //1 arg case
                    sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[%s],%s]", ctx->stack[queryIndex].type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)ctx->stack[queryIndex].data);
                else    //2 args case
                    sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[%s,%s]]", ctx->stack[queryIndex].type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)ctx->stack[queryIndex].data);
            }
            else    //this allows population of single c-string in json
                sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[%s,\"%s\"]]", ctx->stack[queryIndex].type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)ctx->stack[queryIndex].data);
            //printf("queryBuf = \n\n%s\n\n", queryBuf[(queryIndex%2)]+sizeof(ReqlQueryMessageHeader));
        }
        
        //Finally, once we are done serialzing the queries on the TLS query stack
        //We embed the entire serialized query into a REQL run query
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,%s,%s]", continueQuery.type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)continueQuery.data);
        
        //Most helpful for debugging!!!
        //printf("queryBuf = \n\n%s\n\n", queryBuf[(queryIndex)%2]+sizeof(ReqlQueryMessageHeader));
    }
    else
    {
        //if( options )
            sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,%s]", continueQuery.type, (char*)continueQuery.data);
        //else
        //    sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,%s]", continueQuery.type, (char*)continueQuery.data);
    }
    
    //reset the TLS ReqlQuery stack
    ctx->numQueries = 0;
    
    //Populate the network message with a header and the serialized query JSON string
    queryStr = queryBuf[(queryIndex)%2];
    queryHeader.token = queryToken;//++(conn->queryCount);//conn->token;
    
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    queryStrLength = (int32_t)strlen(queryStr + queryHeaderLength);
    
    //printf("queryStrLength = %d\n", queryStrLength);
    queryHeader.length = queryStrLength;// + queryHeaderLength;
    memcpy(queryBuf[(queryIndex)%2], &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    queryBuf[(queryIndex)%2][queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //printf("sendBuffer = \n\n%s\n", queryBuf);
    CTSend(conn, queryStr, queryStrLength + queryHeaderLength);
    return queryHeader.token;
    
}

uint64_t ReqlStopQueryCtx( ReqlQueryContext* ctx, ReqlConnection * conn, uint64_t queryToken, void * options )
{
    //printf("ReqlContinueQuery\n");
  
	int32_t queryHeaderLength, queryStrLength;
	ReqlQueryMessageHeader queryHeader;
    char queryBuf[2][4096];// = {"[3]", "[3]"};
	char * queryStr = NULL;
    ReqlError * errPtr = NULL;
    ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    int queryIndex = 0;
        
    sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[3]");
    queryBuf[queryIndex%2][sizeof(ReqlQueryMessageHeader)+ 3] = '\0';
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    //Get the ReqlQuery stack stored via Posix Thread Local Storage memory
    //ReqlQueryContext * ctx = ReqlQueryGetContext();
    //printf("Num Queries = %d\n", ctx->numQueries);
    //assert( ctx->numQueries > 0 );
    
    //const char * empty_options = "{}";
    //Serialize the ReqlQueryStack to the JSON expected by REQL
    
   
    printf("stop queryBuf = \n\n%s\n\n", queryBuf[(queryIndex)%2]+sizeof(ReqlQueryMessageHeader));

    //reset the TLS ReqlQuery stack
    ctx->numQueries = 0;
    
    //Populate the network message with a header and the serialized query JSON string
    queryStr = queryBuf[(queryIndex)%2];
    queryHeader.token = queryToken;
    ++(conn->queryCount);//conn->token;
    
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    queryStrLength = (int32_t)strlen(queryStr + queryHeaderLength);
    
    printf("queryStrLength = %d\n", queryStrLength);
    queryHeader.length = queryStrLength;// + queryHeaderLength;
    memcpy(queryBuf[(queryIndex)%2], &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    queryBuf[(queryIndex)%2][queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //printf("sendBuffer = \n\n%s\n", queryBuf);
    CTSend(conn, queryStr, queryStrLength + queryHeaderLength);
    return queryHeader.token;
    
}


uint64_t ReqlContinueQuery(ReqlConnection * conn, uint64_t queryToken, void * options )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    return ReqlContinueQueryCtx(ctx, conn, queryToken, options);//, callback);
}

uint64_t ReqlStopQuery(ReqlConnection * conn, uint64_t queryToken, void * options )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    return ReqlStopQueryCtx(ctx, conn, queryToken, options);//, callback);
}



uint64_t ReqlCursorContinue(ReqlCursor *cursor, void * options)
{
    //printf("ReqlCursorContinue\n");
    return ReqlContinueQuery(cursor->conn, cursor->header->token, options);
}