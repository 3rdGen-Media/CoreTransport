//
//  NSTURLSession.h
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

#import <Foundation/Foundation.h>

#define NSTURLSharedSession [NSTURLSession sharedSession]

NS_ASSUME_NONNULL_BEGIN

@interface NSTURLSession : NSObject

+(NSTURLSession*)sharedSession;

-(NSTConnectFunc)connect;

@end

NS_ASSUME_NONNULL_END
