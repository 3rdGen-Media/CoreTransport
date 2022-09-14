/***
**
*    Core Transport -- HappyEyeballs
*
*    Description:  CTransport (CTConnection) C API Example
*
*    @author: Joe Moulton
*    Copyright 2020 Abstract Embedded LLC DBA 3rdGen Multimedia
**
***/

#include "CTAppInterface.h"
#include "CTDeviceInput.h"

//Globals
static int httpRequestCount = 0;
static int reqlQueryCount = 0;

//Predefine ReQL query to use with CTransport C Style API (because message support is not provided at the CTransport/C level)
char* gamestateTableQuery = "[1,[15,[[14,[\"GameState\"]],\"Scene\"]]]\0";

char* httpHeaderLengthCallback(struct CTCursor* pCursor, char* buffer, unsigned long length)
{
    //the purpose of this function is to return the end of the header in the
    //connection's tcp stream to CoreTransport decrypt/recv thread
    char* endOfHeader = strstr(buffer, "\r\n\r\n");
    
    if (!endOfHeader)
    {
        pCursor->headerLength = 0;
        pCursor->contentLength = 0;
        return NULL;
    }
    //set ptr to end of http header
    endOfHeader += sizeof("\r\n\r\n") - 1;

    //The client *must* set the cursor's contentLength property
    //to aid CoreTransport in knowing when to stop reading from the socket
    if( pCursor->queryToken%3 == 0)
        pCursor->contentLength = 1641845;
    else if( pCursor->queryToken%3 == 1)
        pCursor->contentLength = 198744;
    else if( pCursor->queryToken%3 == 2)
        pCursor->contentLength = 572443;

    //pCursor->contentLength = 1256;
    //The cursor headerLength is calculated as follows after this function returns
    //cursor->headerLength = endOfHeader - buffer;
    return endOfHeader;
}

CTCursorCompletionClosure httpResponseClosure = ^void(CTError * err, CTCursor* cursor)
{
    fprintf(stderr, "httpResponseCallback (%d) header:  \n\n%.*s\n\n", (int)cursor->queryToken, (int)cursor->headerLength, cursor->requestBuffer);
    CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
    CTCursorCloseFile(cursor);
};


uint64_t CTCursorRunQueryOnQueue(CTCursor* cursor, uint64_t queryToken)
{
    unsigned long queryMessageLength;
    ReqlQueryMessageHeader* queryHeader = (ReqlQueryMessageHeader*)cursor->requestBuffer;

    //unsigned long requestHeaderLength, requestLength;

    //get the buffer for sending with async iocp
    //char * queryBuffer = cursor->requestBuffer;
    char* queryBuffer = cursor->requestBuffer + sizeof(ReqlQueryMessageHeader);
    char* baseBuffer = cursor->requestBuffer;

    //calulcate the request length
    queryMessageLength = strlen(queryBuffer);//requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

    //Populate the network message buffer with a header and the serialized query JSON string
    queryHeader->token = queryToken;//(conn->queryCount)++;//conn->token;
    queryHeader->length = (int32_t)queryMessageLength;

    //print the request for debuggin
    //printf("CXURLSendRequestWithTokenOnQueue::request (%d) = %.*s\n", (int)queryMessageLength, (int)queryMessageLength, baseBuffer);
    printf("CTCursorRunQueryOnQueue::queryBuffer (%d) = %.*s", (int)queryMessageLength, (int)queryMessageLength, cursor->requestBuffer + sizeof(ReqlQueryMessageHeader));
    queryMessageLength += sizeof(ReqlQueryMessageHeader);// + queryMessageLength;

    //cursor->overlappedResponse.cursor = cursor;
    cursor->conn = &_reqlConn;
    cursor->queryToken = queryToken;

    //Send the HTTP(S) request
    return CTCursorSendOnQueue(cursor, (char**)&baseBuffer, queryMessageLength);//, &overlappedResponse);
}


uint64_t CTCursorSendRequestOnQueue(CTCursor * cursor, uint64_t requestToken)
{
    //a string/stream for serializing the json query to char*
    unsigned long requestLength;

    //get the buffer for sending with async iocp
    char * requestBuffer = cursor->requestBuffer;
    char * baseBuffer = requestBuffer;
    
    //calculate the request length
    requestLength = strlen(requestBuffer);//requestBuffer-baseBuffer;//queryMsgBuffer - queryBuffer - sizeof(ReqlQueryMessageHeader);// (int32_t)strlen(queryMsgBuffer);// jsonQueryStr.length();

    //print the request for debuggin
    fprintf(stderr, "CTCursorSendRequestOnQueue::request (%d) = %.*s\n", (int)requestLength, (int)requestLength, baseBuffer);

    //cursor->overlappedResponse.cursor = cursor;
    cursor->conn = &_httpConn;
    cursor->queryToken = requestToken;

    //Send the HTTP(S) request
    return CTCursorSendOnQueue(cursor, (char **)&baseBuffer, requestLength);//, &overlappedResponse);
}

void createCursorResponseBuffers(CTCursor* cursor, const char* filepath)
{
    if (!filepath)
        filepath = "map.jpg\0";
    cursor->file.path = (char*)filepath;

    //file path to open
    CTCursorCreateMapFileW(cursor, cursor->file.path, ct_system_allocation_granularity() * 256UL);

    memset(cursor->file.buffer, 0, ct_system_allocation_granularity() * 256UL);
    
    //_cursor->file.buffer = _cursor->requestBuffer;

#ifdef _DEBUG
    assert(cursor->file.buffer);
#endif
}
void sendHTTPRequest(CTCursor* cursor)
{
    int requestIndex;

    //send buffer ptr/length
    char* queryBuffer;// = _gQueryBuffer;
    unsigned long queryStrLength;

    //send request
    char GET_REQUEST[1024] = "\0";
    char * imgName = "wood.png";

    if(httpRequestCount % 3 == 0)
        imgName = "wood.png";
    else if(httpRequestCount % 3 == 1)
        imgName = "brickwall.jpg";
    else if(httpRequestCount % 3 == 2)
        imgName = "brickwall_normal.jpg";

    //char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";
    sprintf(GET_REQUEST, "GET /img/textures/%s HTTP/1.1\r\nHost: 3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n", imgName);
    //GET_REQUEST = "GET /img/textures/wood.png HTTP/1.1\r\nHost: 3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";

    //file path to open
#ifdef _WIN32
    char filepath[1024] = "C:\\3rdGen\\CoreTransport\\bin\\img\\http\0";
#elif defined(__APPLE__)
    char filepath[1024] = "\0";

    //get path to documents dir
    const char *home = getenv("HOME");
    strcat(filepath, home);
    strcat(filepath, "/Documents/http\0");
#else
    char filepath[1024] = "/home/jmoulton/3rdGen/CoreTransport/bin/img/http\0";
#endif
    
    memset(cursor, 0, sizeof(CTCursor));

    //itoa(httpRequestCount, filepath + strlen(filepath), 10);
    snprintf(filepath + strlen(filepath), strlen(filepath), "%d", httpRequestCount);
    
    if(httpRequestCount % 3 == 0)
        strcat(filepath, ".png");
    else
        strcat(filepath, ".jpg");
        
    httpRequestCount++;

    //CoreTransport provides support for each cursor to read responses into memory mapped files
    //however, if this is not desired, simply cursor's file.buffer slot to your own memory
    createCursorResponseBuffers(cursor, filepath);

    assert(cursor->file.buffer);

    //cursor->file.buffer = cursor->requestBuffer;

    cursor->headerLengthCallback =  httpHeaderLengthCallback;
    cursor->responseCallback     =  httpResponseClosure;

    //printf("_conn.sslContextRef->Sizes.cbHeader = %d\n", _conn.sslContext->Sizes.cbHeader);
    //printf("_conn.sslContextRef->Sizes.cbTrailer = %d\n", _conn.sslContext->Sizes.cbTrailer);

    for (requestIndex = 0; requestIndex < 1; requestIndex++)
    {
        queryStrLength = strlen(GET_REQUEST);
        queryBuffer = cursor->requestBuffer;
        memcpy(queryBuffer, GET_REQUEST, queryStrLength);
        queryBuffer[queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminato
        fprintf(stderr, "sendHTTPRequest(): \n%s\n", queryBuffer);
        //CTSend(&_httpConn, queryBuffer, queryStrLength);
        CTCursorSendRequestOnQueue(cursor, _httpConn.queryCount);
    }
}

//int _httpConnectionClosure(CTError* err, CTConnection* conn) {
CTConnectionClosure _httpConnectionClosure = ^int(CTError * err, CTConnection * conn) {
    
    int i = 0;
    //TO DO:  Parse error status
    assert(err->id == CTSuccess);

    //Copy CTConnection object memory from coroutine stack
    //to appplication memory (or the connection will go out of scope when this funciton exits)
    //FYI, do not use the pointer memory after this!!!
    _httpConn = *conn;
    _httpConn.response_overlap_buffer_length = 0;

    fprintf(stderr, "HTTP Connection Success\n");
    
    //for(i=0; i<15; i++)
    //    sendHTTPRequest(CTGetNextPoolCursor());

    return err->id;
};

char* reqlHeaderLengthCallback(struct CTCursor* cursor, char* buffer, unsigned long length)
{
    //the purpose of this function is to return the end of the header in the
    //connection's tcp stream to CoreTransport decrypt/recv thread
    char* endOfHeader = buffer + sizeof(ReqlQueryMessageHeader);

    //The client *must* set the cursor's contentLength property
    //to aid CoreTransport in knowing when to stop reading from the socket
    cursor->contentLength = ((ReqlQueryMessageHeader*)buffer)->length;
    uint64_t token = ((ReqlQueryMessageHeader*)buffer)->token;
    assert(token == cursor->queryToken);

    //The cursor headerLength is calculated as follows after this function returns
    //cursor->headerLength = endOfHeader - buffer;
    return endOfHeader;
}

CTCursorCompletionClosure reqlResponseClosure = ^void(CTError * err, CTCursor * cursor)
{
    fprintf(stderr, "reqlCompletionCallback body:  \n\n%.*s\n\n", (int)cursor->contentLength, cursor->file.buffer);
    //fprintf(stderr, "reqlResponseClosure (%d) header:  \n\n%.*s\n\n", (int)cursor->queryToken, (int)cursor->headerLength, cursor->requestBuffer);
    //CTCursorCloseMappingWithSize(cursor, cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
    //CTCursorCloseFile(cursor);
};

void sendReqlQuery(CTCursor* cursor)
{
    int queryIndex;

    //send buffer ptr/length
    char* queryBuffer;
    unsigned long queryStrLength;

    //send request
    //char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";
    //char GET_REQUEST[1024] = "GET /index.html HTTP/1.1\r\nHost: example.com\r\nUser-Agent: CoreTransport\r\nAccept: */*\r\n\r\n\0";

    //file path to open
#ifdef _WIN32
    char filepath[1024] = "C:\\3rdGen\\CoreTransport\\Samples\\HappyEyeballs\\reql\0";
#elif defined(__APPLE__)
    char filepath[1024] = "\0";

    //get path to documents dir
    const char *home = getenv("HOME");
    strcat(filepath, home);
    strcat(filepath, "/Documents/reql\0");
#else
    char filepath[1024] = "/home/jmoulton/3rdGen/CoreTransport/bin/img/reql\0";
#endif

    memset(cursor, 0, sizeof(CTCursor));

    //_itoa(httpRequestCount, filepath + strlen(filepath), 10);
    snprintf(filepath + strlen(filepath), strlen(filepath), "%d\0", reqlQueryCount);

    strcat(filepath, ".txt");
    reqlQueryCount++;

    //CoreTransport provides support for each cursor to read responses into memory mapped files
    //however, if this is not desired, simply cursor's file.buffer slot to your own memory
    //createCursorResponseBuffers(cursor, filepath);
    cursor->file.buffer = cursor->requestBuffer;
    
    assert(cursor->file.buffer);
    //if (!cursor->file.buffer)
    //    return;

    cursor->headerLengthCallback = reqlHeaderLengthCallback;
    cursor->responseCallback = reqlResponseClosure;

    //printf("_conn.sslContextRef->Sizes.cbHeader = %d\n", _conn.sslContext->Sizes.cbHeader);
    //printf("_conn.sslContextRef->Sizes.cbTrailer = %d\n", _conn.sslContext->Sizes.cbTrailer);

    //send query
    for (queryIndex = 0; queryIndex < 1; queryIndex++)
    {
        queryStrLength = (unsigned long)strlen(gamestateTableQuery);
        queryBuffer = cursor->requestBuffer;
        memcpy(queryBuffer + sizeof(ReqlQueryMessageHeader), gamestateTableQuery, queryStrLength);
        queryBuffer[sizeof(ReqlQueryMessageHeader) + queryStrLength] = '\0';  //It is critical for SSL encryption that the emessage be capped with null terminator
        printf("queryBuffer = %s\n", queryBuffer + sizeof(ReqlQueryMessageHeader));

        //CTCursorSendRequestOnQueue(cursor, _reqlConn.queryCount++);

        CTCursorRunQueryOnQueue(cursor, _reqlConn.queryCount);
        ////CTReQLRunQueryOnQueue(&_reqlConn, (const char**)&queryBuffer, queryStrLength, _reqlConn.queryCount++);
    }

}


//int _reqlHandshakeClosure(CTError* err, ReqlConnection* conn) {
CTConnectionClosure _reqlHandshakeClosure = ^ int(CTError * err, CTConnection * conn) {

    assert(conn == &_reqlConn);
    if (err->id == 0 && conn) { /*_reqlConn = conn;*/ }
    else
    {
        printf("CTReQLAsyncLHandshake failed!\n");
        CTCloseConnection(&_reqlConn);
        CTSSLCleanup();
        err->id = CTSASLHandshakeError;
        return err->id;
    }
    //createConnectionResponsBufferQueue(_reqlConn);
    printf("CTReQLAsyncLHandshake Success\n");

    //for(int i=0; i<30; i++)
    //    sendReqlQuery(CTGetNextPoolCursor());

    return err->id;
};

static int rCount = 0;
//int _reqlConnectionClosure(CTError* err, CTConnection* conn) {
CTConnectionClosure _reqlConnectionClosure = ^int(CTError * err, CTConnection * conn) {

    //status
    int status;

    rCount++;
    fprintf(stderr, "rcount = %d\n", rCount);
    //TO DO:  Parse error status
    if (err->id == 0 && conn) { /*_reqlConn = conn;*/ }
    else
    {
        printf("ReQL Connection failed!\n");
        CTCloseConnection(&_reqlConn);
        CTSSLCleanup();
        return err->id;
    }
    //Copy CTConnection object memory from coroutine stack
    //to application memory (or the connection will go out of scope when this funciton exits)
    //FYI, do not use the pointer memory after this!!!
    _reqlConn = *conn;
    _reqlConn.response_overlap_buffer_length = 0;

    fprintf(stderr, "ReQL Connection Success\n");

    //  Now Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
    //  It is always safe to use Async ReQL Handshake after connection
    if ((1))
    {
        if ((status = CTReQLAsyncHandshake(&_reqlConn, _reqlConn.target, _reqlHandshakeClosure)) != CTSuccess)
        {
            assert(1 == 0);
        }
    }
    else if ( /* DISABLES CODE */ ((status = CTReQLHandshake(&_reqlConn, _reqlConn.target)) != CTSuccess))
    {
        printf("CTReQLSASLHandshake failed!\n");
        CTCloseConnection(&_reqlConn);
        CTSSLCleanup();
        err->id = CTSASLHandshakeError;
        return err->id;
    }

    printf("REQL Connection Success\n");
    return err->id;
};


//must return void* in order to use with GCD dispatch_async_f
static void* app_event_loop(void* opaqueQueue)
{
    fprintf(stderr, "\ninit app_event_loop\n");
#ifdef CR_TARGET_WIN32

#elif defined(__APPLE__)
    int kqueue = (int)(uint64_t)opaqueQueue;
     while (1) {
         struct kevent kev;
         CTProcessEvent kev_type;
         
         //idle until we receive kevent from our kqueue
         kev_type = (CTProcessEvent)ct_event_queue_wait_with_timeout(kqueue, &kev, EVFILT_USER, CTProcessEvent_Timeout, CTProcessEvent_OutOfRange, CTProcessEvent_Init, CTProcessEvent_Idle, UINT_MAX);
         
         /*
         if( kev_type == ctevent_platform_event  )
         {
             cr_platform_event_msg cgEvent = (cr_platform_event_msg)(kev.udata);
             handle_platform_event(cgEvent);
         }
         else if( kev_type == CTProcessEvent_MainWindowChanged )
         {
             //CGWindowID windowID = (CGWindowID)(kev.udata);
             //cr_mainWindow = windowID;
             //fprintf(stderr, "\ncrevent_main_window_changed %lld\n", cr_mainWindow);
             assert(1==0);
         }
         else*/ if( kev_type == CTProcessEvent_Init)
         {
             //assert(1==0);
             //init_crgc_views();      //configure crgc_view options as desired before creating platform window backed by an accelerated graphics context
             //create_crgc_views();    //create platform window and graphics context
         }
         /*
         else if( kev_type == ctevent_register_view )
         {
             fprintf(stderr, "\ncrevent_register_view\n");
             
             //get the pointer to the crgc_view that lives in Cocoa Land
             crgc_view * view = (crgc_view*)(kev.udata);
             
             //start the render thread for the view
             create_crgc_view_display_loop(view);
             
             //crgc_view_setup(view);
             //view->displaySyncCallback = crgc_view_render;

             
             //id (*objc_ClassSelector)(Class class, SEL _cmd) = (void*)objc_msgSend;//objc_msgSend(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"))
             //id (*objc_InstanceSelector2)(id self, SEL _cmd, void* param1, void* param2) = (void*)objc_msgSend;//objc_msgSend(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"))
             //id CRApp = objc_ClassSelector(objc_getClass("CRApplication"), sel_registerName("sharedInstance"));
             //objc_InstanceSelector2(CRApp, sel_getUid("setView:displaySyncCallback:"), view, crgc_view_render);
             
             //Pre OSX 10.15
             //((id (*)(id, SEL, void*))objc_msgSend)(objc_msgSend(objc_getClass("CRMetalInterface"), sel_registerName("sharedInstance")), sel_getUid("displayLoop:"), view);
             
         }
         else if( kev_type == ctevent_graphics )
         {
             fprintf(stderr, "ctevent_graphics\n");
             _paintingTexture = *((GLuint*)kev.udata);
             //cr_event_image_load(
             
         }
         */
         else if( kev_type == CTProcessEvent_Exit)
         {
             //assert(1==0);
             //CRCleanup();
         
            //dispatch event to be picked up by Cocoa NSApplication RunLoop notifying it we are ready to terminate the application
            //struct kevent kev;
            //EV_SET(&kev, CTPlatformEvent_Exit, EVFILT_USER, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_FFCOPY|NOTE_TRIGGER|0x1, 0, NULL);
            //kevent(CTPlatformEventQueue.kq, &kev, 1, NULL, 0, NULL);
             
             //[[CTApplcation sharedApplication] replyToApplicationShouldTerminate:YES];

             dispatch_sync(dispatch_get_main_queue(), ^{

                 id (*objc_ClassSelector)(Class class, SEL _cmd) = (void*)objc_msgSend;//objc_msgSend(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"))
                 id (*objc_InstanceSelector1)(id self, SEL _cmd, BOOL param1) = (void*)objc_msgSend;//objc_msgSend(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"))
                 id CTApp = objc_ClassSelector(objc_getClass("CTApplication"), sel_registerName("sharedApplication"));
                 objc_InstanceSelector1(CTApp, sel_getUid("replyToApplicationShouldTerminate:"), YES);

             });
             
         }
         else if( kev_type == CTProcessEvent_Menu)
         {
             assert(1==0);
             //handle_menu_event((crmenu_event)kev.udata);
         }
     }
#endif
     return NULL;
}

void ExitHandler(void)
{
    fprintf(stderr, "Exiting via Exit Handler...\n");

    //pause for leak tracking
    //fscanf(stdin, "c");
}

void StartApplicationEventLoop(void)
{
#ifdef CR_TARGET_WIN32
    
#elif defined(__APPLE__)
    //create kqueue
    CTProcessEventQueue = CTKernelQueueCreate();
    //pthread_mutex_init(&_mutex, NULL);
    
    //register kevents
    for (uint64_t event = CTProcessEvent_Init; event < CTProcessEvent_Idle; ++event) {
        struct kevent kev;
        EV_SET(&kev, event, EVFILT_USER, EV_ADD | EV_ENABLE | EV_CLEAR, 0, 0, NULL);
        kevent(CTProcessEventQueue.kq, &kev, 1, NULL, 0, NULL);
    }
    
    //Create the pthread that will run the kqueue evetnt loop
    //pthread_attr_t attr;
    //struct sched_param sched_param;
    //int sched_policy = SCHED_FIFO;
    //pthread_attr_init(&attr);
    //pthread_attr_setschedpolicy(&attr, sched_policy);
    //sched_param.sched_priority = sched_get_priority_max(sched_policy);
    //pthread_attr_setschedparam(&attr, &sched_param);
    //pthread_create(&_thread, &attr, event_loop_main, (void*)_cgEventQueue);
    //pthread_attr_destroy(&attr);
    
    /***
     *  Launch a concurrent thread pool that will serve as the Main Event Loop in Core Render C-Land
     *  Leaving the main thread to run the Cocoa Obj-C Land NSApplication Run Loop
     *
     *  Its responsibilities include:
     *
     *      --Receiving events from the NSApplication run loop and forwarding them to the appropriate crgc_view window event thread/queue as necessary
     *      --Sending events and notifications back to Cocoa if needed
     *      --Managing the Core Render C-Land Application State
     ***/
    //dispatch_queue_attr_t serialAttr = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_SERIAL, QOS_CLASS_USER_INTERACTIVE, 0);
    dispatch_queue_attr_t concurrentAtt = dispatch_queue_attr_make_with_qos_class(DISPATCH_QUEUE_CONCURRENT, QOS_CLASS_USER_INTERACTIVE, 0);
    
    dispatch_queue_t      processEventDispatchQueue = dispatch_queue_create(kCTProcessEventQueueID, concurrentAtt);
    //dispatch_queue_t      coroEventDispatchQueue = dispatch_queue_create(kCTCoroutineEventQueueID, serialAttr);

    //a neat feature of GCD is that it exposes a hash dictionary for each dispatch_queue_t
    //making GCD a great candidate for cross-platform threading across all platforms
    /*
    for( int viewIndex = 0; viewIndex < NUM_VIEWS; viewIndex++)
    {
        dispatch_queue_set_specific(glView[viewIndex].controlThread, (void*)(glView[viewIndex].window), &(glView[viewIndex]), NULL);
    }
    */
    
    //glView[viewIndex].controlQueue = dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0);
    dispatch_async(processEventDispatchQueue, ^{
        app_event_loop((void*)(uint64_t)CTProcessEventQueue.kq);
    });
    
    /*
    dispatch_async(coroEventDispatchQueue, ^{
       
        //Create a dummy run loop source that will keep the run loop alive
        CFRunLoopSourceContext emptyRunLoopSourceContext = {0};
        CFRunLoopSourceRef dummyRunLoopSource = CFRunLoopSourceCreate(NULL, 0, &emptyRunLoopSourceContext);
        CFRunLoopAddSource( CFRunLoopGetCurrent(), dummyRunLoopSource, kCFRunLoopDefaultMode);

        //Create a mach port to receive keystrokes that will run loop source that will keep the run loop alive
        //CheckAccessibilityPrivileges(); //Check/Prompt user for permissions (so we can get keystrokes)
        //CFMachPortRef tapEventMachPort = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, (1 << kCGEventKeyDown), on_keystroke, NULL);
        //CFRunLoopSourceRef tapEventMachPortRunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tapEventMachPort, 0);
        //CFRunLoopAddSource( CFRunLoopGetCurrent(), tapEventMachPortRunLoopSource, kCFRunLoopDefaultMode);

        //Create an observer that yields to coroutines on the current run loop
        CFRunLoopObserverRef runLoopObserver = CTRoutineCreateRunLoopObserver();

        //Application/Keystroke Event Loop
        CFRunLoopRun();
        
        //Remove and release observer that yields to coroutines on the current run loop
        CFRunLoopRemoveObserver(CFRunLoopGetCurrent(), runLoopObserver, kCFRunLoopDefaultMode);
        CFRelease(runLoopObserver);

        //Remove and release dummy source that keeps run loop alive
        CFRunLoopRemoveSource(CFRunLoopGetCurrent(), dummyRunLoopSource, kCFRunLoopDefaultMode);
        CFRelease(dummyRunLoopSource); //unclear if this needs to be released

        //Remove and release keystroke mach port run loop that keeps run loop alive
        //CFRunLoopRemoveSource(CFRunLoopGetCurrent(), tapEventMachPortRunLoopSource, kCFRunLoopDefaultMode);
               
    });
    */
    
#endif
}


#ifdef __APPLE__
CGEventRef on_keystroke(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *data)
{
    switch (type)
    {
        case kCGEventKeyDown:
        {
#if TARGET_OS_OSX
            int64_t key_code = CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode);
            CGEventFlags flag = CGEventGetFlags(event);
#else
            int64_t key_code;
            CGEventFlags flag;
            assert(1==0);
#endif
            
            bool is_shift = !!(flag & kCGEventFlagMaskShift);
            bool is_cmd = !!(flag & kCGEventFlagMaskCommand);
            bool is_ctrl = !!(flag & kCGEventFlagMaskControl);
            
            const char* key_str = key_code_to_str((int)key_code, is_shift, false);

            char * prefix = "";
            if (is_cmd) prefix = "[cmd] + ";
            if (is_ctrl) prefix = "[ctrl] + ";

            fprintf(stderr, "key: %s%s\n", prefix, key_str);
            
            //Key 'q'
            if( key_code == 12 && (!is_shift && !is_cmd && !is_ctrl) )
            {
                CFRunLoopStop(CFRunLoopGetCurrent());
            }
            else if( key_code == 5 && (!is_shift && !is_cmd && !is_ctrl))  sendHTTPRequest(&CT_APP_CURSORS[httpRequestCount % CT_APP_MAX_INFLIGHT_CURSORS]);
            else if( key_code == 15 && (!is_shift && !is_cmd && !is_ctrl)) sendReqlQuery(CTGetNextPoolCursor());
            break;
        }
        // Right click from a logitech mouse. This is not part of CGEventType >.<
        case 14:
        break;

        default: {
            printf("Unknown type: %d\n", type);
        }
    }

    return event;
}
#else
//Define crossplatform keyboard event loop handler
bool getconchar(KEY_EVENT_RECORD* krec)
{
#ifdef _WIN32
    DWORD cc;
    INPUT_RECORD irec;
    KEY_EVENT_RECORD* eventRecPtr;

    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (h == NULL) return false; // console not found

    for (;;)
    {
        //On Windows, we read console input and then ask if it's key down
        ReadConsoleInput(h, &irec, 1, &cc);
        eventRecPtr = (KEY_EVENT_RECORD*)&(irec.Event);
        if (irec.EventType == KEY_EVENT && eventRecPtr->bKeyDown)//&& ! ((KEY_EVENT_RECORD&)irec.Event).wRepeatCount )
        {
            *krec = *eventRecPtr;
            return true;
        }
    }
#else
    //On Linux we detect a keyboard hit using select and then read the console input
    if (kbhit())
    {
        int r;
        unsigned char c;

        //printf("\nkbhit!\n");
        if ((r = read(0, &c, sizeof(c))) == 1) {
            krec->uChar.AsciiChar = c;
            return true;
        }
    }
    //else fprintf(stderr, "\n!kbhit\n");
#endif
    return false; //future ????
}
#endif


void SystemKeyboardEventLoop(int argc, const char * argv[])
{
#ifdef __APPLE__
    CheckAccessibilityPrivileges(); //Check/Prompt user for permissions (so we can get keystrokes)
#if TARGET_OS_OSX
    //Create a mach port to receive keystrokes that will run loop source that will keep the run loop alive
    CFMachPortRef tapEventMachPort = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionDefault, (1 << kCGEventKeyDown), on_keystroke, NULL);
    CFRunLoopSourceRef tapEventMachPortRunLoopSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, tapEventMachPort, 0);
    CFRunLoopAddSource( CFRunLoopGetCurrent(), tapEventMachPortRunLoopSource, kCFRunLoopDefaultMode);
#else
    //Create a dummy run loop source that will keep the run loop alive
    CFRunLoopSourceContext emptyRunLoopSourceContext = {0};
    CFRunLoopSourceRef dummyRunLoopSource = CFRunLoopSourceCreate(NULL, 0, &emptyRunLoopSourceContext);
    CFRunLoopAddSource( CFRunLoopGetCurrent(), dummyRunLoopSource, kCFRunLoopDefaultMode);
#endif
    
    //Create an observer that yields to coroutines on the current run loop
    CFRunLoopObserverRef runLoopObserver = CTRoutineCreateRunLoopObserver();

    //Application/Keystroke Event Loop
    //StartPlatformEventLoop(argc, argv);
    CFRunLoopRun();
    
    //Remove and release observer that yields to coroutines on the current run loop
    CFRunLoopRemoveObserver(CFRunLoopGetCurrent(), runLoopObserver, kCFRunLoopDefaultMode);
    CFRelease(runLoopObserver);

#if TARGET_OS_OSX
    //Remove and release keystroke mach port run loop that keeps run loop alive
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), tapEventMachPortRunLoopSource, kCFRunLoopDefaultMode);
    //CFRelease(keystrokeMachPortRunLoopSource); //unclear if this needs to be released
#else
    //Remove and release dummy source that keeps run loop alive
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), dummyRunLoopSource, kCFRunLoopDefaultMode);
    CFRelease(dummyRunLoopSource); //unclear if this needs to be released
#endif
    
#else //All other platforms

    //Initialization
#ifndef _WIN32
    //user linux termios to set terminal mode from line read to char read
    set_conio_terminal_mode();
#endif
    
    //Keystroke event loop
    fprintf(stderr, "\nStarting SystemKeyboardEventLoop...\n");
    KEY_EVENT_RECORD key;
    for (;;)
    {
        yield();

        memset(&key, 0, sizeof(KEY_EVENT_RECORD));
        getconchar(&key);
        if (key.uChar.AsciiChar == 'g')
            sendHTTPRequest(CTGetNextPoolCursor());
        else if (key.uChar.AsciiChar == 'r')
            sendReqlQuery(CTGetNextPoolCursor());// &_httpCursor[httpRequestCount % CT_MAX_INFLIGHT_CURSORS]);
        else if (key.uChar.AsciiChar == 'y')
            yield();
        else if (key.uChar.AsciiChar == 'q')
        {
            break;
        }

        yield();
    }
    
#endif
}

void CTPlatformInit(void)
{
    //CTProcessEventQueue = {0, 0};
#ifdef _WIN32
    //cr_mainThread = GetCurrentThread();
    CTMainThreadID = GetCurrentThreadId();
#elif defined(__APPLE__)
#if TARGET_OS_OSX
     //Initialize kernel timing mechanisms for debugging
    //monotonicTimeNanos();

    //We can do some Cocoa initializations early, but don't need to
    //Note that NSApplicationLoad will load default NSApplication class
    //Not your custom NSApplication Subclass w/ custom run loop!!!
    //NSApplicationLoad();

    /***
     *  0 Explicitly
     *
     *  OSX CoreGraphics API requires that we make the process foreground application if we want to
     *  create windows on threads other than main, or we will get an error that RegisterApplication() hasn't been called
     *  Since this is usual startup behavior for User Interactive NSApplication anyway that's fine
     ***/
    //pid_t pid;
    //CGSInitialize();

    ProcessSerialNumber psn = { 0, kCurrentProcess };
    //GetCurrentProcess(&psn);  //this is deprecated, all non-deprecated calls to make other process foreground must route through Cocoa
    /*OSStatus transformStatus = */TransformProcessType(&psn, kProcessTransformToForegroundApplication);
#endif
    RegisterNotificationObservers();
#endif
    StartApplicationEventLoop();
}


int StartPlatformEventLoop(int argc, const char * argv[])
{
    //CoreTransport Gratuitous Client App Event Queue Initialization (not strictly needed for CoreTransport usage)
    CTPlatformInit();

#if defined(__APPLE__) //&& TARGET_IOS
    
#if TARGET_OS_OSX
    atexit(&ExitHandler);
    NSApplicationMain(argc, argv);       //Cocoa Application Event Loop
#else //defined(CR_TARGET_IOS) || defined(CR_TARGET_TVOS)
    
    // Create an @autoreleasepool, using the old-stye API.
    // Note that while NSAutoreleasePool IS deprecated, it still exists
    // in the APIs for a reason, and we leverage that here. In a perfect
    // world we wouldn't have to worry about this, but, remember, this is C.
    
    id (*objc_ClassSelector)(Class class, SEL _cmd) = (void*)objc_msgSend;//objc_msgSend(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"))
    id (*objc_InstanceSelector)(id self, SEL _cmd) = (void*)objc_msgSend;//objc_msgSend(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"))

    //pre OSX 10.15
    //id autoreleasePool = objc_msgSend(objc_msgSend(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"));
    //post OSX 10.15
    id autoreleasePool = objc_InstanceSelector(objc_ClassSelector(objc_getClass("NSAutoreleasePool"), sel_registerName("alloc")), sel_registerName("init"));
    
    // Get a reference to the file's URL
    CFStringRef cfAppClassName = CFStringCreateWithCString(kCFAllocatorDefault, CocoaAppClassName, CFStringGetSystemEncoding());
    CFStringRef cfAppDelegateClassName = CFStringCreateWithCString(kCFAllocatorDefault, CocoaAppDelegateClassName, CFStringGetSystemEncoding());
    UIApplicationMain(argc, (char* _Nullable *)argv, (id)cfAppClassName, (id)cfAppDelegateClassName);
    CFRelease( cfAppClassName );
    CFRelease( cfAppDelegateClassName );
    
    //release all references in the autorelease pool
    objc_InstanceSelector(autoreleasePool, sel_registerName("drain"));
#endif

#else
    SystemKeyboardEventLoop(argc, argv);
#endif
    return 0;
}


#pragma mark -- Main

int main(int argc, const char * argv[])
{
    //Declare Coroutine Bundle Handle
    //Needed for dill global couroutine malloc cleanup on application exit, while just a meaningless int on Win32 platforms
    CTRoutine coroutine_bundle;

    //Declare dedicated Kernel Queues for dequeueing socket connect, read and write operations in our network application
    CTKernelQueue cq, tq, rq;

    //Declare thread handles for thread pools
    CTThread cxThread, txThread, rxThread;

    //Declare Targets (ie specify TCP connection endpoints + options)
    CTTarget httpTarget      = { 0 };
    CTTarget reqlService     = { 0 };
    CTKernelQueue emptyQueue = { 0 };
    
    //Connection & Cursor Pool Initialization
    CTCreateConnectionPool(&(CT_APP_CONNECTIONS[0]), CT_APP_MAX_INFLIGHT_CONNECTIONS);
    CTCreateCursorPool(&(CT_APP_CURSORS[0]), CT_APP_MAX_INFLIGHT_CURSORS);
    
    //Kernel FileDescriptor Event Queue Initialization
    //If cq is *NOT* specified as input to a connection target, [DNS + Connect + TLS Handshake] will occur on the current thread.
    //If cq is specified as input to a connection target, [DNS + Connect] will occur asynchronously on a background thread(pool) and handshake delegated to tq + rq.
    //If tq or rq are not specified, CTransport internal queues will be assigned to the connection (ie async read/write operation operation post connection pipeline via tq + rq is *ALWAYS* mandatory).
    cq = CTKernelQueueCreate();
    tq = CTKernelQueueCreate();
    rq = CTKernelQueueCreate();

    //Initialize Thread Pool for dequeing socket operations on associated kernel queues
    //On Windows platforms we create a thread/pool of threads to associate with each io completion port
    //On BSD platforms we create thread/pool of threads to associate with each kqueue + pipe pair
    cxThread = CTThreadCreate(&cq, CT_Dequeue_Resolve_Connect_Handshake);
    rxThread = CTThreadCreate(&rq, CT_Dequeue_Recv_Decrypt);
    txThread = CTThreadCreate(&tq, CT_Dequeue_Encrypt_Send);

    //TO DO:  check thread failure
    
#ifdef __APPLE__
    CTBundleFilePath("Keys/resolv.conf", "", resolvConfPath);
    CTBundleFilePath("Keys/nsswitch.conf", "", nsswitchConfPath);
    CTBundleFilePath("Keys/3rd-gen.net.der", "", certPath);

    fprintf(stderr, "%s\n", resolvConfPath);
    fprintf(stderr, "%s\n", nsswitchConfPath);
    fprintf(stderr, "%s\n", certPath);
#endif
    
    //Define https connection target
    httpTarget.url.host = (char*)http_server;
    httpTarget.url.port = http_port;
    //httpTarget.proxy.host = (char*)proxy_server;
    //httpTarget.proxy.port = proxy_port;gggg
    httpTarget.ssl.ca = NULL;//(char*)caPath;
    httpTarget.ssl.method = CTSSL_TLS_1_2;
    httpTarget.dns.resconf = (char*)resolvConfPath;
    httpTarget.dns.nssconf = (char*)nsswitchConfPath;
    httpTarget.cq = cq;
    httpTarget.tq = tq;
    httpTarget.rq = rq;

    //Define RethinkDB service connection target
    //We are doing SCRAM authentication over TLS required by RethinkDB
    //For SCHANNEL SSL Cert Verification, the certficate needs to be installed in the system CA trusted authorities store
    //For WolfSSL and SecureTransport Cert Verification, the path to the certificate can be passed
    reqlService.url.host = (char*)rdb_server;
    reqlService.url.port = rdb_port;
    //reqlService.proxy.host = (char*)proxy_server;
    //reqlService.proxy.port = proxy_port;
    reqlService.user = (char*)rdb_user;
    reqlService.password = (char*)rdb_pass;
    reqlService.ssl.ca = (char*)certPath;
    reqlService.ssl.method = CTSSL_TLS_1_2;
    reqlService.dns.resconf = (char*)resolvConfPath;
    reqlService.dns.nssconf = (char*)nsswitchConfPath;
    reqlService.cq = cq;
    reqlService.tq = tq;
    reqlService.rq = rq;

    //Demonstrate CTransport C API Connection (No JSON support)
    coroutine_bundle = CTransport.connect(&reqlService, _reqlConnectionClosure);
    coroutine_bundle = CTransport.connect(&httpTarget,  _httpConnectionClosure);
    
    

    Sleep(2000);
    for (int i = 0; i < 15; i++)
        sendHTTPRequest(CTGetNextPoolCursor());
    for (int i = 0; i < 30; i++)
        sendReqlQuery(CTGetNextPoolCursor());
    for (int i = 0; i < 15; i++)
        sendReqlQuery(CTGetNextPoolCursor());
    for (int i = 0; i < 15; i++)
        sendHTTPRequest(CTGetNextPoolCursor());
    

   // for (int i = 0; i < 15; i++)
    //    sendReqlQuery(CTGetNextPoolCursor());
    //Start a runloop on the main thread for the duration of the application
    //SystemKeyboardEventLoop(argc, argv);
    StartPlatformEventLoop(argc, argv);

    //TO DO:  Wait for asynchronous threads to shutdown

    //Clean up socket connections (Note: closing a socket will remove all associated kevents on kqueues)
    CTCloseConnection(&_httpConn);
    CTCloseConnection(&_reqlConn);

    //Clean Up Global/Thread Auth Memory
    ca_scram_cleanup();

    //Clean Up Global SSL Memory
    CTSSLCleanup();

    //Cleanup Thread Handles
    CTThreadClose(&cxThread);  //stop new connections first
    CTThreadClose(&txThread);  //stop sending next
    CTThreadClose(&rxThread);  //stop receiving last

    //close the dill coroutine bundle to clean up dill mallocs
    CTRoutineClose(coroutine_bundle);

    fprintf(stderr, "Exiting via Main...\n");
    
    //pause for leaks
    //fscanf(stdin, "c");
    
    return 0;
}

