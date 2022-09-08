//
//  NSReQLCursor.h
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

//#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface NSTReQLCursor : NSTCursor

//NSTCursor base class override methods
//-(char*)ProcessResponseHeader:(char*)buffer Length:(unsigned long)bufferLength Cursor:(CTCursor*)cursor;

+(NSTReQLCursor*)newCursor:(NSTConnection*)conn;
+(NSTReQLCursor*)newCursor:(NSTConnection*)conn filePath:(const char*)filepath;

@end

NS_ASSUME_NONNULL_END
