//
//  ReØMQL.c
//
//  Created by Joe Moulton on 5/24/19.
//  Copyright © 2019 3rdGen Multimedia. All rights reserved.
//

#include "stdafx.h"
#include "../ctReQL.h"
#include "ReqlFile.h"


#include <errno.h>
#include <stdint.h>

#ifndef _WIN32
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

//kqueue
//#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <pthread.h>
#include <sched.h>

//posix thread
#include <pthread.h>

#include <sys/aio.h>

#include <sys/socket.h>
#include <sys/uio.h>


#include "cr_file.h"


#endif

//mbded tls
/*
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
*/

#ifdef __APPLE__
#include <execinfo.h>

#import <Security/SecCertificate.h>
#import <Security/SecIdentity.h>
#import <Security/SecImportExport.h>
#import <Security/SecItem.h>
#import <Security/SecTrust.h>
#import <Security/SecKey.h>
#import <Security/SecItem.h>
#import <Security/SecRandom.h>
//#import <Security/SecTrustSettings.h>



//gcd dispatch threads
#include <dispatch/dispatch.h>
#include <objc/message.h> //obj-c runtime messages used for iOS and tvOS only


#include <CoreFoundation/CoreFoundation.h>
#include <CFNetwork/CFNetwork.h>

#endif






//global externs
//const struct ReqlQueryFuncTable _ReqlQueryObjectFuncTable = {ReqlRunQuery, ReqlTableQuery, ReqlFilterQuery, ReqlInsertQuery, ReqlDeleteQuery, ReqlChangesQuery};
//static ReqlClientDriver RethinkDB = {ReqlConnect};//, ReqlDatabaseQuery};

static uint64_t REQL_NUM_CONNECTIONS = 0;


/* This is an implementation that doesn't need compiler support for thread-local
 variables. It creates thread-specific data usin POSIX functions.
 It uses pthread_key_create to register a destructor function which cleans
 the context up after thread exits. However, at least on Linux, the destructor
 is not called for the main thread. Therefore, atexit() function is registered
 to clean the main thread's context. */



static void ReqlQueryContextInit_(struct ReqlQueryContext *ctx) {
    printf("ReqlQueryContextInit\n");
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

static void ReqlQueryContextTerminate_(struct ReqlQueryContext *ctx) {
    
    printf("ReqlQueryContextTerminate\n");
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

/*
static int reql_ismain() {
    return pthread_main_np();
}
static pthread_key_t reql_key;
static pthread_once_t reql_keyonce = PTHREAD_ONCE_INIT;
static void *reql_main = NULL;

static void ReqlQueryContextTerminate(void *ptr) {
    struct ReqlQueryContext *ctx = ptr;
    ReqlQueryContextTerminate_(ctx);
    ///if( ctx ) free(ctx);
    if(reql_ismain()) reql_main = NULL;
}

static void ReqlQueryContextAtExit(void) {
    if(reql_main) ReqlQueryContextTerminate(reql_main);
}

static void ReqlQueryContextMakeKey(void) {
    int rc = pthread_key_create(&reql_key, ReqlQueryContextTerminate);
    assert(!rc);
}

void ReqlQuerySetContext(ReqlQueryContext * ctx) {
    //int rc = pthread_setspecific(reql_key, ctx);
    //assert(rc == 0);
    ReqlQueryContext * threadCtx = ReqlQueryGetContext();
    memcpy(threadCtx, ctx, sizeof(ReqlQueryContext));
}

void ReqlQueryClearContext(ReqlQueryContext* ctx) {
    ctx->numQueries = 0;
}

*/

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


//RethinkDB json keys needed when creating/parsing the json messages associated with the RethinkDB SCRAM Handshake
//These two aren't needed until the protocol version moves beyond 0
//And another authentication method besides SHA-256 is exposed:
//static const char * rdb_protocol_version_key = "protocol_version";
//static const char * rdb_auth_method_key = "authentication_method";
//static const char * rdb_auth_key = "authentication";
//static const char * rdb_success_key = "success";


/*
void ShowCerts(SSL* ssl)
{   X509 *cert;
    char *line;
    
    cert = SSL_get_peer_certificate(ssl);
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("Info: No client certificates configured.\n");
}
*/
/*
 connect to a server
 */
int rethinkdb_connect(struct rethinkdb_connection *rdb_conn) {
    
    /*
    int bio_err = 0;
    
    //Global SSL Context
    SSL_CTX *ctx;
    SSL_METHOD *method;
    
    //shared ssl 
    SSL *ssl;

    //Initialize OpenSSL Library
    if(!bio_err){
        // Global system initialization
        SSL_library_init();
        SSL_load_error_strings();
        ///An error write context
        //bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);
    }
    
    //Aside:
    //Once we've finished transmitting the response, we need to send our close_notify. As before, this is done using SSL_shutdown().
    //Unfortunately, things get a bit trickier when the server closes first. Our first call to SSL_shutdown() sends the close_notify
    //but doesn't look for it on the other side. Thus, it returns immediately but with a value of 0, indicating that the closure sequence isn't finished.
    //It's then the application's responsibility to call SSL_shutdown() again.
    
    //When we [the client] send our close_notify, the other side may send a TCP RST segment, in which case the
    //program will catch a SIGPIPE. We install a dummy SIGPIPE handler in initialize_ctx() to protect against this problem.
    // Set up a SIGPIPE handler
    //signal(SIGPIPE,sigpipe_handle);

    //Is this necessary?
    OpenSSL_add_all_algorithms();       // Load cryptos, et.al. //

    //initialize a global OpenSSL context with specified creation method
    method = TLSv1_2_client_method();   // Create new client-method instance //
    ctx = SSL_CTX_new(method);   // Create new context //
    if ( ctx == NULL )
    {
        printf("SSL_CTX_new FAILED.");
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    // Load our keys and certificates //
    if(!(SSL_CTX_use_certificate_chain_file(ctx, rdb_conn->caPath)))
    {
        //berr_exit("Can't read certificate file");
        printf("SSL_CTX_use_certificate_chain_file FAILED.");
        ERR_print_errors_fp(stderr);
        return -1;
    }
    //The Keyfile may have a password associated with it
    //pass=password;
    //SSL_CTX_set_default_passwd_cb(ctx, password_cb);
    
    //Use the private key PEM file
    //if(keyfile && !(SSL_CTX_use_PrivateKey_file(ctx,keyfile,SSL_FILETYPE_PEM)))
    //    berr_exit("Can't read key file");
    
    // Load the CAs we trust //
    if(!(SSL_CTX_load_verify_locations(ctx,rdb_conn->caPath,0)))
    {
        //berr_exit("Can't read CA list");
        printf("SSL_CTX_load_verify_locations FAILED.");
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    
#if (OPENSSL_VERSION_NUMBER < 0x0090600fL)
    SSL_CTX_set_verify_depth(ctx,1);
#endif

    //Create Standard TCP Socket Connection to server
    //server = OpenConnection(hostname, atoi(portnum));
    //int sd;
    int server;
    struct hostent *host;
    struct sockaddr_in addr;
    
    //if we are running the client against our own device as server then quit?
    if ( (host = gethostbyname(rdb_conn->addr)) == NULL )
    {
        perror(rdb_conn->addr);
        return -1;
    }
    
    server = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = *(in_addr_t*)(host->h_addr);  //set host ip addr
    addr.sin_port = htons(rdb_conn->port);
    
    //Connect the socket to the server
    if ( connect(server, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        close(server);
        perror(rdb_conn->addr);
        return -1;
    }
    
    //store the socket file descriptor on our rethinkdb connection "object"
    rdb_conn->s = server;
    
    //Create New SSL Socket on top of existing socket file descriptor
    ssl = SSL_new(ctx);             // create new SSL connection state //
    
    //Either directly attach the socket directly to the OpenSSL "SSL object"
    SSL_set_fd(ssl, server);        // attach the socket descriptor //
    
    //OR wrap the socket in a BIO and attach the bio to the "SSL object"
    //Note:  (BIO(s) can allow queued writing of the response message before actually sending with SSL_write)
    //sbio=BIO_new_socket(sock,BIO_NOCLOSE);
    //SSL_set_bio(ssl,sbio,sbio);
    
    //Finally, Start the Synchnrous SSL handshake using our blocking socket!!!
    int ret = SSL_connect(ssl);
    if ( ret < 0 )   // perform the connection //
    {
        printf("SSL_connect FAILED.\n");
        ERR_print_errors_fp(stderr);
        close(rdb_conn->s);
        rdb_conn->s = -1;
        return -1;
    }
    else
    {
        //construct reply
        
        rdb_conn->s = server;
        printf("\n\nConnected with %s encryption\n", SSL_get_cipher(ssl));
        ShowCerts(ssl);                             // get any certs
        
        //SSL Encryption Handshake Success!!!
        
        //We can do a test here to send incorrect data and Mongo db will send back
        //a non-json error message.
        
        //OpenSSL Cleanup
        //SSL_free(ssl);        // release connection state //
    }
    //close(server);          // close socket //
    //SSL_CTX_free(ctx);      // release context '//
    
    
    printf("rethinkdb_connect()");
    if (rdb_conn->s >= 0) return 0;
    
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    
    rdb_conn->s = socket(PF_INET, SOCK_STREAM, 0);
    if (rdb_conn->s < 0) {
        return -1;
    }
    printf("rethinkdb_connect() 1");

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(rdb_conn->addr);
    sin.sin_port = htons(rdb_conn->port);
    
    if (connect(rdb_conn->s, (struct sockaddr *) &sin, sizeof(sin))) {
        printf("socket connect error:  %d", errno);
        close(rdb_conn->s);
        rdb_conn->s = -1;
        return -1;
    }
    printf("rethinkdb_connect() 2");
     
     */
    return 0;
    
}


void fatal(const char *func, int rv)
{
    //fprintf(stderr, "%s: %s\n", func, nng_strerror(rv));
    exit(1);
}



#ifndef _WIN32
static uintptr_t kqueue_wait_with_timeout(int kqueue, struct kevent * kev, int16_t eventFilter, uint32_t timeout)
{
    struct timespec _ts;
    struct timespec *ts = NULL;
    if (timeout != UINT_MAX) {
        ts = &_ts;
        ts->tv_sec = (timeout - (timeout % 1000)) / 1000;
        ts->tv_nsec = (timeout % 1000) * 1000;
    }
    
    //struct kevent kev;
    int n = kevent(kqueue, NULL, 0, kev, 1, ts);
    
    //eliminate branching?
    if (n > 0) {
        
        return kev->ident;
        
        /*
        //only return the event if it matches our filter and is within our specified input range
        if ((kev->filter & eventFilter) && kev->ident >= eventRangeStart && kev->ident <= eventRangeEnd) {
            if( kev->filter & EV_EOF)
            {
                printf("EOF Encountered!!!\n");
            }
            return kev->ident;
        }
        return outOfRangeEvent;
        */
    }

    //Example Kqueue Event Loop
    //struct kevent chlist[2];  // events we want to monitor
    //struct kevent evlist[2]; // events that were triggered
    // initialise kevent structures
    //EV_SET(&chlist[0], sckfd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
    //EV_SET(&chlist[1], fileno(stdin), EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
    // loop forever / for (;;) { nev = kevent(kq, chlist, 2, evlist, 2, NULL);
    
    return REQL_EVT_TIMEOUT;
}
#endif


coroutine uintptr_t ReqlSocketWaitWithTimeout(REQL_SOCKET socketfd, int event_queue, int16_t eventFilter, uint32_t timeout)
{
#ifndef _WIN32
    //printf("ReqlSocketWait Start\n");
    struct kevent kev = {0};
    //  Synchronously wait for the socket event by idling until we receive kevent from our kqueue
    return kqueue_wait_with_timeout(event_queue, &kev, eventFilter, timeout);
#else
	return 0;
#endif
	//return ret_event;
}


//JSMN JSON Parser Helpers
/*
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}
*/
/*

static int createMappedFile()
{
    _writeMap = (cr_mmap*)malloc( sizeof(cr_mmap) * _numMemoryMapDivisions );
    
    videoCacheFilePath = [[self cachedPathForVideoFileURL:_videoURL] stringByAppendingString:@".yuv"];//fileSuffix];
    
    //You have to make sure that the file you’re interacting doesn’t get into the cache.  The easiest way to do that is to create your own test file and write to it with non-cached I/O.  You can test whether this is working as expected using mincore.
    
    
    int fileDescriptor = open(videoCacheFilePath.UTF8String, O_RDWR|O_CREAT|O_TRUNC, (mode_t)0700);
    
    //check if the file descriptor is valid
    if (fileDescriptor < 0) {
        printf("\nUnable to open file\n");
    }
    
    
    //disabling disk caching will allow zero performance reads
    //According to John Carmack:  I am pleased to report that fcntl( fd, F_NOCACHE ) works exactly as desired on iOS – I always worry about behavior of historic unix flags on Apple OSs. Using
    //this and page aligned target memory will bypass the file cache and give very repeatable performance ranging from the page fault bandwidth with 32k reads up to 30 mb/s for one meg reads (22
    //mb/s for the old iPod). This is fractionally faster than straight reads due to the zero copy, but the important point is that it won’t evict any other buffer data that may have better
    //temporal locality.
    
    
    //this takes care preventing caching, but we also need to ensure that "target memory" is page aligned
    if ( fcntl( fileDescriptor, F_NOCACHE | F_NODIRECT  ) < 0 )
    {
        
        NSLog(@"F_NOCACHE failed!");
    }
    
    
    //expand to the desired file size
    if (ftruncate(fileDescriptor, fileSize) < 0)
    {
        printf("\nUnable to truncate file\n");
        
    }
    
    
    //first we need to preallocate a contiguous range of memory for the file
    //if this fails we won't have enough space to reliably map a file for zero copy lookup
    fstore_t fst;
    fst.fst_flags = F_ALLOCATECONTIG;
    fst.fst_posmode = F_PEOFPOSMODE;
    fst.fst_offset = 0;
    fst.fst_length = fileSize;
    fst.fst_bytesalloc = 0;
    
    
    if( fcntl( fileDescriptor, F_PREALLOCATE, &fst ) < 0 )
    {
        NSLog(@"F_PREALLOCATED failed!");
    }
    
    
    off_t size = lseek(fileDescriptor, 0, SEEK_END);
    lseek(fileDescriptor, 0, SEEK_SET);
    
    printf("\nfile size = %lld\n", size);
}
*/


static struct gaicb **reqs = NULL;
static int nreqs = 0;

/// Our CFHost callback function for our host; this just extracts the object from the `info`
/// pointer and calls methods on it.

// Stops the query with the supplied error, notifying the delegate if `notify` is true.

#ifdef __APPLE__
static void stopHostWithError(CFHostRef host, const CFStreamError * streamError, bool notify)
{
    
    printf("Stop Host with Error\n");
    CFHostSetClient(host, nil, nil);
    CFHostUnscheduleFromRunLoop(host, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    CFHostCancelInfoResolution(host, kCFHostAddresses);
    //CFRelease( host );
    
    //TO DO: notify
    /*
    id<QHostAddressQueryDelegate> strongDelegate = self.delegate;
    if (notify && (strongDelegate != nil) ) {
        if (error != nil) {
            [strongDelegate hostAddressQuery:self didCompleteWithError:error];
        } else {
            NSArray<NSData *> * addresses = (__bridge NSArray<NSData *> *) CFHostGetAddressing(self.host, NULL);
            [strongDelegate hostAddressQuery:self didCompleteWithAddresses:addresses];
        }
    }
     */
}
#endif 

//A helper function to perform a socket connection to a given service
//Will either return an kqueue fd associated with the input (blocking or non-blocking) socket
//or an error code
REQL_API REQL_INLINE coroutine int ReqlSocketConnect(REQL_SOCKET socketfd, ReqlService * service)
{
    //printf("ReqlSocketConnect\n");
    //yield();
    struct sockaddr_in addr;
 
    //Resolve hostname asynchronously   
    /*
    // Resolve hostname synchronously
     struct hostent *host = NULL;
     if ( (host = gethostbyname(service->host)) == NULL )
    {
        printf("gethostbyname failed with err:  %d\n", errno);
        perror(service->host);
        return ReqlDomainError;
    }
    printf("ReqlSocketConnect 3\n");
    */
    
    // Fill a sockaddr_in socket host address and port
    //bzero(&addr, sizeof(addr));
	memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = service->host_addr.sin_addr.s_addr;//*(in_addr_t*)(service->host_addr.sa_data);//*(in_addr_t*)(host->h_addr);  //set host ip addr
    addr.sin_port = htons(service->port);
    
    /*
    struct sockaddr_in * addr_in = (struct sockaddr_in*)&(service->host_addr);
    addr_in->sin_family = AF_INET;
    //addr_in->sin_addr.s_addr = AF_INET;
    addr_in->sin_port = service->port;
    */
    
#ifdef _WIN32
		// Connect the bsd socket to the remote server
	if ( WSAConnect(socketfd, (struct sockaddr*)&addr, sizeof(addr), NULL, NULL, NULL, NULL) == SOCKET_ERROR )
#elif defined(__APPLE__)
	// Connect the bsd socket to the remote server
    if ( connect(socketfd, (struct sockaddr*)&addr, sizeof(addr)) != 0 ) //== SOCKET_ERROR
#endif
    {
        //errno EINPROGRESS is expected for non blocking sockets
        if( errno != EINPROGRESS )
        {
            printf("socket failed to connect to %s with error: %d\n", service->host, errno);
            ReqlCloseSocket(socketfd);
            perror(service->host);
            return ReqlConnectError;
        }
        else
        {
			printf("\nReqlSocketConnect EINPROGRESS\n");
        }
    }
    //printf("ReqlSocketConnect End\n");
    return ReqlSuccess;
}


//A helpef function to close an SSL Context and a socket in one shot
static int ReqlCloseSSLSocket(ReqlSSLContextRef sslContextRef, REQL_SOCKET socketfd)
{
	ReqlSSLContextDestroy(sslContextRef);
    return ReqlCloseSocket(socketfd);
}

#ifdef __APPLE__

/***
 *  ReqlGenerateKeyPair
 *
 *  Create a Custom Key Pair that can be added to the keychain using Apple Secure Transport API
 ***/
/*
int ReqlGenerateKeyPair()
{
     OSStatus status;
    
     //6  Generate custom Public/Private Key Pair to associate with our certificate in the keychain if not using a pck#12 format that already has an associated key pair
     const void *priv_key_keys[] =    { kSecAttrApplicationTag,            kSecAttrAccessible, kSecAttrIsPermanent };
     const void *priv_key_values[] =  { CFSTR("PRIVATE_KEY"),     kSecAttrAccessibleAlways,    kCFBooleanTrue        };
     CFDictionaryRef privKeyDict = CFDictionaryCreate(kCFAllocatorDefault, priv_key_keys, priv_key_values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
     
     const void *pub_key_keys[] =    { kSecAttrApplicationTag,            kSecAttrAccessible, kSecAttrIsPermanent };
     const void *pub_key_values[] =  { CFSTR("PUBLIC_KEY"),     kSecAttrAccessibleAlways,    kCFBooleanTrue        };
     CFDictionaryRef pubKeyDict = CFDictionaryCreate(kCFAllocatorDefault, pub_key_keys, pub_key_values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
     
     int eDepth = 2048;
     CFNumberRef encryptionDepth = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &eDepth);
     const void *pair_keys[] =    { kSecAttrKeyType,            kSecAttrKeySizeInBits, kSecPrivateKeyAttrs, kSecPublicKeyAttrs };
     const void *pair_values[] =  { kSecAttrKeyTypeRSA,         encryptionDepth,    privKeyDict, pubKeyDict        };
     CFDictionaryRef keypairDict = CFDictionaryCreate(kCFAllocatorDefault, pair_keys, pair_values, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
     
     SecKeyRef publicKey = NULL;
     SecKeyRef privateKey = NULL;
     
     status = SecKeyGeneratePair(keypairDict, &publicKey, &privateKey);
     if( status != errSecSuccess )
     {
         printf("SecKeyGeneratePair failed with error:  %d", status);
         return -1;
     }
     
     // Remember we need to release CF associated memory!!!
     
     // 6.1  we can store the public key in the keychain as a "generic password" so that it doesn't interfere with retrieving certificates
     // The keychain will normally only store the private key and the certificate
     // As we want to keep a reference to the public key itself without having to ASN.1 parse it out of the certificate
     // we can stick it in the keychain as a "generic password" for convenience
     const void *fkq_keys[] =    { kSecClass,            kSecValueRef, kSecAttrKeyType,        kSecReturnData };
     const void *fkq_values[] =  { kSecClassKey,         publicKey,        kSecAttrKeyTypeRSA, kCFBooleanTrue        };
     CFDictionaryRef findKeyQuery = CFDictionaryCreate(kCFAllocatorDefault, fkq_keys, fkq_values, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
     
     CFDataRef keyCopyData;
     status = SecItemCopyMatching(findKeyQuery, (CFTypeRef*)&keyCopyData);
     if( status != errSecSuccess )
     {
     printf("SecItemCopyMatching (findKeyQuery) failed with error:  %d", status);
     return -1;
     }
     
     assert( keyCopyData );
     
     // 6.2  now we have the public key data, add it in as a generic password
     const void *cgpq_keys[] =    { kSecClass,                        kSecAttrAccessible,                            kSecAttrService,                         kSecAttrAccount, kSecValueData, kSecReturnAttributes  };
     const void *cgpq_values[] =  { kSecClassGenericPassword,         kSecAttrAccessibleAlways,                      CFSTR("3rdGen RethinkDB Client Driver"), CFSTR("admin"),  keyCopyData,  kCFBooleanTrue };
     CFDictionaryRef createGenericPasswordQuery = CFDictionaryCreate(kCFAllocatorDefault, cgpq_keys, cgpq_values, 6, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
     
     //6.3 Check to see if the generic password [ie public key] we are setting is already in the device system keychain
     CFDictionaryRef gpCopyResult = nil;
     status = SecItemCopyMatching(createGenericPasswordQuery, (CFTypeRef *)&gpCopyResult);
     if( status != errSecSuccess )
     {
     printf("SecItemCopyMatching failed with error: %d\n", status);
     return -1;
     }
     
     //6.4 If the certificate matching the dict query was already in the device system keychain, then delete it and re-add it (or directly attempt to get the identity cert with it)
     if( gpCopyResult )
     {
     //Delete SecurityCertificateRef matching query from keychain
     status = SecItemDelete( createGenericPasswordQuery );
     if( status != errSecSuccess )
     {
     printf("SecItemDelete failed with error:  %d\n", status);
     return -1;
     }
     else
     {
     printf("SecItemDelete (createGenericPasswordQuery):  Success\n");
     }
     
     //release, we are about to use this pointer for an allocated copy again
     CFRelease(gpCopyResult);
     gpCopyResult = NULL;
     }
     
     
     //Not sure what CFTypeRef type this copy would be
     //CFDictionaryRef * gpCopyResult;
     status = SecItemAdd(createGenericPasswordQuery, NULL);
     if( status != errSecSuccess )
     {
         printf("SecItemAdd (createGenericPasswordQuery) failed with error: %d", status);
         return -1;
     }
     
     //6.3 Delete the public key from the keychain store as it interferes with correctly adding the certificate(s)
     
     const void *dpkq_keys[] =   {kSecClass,   kSecValueRef };
     const void *dpkq_values[] = {kSecClassKey,  publicKey};
     CFDictionaryRef deletePubKeyQuery = CFDictionaryCreate(kCFAllocatorDefault, dpkq_keys, dpkq_values, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
     
     status = SecItemDelete(deletePubKeyQuery);
     if( status != errSecSuccess )
     {
         printf("SecItemAdd (createGenericPasswordQuery) failed with error: %d", status);
         return -1;
     }
     
     //6.4  Key Generation Section Cleanup
     CFRelease( encryptionDepth );
     //CFRelease( gpCopyResult );
     CFRelease( privKeyDict);
     CFRelease( pubKeyDict );
     CFRelease( keypairDict );
     //CFRelease( publicKey );
     //CFRelease( privateKey );
     CFRelease( findKeyQuery );
     CFRelease( createGenericPasswordQuery );
     CFRelease( deletePubKeyQuery );
     //CFRelease( deletePrivKeyQuery );
    
}
 */

/*

static void ReqlSSLCertificateIdentity()
{
     const void *p12_keys[] =    { kSecImportExportPassphrase, kSecReturnPersistentRef, kSecReturnAttributes, kSecAttrAccessible };
     const void *p12_values[] =  { CFSTR("ssl.pw.4.p12"), kCFBooleanTrue, kCFBooleanTrue, kSecAttrAccessibleAlways };
     CFDictionaryRef p12Dict = CFDictionaryCreate(kCFAllocatorDefault, p12_keys, p12_values, 4, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
 
     //SecCertificateRef certRef = nil;
     //CFArrayRef certsArray = nil;
     CFArrayRef certsArray = NULL;//CFArrayCreate(kCFAllocatorDefault, 0, 0, &kCFTypeArrayCallBacks);
     status = SecPKCS12Import(caData, p12Dict, &certsArray);
     if( status != errSecSuccess )
     {
        printf("SecPKCS12Import failed with error: %d", status);
        return -1;
     }
 
     assert( certsArray );
     //assert( CFArrayGetCount(certsArray) > 0 );
     */

    //we can get the identref, but we have a certs array we can pass directly to ssl now
    //CFIndex idx= 0;
    //SecIdentityRef identRef = (SecIdentityRef)CFArrayGetValueAtIndex(certsArray, idx);
    //SecIdentityCopyCertificate(secIdentity , &certificateRef)
    //SSLSetCertificateAuthorities()
    /*
     //8  Retrieve a SecIdentityRef it is needed for the SSLSetCertificate call below.  Remember it only gets generated from combinations of key pairs + certs that are added to the keychain.
     //   SecItemAdd returned the dictionary containing the SecurityCertificateRef properties we just added to the keychain (because we specified kSecReturnAttributes)
     //   but with some extra properties added that we need to retrieve in order retrieve the associated SecIdentityRef
     assert(certCopyResult);
     //assert( CFDictionaryGetTypeID() == certCopyResult->type() )
     
     //8.1  Retreive SecCertificateRef issuer
     CFDataRef issuerData = NULL;
     status = CFDictionaryGetValueIfPresent(certCopyResult, kSecAttrIssuer, &issuerData);
     if( !status )
     printf("Failed to find kSecAttrIssue data in copyResult\n");
     
     //8.2  Retreive SecCertificateRef Serial Number
     CFDataRef SNData = NULL;
     status = CFDictionaryGetValueIfPresent(certCopyResult, kSecAttrSerialNumber, &SNData);
     if( !status )
     printf("Failed to find kSecAttrIssue data in copyResult\n");
     
     assert( issuerData );
     assert( SNData );
     
     //Now we need to get a SecIdentityRef from our SecCertificateRef.  This was tricky to figure out and
     //we can't use the following on iOS:
     //SecIdentityRef identity = NULL;
     //status = SecIdentityCreateWithCertificate(NULL,   // Default keychain search list
     //                                 certificates[0],
     //                                 &identity);
     //if (status != errSecSuccess) { printf("SecIdentityCreateFailed:  %d", status); return -1; }
     
     // 8.3  It happens that a SecIdentityRef was created from the SecCertificateRef + SecKeyRef Pair when you added them to the keychain with the code above
     // Retrieve a persistent reference to the identity consisting of the client certificate and the pre-existing private key (remember, we removed the private key and stored separately in the keychain because of a bug!)
     // the identity will have the same label property as that supplied when creating SecCertificateRef, if one was supplied
     // kSecReturnPersistentRef is important for persisting the object in the keychain for the duration of the application session
     const void *ciq_keys[] =    { kSecAttrApplicationTag, kSecClass,        kSecAttrIssuer, kSecAttrSerialNumber,   kSecReturnPersistentRef  };
     const void *ciq_values[] =  { CFSTR("PRIVATE_KEY"), kSecClassIdentity,issuerData,     SNData,                 kCFBooleanTrue        };
     CFDictionaryRef certIdentQuery = CFDictionaryCreate(kCFAllocatorDefault, ciq_keys, ciq_values, 5, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
     assert( certIdentQuery );
     
     SecIdentityRef identityCert = NULL;
     status = SecItemCopyMatching( certIdentQuery, NULL );
     if( status != errSecSuccess )
     {
        printf("SecItemCopyMatching (certIdentQuery) failed with error:  %d", status);
        //return -1;
     }
     
     //8.6  Get SecIdentityRef Section Cleanup

     //CFRelease(issuerData);
     //CFRelease(SNData);
     //CFRelease(certIdentQuery);
     //CFRelease(identityCert);
}
*/

/***
 *  ReqlSSLHandshake
 *
 *  Perform a custom SSL Handshake using the Apple Secure Transport API
 *
 *  --Overrides PeerAuth step (errSSLPeerAuthCompleted) to validate trust against the input ca root certificate
 ***/
OSStatus ReqlSSLHandshake(ReØMQL * conn, SecCertificateRef rootCert)
{
    OSStatus status;
 
    struct kevent kev;
    uintptr_t timout_event = -1;
    uintptr_t out_of_range_event = -2;
    uint32_t timeout = 0;  //uint max will translate to timeout structØ of null, which is timeout of forever
    
    // Finally it is time to populate an array with our certificate chain.  We can have as many SecCertificateRefs as we like,
    // but the first item in the array must always be a SecIdentityRef of the root cert.  In this case, we only have the root cert to add.
    //printf("Creating certificates chain array\n");
    SecCertificateRef certsArray[1] = {rootCert};
    CFArrayRef certificatesArray = CFArrayCreate(kCFAllocatorDefault, (const void **)certsArray, 1, &kCFTypeArrayCallBacks);
    assert(certificatesArray);
    assert( CFArrayGetCount(certificatesArray) > 0 );
    
    //8.5  Set the certificate chain on the SSL Context (this only works if you have a SecCertificateRef that is part of a valid SecIdentityRef)
    /*
     status =  SSLSetCertificate ( sslContext, certificatesArray );
     if( status != noErr )
     {
     printf("SSLSetCertificate failed with error: %d\n", status);
     return -1;
     }
    */
    int i = 0;
    //Do the SSL Handshake using the Apple Secure Transport API
    //printf("SSL Handshake iteration:  %d", i++);
    status  = SSLHandshake(conn->sslContext);
    


    //return from calling event loop
    //if server has not finished responding, yield again
    /*
    if ( kqueue_wait_with_timeout(conn->event_queue, &kev, EVFILT_WRITE | EV_EOF, timout_event, out_of_range_event, conn->socket, conn->socket, 0) != conn->socket )
    {
        yield();
        CFRunLoopWakeUp(CFRunLoopGetCurrent());
    }
    */
    
    while ( status == errSSLWouldBlock )
    {
        //yield to calling event loop
        yield();
        CFRunLoopWakeUp(CFRunLoopGetCurrent());
        
        //return from calling event loop
        //if server has not finished responding, yield again
        
        //if ( kqueue_wait_with_timeout(conn->event_queue, &kev, EVFILT_READ | EVFILT_WRITE| EV_EOF, timout_event, out_of_range_event, conn->socket, conn->socket, timeout) != conn->socket )
        if( ReqlSocketWait(conn->socket, conn->event_queue, EVFILT_READ | EVFILT_WRITE| EV_EOF) != conn->socket )
        {
            //printf("Kqueue return != socket\n");
            continue;//
            //yield();
            //CFRunLoopWakeUp(CFRunLoopGetCurrent());
        }

        //printf("SSL Handshake iteration:  %d", i++);
        //  On the first call to SSLHandshake it will call our writeToSocket callback with the entire client message
        //  On subsequent calls to SSLHandshake it will just keep trying to read from the socket until bytes are present,
        //  So better to relinquish operation back to calling run loop by calling yield() and come back to handshake later
        status  = SSLHandshake(conn->sslContext);

        //yield to calling event loop
        //yield();
        //CFRunLoopWakeUp(CFRunLoopGetCurrent());

        
        if( status  == errSSLPeerAuthCompleted )
        {
            //Officially, since we are only doing client side auth and our database service is doing
            //nothing to authenticate the SSL connection (only auth after SSL), the SSL connection has been established after the first handshake
            //and SSLRead or SSLWrite can be used at any time to communicate with the server
            
            //However, we need to do client side auth
            //because this is the only opportunity we have to
            //printf("SSL Handshake Interlude -- errSSLPeerAuthCompleted\n");
            
            //Can't get a peer trust before calling handshake
            //Can't set SSLSetCertificate with anything but a SecIdentityRef in the keychain before the handshake
            //So this is the only way to use ssl w/ a root ca secure transport api without:
            // 1)  Loading a .p12 file that has both key pair and certificate making up identity
            // 2)  Generating key pairs and certificates otf between client and server
            
            //Get a peer trust object that contains our remote server's certificate
            //This call should be followed up by before examaning the results
            //Caller must CFRelease the returned trust reference
            SecTrustRef trust = NULL;
            status = SSLCopyPeerTrust(conn->sslContext, &trust);
            if( status != errSecSuccess )
            {
                printf("SSLCopyPeerTrust failed with error: %d\n", status);
                return -1;
            }
            
            // Perform your custom trust rules here.  e.g. we were unsuccessfull in setting and retrieving a SecIdentityRef from the keychain store
            // because we only have a root CA .pem file provided by our RethinkDB Service (and no key pairs in the file preventing us from the criteria we need for SecIdentityRef)
            bool bTrusted = false;
            
            // However, we were successfull in creating a SecCertificateRef and we can use this now to see whether we trust our server's cert
            // Just like OpenSSL or MBEDTLS would under the hood when negotiating an SSL connection
            status = SecTrustSetAnchorCertificates(trust, (CFArrayRef)certificatesArray);
            if( status != errSecSuccess )
            {
                printf("SecTrustSetAnchorCertificates failed with error: %d\n", status);
                return -1;
            }
            
            /* Uncomment this to enable both system certs and the one we are supplying */
            //result = SecTrustSetAnchorCertificatesOnly(trust, NO);
            SecTrustResultType trustResult;
            status = SecTrustEvaluate(trust, &trustResult);
            if (status == errSecSuccess)
            {
                switch (trustResult)
                {
                    case kSecTrustResultProceed:
                    case kSecTrustResultUnspecified:
                        bTrusted = true;
                        break;
                        
                    case kSecTrustResultInvalid:
                    case kSecTrustResultDeny:
                    case kSecTrustResultRecoverableTrustFailure:
                    case kSecTrustResultFatalTrustFailure:
                    case kSecTrustResultOtherError:
                        printf("trust failure\n");
                    default:
                        break;
                }
            }
            else
            {
                printf("SecTrustEvaluate failed with error: %d\n", status);
                return -1;
            }
            
            // If trusted, continue the handshake
            if (bTrusted)
            {
                
                SecCertificateRef secCert = SecTrustGetCertificateAtIndex(trust, 0);
                CFStringRef cfSubject = SecCertificateCopySubjectSummary(secCert);
                CFShow(cfSubject);
                
                
                //attempt ot set the cert
                SecCertificateRef tCerts[1]; tCerts[0]=secCert;
                //printf("Creating certificates chain array\n");
                CFArrayRef trustedCerts = CFArrayCreate(kCFAllocatorDefault, (const void **)tCerts, 1, &kCFTypeArrayCallBacks);
                assert(trustedCerts);
                assert( CFArrayGetCount(trustedCerts) > 0 );
                
                //OMFG!! Not sure if this is necessary anymore at this stage, but this works!!!
                //It may in fact be necessary for subsequent SSL reads/writes
                SSLSetCertificate(conn->sslContext, trustedCerts);
                if( status != noErr )
                {
                    printf("SSLSetCertificate failed with error: %d\n", status);
                    return status;
                }
                
                //printf("\nTrust passed. continuing handshake...\n");
                status = SSLHandshake(conn->sslContext);
                
            }
            else
            {
                /* Your trust failure handling here ~ disconnect */
                printf("Trust failed\n");
                return -1;
            }
            
        }
        /*
        else if( status == errSSLClientCertRequested)
        {
            printf("errSSLClientCertRequested\n");
            
            status =  SSLSetCertificate ( sslContext, certificatesArray );
            if( status != noErr )
            {
                printf("SSLSetCertificate failed with error: %d\n", status);
                return -1;
            }
            
        }
        */
        
    }
    
    if( status != 0 && status != errSSLPeerAuthCompleted )
    {
        printf("SSL Handshake Error Status: %d", status);
        return status;
    }
    
    return noErr;
}


#endif

#define REQL_NUM_NONCE_BYTES 20
const char * clientKeyData = "Client Key";
const char * serverKeyData = "Server Key";
const char* authKey = "\"authentication\"";
const char * successKey = "\"success\"";

    
static const char *JSON_PROTOCOL_VERSION = "{\"protocol_version\":0,\0";
static const char *JSON_AUTH_METHOD = "\"authentication_method\":\"SCRAM-SHA-256\",\0";
REQL_API REQL_INLINE int ReqlSASLHandshake(ReØMQL * r, ReqlService * service)
{
	    OSStatus status;

	

    //SCRAM HMAC SHA-256 Auth Buffer Declarations

    char clientNonce[REQL_NUM_NONCE_BYTES];                //a buffer to hold randomly generated UTF8 characters
    
    char saltedPassword[CC_SHA256_DIGEST_LENGTH];   //a buffer to hold hmac salt at each iteration

    char clientKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold SCRAM generated clientKey
    char serverKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold SCRAM generated serverKey

    char storedKey[CC_SHA256_DIGEST_LENGTH];        //a buffer to hold the SHA-256 hash of clientKey
    char clientSignature[CC_SHA256_DIGEST_LENGTH];  //used in combination with clientKey to generate a clientProof that the server uses to validate the client
    char serverSignature[CC_SHA256_DIGEST_LENGTH];  //used to generated a serverSignature the client uses to validate that sent by the server
    char clientProof[CC_SHA256_DIGEST_LENGTH];


    
    //SCRAM Message Exchange Buffer Declarations
    //  AuthMessage := client-first-message-bare + "," + server-first-message + "," + client-final-message-without-proof
    
    //RethinkDB JSON message exchange buffer declarations
    char MAGIC_NUMBER_RESPONSE_JSON[1024];

    char SERVER_FIRST_MESSAGE_JSON[1024];
    char CLIENT_FINAL_MESSAGE_JSON[1024];
    char SERVER_FINAL_MESSAGE_JSON[1024];
    
	//All buffers provided to ReqlSend must be in-place writeable
	char REQL_MAGIC_NUMBER_BUF[4] = "\xc3\xbd\xc2\x34";
	char CLIENT_FIRST_MESSAGE_JSON[1024] = "\0";
	char CLIENT_FINAL_MESSAGE[1024] = "\0";
    char SCRAM_AUTH_MESSAGE[1024] = "\0";

	char* successVal = NULL;
	char* authValue = NULL;
	char* tmp = NULL;
	void * base64SS = NULL;
	void * base64Proof = NULL;
	char * salt = NULL;

	char * mnResponsePtr = NULL;
	char * sFirstMessagePtr = NULL;
	char * sFinalMessagePtr = NULL;
	
	char delim[] = ",";
    char *token = NULL;//strtok(authValue, delim);
    char * serverNonce = NULL;
    char * base64Salt = NULL;
    char * saltIterations = NULL;

	bool magicNumberSuccess = false;
	bool sfmSuccess = false;


	//Declare some variables to store the variable length of the buffers
	//as we perform message exchange/SCRAM Auth
	unsigned long AuthMessageLength = 0;/// = 0;
	unsigned long magicNumberResponseLength = 0;// = 1024;
	unsigned long readLength = 0;// = 1024;
	unsigned long authLength = 0;// = 0;
	size_t base64SSLength = 0;
	size_t base64ProofLength = 0;
	size_t saltLength = 0;

	int charIndex;


	//Declare some variables to store the variable length of the buffers
    //as we perform message exchange/SCRAM Auth
    AuthMessageLength = 0;
    magicNumberResponseLength = 1024;
    readLength = 1024;
	authLength = 0;
	saltLength = 0;

	printf("ReqlSASLHandshake begin\n");
    //  Generate a random client nonce for sasl scram sha 256 required by RethinkDB 2.3
    //  for populating the SCRAM client-first-message
    //  We must wrap SecRandomCopyBytes in a function and use it one byte at a time
    //  to ensure we get a valid UTF8 String (so this is pretty slow)
    if( (status = ct_scram_gen_rand_bytes( clientNonce, REQL_NUM_NONCE_BYTES )) != noErr)
    {
        printf("ct_scram_gen_rand_bytes failed with status:  %d\n", status);
        return ReqlEncryptionError;
    }
    
    //  Populate SCRAM client-first-message (ie client-first-message containing username and random nonce)
    //  We choose to forgo creating a dedicated buffer for the client-first-message and populate
    //  directly into the SCRAM AuthMessage Accumulation buffer
    sprintf(SCRAM_AUTH_MESSAGE, "n=%s,r=%.*s", service->user, (int)REQL_NUM_NONCE_BYTES, clientNonce);
    AuthMessageLength += (unsigned long)strlen(SCRAM_AUTH_MESSAGE);
    //printf("CLIENT_FIRST_MESSAGE = \n\n%s\n\n", SCRAM_AUTH_MESSAGE);
    
    //  Populate client-first-message in JSON wrapper expected by RethinkDB ReQL Service
    //static const char *JSON_PROTOCOL_VERSION = "{\"protocol_version\":0,\0";
    //static const char *JSON_AUTH_METHOD = "\"authentication_method\":\"SCRAM-SHA-256\",\0";
    strcat( CLIENT_FIRST_MESSAGE_JSON, JSON_PROTOCOL_VERSION);//, strlen(JSON_PROTOCOL_VERSION));
    strcat( CLIENT_FIRST_MESSAGE_JSON, JSON_AUTH_METHOD);//, strlen(JSON_PROTOCOL_VERSION));
    sprintf(CLIENT_FIRST_MESSAGE_JSON + strlen(CLIENT_FIRST_MESSAGE_JSON), "\"authentication\":\"n,,%s\"}", SCRAM_AUTH_MESSAGE);
    //printf("CLIENT_FIRST_MESSAGE_JSON = \n\n%s\n\n", CLIENT_FIRST_MESSAGE_JSON);
    
    //  The client-first-message is already in the AuthMessage buffer
    //  So we don't need to explicitly copy it
    //  But we do need to add a comma after the client-first-message
    //  and a null char to prepare for appending the next SCRAM messages
    SCRAM_AUTH_MESSAGE[AuthMessageLength] = ',';
    AuthMessageLength +=1;
    SCRAM_AUTH_MESSAGE[AuthMessageLength] = '\0';
    
    //  Asynchronously send (ie non-blocking send) both magic number and client-first-message-json
    //  buffers in succession over the (non-blocking) socket TLS+TCP connection
    ReqlSend(r, (void*)REQL_MAGIC_NUMBER_BUF, 4);
    ReqlSend(r, CLIENT_FIRST_MESSAGE_JSON, strlen(CLIENT_FIRST_MESSAGE_JSON)+1);  //Note:  Raw JSON messages sent to ReQL Server always needs the extra null character to determine EOF!!!
                                                                                  //       However, Reql Query Messages must NOT contain the additional null character
    //yield();
    
    //  Synchronously Wait for magic number response and SCRAM server-first-message
    //  --For now we just care about getting 'success: true' from the magic number response
    //  --The server-first-message contains salt, iteration count and a server nonce appended to our nonce for use in SCRAM HMAC SHA-256 Auth
    mnResponsePtr = (char*)ReqlRecv(r, MAGIC_NUMBER_RESPONSE_JSON, &magicNumberResponseLength);
    sFirstMessagePtr = (char*)ReqlRecv(r, SERVER_FIRST_MESSAGE_JSON, &readLength);
    printf("MAGIC_NUMBER_RESPONSE_JSON = \n\n%s\n\n", mnResponsePtr);
	printf("SERVER_FIRST_MESSAGE_JSON = \n\n%s\n\n", sFirstMessagePtr);
    
	if (!mnResponsePtr || !sFirstMessagePtr)
		return ReqlSASLHandshakeError;
    //  Parse the magic number json response buffer to see if magic number exchange was successful
    //  Note:  Here we are counting on the fact that rdb server json does not send whitespace!!!
    //bool magicNumberSuccess = false;
    //const char * successKey = "\"success\"";
    //char * successVal;
    if( (successVal = strstr(mnResponsePtr, successKey)) != NULL )
    {
        successVal += strlen(successKey) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *successVal == '"') successVal+=1;
        if( strncmp( successVal, "true", MIN(4, strlen(successVal) ) ) == 0 )
            magicNumberSuccess = true;
    }
    if( !magicNumberSuccess )
    {
        printf("RDB Magic Number Response Failure!\n");
        return ReqlSASLHandshakeError;
    }
    
    //  Parse the server-first-message json response buffer to see if client-first-message exchange was successful
    //  Note:  Here we are counting on the fact that rdb server json does not send whitespace!!!
    //bool sfmSuccess = false;
    if( (successVal = strstr(sFirstMessagePtr, successKey)) != NULL )
    {
        successVal += strlen(successKey) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *successVal == '"') successVal+=1;
        if( strncmp( successVal, "true", MIN(4, strlen(successVal) ) ) == 0 )
            sfmSuccess = true;
    }
    if( !sfmSuccess )//|| !authValue || authLength < 1 )
    {
        printf("RDB server-first-message response failure!\n");
        return ReqlSASLHandshakeError;
    }
    
    //  The nonce, salt, and iteration count (ie server-first-message) we need for SCRAM Auth are in the 'authentication' field
    //const char* authKey = "\"authentication\"";
    authValue = strstr(sFirstMessagePtr, authKey);
    //size_t authLength = 0;
    if( authValue )
    {
        authValue += strlen(authKey) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *authValue == '"')
        {
            authValue+=1;
            tmp = (char*)authValue;
            while( *(tmp) != '"' ) tmp++;
            authLength = (unsigned long)(tmp - authValue);
        }
    }

    //  Copy the server-first-message UTF8 string to the AuthMessage buffer
    strncat( SCRAM_AUTH_MESSAGE, authValue, authLength);
    AuthMessageLength += authLength;
    SCRAM_AUTH_MESSAGE[AuthMessageLength] = ',';
    AuthMessageLength +=1;
    SCRAM_AUTH_MESSAGE[AuthMessageLength] = '\0';
    
    //  Parse the SCRAM server-first-message fo salt, iteration count and nonce
    //  We are looking for r=, s=, i=
    //char delim[] = ",";
    token = strtok(authValue, delim);
    //char * serverNonce = NULL;
    //char * base64Salt = NULL;
    //char * saltIterations = NULL;
    while(token != NULL)
    {
        if( token[0] == 'r'  )
            serverNonce = &(token[2]);
        else if( token[0] == 's' )
            base64Salt = &(token[2]);
        else if( token[0] == 'i' )
            saltIterations = &(token[2]);
        token = strtok(NULL, delim);
    }
    assert(serverNonce);
    assert(base64Salt);
    assert(saltIterations);
    //printf("serverNonce = %s\n", serverNonce);
    //printf("base64Salt = %s\n", base64Salt);
    //printf("saltIterations = %d\n", atoi(saltIterations));
    //printf("password = %s\n", service->password);
    
    //  While the nonce(s) is (are) already in ASCII encoding sans , and " characters,
    //  the salt is base64 encoded, so we have to decode it to UTF8
   // size_t saltLength = 0;
    salt = (char*)cr_base64_to_utf8(base64Salt, strlen(base64Salt), &saltLength );
   // assert( salt && saltLength > 0 );
	
    //  Run iterative hmac algorithm n times and concatenate the result in an XOR buffer in order to salt the password
    //      SaltedPassword  := Hi(Normalize(password), salt, i)
    //  Note:  Not really any way to tell if ct_scram_... fails
	memset(saltedPassword, 0, CC_SHA256_DIGEST_LENGTH);
    ct_scram_salt_password( service->password, strlen(service->password), salt, saltLength, atoi(saltIterations), saltedPassword );
    //printf("salted password = %.*s\n", CC_SHA256_DIGEST_LENGTH, saltedPassword);
    
    //  Generate Client and Server Key buffers
    //  by encrypting the verbatim strings "Client Key" and "Server Key, respectively,
    //  using keyed HMAC SHA 256 with the salted password as the secret key
    //      ClientKey       := HMAC(SaltedPassword, "Client Key")
    //      ServerKey       := HMAC(SaltedPassword, "Server Key")
    ct_scram_hmac(saltedPassword, CC_SHA256_DIGEST_LENGTH, clientKeyData, strlen(clientKeyData), clientKey );
    ct_scram_hmac(saltedPassword, CC_SHA256_DIGEST_LENGTH, serverKeyData, strlen(serverKeyData), serverKey );
    //printf("clientKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, clientKey);
    //printf("serverKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, serverKey);
    
    //  Calculate the "Normal" SHA 256 Hash of the clientKey
    //      storedKey := H(clientKey)
    ct_scram_hash(clientKey, CC_SHA256_DIGEST_LENGTH, storedKey);
    //printf("storedKey = %.*s\n", CC_SHA256_DIGEST_LENGTH, storedKey);
    
    //  Populate SCRAM client-final-message (ie channel binding and random nonce WITHOUT client proof)
    //  E.g.  "authentication": "c=biws,r=rOprNGfwEbeRWgbNEkqO%hvYDpWUa2RaTCAfuxFIlj)hNlF$k0,p=dHzbZapWIk4jUhN+Ute9ytag9zjfMHgsqmmiz7AndVQ="
    sprintf(CLIENT_FINAL_MESSAGE, "c=biws,r=%s", serverNonce);
    //printf("CLIENT_FINAL_MESSAGE = \n\n%s\n\n", CLIENT_FINAL_MESSAGE);
    
    //Copy the client-final-message (without client proof) to the AuthMessage buffer
    strncat( SCRAM_AUTH_MESSAGE, CLIENT_FINAL_MESSAGE, strlen(CLIENT_FINAL_MESSAGE));
    AuthMessageLength += (unsigned long)strlen(CLIENT_FINAL_MESSAGE);
    //printf("SCRAM_AUTH_MESSAGE = \n\n%s\n\n", SCRAM_AUTH_MESSAGE);
    
    //Now that we have the complete Auth Message we can use it in the SCRAM procedure
    //to calculate client and server signatures
    // ClientSignature := HMAC(StoredKey, AuthMessage)
    // ServerSignature := HMAC(ServerKey, AuthMessage)
    ct_scram_hmac(storedKey, CC_SHA256_DIGEST_LENGTH, SCRAM_AUTH_MESSAGE, AuthMessageLength, clientSignature );
    ct_scram_hmac(serverKey, CC_SHA256_DIGEST_LENGTH, SCRAM_AUTH_MESSAGE, AuthMessageLength, serverSignature );
    
    //And finally we calculate the client proof to put in the client-final message
    //ClientProof     := ClientKey XOR ClientSignature
    for(charIndex=0; charIndex<CC_SHA256_DIGEST_LENGTH; charIndex++)
        clientProof[charIndex] = (clientKey[charIndex] ^ clientSignature[charIndex]);
    
    //The Client Proof Bytes need to be Base64 encoded
    base64ProofLength = 0;
    base64Proof = cr_utf8_to_base64(clientProof, CC_SHA256_DIGEST_LENGTH, 0, &base64ProofLength );
    assert( base64Proof && base64ProofLength > 0 );
    
    //The Client Proof Bytes need to be Base64 encoded
    base64SSLength = 0;
    base64SS = cr_utf8_to_base64(serverSignature, CC_SHA256_DIGEST_LENGTH, 0, &base64SSLength );
    assert( base64SS && base64SSLength > 0 );
    //printf("Base64 Server Signature:  %s\n", base64SS);

    //Add the client proof to the end of the client-final-message
    sprintf(CLIENT_FINAL_MESSAGE_JSON, "{\"authentication\":\"%s,p=%.*s\"}", CLIENT_FINAL_MESSAGE, (int)base64ProofLength, (char*)base64Proof);
    //printf("CLIENT_FINAL_MESSAGE_JSON = \n\n%s", CLIENT_FINAL_MESSAGE_JSON);
    
    //Send the client-final-message wrapped in json
	//CLIENT_FINAL_MESSAGE_JSON[strlen(CLIENT_FINAL_MESSAGE_JSON)] = '\0';
    ReqlSend(r, CLIENT_FINAL_MESSAGE_JSON, strlen(CLIENT_FINAL_MESSAGE_JSON)+1);  //Note:  JSON always needs the extra null character to determine EOF!!!
    
    //yield();

    //Wait for SERVER_FINAL_MESSAGE
    readLength = 1024;
    sFinalMessagePtr = (char*)ReqlRecv(r, SERVER_FINAL_MESSAGE_JSON, &readLength);
    printf("SERVER_FINAL_MESSAGE_JSON = \n\n%s\n", sFinalMessagePtr);
    
    //Validate Server Signature
    authLength = 0;
    if( (authValue = strstr(sFinalMessagePtr, authKey)) != NULL )
    {
        authValue += strlen(authKey) + 1;
        //if( *successVal == ':') successVal += 1;
        if( *authValue == '"')
        {
            authValue+=1;
            tmp = (char*)authValue;
            while( *(tmp) != '"' ) tmp++;
            authLength = (unsigned long)(tmp - authValue);
        }
        
        if( *authValue == 'v' && *(authValue+1) == '=')
        {
            authValue += 2;
            authLength -= 2;
        }
        else
            authValue = NULL;
    }
    else//if( !authValue )
    {
        printf("Failed to retrieve server signature from SCRAM server-final-message.\n");
        return ReqlSASLHandshakeError;
    }
    
    //  Compare server sent signature to client generated server signature
    if( strncmp((char*)base64SS,  authValue, authLength) != 0)
    {
        printf("Failed to authenticate server signature: %s\n", authValue);
        return ReqlSASLHandshakeError;
    }
    
    //free Base64 <-> UTF8 string conversion allocations
    free( salt );
    free( base64Proof );
    free( base64SS );
    
    return ReqlSuccess;
}

#ifdef __APPLE__
void ReqlRunLoopObserver(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info)
{
    //printf("ReqlRunLoopObserver\n");
    
    //yield to any pending coroutines and wake up the run loop
    yield();
    CFRunLoopWakeUp(CFRunLoopGetCurrent());
}
#endif

coroutine int ReqlSSLRoutine(ReØMQL *conn, char * hostname, char * caPath)
{
	int ret;
    	char REQL_MAGIC_NUMBER_BUF[5] = "\xc3\xbd\xc2\x34\0";

	//OSStatus status;
	//Note:  PCredHandle (PSecHandle) is the WIN32 equivalent of SecCertificateRef
    ReqlSecCertificateRef rootCertRef = NULL;

	// Create credentials.

	//  ReqSSLCertificate (Obtains a native Security Certificate object/handle) 
	//
	//  MBEDTLS NOTES:
	//
	//  WIN32 NOTES:
	//		ReqlSecCertificateRef is of type PCredHandle on WIN32, but it needs to point to allocated memory before supplying as input to AcquireCredentialHandle (ACH)
	//		so ReqlSSLCertificate will allocate memory for this pointer and return it to the client, but the client is responsible for freeing it later 
	//
	//  DARWIN NOTES:
	//
    //		Read CA file in DER format from disk so we can use it/add it to the trust chain during the SSL Handshake Authentication
    //		-- Non (password protected) PEM files can be converted to DER format offline with OpenSSL or in app with by removing PEM headers/footers and converting (from or to?) BASE64.  
	//		   DER data may need headers chopped of in some instances.
    //		-- Password protected PEM files must have their private/public key pairs generated using PCK12.  This can be done offline with OpenSSL or in app using Secure Transport
	//
#if defined(REQL_USE_MBED_TLS) || defined(__APPLE__)
	if (caPath)
	{
		if ((rootCertRef = ReqlSSLCertificate(caPath)) == NULL)
		{
			printf("ReqlSSLCertificate failed to generate SecCertificateRef\n");
			ReqlCloseSSLSocket(conn->sslContext, conn->socket);
			return ReqlCertificateError;
		}
	}
#endif
	//ReqlSSLInit();
	
	printf("----- Credentials Initialized\n");

	//ReqlSSLContextRef sslContext = NULL;
	//  Create an SSL/TLS Context using a 3rd Party Framework such as Apple SecureTransport, OpenSSL or mbedTLS
	if( (conn->sslContext = ReqlSSLContextCreate(&(conn->socket), &rootCertRef, hostname)) == NULL )
	{
		//ReqlSSLCertificateDestroy(&rootCertRef);
        return ReqlSecureTransportError;
	}	
	printf("----- SSL Context Initialized\n");
#ifdef __APPLE__   
    
    //7.#  Elevate Trust Settings on the CA (Not on iOS, mon!)
    //const void *ctq_keys[] =    { kSecTrustSettingsResult };
    //const void *ctq_values[] =  { kSecTrustSettingsResultTrustRoot };
    //CFDictionaryRef newTrustSettings = CFDictionaryCreate(kCFAllocatorDefault, ctq_keys, ctq_values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    //status = SecTrustSettingsSetTrustSettings(rootCert, kSecTrustSettingsDomainAdmin, newTrustSettings);
    //if (status != errSecSuccess) {
    //    printf("Could not change the admin trust setting for a certificate. Error: %d", status);
    //    return -1;
    //}
    
    //  Add the CA Root Certificate to the device keychain
    //  (thus letting the system determine for us that it is a valid certificate in a SecCertificateRef, but not SecIdentityRef)
    if( (status = ReqlSaveCertToKeychain( rootCertRef )) != noErr )
    {
        printf("Failed to save root ca cert to device keychain!\n");
        ReqlCloseSSLSocket(conn->sslContext, conn->socket);
        CFRelease(rootCertRef);
        return ReqlKeychainError;
    }
    //Note:  P12 stuff used to be sandwiched here
    //SecIdentityRef certIdentity = ReqlSSLCertficateIdentity();
#endif 

	
    //  Perform the SSL Layer Auth Handshake over TLS
	if( (ret = ReqlSSLHandshake(conn->socket, conn->sslContext, rootCertRef)) != noErr )
	//if( (status = ReqlSSLHandshake(conn, rootCert)) != noErr )
    {
		//ReqlSSLCertificateDestroy(&rootCertRef);
        printf("ReqlSLLHandshakeFailed with status:  %d\n", ret);
        ReqlCloseSSLSocket(conn->sslContext, conn->socket);
        return ReqlSSLHandshakeError;
    }

	if ( (ret = ReqlVerifyServerCertificate(conn->sslContext, hostname)) != noErr)
	{
		//ReqlSSLCertificateDestroy(&rootCertRef);
        printf("ReqlVerifyServerCertificate with status:  %d\n", ret);
        ReqlCloseSSLSocket(conn->sslContext, conn->socket);
        return ReqlSSLHandshakeError;
	}
	

	//mbedtls_ssl_flush_output(&(conn->sslContext->ctx));

	//printf("conn->Socket = %d\n", conn->socket);
	//printf("conn->SocketContext.socket = %d\n", conn->socketContext.socket);
	//printf("conn->SocketContext.ctx.fd = %d\n", conn->socketContext.ctx.fd);

	//printf("conn->sslContext = %llu\n", conn->sslContext);

	/*
	//char REQL_MAGIC_NUMBER_BUF[5] = "\xc3\xbd\xc2\x34";
	size_t bytesToWrite = 4;
	int errOrBytesWritten = mbedtls_ssl_write(&(conn->sslContext->ctx), (const unsigned char *)REQL_MAGIC_NUMBER_BUF, bytesToWrite);
	if (errOrBytesWritten < 0) {
		if (errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_READ &&
			errOrBytesWritten != MBEDTLS_ERR_SSL_WANT_WRITE) {

			printf("ReqlSSLWrite::mbedtls_ssl_write = %d\n", errOrBytesWritten);

		}
}
	printf("ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);
	//printf("ReqlSSLWrite::erroOrBytesWritten = %d\n", errOrBytesWritten);

	system("pause");

	*/
	//ReqlSend(&conn, (void*)REQL_MAGIC_NUMBER_BUF, 4);


 #ifdef __APPLE__   
    CFRelease(rootCert);
#endif
    return ReqlSuccess;
}



//#ifndef socklen_t
typedef int socklen_t;
//#endif
/**
 *  ReqlSocketError
 *
 *  Wait for socket event on provided input kqueue event_queue
 ***/
coroutine int ReqlGetSocketError(REQL_SOCKET socketfd)
{
    //printf("ReqlGetSocketError\n");
    int result = 0;
   //get the result of the socket connection
    socklen_t result_len = sizeof(result);
    if ( getsockopt(socketfd, SOL_SOCKET, SO_ERROR, (char*)&result, &result_len) != 0 ) 
	{
        // error, fail somehow, close socket
        printf("getsockopt(socketfd, SOL_SOCKET, SO_ERROR,...) failed with error %d\n", errno);
        return result;//ReqlSysCallError;
    }
    return result;
}

#ifdef __APPLE__
CFRunLoopSourceContext REQL_RUN_LOOP_SRC_CTX = {0};
const CFOptionFlags REQL_RUN_LOOP_WAITING = kCFRunLoopBeforeWaiting | kCFRunLoopAfterWaiting;
const CFOptionFlags REQL_RUN_LOOP_RUNNING  = kCFRunLoopAllActivities;
#endif


#ifndef EVFILT_READ
#define EVFILT_READ		(-1)
#define EVFILT_WRITE	(-2)
#define EVFILT_TIMER	(-7)
#define EV_EOF			0x8000
#endif

coroutine int ReqlConnectRoutine(ReqlService * service, ReqlConnectionClosure callback)
{
    OSStatus status;    //But Reql API functions that purely rely on system OSStatus functions will return OSStatus
	//int ret;
	//char port[6];

    //  Coroutines rely on integration with a run loop [running on a thread] to return operation
    //  to the coroutine when it yields operation back to the calling run loop
    //
    //  For Darwin platforms that either:
    //   1)  A CFRunLoop associated with a main thread in a cocoa app [Cocoa]
    //   2)  A CFRunLoop run from a background thread in a Darwin environment [Core Foundation]
    //   3)  A custom event loop implemented with kqueue [Custom]
    //CFRunLoopRef         runLoop;
    //CFRunLoopSourceRef   runLoopSource;
    //CFRunLoopObserverRef runLoopObserver;
    
    //Allocate connection as stack memory until we are sure the connection is valid
    //At which point we will copy result to the client via the closure result block
    ReØMQL conn = {0};
    ReqlError error = {(ReqlErrorClass)0,0,0};    //Reql API client functions will generally return ints as errors

	conn.socketContext.host = service->host;
    //Allocate connection as dynamic memory
    //ReØMQL * conn = (ReØMQL*)calloc( 1, sizeof(ReØMQL));//{0};
    
    /*
    //Get current run loop
    runLoop = CFRunLoopGetCurrent();
    
    //Create and install a dummy run loop source as soon as possible to keep the loop alive until the async coroutine completes
    runLoopSource = CFRunLoopSourceCreate(NULL, 0, &REQL_RUN_LOOP_SRC_CTX);
    CFRunLoopAddSource( runLoop, runLoopSource, kCFRunLoopDefaultMode);
    
    //Create a run loop observer to inject yields from the run loop back to this (and other) coroutines
    // TO DO:  It would be nice to avoid this branching
    CFOptionFlags runLoopActivities = ( runLoop == CFRunLoopGetMain() ) ? REQL_RUN_LOOP_WAITING : REQL_RUN_LOOP_RUNNING;
    if ( !(runLoopObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, runLoopActivities, true, 0, &ReqlRunLoopObserver, NULL)) )
    {
        printf("Failed to create run loop observer!\n");
        error.id = ReqlRunLoopError;
        goto CONN_CALLBACK;
    }

    //  Install a repeating run loop observer so we can call coroutine API yield
    CFRunLoopAddObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
    */

#ifdef _WIN32//REQL_USE_MBED_TLS
	printf("Before ReqlSocket()\n");
    //  Create a [non-blocking] TCP socket and return a socket fd
    if( (conn.socket = ReqlSocket()) < 0)
    {
        error.id = (int)conn.socket; //Note: hope that 64 bit socket handle is within 32 bit range?!
        goto CONN_CALLBACK;
        //return conn.socket;
    }

		printf("Before ReqlSocketCreateEventQueue(0)\n");

	if( ReqlSocketCreateEventQueue(&(conn.socketContext)) < 0)
	{
		printf("ReqlSocketCreateEventQueue failed\n");
		error.id = (int)conn.event_queue;
        goto CONN_CALLBACK;
	}

			printf("Before ReqlSocketConnect\n");

    //  Connect the [non-blocking] socket to the server asynchronously and return a kqueue fd
    //  associated with the socket so we can listen/wait for the connection to complete
    if( (conn.event_queue = ReqlSocketConnect(conn.socket, service)) < 0 )
    {
        error.id = (int)conn.socket;
        goto CONN_CALLBACK;
        //return conn.event_queue;
    }
	//printf("ReqlSocketConnect End\n");
#else
	itoa((int)service->port, port, 10);
	mbedtls_net_init(&(conn.socketContext.ctx));
	if ( (ret = mbedtls_net_connect(&(conn.socketContext.ctx), service->host, port, MBEDTLS_NET_PROTO_TCP)) != 0)
	{
		Reql_printf("ReqlConnectRoutine::mbedtls_net_connect failed and returned %d\n\n", ret);
		return ret;
	}
#endif

    //Yield operation back to the calling run loop
    //This wake up is critical for pumping on idle, but I am not sure why it needs to come after the yield
#ifndef _WIN32
    yield();
#endif
#ifdef __APPLE__
    CFRunLoopWakeUp(CFRunLoopGetCurrent());
#endif

	printf("Before ReqlSocketWait\n");

    //Wait for socket connection to complete
    if( ReqlSocketWait(conn.socket, conn.event_queue, EVFILT_WRITE) != conn.socket )
    {
        printf("ReqlSocketWait failed\n");
        error.id = ReqlEventError;
        ReqlCloseSocket(conn.socket);
        goto CONN_CALLBACK;
        //return ReqlEventError;
    }

		printf("Before ReqlGetSocketError\n");

    //Check for socket error to ensure that the connection succeeded
    if( (error.id = ReqlGetSocketError(conn.socket)) != 0 )
    {
        // connection failed; error code is in 'result'
        printf("socket async connect failed with result: %d", error.id );
        ReqlCloseSocket(conn.socket);
        error.id = ReqlConnectError;
        goto CONN_CALLBACK;
        //return ReqlConnectError;
    }
    
    //There is some short order work that would be good to parallelize regarding loading the cert and saving it to keychain
    //However, the SSLHandshake loop has already been optimized with yields for every asynchronous call

	printf("Before ReqlSSLRoutine Host = %s\n", conn.socketContext.host);
	printf("Before ReqlSSLRoutine Host = %s\n", service->host);

    if( (status = ReqlSSLRoutine(&conn, conn.socketContext.host, service->ssl.ca)) != 0 )
    {
        printf("ReqlSSL failed with error: %d\n", (int)status );
        error.id = ReqlSSLHandshakeError;
        goto CONN_CALLBACK;
        //return status;
    }
       
    //  Now Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
    if( (status = ReqlSASLHandshake(&conn, service)) != ReqlSuccess )
    {
        printf("ReqlSASLHandshake failed!\n");
        ReqlCloseSSLSocket(conn.sslContext, conn.socket);
        error.id = ReqlSASLHandshakeError;
        goto CONN_CALLBACK;
        //return status;
    }

    //printf("ReqlSASLHandshake success!\n");

    //The connection completed successfully, allocate memory to return to the client
    error.id = ReqlSuccess;
    
    //generate a client side token for the connection
    conn.token = ++REQL_NUM_CONNECTIONS;
    //conn.query_buf[0] = (void*)malloc( REQL_QUERY_BUFF_SIZE);
    //conn.query_buf[1] = (void*)malloc( REQL_QUERY_BUFF_SIZE);
    //conn.response_buf[0] = (void*)malloc( REQL_RESPONSE_BUFF_SIZE);
    //conn.response_buf[1] = (void*)malloc( REQL_RESPONSE_BUFF_SIZE);
    
CONN_CALLBACK:
    //printf("before callback!\n");

	conn.service = service;
    callback(&error, &conn);
    //printf("After callback!\n");

    /*
    //Remove observer that yields to coroutines
    CFRunLoopRemoveObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
    //Remove dummy source that keeps run loop alive
    CFRunLoopRemoveSource(runLoop, runLoopSource, kCFRunLoopDefaultMode);
    
    //release runloop object memory
    CFRelease(runLoopObserver);
    CFRelease(runLoopSource);
    */
    
    //return the error id
    return error.id;
}


#ifdef __APPLE__
static void ReqlResolveHostAddressCallback(CFHostRef theHost, CFHostInfoType typeInfo, const CFStreamError * __nullable error, void * __nullable info) {
    
    //#pragma unused(theHost)
    //#pragma unused(typeInfo)
    printf("ReqlResolveHostAddressCallback\n");

    if ( error && (error->error != 0) )
    {
        printf("HostClientCallback DNS Resolve failed with error:  %d  and domain:  %d\n", (int)(error->error), (int)(error->domain) );
        stopHostWithError(theHost, error, false);
        return;
    }
    
    
    //get a sockaddr containing the resolved DNS address from the CFHostRef
    Boolean hasBeenResolved;
    CFArrayRef resolvedAddresses = CFHostGetAddressing(theHost, &hasBeenResolved);
    //assert(hasBeenResolved);
    
    //CFIndex addressCount = CFArrayGetCount(resolvedAddresses);
    //assert(addressCount > 0);
    
    //Each address is a CFDataRef wrapping a struct sockaddr.
    //Just use the first resolved address for now.
    CFDataRef sockaddrDataRef = (CFDataRef)CFArrayGetValueAtIndex(resolvedAddresses, 0);
    
    ReqlService * service = (ReqlService*)info;
    assert(service);
    
    //pull the closure out of the service
    ReqlConnectionClosure callback = *(ReqlConnectionClosure*)&(service->host_addr);
    
    //printf("%s\n", conn->);
    //  We can get the sockaddr from the CFDataRef in one of two ways:
    //  1)  Copy the data
    CFDataGetBytes(sockaddrDataRef, CFRangeMake(0, CFDataGetLength(sockaddrDataRef)), (UInt8*)&(service->host_addr));
    //assert(CFDataGetLength(sockaddrDataRef) == sizeof(struct sockaddr_in));
    //printf("sockaddr = %s\n", conn->service->host_addr.sin_addr);
    //  2)  Get a pointer to CFDataRef internal memory
    
    printf("Commencing go routine\n");
    //Commence Connection with Async Goroutine Procedure
    go(ReqlConnectRoutine(service, callback));
}

CFRunLoopSourceRef ReqlCreateRunLoopSource()
{
    //Create and install a dummy run loop source as soon as possible to keep the loop alive until the async coroutine completes
    CFRunLoopSourceRef runLoopSource = CFRunLoopSourceCreate(NULL, 0, &REQL_RUN_LOOP_SRC_CTX);
    if( runLoopSource) CFRunLoopAddSource( CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    return runLoopSource;
}

CFRunLoopObserverRef  ReqlCreateRunLoopObserver()
{
    //  Coroutines rely on integration with a run loop [running on a thread] to return operation
    //  to the coroutine when it yields operation back to the calling run loop
    //
    //  For Darwin platforms that either:
    //   1)  A CFRunLoop associated with a main thread in a cocoa app [Cocoa]
    //   2)  A CFRunLoop run from a background thread in a Darwin environment [Core Foundation]
    //   3)  A custom event loop implemented with kqueue [Custom]
    CFRunLoopObserverRef runLoopObserver = NULL;
    
    //Get current run loop
    CFRunLoopRef runLoop = CFRunLoopGetCurrent();
    
    //Create a run loop observer to inject yields from the run loop back to this (and other) coroutines
    // TO DO:  It would be nice to avoid this branching
    CFOptionFlags runLoopActivities = ( runLoop == CFRunLoopGetMain() ) ? REQL_RUN_LOOP_WAITING : REQL_RUN_LOOP_RUNNING;
    //if ( !(runLoopObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, runLoopActivities, true, 0, &ReqlRunLoopObserver, NULL)) )
    //{
    //    return ReqlRunLoopError;
    //}
    runLoopObserver = CFRunLoopObserverCreate(kCFAllocatorDefault, runLoopActivities, true, 0, &ReqlRunLoopObserver, NULL);
    //  Install a repeating run loop observer so we can call coroutine API yield
    if( runLoopObserver ) CFRunLoopAddObserver(runLoop, runLoopObserver, kCFRunLoopDefaultMode);
    return runLoopObserver;
}

#endif

/***
 *  ReqlServiceResolveHost
 *
 *  DNS Lookups can take a long time on iOS, so we need an async DNS resolve mechanism.
 *  However, getaddrinfo_a is not available for Darwin, so we use CFHost API to schedule
 *  the result of the async DNS lookup on internal CFNetwork thread which will post our result
 *  callback to the calling runloop
 *
 ***/
coroutine int ReqlServiceResolveHost(ReqlService * service, ReqlConnectionClosure callback)
{
 
#ifndef _WIN32
    //Launch asynchronous query using modified version of Sustrik's libdill DNS implementation
    struct dill_ipaddr addr[1];
    struct dns_addrinfo *ai = NULL;
    if( dill_ipaddr_dns_query_ai(&ai, addr, 1, service->host, service->port, DILL_IPADDR_IPV4, service->dns.resconf, service->dns.nssconf) != 0 || ai == NULL )
    {
        printf("service->dns.resconf = %s", service->dns.resconf);
        printf("dill_ipaddr_dns_query_ai failed with errno:  %d", errno);
        ReqlError err = {ReqlDriverErrorClass, ReqlDNSError, NULL};
        callback(&err, NULL);
        return ReqlDNSError;
    }
    
    //TO DO:  We could use this opportunity to do work that validates ReqlService properties, like loading SSL Certificate
    //CFRunLoopWakeUp(CFRunLoopGetCurrent());
    //CFRunLoopSourceSignal(runLoopSource);
    CFRunLoopWakeUp(CFRunLoopGetCurrent());

    //printf("Before DNS wait\n");
    yield();

    int numResolvedAddresses = 0;
    if( (numResolvedAddresses = dill_ipaddr_dns_query_wait_ai(ai, addr, 1, service->port, DILL_IPADDR_IPV4, -1)) < 1 )
    {
        //printf("dill_ipaddr_dns_query_wait_ai failed to resolve any IPV4 addresses!");
        ReqlError err = {ReqlDriverErrorClass, ReqlDNSError, NULL};
        callback(&err, NULL);
        return ReqlDNSError;
    }
 
    //printf("DNS resolved %d ip address(es)\n", numResolvedAddresses);
    struct sockaddr_in * ptr = (struct sockaddr_in*)addr;//(addr[0]);
    service->host_addr = *ptr;//(struct sockaddr_in*)addresses;
#else

//TODO:
	//--------------------------------------------------------------------
//  ConnectAuthSocket establishes an authenticated socket connection 
//  with a server and initializes needed security package resources.

	DWORD dwRetval;
	char port[6];
	struct addrinfo *result = NULL;
    struct addrinfo *ptr = NULL;
    struct addrinfo hints;
	int i = 0;

    struct sockaddr_in  *sockaddr_ipv4;
    //unsigned long  ulAddress;
	struct hostent *pHost = NULL;
    //sockaddr_in    sin;

    //--------------------------------------------------------------------
    //  Lookup the server's address.
		printf("ReqlServiceResolveHost host = %s\n", service->host);

    service->host_addr.sin_addr.s_addr = inet_addr (service->host);
    if (INADDR_NONE == service->host_addr.sin_addr.s_addr ) 
    {	
		
		/*
		pHost = gethostbyname (service->host);
		if ( pHost==NULL) {
			printf("gethostbyname failed\n");
			//WSACleanup();
			return -1;
		}
		memcpy((void*)&(service->host_addr.sin_addr.s_addr) , (void*)(pHost->h_addr), (size_t)(strlen(pHost->h_addr)) );		 

		//memcpy((void*)&(service->addr) , (void*)*(pHost->h_addr_list), (size_t)(pHost->h_length));		 
		*/
		
		//set port param
		_itoa((int)service->port, port, 10);
		//
		memset(&hints, 0, sizeof(struct addrinfo));
		dwRetval = getaddrinfo(service->host, port, &hints, &result);
		if ( dwRetval != 0 ) {
			printf("getaddrinfo failed with error: %d\n", dwRetval);
			//WSACleanup();
			return -1;
		}
		
		 // Retrieve each address and print out the hex bytes
		for(ptr=result; ptr != NULL ;ptr=ptr->ai_next) 
		{



			printf("getaddrinfo response %d\n", i++);
			printf("\tFlags: 0x%x\n", ptr->ai_flags);
			printf("\tFamily: ");
			printf("\tLength of this sockaddr: %d\n", (int)(ptr->ai_addrlen));
			printf("\tCanonical name: %s\n", ptr->ai_canonname);
		
			if( ptr->ai_family != AF_INET )
			{
				continue;
			}
			else
			{
				sockaddr_ipv4 = (struct sockaddr_in*)result->ai_addr;
					printf("ReqlServiceResolveHost host before copy = %s\n", service->host);

				memcpy(&(service->host_addr) , sockaddr_ipv4, result->ai_addrlen);		 
						printf("ReqlServiceResolveHost host after copy = %s\n", service->host);

				break;
			}
		
		 }
	    freeaddrinfo(result);
		

	}
	
#endif    
    //Perform the socket connection
	printf("ReqlServiceResolveHost host = %s\n", service->host);
    ReqlConnectRoutine(service, callback);

    /*
    //Remove observer that yields to coroutines
    CFRunLoopRemoveObserver(CFRunLoopGetCurrent(), runLoopObserver, kCFRunLoopDefaultMode);
    //Remove dummy source that keeps run loop alive
    CFRunLoopRemoveSource(CFRunLoopGetCurrent(), runLoopSource, kCFRunLoopDefaultMode);
    
    //release runloop object memory
    CFRelease(runLoopObserver);
    CFRelease(runLoopSource);
    */
    
    return ReqlSuccess;
}

/*
void ReqlReinitSecurityContext(ReqlConnection * conn)
{
	
	ReqlSSLRoutine(conn, conn->socketContext.host);
}
*/
/***
 *  ReqlConnect
 *
 *  Asynchronously connect to a remote RethinkDB Service with a ReqlService
 *
 *  Schedules DNS host resolve and returns immediately
 ***/
int ReqlConnect(ReqlService * service, ReqlConnectionClosure callback)
{
	//-------------------------------------------------------------------
    //  Initialize the socket libraries (ie Winsock 2)
	ReqlSocketInit();

	//-------------------------------------------------------------------
    //  Initialize the socket and the SSP security package.
	//ReqlSSLInit();


	
	//-------------------------------------------------------------------
    //  Initialize the scram SSP security package.
	ct_scram_init();
	//assert((*r) == NULL);
    //OSStatus status = 0;

    //  Allocate memory on the stack until we are ready to copy it to the user
    //ReØMQL conn = {0};

    //  Allocate + init memory to zero for a ReØMQL [ReqlConnection] object,
    //  which contains all the items we need to create/populate
    //  to connect to a RethinkDB ReQL Service [ReqlService].
    //ReØMQL * conn = (ReØMQL*)calloc(1, sizeof(ReØMQL));
    
    //  Store a reference to the ReqlService [ReqlConnectionOptions] input on the connection object
    //conn->service = service;
    
    //int socketfd;
    //int event_queue;
    //SSLContextRef sslContext;
    //SecCertificateRef  rootCert;
    
    //TO DO:  Check ReqlService input for validity/reachability

    //go(ReqlConnectRoutine(service, callback));
    //return ReqlSuccess;
    
    


    //  Issue a request to resolve Host DNS to a sockaddr asynchonrously using CFHost API
    //  It will perform the DNS request asynchronously on a bg thread internally maintained by CFHost API
    //  and return the result via callback to the current run loop
    //go(ReqlServiceResolveHost(service, callback));
	ReqlServiceResolveHost(service, callback);
	return 0;
    //  Resolve DNS Hostname to asynchronously generate a hostent
    //  Note:  This is the first network call we will have to wait for.
    //  So this is a good opportunity to return to the user and perform the remaining connection work/messaging asynchronously.
    //  But first, we must prepare the coroutines to be scheduled
    //if( (status = ReqlResolveHostAddress( )) < 0 )
    //{
    //    ReqlCloseSocket(conn->socket);
    //    return status;
    //}
    
    //  Connect to the socket and return a kqueue fd associated with the socket
    //  Note:  If the socket was created as a non-blocking socket (which it always is),
    //  the result of the connection must be listened for asychronously, as is intended behavior
    //if( (conn->event_queue = ReqlSocketConnect(conn->socket, service)) < 0 )
    //    return conn->event_queue;
    
    /*
    //Now we will continue the remainder of the connection procedure asynchronously
    //By starting coroutine(s) and injecting calls to yield()back to the calling runloop
    //We create a libdill bundle to put our single connect coroutine in
    int connection_bundle = bundle();
    bundle_go(connection_bundle, ReqlConnectRoutine(conn, service));
    printf("After ReqlCompletConnection\n");
    */
    //return ReqlSuccess;
    
    /*
     //  Create an SSL/TLS Context using a 3rd Party Framework such as Apple SecureTransport, OpenSSL or mbedTLS
     if( (sslContext = ReqlSSLContext(socketfd)) == NULL )
        return ReqlSecureTransportError;
     
    //  Read CA file in DER format from disk so we can use it/add it to the trust chain during the SSL Handshake Authentication
    //  -- Non (password protected) PEM files can be converted to DER format offline with OpenSSL or in app with by removing PEM headers/footers and converting (from or to?) BASE64.  DER data may need headers chopped of in some instances.
    //  -- Password protected PEM files must have their private/public key pairs generated using PCK12.  This can be done offline with OpenSSL or in app using Secure Transport
    if( (rootCert = ReqlSSLCertificate(service->caPath)) == NULL )
    {
        printf("ReqlSSLCertificate failed to generate SecCertificateRef\n");
        ReqlCloseSSLSocket(sslContext, socketfd);
        return ReqlCertificateError;
    }
    
    //7.#  Elevate Trust Settings on the CA (Not on iOS, mon!)
    //const void *ctq_keys[] =    { kSecTrustSettingsResult };
    //const void *ctq_values[] =  { kSecTrustSettingsResultTrustRoot };
    //CFDictionaryRef newTrustSettings = CFDictionaryCreate(kCFAllocatorDefault, ctq_keys, ctq_values, 3, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    //status = SecTrustSettingsSetTrustSettings(rootCert, kSecTrustSettingsDomainAdmin, newTrustSettings);
    //if (status != errSecSuccess) {
    //    printf("Could not change the admin trust setting for a certificate. Error: %d", status);
    //    return -1;
    //}
    
    //  Add the CA Root Certificate to the device keychain
    //  (thus letting the system determine for us that it is a valid certificate in a SecCertificateRef, but not SecIdentityRef)
    if( (status = ReqlSaveCertToKeychain( rootCert )) != noErr )
    {
        printf("Failed to save root ca cert to device keychain!\n");
        ReqlCloseSSLSocket(sslContext, socketfd);
        CFRelease(rootCert);
        return ReqlKeychainError;
    }
    
    //Note:  P12 stuff used to be sandwiched here
    //SecIdentityRef certIdentity = ReqlSSLCertficateIdentity();
    
    //  Perform the SSL Layer Auth Handshake over TLS
    if( (status = ReqlSSLHandshake(sslContext, rootCert)) != noErr )
    {
        printf("Failed to save root ca cert to device keychain with status:  %d\n", status);
        ReqlCloseSSLSocket(sslContext, socketfd);
        return ReqlSSLHandshakeError;
    }
    
    //  If everything went well, we have a valid TLS + TCP Connection
    //  and we can allocate memory for the ReØMQL object struct
    //  to set the properties we care about for subsequent socket use and return to the client
    *r = calloc(1, sizeof(struct rethinkdb_connection));
    (*r)->socket = socketfd;
    (*r)->sslContext = sslContext;
    (*r)->event_queue = event_queue;
    
    //  Now Perform RethinkDB ReQL SASL Layer Auth Handshake (using SCRAM HMAC SHA-256 Auth) over TCP
    if( (status = ReqlSASLHandshake(*r, service)) != ReqlSuccess )
    {
        printf("ReqlSASLHandshake failed!\n");
        ReqlCloseSSLSocket(sslContext, socketfd);
        free(*r); *r = NULL;
        CFRelease(rootCert);
        return status;
    }

    ReqlSendQuery(*r, NULL, 0);
    
    //Cleanup
    CFRelease(rootCert);
     */
    return ReqlSuccess;
}

extern int ReqlCloseConnection( ReqlConnection * conn )
{
	
	//if( conn->query_buffers )
	//	free(conn->query_buffers);
	
	if (conn->query_buffers)
	{
		VirtualFree(conn->query_buffers, 0, MEM_RELEASE);
		//free( conn->response_buffers );
	}
	conn->query_buffers = NULL;
	
	if( conn->response_buffers )
	{
		VirtualFree(conn->response_buffers, 0, MEM_RELEASE);
		//free( conn->response_buffers );
	}
	conn->response_buffers = NULL;
	/*
	if( conn->query_buf[0] )
		free(conn->query_buf[0]);
	if( conn->query_buf[1] )
		free(conn->query_buf[1]);
	if( conn->response_buf[0] )
		free(conn->response_buf[0]);
	if( conn->response_buf[1] )
		free(conn->response_buf[1]);
	*/
	
    return ReqlCloseSSLSocket(conn->sslContext, conn->socket);
}


int ReqlSendQuery(ReØMQL * conn, Query * query)
{
 
    /*
    ReqlQueryMessageHeader queryHeader;
    printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    ReqlQuery queryStack[10];
    queryStack[0] = ReqlTableQuery("EARTHFORMS_DEV");
    queryStack[1] = ReqlTableQuery("users");
    int numQueries = 1;
    
    char queryBuf[4096];
    char * queryStr = queryBuf + sizeof(ReqlQueryMessageHeader);
    *queryStr = '\0';
    
    //int queryStackType = queryStack[0].type;
    char dbQueryBuf[4096];
    sprintf(dbQueryBuf, "%s", "[15, [[14, [\"EARTHFORMS_DEV\"]], \"users\"]]");

    printf("dbQueryBuf = \n\n%s\n\n", dbQueryBuf);

    sprintf(queryStr, "[1,%s,{}]", dbQueryBuf);
    
    
    for( int queryIndex=1; queryIndex<numQueries; queryIndex++)
    {
        sprintf(queryStr, "[%d,[%s],\"%s\"]", queryStack[queryIndex].type, dbQueryBuf, (char*)queryStack[queryIndex].data);
    }
    printf("queryStr = \n\n%s\n\n", queryStr);
    
    static uint64_t queryID = 0;
    queryHeader.token = ++queryID;//conn->token;
    queryHeader.length = (int32_t)strlen(queryStr);

    int32_t queryBufLength = (int32_t)(sizeof(ReqlQueryMessageHeader) + strlen(queryStr)) + 1;
    //char queryBuf[queryBufLength];// = "c3bdc234";
    
    memcpy(queryBuf, &queryHeader, sizeof(ReqlQueryMessageHeader));
    //memcpy(queryBuf + sizeof(ReqlQueryMessageHeader), queryStr, strlen(queryStr));
    
    
    queryBuf[0] = 0;
    queryBuf[1] = 0;
    queryBuf[2] = 0;
    queryBuf[3] = 0;
    queryBuf[4] = 0;
    queryBuf[5] = 0;
    queryBuf[6] = 0;
    queryBuf[7] = 1;
    
    //queryBuf[8] = 0x0c;
    //queryBuf[9] = 0;
    //queryBuf[10] = 0;
    //queryBuf[11] = 0;

    queryBuf[queryBufLength-1] = '\0';
    
    printf("sendBuffer = \n\n%s\n", queryBuf);
    ReqlSend(conn, queryBuf, queryBufLength);
    
    //0.1 Sync Wait for magic number response
    char readBuffer[4096];
    size_t readLength = 4096;
    ReqlRecv(conn, readBuffer, &readLength);
    printf("readBuffer = \n\n%.*s\n", readLength - 12, readBuffer+12);
    
    return 0;
     */
    
    return 0;
}

/***
 *  ReqlRecv
 *
 *  ReqlRecv will wait on a ReqlConnection's full duplex TCP socket
 *  for an EOF read event and return the buffer to the calling function
 ***/

void* ReqlRecv(ReqlConnection* conn, void * msg, unsigned long * msgLength)
{
    //OSStatus status;
    int ret; 
	char * decryptedMessagePtr = NULL; 
	unsigned long  totalMsgLength = 0;
	unsigned long remainingBufferSize = *msgLength;

	 //printf("SSLRead remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);

    //struct kevent kev;
    //uintptr_t timout_event = -1;
    //uintptr_t out_of_range_event = -2;
    //uint32_t timeout = UINT_MAX;  //uint max will translate to timeout struct of null, which is timeout of forever
    
    //printf("ReqlRecv Waiting for read...\n");
    //uintptr_t ret_event = kqueue_wait_with_timeout(r->event_queue, &kev, EVFILT_READ | EV_EOF, timout_event, out_of_range_event, r->socket, r->socket, timeout);
    if( ReqlSocketWait(conn->socket, conn->event_queue, EVFILT_READ | EV_EOF) != conn->socket )
    {
        printf("something went wrong\n");
        return decryptedMessagePtr;
    }
#ifdef _WIN32
	ret = ReqlSSLRead(conn->socket, conn->sslContext, msg, &remainingBufferSize);
	if( ret <= 0 )
		ret = 0;
	else
	{
#ifdef REQL_USE_MBED_TLS
		decryptedMessagePtr = msg;
#elif defined(_WIN32)
		decryptedMessagePtr = (char*)msg + conn->sslContext->Sizes.cbHeader;
#else
		decryptedMessagePtr = msg;
#endif
	}
	/*
	ret = (size_t)recv(conn->socket, (char*)msg, remainingBufferSize, 0 );
	
	if(ret == SOCKET_ERROR)
    {
        printf("**** ReqlRecv::Error %d reading data from server\n", WSAGetLastError());
        //scRet = SEC_E_INTERNAL_ERROR;
        return -1;
    }
    else if(ret== 0)
    {
        printf("**** ReqlRecv::Server unexpectedly disconnected\n");
        //scRet = SEC_E_INTERNAL_ERROR;
        return -1;
    }
    printf("ReqlRecv::%d bytes of handshake data received\n\n%.*s\n\n", ret, ret, (char*)msg);
	*/

	*msgLength = (size_t)ret;
	totalMsgLength += *msgLength;
	remainingBufferSize -= *msgLength;
#elif defined(__APPLE__)
    status = SSLRead(conn->sslContext, msg, remainingBufferSize, msgLength);

	totalMsgLength += *msgLength;
    remainingBufferSize -= *msgLength;

    //printf("Before errSSLWouldBlock...\n");

    while( status == errSSLWouldBlock)
    {
        //TO DO:  Figure out why this is happening for changefeeds ...
        printf("SSLRead remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);
        status = SSLRead(conn->sslContext, msg+totalMsgLength, remainingBufferSize, msgLength);
        totalMsgLength += *msgLength;
        remainingBufferSize -= *msgLength;
    }

	if( status != noErr )
    {
        printf("ReqlRecv::SSLRead failed with error: %d\n", (int)status);
    }
#endif
    //printf("SSLRead final remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);
    *msgLength = totalMsgLength;
    return decryptedMessagePtr;
}
           
/***
 *  ReqlSend
 *
 *  Sends Raw Bytes ReqlConnection socket TLS + TCP Stream
 *  Will wait for the socket to begin writing to the network before returning
 ****/
int ReqlSend(ReqlConnection* conn, void * msg, unsigned long msgLength )
{
    //OSStatus status;
	int ret = 0;
    unsigned long bytesProcessed = 0;
    unsigned long max_msg_buffer_size = 4096;
    
    //TO DO:  check response and return to user if failure
    unsigned long totalBytesProcessed = 0;
    //printf("\nconn->token = %llu", conn->token);
    //printf("msg = %s\n", (char*)msg + sizeof( ReqlQueryMessageHeader) );
    uint8_t * msg_chunk = (uint8_t*)msg;
    
	assert(conn);
    assert(msg);
	
	while( totalBytesProcessed < msgLength )
    {
        unsigned long remainingBytes = msgLength - totalBytesProcessed;
        unsigned long msg_chunk_size = (max_msg_buffer_size < remainingBytes) ? max_msg_buffer_size : remainingBytes;

#ifdef _WIN32
		printf("ReqlSend::preparing to send: \n%.*s\n", msgLength, (char*)msg);
		//ret = send(conn->socket, (const char*)msg_chunk, msg_chunk_size, 0 );
		assert( conn );
		ret = ReqlSSLWrite(conn->socket, conn->sslContext, msg_chunk, &msg_chunk_size);
		if(ret == SOCKET_ERROR || ret == 0)
        {
			printf( "****ReqlSend::Error %d sending data to server\n",  WSAGetLastError() );
            return ret;
        }
		//printf("%d bytes of handshake data sent\n", ret);
		bytesProcessed = ret;
#elif defined(__APPLE__)
        status = SSLWrite(conn->sslContext, msg_chunk, msg_chunk_size, &bytesProcessed);
#endif
		totalBytesProcessed += bytesProcessed;
        msg_chunk += bytesProcessed;
    }
    /*
    printf("msg length = %lu\n", msgLength);

    assert(msg);
    status = SSLWrite(conn->sslContext, msg, msgLength, &bytesProcessed);
    if( status != noErr )
    {
        while( status == errSSLWouldBlock )
        {
            printf("Bytes processed = %lu\n", bytesProcessed);
            msgLength -= bytesProcessed;
            status = SSLWrite(conn->sslContext, msg, msgLength, &bytesProcessed );
        }
        
        printf("SSLWrite failed with error: %d\n", status);
    }
    */
    //wait for write on ReØMQL object write queue
    //printf("kqueue wait\n...they don't love you like i love you\n");
    //idle until we receive kevent from our kqueue
    //struct kevent kev;
    //uintptr_t timout_event = -1;
    //uintptr_t out_of_range_event = -2;
    //uint32_t timeout = UINT_MAX;  //uint max will translate to timeout struct of null, which is timeout of forever
    
    //uintptr_t ret_event = kqueue_wait_with_timeout(r->event_queue, &kev, EVFILT_WRITE | EV_EOF, timout_event, out_of_range_event, r->socket, r->socket, timeout);
    
    //CFRunLoopWakeUp(CFRunLoopGetCurrent());
    //printf("Reql Send Wait for socket\n");
    //yield();
    //CFRunLoopWakeUp(CFRunLoopGetCurrent());

    //printf("Send Socket wait\n");
    //Any subsequent
    //So for now, our api will wait until the non-blocking socket begins to write to the network
    //before returning
    if( ReqlSocketWait(conn->socket, conn->event_queue, EVFILT_WRITE | EV_EOF) != conn->socket )
    {
        printf("something went wrong\n");
        return -1;
    }
    //printf("Send Socket wait End\n");

    //printf("ReqlSend wait complete\n");
    //printf("After Socket wait\n");

    return 0;
}


/***
 *	ReqlSendWithQueue
 * 
 *  Sends a message buffer to a dedicated platform "Queue" mechanism
 *  for encryption and then transmission over socket
 ***/
ReqlDriverError ReqlSendWithQueue(ReqlConnection* conn, void * msg, unsigned long * msgLength)
{
#ifdef _WIN32

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;
	//Send Query Asynchronously with Windows IOCP
	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the send buffer is large enough and tack it on the end
	ReqlOverlappedQuery * overlappedQuery = (ReqlOverlappedQuery*) ( (char*)msg + *msgLength + 1 ); //+1 for null terminator needed for encryption
	//overlappedQuery = (ReqlOverlappedQuery *)malloc( sizeof(ReqlOverlappedQuery) );
	ZeroMemory(overlappedQuery, sizeof(ReqlOverlappedQuery)); //this is critical for proper overlapped/iocp operation!
	overlappedQuery->Overlapped.hEvent = CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedQuery->buf = (char*)msg;
	overlappedQuery->len = *msgLength;
	overlappedQuery->conn = conn;
	overlappedQuery->queryToken = *(uint64_t*)msg;

	printf("overlapped->queryBuffer = %s\n", (char*)msg + sizeof(ReqlQueryMessageHeader));

	//Post the overlapped object message asynchronously to the socket transmit thread queue using Win32 Overlapped IO and IOCP
	if( !PostQueuedCompletionStatus(conn->socketContext.txQueue, *msgLength, dwCompletionKey, &(overlappedQuery->Overlapped) ) )
	{
		printf("\nReqlAsyncSend::PostQueuedCompletionStatus failed with error:  %d\n", GetLastError());
		return (ReqlDriverError)GetLastError();
	}

	/*
	//Issue the async send
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
	if( WSASend(conn->socket, &(overlappedConn->wsaBuf), 1, msgLength, overlappedConn->Flags, &(overlappedConn->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			printf( "****ReqlAsyncSend::WSASend failed with error %d \n",  WSAGetLastError() );	
		}
		
		//forward the winsock system error to the client
		return (ReqlDriverError)WSAGetLastError();
	}
	*/

#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return ReqlSuccess;

}

static volatile int _reading  =0;
static volatile int _writing  =0;

/***
 *  ReqlRecv
 *
 *  ReqlRecv will wait on a ReqlConnection's full duplex TCP socket
 *  for an EOF read event and return the buffer to the calling function
 ***/

int ReqlRecvQueryResponse(ReqlConnection* conn, void * msg, size_t * msgLength)
{
	ReqlQueryMessageHeader * queryHeader;
    OSStatus status = noErr;
     size_t remainingMessageLength, remainingBufferSize, totalMsgLength = 0;
    remainingBufferSize = 0;
    //assert(!_reading && !_writing);
    _reading = 1;
    //struct kevent kev;
    //uintptr_t timout_event = -1;
    //uintptr_t out_of_range_event = -2;
    //uint32_t timeout = UINT_MAX;  //uint max will translate to timeout struct of null, which is timeout of forever
    
    printf("ReqlRecvQueryResponse for read... %d\n", (int)conn->queryCount);
    
    //size_t totalMsgLength = 0;
    //size_t remainingBufferSize = *msgLength;
    *msgLength = 0;
    
    
    while( *msgLength == 0 )
    {
        //uintptr_t ret_event = kqueue_wait_with_timeout(r->event_queue, &kev, EVFILT_READ | EV_EOF, timout_event, out_of_range_event, r->socket, r->socket, timeout);
        uintptr_t event = ReqlSocketWait(conn->socket, conn->event_queue, EVFILT_READ | EV_EOF | EVFILT_TIMER);
        if(  event != conn->socket )
        {
            printf("something went wrong\n");
            if( event == REQL_EVT_TIMEOUT)
                printf("REQL_EVT_TIMEOUT");
            _reading = 0;
            return -1;
        }
        
        printf("SSLRead remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);

#ifdef _WIN32

#elif defined(__APPLE__)
        status = SSLRead(conn->sslContext, msg, sizeof(ReqlQueryMessageHeader), msgLength);
#endif
	}
    
    totalMsgLength = *msgLength;
    remainingBufferSize -= *msgLength;
    
    //printf("Before errSSLWouldBlock...\n");    
    queryHeader = (ReqlQueryMessageHeader*)msg;
    remainingMessageLength = queryHeader->length;// - ( *msgLength - sizeof(ReqlQueryMessageHeader));
    
    printf("ReqlRecvQueryResponse msg length = %d\n", (int)*msgLength);
    printf("ReqlRecvQueryResponse header length = %d for queryToken: %d\n", queryHeader->length, (int)queryHeader->token);
    printf("ReqlRecvQueryResponse remainingMessage length = %d\n", (int)remainingMessageLength);

    while( remainingMessageLength > 0 )//|| status == errSSLWouldBlock)
    {
        /*
        if( ReqlSocketWait(conn->socket, conn->event_queue, EVFILT_READ ) != conn->socket )
        {
            printf("something went wrong\n");
            return -1;
        }
        */
        
        //TO DO:  Figure out why this is happening for changefeeds ...
        //printf("SSLRead remainingMessageLength = %d, totalMsgLength = %d, msgLength = %d\n", (int)remainingMessageLength, (int)totalMsgLength, (int)*msgLength);

#ifdef _WIN32

#elif defined(__APPLE__)
        status = SSLRead(conn->sslContext, (uint8_t*)msg + totalMsgLength, remainingMessageLength, msgLength);
#endif
		totalMsgLength += *msgLength;
        remainingBufferSize -= *msgLength;
        remainingMessageLength -= *msgLength;
    }
    
    
    
    //printf("SSLRead final remainingBufferSize = %d, msgLength = %d\n", (int)remainingBufferSize, (int)*msgLength);
    
    //printf("After errSSLWouldBlock...\n");
    
    if( status != noErr )
    {
        printf("SSLRead failed with error: %d\n", (int)status);
    }
    //printf("After SSLRead..\n");
    
    *msgLength = totalMsgLength;
    _reading = 0;
    return 0;
}

//this is a client call so it should return ReqlDriverError 
//msgLength must be unsigned long to match DWORD type on WIN32
ReqlDriverError ReqlAsyncRecv(ReqlConnection* conn, void * msg, unsigned long * msgLength)
{
	//uint64_t queryToken = *(uint64_t*)msg;
#ifdef _WIN32

	//WSAOVERLAPPED wOverlapped;
	//OVERLAPPED ol;

	//DWORD lpNumberOfBytesRecvd = *msgLength;
	//DWORD flags = 0;
	//DWORD numBuffers = 1;
	//Recv Query Asynchronously with Windows IOCP
	ReqlOverlappedResponse * overlappedResponse;
	unsigned long overlappedOffset = 0;// *msgLength - sizeof(ReqlOverlappedResponse);
	if (*msgLength > 0)
		overlappedOffset = *msgLength - sizeof(ReqlOverlappedResponse);;

	//Create an overlapped connetion object to pass what we need to the iocp callback
	//Instead of allocating memory we will assume the recv buffer is large enough and tack it on the end
	overlappedResponse = (ReqlOverlappedResponse*) ( (char*)msg + overlappedOffset ); //+1 for null terminator needed for encryption
	//overlappedResponse = (ReqlOverlappedResponse *)malloc( sizeof(ReqlOverlappedResponse) );
	ZeroMemory(overlappedResponse, sizeof(ReqlOverlappedResponse)); //this is critical!
	overlappedResponse->Overlapped.hEvent = NULL;// WSACreateEvent();//CreateEvent(NULL, 0, 0, NULL); //Manual vs Automatic Reset Events Affect GetQueued... Operation!!!
	overlappedResponse->wsaBuf.buf = (char*)msg;//(char*)(conn->response_buf[queryToken%2]);
	overlappedResponse->wsaBuf.len = *msgLength;
	overlappedResponse->buf = (char*)msg;
	overlappedResponse->len = *msgLength;
	overlappedResponse->conn = conn;
	overlappedResponse->Flags = 0;

	//PER_IO_DATA *pPerIoData = new PER_IO_DATA;
	//ZeroMemory(pPerIoData, sizeof(PER_IO_DATA));;
	//pPerIoData->Socket = conn->socket;
	//conn->socketContext.Overlapped.hEvent = WSACreateEvent();
	//conn->socketContext.wsaBuf.buf = (char*)(conn->response_buf);
	//conn->socketContext.wsaBuf.len = REQL_RESPONSE_BUFF_SIZE;

	//Issue the async receive
	//If WSARecv returns 0, the overlapped operation completed immediately and msgLength has been updated
	if( WSARecv(conn->socket, &(overlappedResponse->wsaBuf), 1, msgLength, &(overlappedResponse->Flags), &(overlappedResponse->Overlapped), NULL) == SOCKET_ERROR )
	{
		//WSA_IO_PENDING
		if( WSAGetLastError() != WSA_IO_PENDING )
		{
			printf( "****ReqlAsyncRecv::WSARecv failed with error %d \n",  WSAGetLastError() );	
		} 
		
		//forward the winsock system error to the client
		return (ReqlDriverError)WSAGetLastError();
	}


#elif defined(__APPLE__)
	//TO DO?
#endif

	//ReqlSuccess will indicate the async operation finished immediately
	return ReqlSuccess;

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

coroutine uint64_t ReqlRunQuery(const char ** queryBufPtr, ReqlConnection * conn)//, void * options)//, ReqlQueryClosure callback)
{
    //ReqlError * errPtr = NULL;
    //ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
	 char * sendBuffer;
    const char* queryBuf = *queryBufPtr;
	int32_t queryHeaderLength, queryStrLength;
    
    //char ** queryBuf = (char*)conn->query_buf;//[2][PAGE_SIZE * 128];
    ReqlQueryMessageHeader queryHeader;
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    printf("ReqlRunQuery\n\n");
    assert(*queryBuf && strlen(queryBuf) > sizeof(ReqlQueryMessageHeader));
    
    //Populate the network message with a header and the serialized query JSON string
    //char * queryStr = queryBuf[(queryIndex)%2];
    //static uint64_t queryID = 0;
    queryHeader.token = (conn->queryCount)++;//conn->token;
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    queryStrLength = (int32_t)strlen(queryBuf);
    
    //printf("queryStrLength = %d\n", queryStrLength);
    queryHeader.length = queryStrLength;// + queryHeaderLength;
    //memcpy(queryBuf, &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    //queryBuf[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    sendBuffer = (char*)malloc( queryStrLength + queryHeaderLength + 1);
    memcpy(sendBuffer, &queryHeader, queryHeaderLength); //copy query header
    memcpy(sendBuffer+queryHeaderLength, queryBuf, queryStrLength); //copy query header
    sendBuffer[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //printf("sendBuffer = \n\n%s\n", queryBuf);
    //assert(queryBuf[queryIndex%2]);
    //ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
    ReqlSend(conn, (void*)sendBuffer, queryStrLength + queryHeaderLength);
    
    free( sendBuffer);
    //printf("ReqlRunQueryCtx End\n\n");
    return queryHeader.token;
    /*
    yield();

    // Sync Wait for magic query response
    char readBuffer[4096];
    size_t readLength = 4096;
    ReqlCursor cursor = {(ReqlQueryMessageHeader*)readBuffer, readLength};

    ReqlRecv(conn, readBuffer, &(cursor.length));
    printf("readBuffer = \n\n%.*s\n", (int)cursor.length - queryHeaderLength, readBuffer+queryHeaderLength);
    
    //if( ReqlCursorGetResponseType(&cursor) == REQL_SUCCESS_PARTIAL )
    //    ReqlCursorContinue(conn, &cursor);
    
    //Return the result of the request to the client
    //TO DO: enumerate and return errors (such as timeout)
    assert(callback);
    callback(&err, cursor);
    */
}


uint64_t ReqlRunQueryWithToken(const char ** queryBufPtr, ReqlConnection * conn, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{
    //ReqlError * errPtr = NULL;
    //ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    //assert(!_writing && !_reading);
	int32_t queryHeaderLength;// = sizeof(ReqlQueryMessageHeader);
    int32_t queryStrLength;// = (int32_t)strlen(queryBuf);
	//char * sendBuffer;
	//unsigned long queryMessageLength;
    const char* queryBuf = *queryBufPtr + sizeof(ReqlQueryMessageHeader);
    //printf("queryBuf = \n\n%s\n\n", queryBuf);

    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)*queryBufPtr;
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    //printf("ReqlRunQueryWithToken\n\n");
    //assert(*queryBuf && strlen(queryBuf) > sizeof(ReqlQueryMessageHeader));
    
    //Populate the network message with a header and the serialized query JSON string
    queryHeader->token = queryToken;//(conn->queryCount)++;//conn->token;
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    queryStrLength = (int32_t)strlen(queryBuf);    
    queryHeader->length = queryStrLength;// + queryHeaderLength;

	//memcpy(queryBuf, &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    //queryBuf[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //sendBuffer = (char*)malloc( queryStrLength + queryHeaderLength);

	//copy the header to the buffer
	//memcpy(conn->query_buf[0], &queryHeader, queryHeaderLength); //copy query header
    //memcpy((char*)(conn->query_buf[0])+queryHeaderLength, queryBuf, queryStrLength); //copy query header
    //sendBuffer[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //_writing = 1;    
    //printf("sendBuffer = \n\n%s\n", sendBuffer + queryHeaderLength);
    //assert(queryBuf[queryIndex%2]);
    //ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);

	//queryMessageLength = queryStrLength + queryHeaderLength;
    ReqlSend(conn, (void*)(*queryBufPtr), queryStrLength + sizeof(ReqlQueryMessageHeader));
    //free( sendBuffer);
    //printf("ReqlRunQueryCtx End\n\n");
    
    //_writing = 0;
    
    return queryHeader->token;

}

/***
 *
 *
 *	
 ***/
uint64_t ReqlRunQueryWithTokenOnQueue(const char ** queryBufPtr, unsigned long queryStrLength, ReqlConnection * conn, uint64_t queryToken)//, void * options)//, ReqlQueryClosure callback)
{
    //ReqlError * errPtr = NULL;
    //ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    //assert(!_writing && !_reading);
	unsigned long queryHeaderLength, /*queryStrLength,*/ queryMessageLength;
    ReqlQueryMessageHeader * queryHeader = (ReqlQueryMessageHeader*)*queryBufPtr;
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
	const char* queryBuf = *queryBufPtr + sizeof(ReqlQueryMessageHeader);
	//printf("queryBuf = \n\n%s\n\n", queryBuf);
    
    //printf("ReqlRunQueryWithToken\n\n");
    //assert(*queryBuf && strlen(queryBuf) > sizeof(ReqlQueryMessageHeader));
    
    //Populate the network message buffer with a header and the serialized query JSON string
    queryHeader->token = queryToken;//(conn->queryCount)++;//conn->token;
    queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    //queryStrLength = (int32_t)strlen(queryBuf);    
    queryHeader->length = queryStrLength;// + queryHeaderLength;

	//memcpy(queryBuf, &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    //queryBuf[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
	//copy the header to the buffer
    //memcpy(conn->query_buf[queryToken%2], &queryHeader, queryHeaderLength); //copy query header
    //memcpy((char*)(conn->query_buf[queryToken%2]) + queryHeaderLength, queryBuf, queryStrLength); //copy query header
    //sendBuffer[queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
	printf("ReqlRunQueryWithTokenOnQueue::queryBufPtr (%d) = %.*s", (int)queryStrLength, (int)queryStrLength, *queryBufPtr + sizeof(ReqlQueryMessageHeader));
    //_writing = 1;    
    queryMessageLength = queryHeaderLength + queryStrLength;
	//ReqlSend(conn, (void*)&queryHeader, queryHeaderLength);
	ReqlSendWithQueue(conn, (void*)(*queryBufPtr), &queryMessageLength);
    //free( sendBuffer);
    //printf("ReqlRunQueryCtx End\n\n");
    
    //_writing = 0;
    
    return queryHeader->token;

}


/*
coroutine uint64_t ReqlRunQueryCtx(ReqlQueryContext * ctx, ReqlConnection * conn, void * options)//, ReqlQueryClosure callback)
{
    ReqlError * errPtr = NULL;
    ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
    char ** queryBuf = (char*)conn->query_buf;//[2][PAGE_SIZE * 128];
    ReqlQueryMessageHeader queryHeader;
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    
    //printf("ReqlRunQueryCtx\n\n");
    //Get the ReqlQuery stack stored via Posix Thread Local Storage memory
    //ReqlQueryContext * ctx = ReqlQueryGetContext();
    //printf("Num Queries = %d\n", ctx->numQueries);
    assert( ctx->numQueries > 0 );
    
    const char * empty_options = "{}";
    //Serialize the ReqlQueryStack to the JSON expected by REQL
    ReqlQuery runQuery = {1, "{}", NULL};
    if( options )
        runQuery.data = options;
    //ctx->stack[ctx->numQueries++] = runQuery;
    
    //Assume the first is always a REQL database query
    int queryIndex = 0;
    
    if( ctx->stack[queryIndex%2].type == REQL_EXPR )
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "%s", (char*)ctx->stack[queryIndex].data);
    else if( ctx->stack[queryIndex%2].type == REQL_DO )
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,%s]", ctx->stack[0].type, (char*)ctx->stack[0].data );
    else
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[\"%s\"]]", ctx->stack[0].type, (char*)ctx->stack[0].data );
    queryIndex++;
    
    //Serialize all subsequent queries in a for  loop
    //TO DO:  consider unrolling?
    for(;queryIndex<ctx->numQueries; queryIndex++)
    {
        if( !( ctx->stack[queryIndex].type == REQL_TABLE )) //|| ctx->stack[queryIndex].type == REQL_ORDER_BY) )
        {
            //This is an annoying branch:
            //  Some Queries expect 1 argument while other queries expect 2 arguments
            //  While any queries can have no arguments if such data is empty
            

            if( ctx->stack[queryIndex].options )  //2 args + options case
                sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[%s,%s],%s]", ctx->stack[queryIndex].type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)ctx->stack[queryIndex].data, (char*)ctx->stack[queryIndex].options);
            else if( !ctx->stack[queryIndex].data )  //no arg case
                sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[%s]]", ctx->stack[queryIndex].type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader));
            else if(  ctx->stack[queryIndex].type == REQL_CHANGES )//|| ctx->stack[queryIndex].type == REQL_ORDER_BY) //1 arg case
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
    sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,%s,%s]", runQuery.type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)runQuery.data);
    printf("queryBuf = \n\n%s\n\n", queryBuf[(queryIndex)%2]+sizeof(ReqlQueryMessageHeader));
    
    //reset the TLS ReqlQuery stack
    ctx->numQueries = 0;
    
    //Populate the network message with a header and the serialized query JSON string
    char * queryStr = queryBuf[(queryIndex)%2];
    //static uint64_t queryID = 0;
    queryHeader.token = (conn->queryCount)++;//conn->token;
    
    int32_t queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    int32_t queryStrLength = (int32_t)strlen(queryStr + queryHeaderLength);
    
    //printf("queryStrLength = %d\n", queryStrLength);
    queryHeader.length = queryStrLength;// + queryHeaderLength;
    memcpy(queryBuf[(queryIndex)%2], &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    queryBuf[(queryIndex)%2][queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //printf("sendBuffer = \n\n%s\n", queryBuf);
    assert(queryBuf[queryIndex%2]);
    ReqlSend(conn, queryBuf[queryIndex%2], queryStrLength + queryHeaderLength);
    
    //printf("ReqlRunQueryCtx End\n\n");
    return queryHeader.token;

}


coroutine uint64_t ReqlRunQueryCtxOnQueue(ReqlQueryContext * context, ReqlConnection * conn, void * options, dispatch_queue_t queryQueue)//, ReqlQueryClosure callback)
{
    __block uint64_t queryToken = conn->queryCount;
    __block ReqlQueryContext * ctx = (ReqlQueryContext *)malloc(sizeof(ReqlQueryContext));
    memcpy( ctx, context, sizeof(ReqlQueryContext));

    ReqlError * errPtr = NULL;
    ReqlError err = {ReqlDriverErrorClass, ReqlSuccess, NULL};
    
        char * queryBuf[2];//[PAGE_SIZE * 128];
        queryBuf[0] = (char*)malloc(PAGE_SIZE * 128);
        queryBuf[1] = (char*)malloc(PAGE_SIZE * 128);

    ReqlQueryMessageHeader queryHeader;
    //printf("sizeof(ReQLHeader) == %lu\n", sizeof(ReqlQueryMessageHeader));
    
    
    //printf("ReqlRunQueryCtx\n\n");
    //Get the ReqlQuery stack stored via Posix Thread Local Storage memory
    //ReqlQueryContext * ctx = ReqlQueryGetContext();
    //printf("Num Queries = %d\n", ctx->numQueries);
    assert( ctx->numQueries > 0 );
    
    const char * empty_options = "{}";
    //Serialize the ReqlQueryStack to the JSON expected by REQL
    ReqlQuery runQuery = {1, "{}", NULL};
    if( options )
        runQuery.data = options;
    //ctx->stack[ctx->numQueries++] = runQuery;
    
    //Assume the first is always a REQL database query
    int queryIndex = 0;
    
    if( ctx->stack[queryIndex%2].type == REQL_EXPR )
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "%s", (char*)ctx->stack[queryIndex].data);
    else if( ctx->stack[queryIndex%2].type == REQL_DO )
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,%s]", ctx->stack[0].type, (char*)ctx->stack[0].data );
    else
        sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[\"%s\"]]", ctx->stack[0].type, (char*)ctx->stack[0].data );
   
        
        queryIndex++;
    

    //Serialize all subsequent queries in a for  loop
    //TO DO:  consider unrolling?
    for(;queryIndex<ctx->numQueries; queryIndex++)
    {
        if( !( ctx->stack[queryIndex].type == REQL_TABLE )) //|| ctx->stack[queryIndex].type == REQL_ORDER_BY) )
        {
            //This is an annoying branch:
            //  Some Queries expect 1 argument while other queries expect 2 arguments
            //  While any queries can have no arguments if such data is empty
            
            
            if( ctx->stack[queryIndex].options )  //2 args + options case
                sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[%s,%s],%s]", ctx->stack[queryIndex].type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)ctx->stack[queryIndex].data, (char*)ctx->stack[queryIndex].options);
            else if( !ctx->stack[queryIndex].data )  //no arg case
                sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,[%s]]", ctx->stack[queryIndex].type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader));
            else if(  ctx->stack[queryIndex].type == REQL_CHANGES )//|| ctx->stack[queryIndex].type == REQL_ORDER_BY) //1 arg case
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
    sprintf(queryBuf[queryIndex%2]+sizeof(ReqlQueryMessageHeader), "[%d,%s,%s]", runQuery.type, queryBuf[(queryIndex+1)%2]+sizeof(ReqlQueryMessageHeader), (char*)runQuery.data);
    printf("queryBuf = \n\n%s\n\n", queryBuf[(queryIndex)%2]+sizeof(ReqlQueryMessageHeader));
    
    //reset the TLS ReqlQuery stack
    ctx->numQueries = 0;
    
    //Populate the network message with a header and the serialized query JSON string
    char * queryStr = queryBuf[(queryIndex)%2];
    //static uint64_t queryID = 0;
    queryHeader.token = (conn->queryCount)++;//conn->token;
    assert(queryHeader.token == queryToken);
    int32_t queryHeaderLength = sizeof(ReqlQueryMessageHeader);
    int32_t queryStrLength = (int32_t)strlen(queryStr + queryHeaderLength);
    
    //printf("queryStrLength = %d\n", queryStrLength);
    queryHeader.length = queryStrLength;// + queryHeaderLength;
    memcpy(queryBuf[(queryIndex)%2], &queryHeader, sizeof(ReqlQueryMessageHeader)); //copy query header
    queryBuf[(queryIndex)%2][queryStrLength + queryHeaderLength] = '\0';  //cap with null char
    
    //printf("sendBuffer = \n\n%s\n", queryBuf);
    assert(queryBuf[queryIndex%2]);
    
    __block char * queryBufPtr = queryBuf[queryIndex%2];
    dispatch_async(queryQueue, ^{
        ReqlSend(conn, queryBufPtr, queryStrLength + queryHeaderLength);
    });

    
    //printf("ReqlRunQueryCtx End\n\n");
    return queryToken;//queryHeader.token;
        
}
*/

/*
dill_coroutine uint64_t ReqlRunQuery(ReqlConnection * conn, void * options, ReqlQueryClosure callback)
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    return ReqlRunQueryCtx(ctx, conn, options);//, callback);
}
*/

/*
ReqlQueryObject ReqlTableQuery( void * name )//ReqlConnection * conn, char *dbname, char *tablename, char *datacenter, char *pkey, int use_outdated )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ReqlQueryObject queryObj = {{REQL_TABLE, name, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}

ReqlQueryObject ReqlGetQuery( void * name )//ReqlConnection * conn, char *dbname, char *tablename, char *datacenter, char *pkey, int use_outdated )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ReqlQueryObject queryObj = {{REQL_GET, name, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}

ReqlQueryObject ReqlPluckQuery( void * name )//ReqlConnection * conn, char *dbname, char *tablename, char *datacenter, char *pkey, int use_outdated )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ReqlQueryObject queryObj = {{REQL_PLUCK, name, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}



ReqlQueryObject ReqlInsertQuery( void * documents )//ReqlConnection * conn, char *dbname, char *tablename, char *datacenter, char *pkey, int use_outdated )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ReqlQueryObject queryObj = {{REQL_INSERT, documents, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}



ReqlQueryObject ReqlFilterQuery( void * predicateString )
{
    ReqlQueryObject queryObj = {{REQL_FILTER, predicateString, NULL}, _ReqlQueryObjectFuncTable};
    //Only filter actual data, but we can return the empty query to the user
    if( predicateString )
    {
        ReqlQueryContext * ctx = ReqlQueryGetContext();
        ctx->stack[ctx->numQueries++] = queryObj.query;
    }
    return queryObj;
}


ReqlQueryObject ReqlMergeQuery( void * predicateString )
{
    ReqlQueryObject queryObj = {{REQL_MERGE, predicateString, NULL}, _ReqlQueryObjectFuncTable};
    //Only filter actual data, but we can return the empty query to the user
    if( predicateString )
    {
        ReqlQueryContext * ctx = ReqlQueryGetContext();
        ctx->stack[ctx->numQueries++] = queryObj.query;
    }
    return queryObj;
}

ReqlQueryObject ReqlCountQuery()
{
    ReqlQueryObject queryObj = {{REQL_COUNT, NULL, NULL}, _ReqlQueryObjectFuncTable};
    //Only filter actual data, but we can return the empty query to the user
    //if( predicateString )
    //{
        ReqlQueryContext * ctx = ReqlQueryGetContext();
        ctx->stack[ctx->numQueries++] = queryObj.query;
    //}
    return queryObj;
}

ReqlQueryObject ReqlUpdateQuery( void * predicateString )
{
    ReqlQueryObject queryObj = {{REQL_UPDATE, predicateString, NULL}, _ReqlQueryObjectFuncTable};
    //Only filter actual data, but we can return the empty query to the user
    if( predicateString )
    {
        ReqlQueryContext * ctx = ReqlQueryGetContext();
        ctx->stack[ctx->numQueries++] = queryObj.query;
    }
    return queryObj;
}

ReqlQueryObject ReqlGetFieldQuery( void * name )//ReqlConnection * conn, char *dbname, char *tablename, char *datacenter, char *pkey, int use_outdated )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ReqlQueryObject queryObj = {{REQL_GET_FIELD, name, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}

ReqlQueryObject ReqlCoerceToQuery( void * name )//ReqlConnection * conn, char *dbname, char *tablename, char *datacenter, char *pkey, int use_outdated )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ReqlQueryObject queryObj = {{REQL_COERCE_TO, name, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}

ReqlQueryObject ReqlAppendQuery( void * jsonObjectString )//ReqlConnection * conn, char *dbname, char *tablename, char *datacenter, char *pkey, int use_outdated )
{
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ReqlQueryObject queryObj = {{REQL_APPEND, jsonObjectString, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}


ReqlQueryObject ReqlGeoIntersectionQuery( void * geometryString, void * geospatialIndexString )
{
    ReqlQueryObject queryObj = {{REQL_GET_INTERSECTING, geometryString, geospatialIndexString}, _ReqlQueryObjectFuncTable};
    //Only filter actual data, but we can return the empty query to the user
    if( geometryString && geospatialIndexString )
    {
        ReqlQueryContext * ctx = ReqlQueryGetContext();
        ctx->stack[ctx->numQueries++] = queryObj.query;
    }
    return queryObj;
}


ReqlQueryObject ReqlOrderByQuery( void * fieldName )
{
    ReqlQueryObject queryObj = {{REQL_ORDER_BY, fieldName, NULL}, _ReqlQueryObjectFuncTable};
    //Only filter actual data, but we can return the empty query to the user
    if( fieldName )
    {
        ReqlQueryContext * ctx = ReqlQueryGetContext();
        ctx->stack[ctx->numQueries++] = queryObj.query;
    }
    return queryObj;
}

ReqlQueryObject ReqlDeleteQuery( void * options )
{
    ReqlQueryObject queryObj = {{REQL_DELETE, options, NULL}, _ReqlQueryObjectFuncTable};
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}

ReqlQueryObject ReqlChangesQuery( void * options )
{
    ReqlQueryObject queryObj = {{REQL_CHANGES, options, NULL}, _ReqlQueryObjectFuncTable};
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    ctx->stack[ctx->numQueries++] = queryObj.query;
    return queryObj;
}


*/


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
    ReqlSend(conn, queryStr, queryStrLength + queryHeaderLength);
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
    ReqlSend(conn, queryStr, queryStrLength + queryHeaderLength);
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

uint64_t ReqlCursorStop(ReqlCursor *cursor, void * options)
{
    //printf("ReqlCursorContinue\n");
    return ReqlStopQuery(cursor->conn, cursor->header->token, options);
}



/*
coroutine void ReqlChainRoutine(ReqlQuery * query)
{
    printf("ReqlChainRoutine Start\n");

    //yield();
    
    printf("ReqlChainRoutine Exit\n");
}

dill_coroutine ReqlQueryMakePtrRet ReqlDatabaseQueryChain( void * query )
{
    //printf("ReqlDatabaseQueryChain:  %s\n", name);
    //char * namePtr = name;
    //namePtr++;
    return ReqlQueryChain;//((ReqlQuery*)query)->chain;
}
*/

/*
coroutine ReqlQueryObject ReqlDoQuery( void* queryString )
{
    
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    //char ** namePtr = (char**)name;
    
    ReqlQueryObject queryObj = {{REQL_DO, queryString, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    //allocate memory
    //*namePtr = (char*)calloc( 1, 1024 );
    //strcpy(*namePtr, queryData);
    
    return queryObj;
}



coroutine ReqlQueryObject ReqlExprQuery( void* queryString )
{
    
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    //char ** namePtr = (char**)name;

    ReqlQueryObject queryObj = {{REQL_EXPR, queryString, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    //allocate memory
    //*namePtr = (char*)calloc( 1, 1024 );
    //strcpy(*namePtr, queryData);
    
    return queryObj;
}



coroutine ReqlQueryObject ReqlRowQuery( void* name )
{
    
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    //char ** namePtr = (char**)name;
    
    ReqlQueryObject queryObj = {{REQL_DB, name, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    //allocate memory
    //*namePtr = (char*)calloc( 1, 1024 );
    //strcpy(*namePtr, queryData);
    
    return queryObj;
}

coroutine ReqlQueryObject ReqlMatchQuery( void* name )
{
    
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    //char ** namePtr = (char**)name;
    
    ReqlQueryObject queryObj = {{REQL_DB, name, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    //allocate memory
    //*namePtr = (char*)calloc( 1, 1024 );
    //strcpy(*namePtr, queryData);
    
    return queryObj;
}


coroutine ReqlQueryObject ReqlDatabaseQuery( void* name )
{
    
    ReqlQueryContext * ctx = ReqlQueryGetContext();
    //char ** namePtr = (char**)name;

    ReqlQueryObject queryObj = {{REQL_DB, name, NULL}, _ReqlQueryObjectFuncTable};
    ctx->stack[ctx->numQueries++] = queryObj.query;
    //allocate memory
    //*namePtr = (char*)calloc( 1, 1024 );
    //strcpy(*namePtr, queryData);
    
    return queryObj;
}


*/
