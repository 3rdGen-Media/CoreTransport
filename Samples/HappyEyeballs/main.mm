/***
**  
*	Core Transport -- HappyEyeballs
*
*	Description:   CXTransport (CXConnection) C++ API Example
*
*	@author: Joe Moulton
*	Copyright 2020 Abstract Embedded LLC DBA 3rdGen Multimedia
**
***/

#include "CTAppInterface.h"
#include "CTDeviceInput.h"

void LoadGameStateQuery()
{
	//Send the ReQL Query to find some URLs to download
	//Demonstrate issue of query and callback lambda using CXReQL API
	auto queryCallback = [&] (CTError * error, std::shared_ptr<CXCursor> cxCursor) {

		fprintf(stderr, "LoadGameState response:  \n\n%.*s\n\n", (int)cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);
		CTCursorCloseMappingWithSize(cxCursor->_cursor, cxCursor->_cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
		CTCursorCloseFile(cxCursor->_cursor);
	};
	CXReQL.db("GameState").table("Scene").get("BasicCharacterTest").run(_reqlCXConn, NULL, queryCallback);
}

static int httpsRequestCount = 0;
void SendHTTPRequest()
{
    //file path to open
#ifdef _WIN32
    char filepath[1024] = "C:\\3rdGen\\CoreTransport\\bin\\img\\http\0";
#elif defined(__APPLE__)
    //char filepath[1024]= "/Users/jmoulton/Pictures/http\0";
    //copy documents dir to asset paths
    char filepath[1024] = "\0";

    //get path to documents dir
    const char *home = getenv("HOME");
    strcat(filepath, home);
    strcat(filepath, "/Documents/http\0");
    //CTFilePathInDocumentsDir("http\0", filepath);
#else
    char filepath[1024] = "/home/jmoulton/3rdGen/CoreTransport/bin/img/http\0";
#endif


	char* urlPath = "/index.html";

	if (httpsRequestCount % 3 == 0)
		urlPath = "/img/textures/wood.png";
	else if (httpsRequestCount % 3 == 1)
		urlPath = "/img/textures/brickwall.jpg";
	else if (httpsRequestCount % 3 == 2)
		urlPath = "/img/textures/brickwall_normal.jpg";

	//_itoa(httpsRequestCount, filepath + 5, 10);
	snprintf(filepath + strlen(filepath), strlen(filepath), "%d", httpsRequestCount);

	//strcat(filepath, ".txt");
	
	if (httpsRequestCount % 3 == 0)
		strcat(filepath, ".png");
	else
		strcat(filepath, ".jpg");
	
	httpsRequestCount++;
	
	//Create a CXTransport API C++ CXURLRequest
	//std::shared_ptr<CXURLRequest> getRequest = CXURL.GET("/index.html", filepath);
	std::shared_ptr<CXURLRequest> getRequest = CXURL.GET(urlPath, filepath);

	getRequest->setValueForHTTPHeaderField("Host", "3rdgen-sandbox-html-resources.s3.us-west-1.amazonaws.com:443");
	//Add some HTTP headers to the CXURLRequest
	getRequest->setValueForHTTPHeaderField("Accept", "*/*");

	
	//Define the response callback to return response buffers using CXTransport CXCursor object returned via a Lambda Closure
	auto requestCallback = [&] (CTError * error, std::shared_ptr<CXCursor> cxCursor) {

		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorMapFileR(cursor);
		fprintf(stderr, "SendHTTPRequest response header:  %.*s\n", (int)cxCursor->_cursor->headerLength, cxCursor->_cursor->requestBuffer);//%.*s\n", cursor->_cursor.length - sizeof(ReqlQueryMessageHeader), (char*)(cursor->_cursor.header) + sizeof(ReqlQueryMessageHeader)); return;  
		fprintf(stderr, "SendHTTPRequest response body:  %.*s\n", (int)cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));

		CTCursorCloseMappingWithSize(cxCursor->_cursor, cxCursor->_cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
		CTCursorCloseFile(cxCursor->_cursor);
	};

	//Pass the CXConnection and the lambda to populate the request buffer and asynchronously send it on the CXConnection
	//The lambda will be executed so the code calling the request can interact with the asynchronous response buffers
	getRequest->send(_httpCXConn, requestCallback);
	
}

int _cxURLConnectionClosure(CTError* err, CXConnection* conn) {
	//CXConnectionClosure _cxURLConnectionClosure = ^ int(CTError * err, CXConnection * conn) {
	if (err->id == CTSuccess && conn)
	{
		_httpCXConn = conn;
	}
	else { assert(1 == 0); } //process errors

	fprintf(stderr, "HTTP Connection Success\n");

	for (int i = 0; i < 30; i++)
		SendHTTPRequest();

	return err->id;
};

int _cxReQLConnectionClosure(CTError* err, CXConnection* conn) {
	//CTConnectionClosure _cxReQLConnectionClosure = ^ int(CTError * err, CTConnection * conn) {

	if (err->id == CTSuccess && conn)
	{
		_reqlCXConn = conn;
	}
	else { assert(1 == 0); } //process errors

	fprintf(stderr, "ReQL Connection Success\n");

	for (int i = 0; i < 30; i++)
        LoadGameStateQuery();

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
             //CTCleanup();
         
            //dispatch event to be picked up by Cocoa NSApplication RunLoop notifying it we are ready to terminate the application
            //struct kevent kev;
            //EV_SET(&kev, CTPlatformEvent_Exit, EVFILT_USER, EV_ADD | EV_ENABLE | EV_ONESHOT, NOTE_FFCOPY|NOTE_TRIGGER|0x1, 0, NULL);
            //kevent(CTPlatformEventQueue.kq, &kev, 1, NULL, 0, NULL);
                          
             dispatch_sync(dispatch_get_main_queue(), ^{
                 [CTApplication.sharedInstance replyToApplicationShouldTerminate:YES];
             });
         }
         else if( kev_type == CTProcessEvent_Menu)
         {
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
        app_event_loop((void*)CTProcessEventQueue.kq);
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
    NSString * appClassName;
    NSString * appDelegateClassName;
    @autoreleasepool {
        // Setup code that might create autoreleased objects goes here.
        appClassName = [NSString stringWithUTF8String:CocoaAppClassName];
        appDelegateClassName = [NSString stringWithUTF8String:CocoaAppDelegateClassName];
    }
    return UIApplicationMain(argc, (char* _Nullable*)argv, appClassName, appDelegateClassName);
#endif
#endif
    return -1;
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
            else if( key_code == 5 && (!is_shift && !is_cmd && !is_ctrl))
                SendHTTPRequest();
            else if( key_code == 15 && (!is_shift && !is_cmd && !is_ctrl))
                LoadGameStateQuery();

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
#ifndef _WIN32	
	//user linux termios to set terminal mode from line read to char read
	set_conio_terminal_mode();
#endif
	printf("\nStarting SystemKeyboardEventLoop...\n");
	//int64_t timestamp = 12345678912345;

	KEY_EVENT_RECORD key;
	for (;;)
	{
		yield();

		memset(&key, 0, sizeof(KEY_EVENT_RECORD));
		getconchar(&key);

		//if (key.uChar.AsciiChar == 's')
		//	StartTransientsChangefeedUpdate();
		//CreateUserQuery(user_email, user_name, user_password, _reqlCXConn);

		if (key.uChar.AsciiChar == 'g')
			SendHTTPRequest();
		else if (key.uChar.AsciiChar == 'r')		
			LoadGameStateQuery();		
		else if (key.uChar.AsciiChar == 'y')
			yield();			`
		else if (key.uChar.AsciiChar == 'q')
			break;

		yield();

	}
#endif

}


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
    CTTarget httpTarget  = { 0 };
    CTTarget reqlService = { 0 };
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
    cxThread = CTThreadCreate(&cq, CX_Dequeue_Resolve_Connect_Handshake);
    rxThread = CTThreadCreate(&rq, CX_Dequeue_Recv_Decrypt);
    txThread = CTThreadCreate(&tq, CX_Dequeue_Encrypt_Send);

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

    //Define RethinkDB service connection targetq
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

	//Use CXTransport CXURL C++ API to connect to our HTTPS target server
	coroutine_bundle = CXURL.connect(&httpTarget, _cxURLConnectionClosure);

	//User CXTransport CXReQL C++ API to connect to our RethinkDB service
	//coroutine_bundle = CXReQL.connect(&reqlService, _cxReQLConnectionClosure);

    //Start a runloop on the main thread for the duration of the application
    //SystemKeyboardEventLoop(argc, argv);
    StartPlatformEventLoop(argc, argv);

    //TO DO:  Wait for asynchronous threads to shutdown

    //Clean up socket connections (Note: closing a socket will remove all associated kevents on kqueues)
    delete _httpCXConn;
    //delete _reqlCXConn;

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
