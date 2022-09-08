//
//  NSTURLInterface.h
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSTURL : NSObject

+(NSTConnectFunc)connect;


/*
//HTTP PROXY
std::shared_ptr<CXURLRequest> OPTIONS(const char* requestPath, const char* responseFilePath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_OPTIONS, requestPath, NULL, NULL, 0, responseFilePath)); return request; }
std::shared_ptr<CXURLRequest> OPTIONS(const char* requestPath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_OPTIONS, requestPath, NULL, NULL, 0)); return request; }

//HTTP PROXY + TLS UPGRADE
std::shared_ptr<CXURLRequest> CONNECT(const char* requestPath, const char* responseFilePath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_CONNECT, requestPath, NULL, NULL, 0, responseFilePath)); return request; }
std::shared_ptr<CXURLRequest> CONNECT(const char* requestPath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_CONNECT, requestPath, NULL, NULL, 0)); return request; }
*/

//GET

+(NSTURLRequest*)GET:(const char*)requestPath;
+(NSTURLRequest*)GET:(const char*)requestPath responsePath:(const char*)responsePath;

//POST
/*
std::shared_ptr<CXURLRequest> POST(const char* requestPath, const char* responseFilePath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_POST, requestPath, NULL, NULL, 0, responseFilePath)); return request; }
std::shared_ptr<CXURLRequest> POST(const char* requestPath) { std::shared_ptr<CXURLRequest> request(new CXURLRequest(URL_POST, requestPath, NULL, NULL, 0)); return request; }
*/

@end

NS_ASSUME_NONNULL_END
