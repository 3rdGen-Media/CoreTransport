#ifndef ReqlError_h
#define ReqlError_h

//#pragma mark -- ReqlError Enums

typedef enum ReqlErrorClass
{
    ReqlCompileErrorClass,
    ReqlDriverErrorClass,
    ReqlRuntimeErrorClass
}ReqlErrorClass;

/* * *
 *  ReqlDriverError Enums
 *
 *  We custom define ReqlError(s) because it is not present in the protobuf definitions (or I cannot figure out how to acces it)
 *  We will use negative values for our custom error codes that do not exist in the official spec
 * * */

#ifndef _WIN32
#define WSA_IO_PENDING -140
#endif

typedef enum ReqlDriverError
{
	ReqlIOPending				 = WSA_IO_PENDING,	 //ReqlAsyncSend is sending in an async state
    ReqlRunLoopError             = -130,
    ReqlSysCallError             = -120,
    ReqlDNSError                 = -110, //Unable to resolve the ReqlService host address via DNS
    ReqlEncryptionError          = -100,
    ReqlSASLHandshakeError       = -90, //An SSL/SCRAM handshake call produced the error
    ReqlSSLHandshakeError        = -80, //An SSL handkshake call produced the error
    ReqlKeychainError            = -70, //A device keychain call produced the error
    ReqlCertificateError         = -60, //An SSL Certificate call produced the error
    ReqlSecureTransportError     = -50, //An SSL/TLS layer call produced the error
    ReqlEventError               = -40, //An unexpected kqueue/kevent behavior produced the error
    ReqlConnectError             = -30, //A bsd socket connection call produced the error
    ReqlDomainError              = -20, //An invalid input domain produced the error
    ReqlSocketError              = -10, //A bsd socket call produced the error
    ReqlSuccess                  = 0,   //No Error
    ReqlAuthError                = 10,  //Standard ReqlDriverError:ReqlAuthError
}ReqlDriverError;


#endif