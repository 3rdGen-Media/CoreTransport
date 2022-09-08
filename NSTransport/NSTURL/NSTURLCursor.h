//
//  NSURLCursor.h
//  NSTransport
//
//  Created by Joe Moulton on 9/6/22.
//

NS_ASSUME_NONNULL_BEGIN

@interface NSTURLCursor : NSTCursor

//NSTCursor base class override methods
//-(char*)ProcessResponseHeader:(char*)buffer Length:(unsigned long)bufferLength Cursor:(CTCursor*)cursor;

+(NSTURLCursor*)newCursor:(NSTConnection*)conn;
+(NSTURLCursor*)newCursor:(NSTConnection*)conn filePath:(const char*)filepath;

@end

NS_ASSUME_NONNULL_END
