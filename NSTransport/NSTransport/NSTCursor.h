//
//  NSReQLCursor.h
//  MastryAdmin
//
//  Created by Joe Moulton on 6/7/19.
//  Copyright © 2019 VRTVentures LLC. All rights reserved.
//

#import <Foundation/Foundation.h>

#define class extern_class
#define delete extern_delete
//#include <map>
//#include <functional>
//#include <iostream>
#undef delete
#undef class

NS_ASSUME_NONNULL_BEGIN

//Forwrd declare class
@class NSTConnection;

//@protocol NSTCursorProtocol
//-(char*)ProcessResponseHeader:(char*)buffer Length:(unsigned long)bufferLength Cursor:(CTCursor*)cursor;
//@end

@interface NSTCursor : NSObject

@property(nonatomic, weak) NSTConnection* nstConnection;


//virtual uint64_t Continue() { fprintf(stderr, "CXCursor::continue should be overidden!!!"); return 0;  }
//virtual char* ProcessResponseHeader(CTCursor * cursor, char * buffer, unsigned long bufferLength){fprintf(stderr, "CXCursor::ProcessResponseHeaders should be overidden!!!"); return NULL; }

//std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)> callback;
//void createResponseBuffers(const char * filepath);

//@property (weak) id <NSTCursorProtocol>                 delegate;
@property (nonatomic)           CTCursor*               cursor;
@property (nonatomic, copy)     NSTRequestClosure       callback;

-(id)initWithConnection:(NSTConnection*)conn;
-(id)initWithConnection:(NSTConnection*)conn andFilePath:(const char*)filepath;

//Convenience Initializer Wraps Copy Constructor:
//Create NSReQLCursor object from a ReqlCursor C-API object
//+(NSTCursor*)newCursor:(NSTConnection*)conn;
//+(NSTCursor*)newCursor:(NSTConnection*)conn filePath:(const char*)filepath;

-(NSTConnection*)connection;

-(char*)ProcessResponseHeader:(char*)buffer Length:(unsigned long)bufferLength Cursor:(CTCursor*)cursor;


//-(NSArray*)toArray;
//-(ReqlResponseType)responseType;
//-(uint64_t)token;
//-(void)continue:(NSDictionary*)options;
//typedef class CXConnection;


@end

NS_ASSUME_NONNULL_END
