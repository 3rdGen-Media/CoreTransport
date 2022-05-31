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
    //fprintf(stderr, "ReqlContinueQuery\n");
    
	int32_t queryHeaderLength, queryStrLength;
    ReqlQueryMessageHeader queryHeader;
	char * queryStr;
	char queryBuf[2][4096];
	 int queryIndex = 0;
	ReqlError * errPtr = NULL;
    ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    //fprintf(stderr, "sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    //Get the ReqlQuery stack stored via Posix Thread Local Storage memory
    //ReqlQueryContext * ctx = ReqlQueryGetContext();
    //fprintf(stderr, "Num Queries = %d\n", ctx->numQueries);
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
            //fprintf(stderr, "queryBuf = \n\n%s\n\n", queryBuf[(queryIndex%2)]+sizeof(ReqlQueryMessageHeader));
        }
        
        //Finally, once we are done serialzing the queries on the TLS query stack
        //We embed the entire serialized query into a REQL run query
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,%s,%s]", continueQuery.type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)continueQuery.data);
        
        //Most helpful for debugging!!!
        //fprintf(stderr, "queryBuf = \n\n%s\n\n", queryBuf[(queryIndex)%2]+sizeof(ReqlQueryMessageHeader));
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
    
    //fprintf(stderr, "queryStrLength = %d\n", queryStrLength);
    queryHeader.length = queryStrLength;// + queryHeaderLength;
    memcpy(queryBuf[(queryIndex)%2], &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    queryBuf[(queryIndex)%2][queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //fprintf(stderr, "sendBuffer = \n\n%s\n", queryBuf);
    CTSend(conn, queryStr, queryStrLength + queryHeaderLength);
    return queryHeader.token;
    
}

uint64_t ReqlStopQueryCtx( ReqlQueryContext* ctx, ReqlConnection * conn, uint64_t queryToken, void * options )
{
    //fprintf(stderr, "ReqlContinueQuery\n");
  
	int32_t queryHeaderLength, queryStrLength;
	ReqlQueryMessageHeader queryHeader;
    char queryBuf[2][4096];// = {"[3]", "[3]"};
	char * queryStr = NULL;
    ReqlError * errPtr = NULL;
    ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    int queryIndex = 0;
        
    sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[3]");
    queryBuf[queryIndex%2][sizeof(ReqlQueryMessageHeader)+ 3] = '\0';
    //fprintf(stderr, "sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    //Get the ReqlQuery stack stored via Posix Thread Local Storage memory
    //ReqlQueryContext * ctx = ReqlQueryGetContext();
    //fprintf(stderr, "Num Queries = %d\n", ctx->numQueries);
    //assert( ctx->numQueries > 0 );
    
    //const char * empty_options = "{}";
    //Serialize the ReqlQueryStack to the JSON expected by REQL
    
   
    fprintf(stderr, "stop queryBuf = \n\n%s\n\n", queryBuf[(queryIndex)%2]+sizeof(ReqlQueryMessageHeader));

    //reset the TLS ReqlQuery stack
    ctx->numQueries = 0;
    
    //Populate the network message with a header and the serialized query JSON string
    queryStr = queryBuf[(queryIndex)%2];
    queryHeader.token = queryToken;
    ++(conn->queryCount);//conn->token;
    
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    queryStrLength = (int32_t)strlen(queryStr + queryHeaderLength);
    
    fprintf(stderr, "queryStrLength = %d\n", queryStrLength);
    queryHeader.length = queryStrLength;// + queryHeaderLength;
    memcpy(queryBuf[(queryIndex)%2], &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    queryBuf[(queryIndex)%2][queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //fprintf(stderr, "sendBuffer = \n\n%s\n", queryBuf);
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
    //fprintf(stderr, "ReqlCursorContinue\n");
    return ReqlContinueQuery(cursor->conn, cursor->header->token, options);
}



CTRANSPORT_API CTRANSPORT_INLINE int CTReQLHandshakeProcessMagicNumberResponse(char* mnResponsePtr, size_t mnBufferLen)
{

    bool magicNumberSuccess = false;
    char* successVal = NULL;
    //char* mnResponsePtr = (char*)MAGIC_NUMBER_RESPONSE_BUFFER.data();
    fprintf(stderr, "CTReQLHandshakeProcessMagicNumberResponse Buffer = \n\n%s\n\n", mnResponsePtr);

    //if (!mnResponsePtr || !sFirstMessagePtr)
    //    return VTSASLHandshakeError;

    //  Parse the magic number json response buffer to see if magic number exchange was successful
    //  Note:  Here we are counting on the fact that rdb server json does not send whitespace!!!
    //bool magicNumberSuccess = false;
    //const char * successKey = "\"success\"";
    //char * successVal;
    if ((successVal = strstr(mnResponsePtr, REQL_SUCCESS_KEY)) != NULL)
    {
        successVal += strlen(REQL_SUCCESS_KEY) + 1;
        //if( *successVal == ':') successVal += 1;
        if (*successVal == '"') successVal += 1;
        if (strncmp(successVal, "true", MIN(4, strlen(successVal))) == 0)
            magicNumberSuccess = true;
    }
    if (!magicNumberSuccess)
    {
        fprintf(stderr, "RDB Magic Number Response Failure!\n");
        //struct CTError vError = { (CTErrorClass)0,ec.value(),(void*)(ec.message().c_str()) };    //Reql API client functions will generally return ints as errors
        //callback(&vError, r);
        return -1;
        //return VTSASLHandshakeError;
    }

    return CTSuccess;

}


CTRANSPORT_API CTRANSPORT_INLINE void* CTReQLHandshakeProcessFirstMessageResponse(char* sFirstMessagePtr, size_t sFirstMessageLen, char* password)
{
    fprintf(stderr, "CTReQLHandshakeProcessFirstMessageResponse Buffer = \n\n%s\n\n", sFirstMessagePtr);

    bool sfmSuccess = false;
    char* successVal = NULL;


    char* authValue = NULL;
    char* tmp = NULL;
    void* base64SS = NULL;
    void* base64Proof = NULL;
    char* salt = NULL;

    char delim[] = ",";
    char* token = NULL;//strtok(authValue, delim);
    char* serverNonce = NULL;
    char* base64Salt = NULL;
    char* saltIterations = NULL;

    //Declare some variables to store the variable length of the buffers
    //as we perform message exchange/SCRAM Auth
    //unsigned long AuthMessageLength = 0;/// = 0;
    unsigned long authLength = 0;// = 0;
    size_t base64SSLength = 0;
    size_t base64ProofLength = 0;
    size_t saltLength = 0;
    int charIndex = 0;
    //SCRAM HMAC SHA-256 Auth Buffer Declarations


    CTRANSPORT_ALIGN(8) char saltedPassword[CC_SHA256_DIGEST_LENGTH];   //a buffer to hold hmac salt at each iteration

    CTRANSPORT_ALIGN(8) char clientKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold SCRAM generated clientKey
    CTRANSPORT_ALIGN(8) char serverKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold SCRAM generated serverKey

    CTRANSPORT_ALIGN(8) char storedKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold the SHA-256 hash of clientKey
    CTRANSPORT_ALIGN(8) char clientSignature[CC_SHA256_DIGEST_LENGTH];  //used in combination with clientKey to generate a clientProof that the server uses to validate the client
    CTRANSPORT_ALIGN(8) char serverSignature[CC_SHA256_DIGEST_LENGTH];  //used to generated a serverSignature the client uses to validate that sent by the server
    CTRANSPORT_ALIGN(8) char clientProof[CC_SHA256_DIGEST_LENGTH];

    char CLIENT_FINAL_MESSAGE[1024] = "\0";
    //char CLIENT_FINAL_MESSAGE_JSON[1024];

    char* CLIENT_FINAL_MESSAGE_JSON = sFirstMessagePtr;
    //CLIENT_FINAL_MESSAGE[0] = '\0';

    //ct_scram_init();

    //fprintf(stderr, "SERVER_FIRST_MESSAGE_BUFFER (%d) = \n\n%s\n\n", strlen(sFirstMessagePtr), sFirstMessagePtr);

    //  Parse the server-first-message json response buffer to see if client-first-message exchange was successful
    //  Note:  Here we are counting on the fact that rdb server json does not send whitespace!!!
    //bool sfmSuccess = false;
    if ((successVal = strstr(sFirstMessagePtr, REQL_SUCCESS_KEY)) != NULL)
    {
        successVal += strlen(REQL_SUCCESS_KEY) + 1;
        //if( *successVal == ':') successVal += 1;
        if (*successVal == '"') successVal += 1;
        if (strncmp(successVal, "true", MIN(4, strlen(successVal))) == 0)
            sfmSuccess = true;
    }
    if (!sfmSuccess)//|| !authValue || authLength < 1 )
    {
        fprintf(stderr, "RDB server-first-message response failure!\n");
        //struct CTError vError = { (CTErrorClass)0,ec.value(),(void*)(ec.message().c_str()) };    //Reql API client functions will generally return ints as errors
        //callback(&vError, r);
        return NULL;
    }

    //  The nonce, salt, and iteration count (ie server-first-message) we need for SCRAM Auth are in the 'authentication' field
    //const char* authKey = "\"authentication\"";
    authValue = strstr(sFirstMessagePtr, REQL_AUTH_KEY);
    //size_t authLength = 0;
    if (authValue)
    {
        authValue += strlen(REQL_AUTH_KEY) + 1;
        //if( *successVal == ':') successVal += 1;
        if (*authValue == '"')
        {
            authValue += 1;
            tmp = (char*)authValue;
            while (*(tmp) != '"') tmp++;
            authLength = (unsigned long)(tmp - authValue);
        }
    }

    
    //char* SCRAM_AUTH_MESSAGE_PTR = (char*)&(SCRAM_AUTH_MESSAGE[0]);
    char* SCRAM_AUTH_MESSAGE_PTR = sFirstMessagePtr + 256;//firstMsgCursor->requestBuffer + 256;

    //void* base64SSPtr = base64SS;
    unsigned long AuthMessageLengthCopy = strlen(SCRAM_AUTH_MESSAGE_PTR);// AuthMessageLength;
    //  Copy the server-first-message UTF8 string to the AuthMessage buffer
    strncat(SCRAM_AUTH_MESSAGE_PTR, authValue, authLength);
    AuthMessageLengthCopy += authLength;
    SCRAM_AUTH_MESSAGE_PTR[AuthMessageLengthCopy] = ',';
    AuthMessageLengthCopy += 1;
    SCRAM_AUTH_MESSAGE_PTR[AuthMessageLengthCopy] = '\0';

    //  Parse the SCRAM server-first-message fo salt, iteration count and nonce
    //  We are looking for r=, s=, i=
    //char delim[] = ",";
    token = strtok(authValue, delim);
    //char * serverNonce = NULL;
    //char * base64Salt = NULL;
    //char * saltIterations = NULL;
    while (token != NULL)
    {
        if (token[0] == 'r')
            serverNonce = &(token[2]);
        else if (token[0] == 's')
            base64Salt = &(token[2]);
        else if (token[0] == 'i')
            saltIterations = &(token[2]);
        token = strtok(NULL, delim);
    }
    assert(serverNonce);
    //Note:  The CLIENT_FINAL_MESSAGE used to be populated below with sprintf, but this was causing crashes on Linux
    //       and so was moved to a memcpy here closer to the serverNonce ptr population
    memcpy(CLIENT_FINAL_MESSAGE, "c=biws,r=", 9);
    memcpy(CLIENT_FINAL_MESSAGE + 9, serverNonce, 40); //We always know the CLIENT + SERVER nonces will be 20 + 20 characters (ie bytes)
    fprintf(stderr, "CLIENT_FINAL_MESSAGE = \n\n%s\n\n", CLIENT_FINAL_MESSAGE);

    assert(base64Salt);
    assert(saltIterations);


    fprintf(stderr, "serverNonce = %s\n", serverNonce);
    fprintf(stderr, "base64Salt = %s\n", base64Salt);
    fprintf(stderr, "saltIterations = %d\n", atoi(saltIterations));
    fprintf(stderr, "password = %s\n", password);

    
    //  While the nonce(s) is (are) already in ASCII encoding sans , and " characters,
    //  the salt is base64 encoded, so we have to decode it to UTF8
   // size_t saltLength = 0;
    salt = (char*)cr_base64_to_utf8(base64Salt, strlen(base64Salt), &saltLength);
    // assert( salt && saltLength > 0 );

     //  Run iterative hmac algorithm n times and concatenate the result in an XOR buffer in order to salt the password
     //  SaltedPassword  := Hi(Normalize(password), salt, i)
     //  Note:  Not really any way to tell if ct_scram_... fails
    int si = atoi(saltIterations);
    memset(saltedPassword, 0, CC_SHA256_DIGEST_LENGTH);
    ct_scram_salt_password(password, strlen(password), salt, saltLength, si, saltedPassword);
    //fprintf(stderr, "salted password = %.*s\n", CC_SHA256_DIGEST_LENGTH, saltedPassword);

    //  Generate Client and Server Key buffers
    //  by encrypting the verbatim strings "Client Key" and "Server Key, respectively,
    //  using keyed HMAC SHA 256 with the salted password as the secret key
    //      ClientKey       := HMAC(SaltedPassword, "Client Key")
    //      ServerKey       := HMAC(SaltedPassword, "Server Key")
    ct_scram_hmac(saltedPassword, CC_SHA256_DIGEST_LENGTH, REQL_CLIENT_KEY, strlen(REQL_CLIENT_KEY), clientKey);
    ct_scram_hmac(saltedPassword, CC_SHA256_DIGEST_LENGTH, REQL_SERVER_KEY, strlen(REQL_SERVER_KEY), serverKey);
    //fprintf(stderr, "clientKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, clientKey);
    //fprintf(stderr, "serverKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, serverKey);

    //  Calculate the "Normal" SHA 256 Hash of the clientKey
    //      storedKey := H(clientKey)
    ct_scram_hash(clientKey, CC_SHA256_DIGEST_LENGTH, storedKey);
    //fprintf(stderr, "storedKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, storedKey);

    //  Populate SCRAM client-final-message (ie channel binding and random nonce WITHOUT client proof)
    //  E.g.  "authentication": "c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ="
    //  Note:  The CLIENT_FINAL_MESSAGE used to be populated here with sprintf, but this was causing crashes on Linux
    //         and so was moved to a memcpy closer to the serverNonce ptr above
    //sprintf(CLIENT_FINAL_MESSAGE, "c=biws,r=%.*s", serverNonce);
    //fprintf(stderr, "CLIENT_FINAL_MESSAGE = \n\n%s\n\n", CLIENT_FINAL_MESSAGE);

    //Copy the client-final-message (without client proof) to the AuthMessage buffer
    strncat(SCRAM_AUTH_MESSAGE_PTR, CLIENT_FINAL_MESSAGE, strlen(CLIENT_FINAL_MESSAGE));
    AuthMessageLengthCopy += (unsigned long)strlen(CLIENT_FINAL_MESSAGE);
    //fprintf(stderr, "SCRAM_AUTH_MESSAGE = \n\n%s\n\n", SCRAM_AUTH_MESSAGE);

    //Now that we have the complete Auth Message we can use it in the SCRAM procedure
    //to calculate client and server signatures
    // ClientSignature := HMAC(StoredKey, AuthMessage)
    // ServerSignature := HMAC(ServerKey, AuthMessage)
    ct_scram_hmac(storedKey, CC_SHA256_DIGEST_LENGTH, SCRAM_AUTH_MESSAGE_PTR, AuthMessageLengthCopy, clientSignature);
    ct_scram_hmac(serverKey, CC_SHA256_DIGEST_LENGTH, SCRAM_AUTH_MESSAGE_PTR, AuthMessageLengthCopy, serverSignature);

    //And finally we calculate the client proof to put in the client-final message
    //ClientProof     := ClientKey XOR ClientSignature
    for (charIndex = 0; charIndex < CC_SHA256_DIGEST_LENGTH; charIndex++)
        clientProof[charIndex] = (clientKey[charIndex] ^ clientSignature[charIndex]);

    //The Client Proof Bytes need to be Base64 encoded
    base64ProofLength = 0;
    base64Proof = cr_utf8_to_base64(clientProof, CC_SHA256_DIGEST_LENGTH, 0, &base64ProofLength);
    assert(base64Proof && base64ProofLength > 0);

    //The Client Proof Bytes need to be Base64 encoded
    base64SSLength = 0;

    //char* base64SSPtr = (char*)base64SS;
    //char** base64SSPtrPtr = &base64SSPtr;
    //base64SS = va_utf8_to_base64(serverSignature, CC_SHA256_DIGEST_LENGTH, 0, &base64SSLength);
    //bc of lambda bs, we will temporarily store base64SS in the connection->responseBufferQueue
    base64SS = cr_utf8_to_base64(serverSignature, CC_SHA256_DIGEST_LENGTH, 0, &base64SSLength);

    assert(base64SS && base64SSLength > 0);
    fprintf(stderr, "Base64 Server Signature:  %s\n\n", (char*)base64SS);

    //Add the client proof to the end of the client-final-message
    sprintf(CLIENT_FINAL_MESSAGE_JSON, "{\"authentication\":\"%s,p=%.*s\"}\0", CLIENT_FINAL_MESSAGE, (int)base64ProofLength, (char*)base64Proof);
    fprintf(stderr, "CLIENT_FINAL_MESSAGE_JSON = \n\n%s\n\n", CLIENT_FINAL_MESSAGE_JSON);

    //Send the client-final-message wrapped in json
    //CLIENT_FINAL_MESSAGE_JSON[strlen(CLIENT_FINAL_MESSAGE_JSON)] = '\0';
    //VTSend(r, CLIENT_FINAL_MESSAGE_JSON, strlen(CLIENT_FINAL_MESSAGE_JSON) + 1);  //Note:  JSON always needs the extra null character to determine EOF!!!

    //send the second message
    /*
#ifdef VTRANSPORT_ASIO_OPENSSL
    if (r->isSecure())
        asio::async_write(*(r->secureSocket()), asio::buffer(CLIENT_FINAL_MESSAGE_JSON, strlen(CLIENT_FINAL_MESSAGE_JSON) + 1), writeMessage5CompletionBlock);
    else
#endif
        asio::async_write(*(r->socket()), asio::buffer(CLIENT_FINAL_MESSAGE_JSON, strlen(CLIENT_FINAL_MESSAGE_JSON) + 1), writeMessage5CompletionBlock);
    */

    //free Base64 <-> UTF8 string conversion allocations
    free(salt);
    free(base64Proof);
    //ct_scram_cleanup();
    
    return base64SS;

}



CTRANSPORT_API CTRANSPORT_INLINE int CTReQLHandshakeProcessFinalMessageResponse(char* sFinalMessagePtr, size_t sFinalMessageLen, char* base64SS)
{
    //fprintf(stderr, "CTReQLHandshakeProcessFinalMessageResponse Buffer = \n\n%s\n\n", sFinalMessagePtr);\

    //Wait for SERVER_FINAL_MESSAGE
    char* authValue = NULL;
    char* tmp = NULL;
    unsigned long readLength = 1024;

    readLength = 1024;
    //char* sFinalMessagePtr = (char*)SERVER_FINAL_MESSAGE_BUFFER.data();// (char*)VTRecv(r, SERVER_FINAL_MESSAGE_JSON, &readLength);
    fprintf(stderr, "SERVER_FINAL_MESSAGE_JSON = \n\n%s\n\n", sFinalMessagePtr);

    //Validate Server Signature
    unsigned long authLength = 0;
    if ((authValue = strstr(sFinalMessagePtr, REQL_AUTH_KEY)) != NULL)
    {
        authValue += strlen(REQL_AUTH_KEY) + 1;
        //if( *successVal == ':') successVal += 1;
        if (*authValue == '"')
        {
            authValue += 1;
            tmp = (char*)authValue;
            while (*(tmp) != '"') tmp++;
            authLength = (unsigned long)(tmp - authValue);
        }

        if (*authValue == 'v' && *(authValue + 1) == '=')
        {
            authValue += 2;
            authLength -= 2;
        }
        else
            authValue = NULL;

        //Compare server sent signature to client generated server signature
        //char* base64Signature = *base64SSPtr;
        if (authValue && strncmp(base64SS, authValue, authLength) != 0)
        {
            fprintf(stderr, "Failed to authenticate server signature: %s\n", authValue);
            //vError.errClass = VTRuntimeErrorClass;
            //vError.id = VTSASLHandshakeError;
            //vError.description = ...
            return CTSASLHandshakeError;
        }
    }
    else//if( !authValue )
    {
        fprintf(stderr, "Failed to retrieve server signature from SCRAM server-final-message.\n");
        //vError.errClass = VTRuntimeErrorClass;
        //vError.id = VTSASLHandshakeError;
        //vError.description = ...
        return CTSASLHandshakeError;
    }

    free(base64SS);
    base64SS = NULL;

    return CTSuccess;
}