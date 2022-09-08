//
//  NSURLRequest.h
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSTURLRequest : NSObject

+(NSTURLRequest*)newRequest:(URLRequestType)command requestPath:(const char*)requestPath headers:(NSDictionary* _Nullable)headers body:(char* _Nullable)body contentLength:(unsigned long)contentLength;
+(NSTURLRequest*)newRequest:(URLRequestType)command requestPath:(const char*)requestPath headers:(NSDictionary* _Nullable)headers body:(char* _Nullable)body contentLength:(unsigned long)contentLength responsePath:(const char*)responsePath;

//void doNothing() { return; }

-(void)setValue:(NSString*)value forHTTPHeaderField:(NSString*)field;
-(void)setContentValue:(char*)value Length:(unsigned long)contentLength;

-(uint64_t)sendOnConnection:(NSTConnection*)conn withCallback:(NSTRequestClosure)callback;

@end

NS_ASSUME_NONNULL_END
