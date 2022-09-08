//
//  NSTURLInterface.m
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

#include "../NSTURL.h"

@implementation NSTURL

+(NSTConnectFunc)connect
{
    //CXReQLSession* sharedSession = sharedSession->getInstance();
    //return sharedSession->connect(target, callback);
    return NSTURLSession.sharedSession.connect;
    //return NSReQLSession.connect;
}


+(NSTURLRequest*)GET:(const char*)requestPath
{
    return [NSTURLRequest newRequest:URL_GET requestPath:requestPath headers:NULL body:NULL contentLength:0];
}
+(NSTURLRequest*)GET:(const char*)requestPath responsePath:(const char*)responsePath
{
    return [NSTURLRequest newRequest:URL_GET requestPath:requestPath headers:NULL body:NULL contentLength:0 responsePath:responsePath];
}

@end
