/***
**  
*	Core Transport -- HappyEyeballs
*
*	Description:  
*	
*
*	@author: Joe Moulton III
*	Copyright 2020 Abstract Embedded LLC DBA 3rdGen Multimedia
**
***/

//cr_display_sync is part of Core Render crPlatform library
#ifdef _DEBUG
#include <vld.h>                //Visual Leak Detector
#endif

#include "stdio.h"
#include "stdlib.h"
#include <process.h>            //threading routines for the CRT

//Core Transport ReQL C API
//#include <CoreTransport/CTransport.h>

//ctReQL can be optionally built with native SSL encryption
//or against MBEDTLS
#ifndef CTRANSPORT_USE_MBED_TLS
#pragma comment(lib, "crypt32.lib")
//#pragma comment(lib, "user32.lib")
#pragma comment(lib, "secur32.lib")
#else
// mbded tls
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/debug.h"
#endif

//CoreTransport CXURL and ReQL C++ APIs uses CTransport (CTConnection) API internally
#include <CoreTransport/CXURL.h>
#include <CoreTransport/CXReQL.h>

//CXReQL uses RapidJSON for json serialization internally
//#include "rapidjson/document.h"
//#include "rapidjson/error/en.h"
//#include "rapidjson/stringbuffer.h"
//#include <rapidjson/writer.h>

/***
 * Start CTransport (CTConnection) C API Example
 ***/

 //Define a CTransport API CTTarget C style struct to initiate an HTTPS connection with a CTransport API CTConnection
static const char* http_server = "example.com";
static const unsigned short		http_port = 443;

//Proxies use a prefix to specify the proxy protocol, defaulting to HTTP Proxy
static const char* proxy_server = "socks5://172.20.10.1";// "54.241.100.168";
static const unsigned short		proxy_port = 443;// 1080;

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
//static const char * rdb_server = "RETHINKDB_SERVER";
//static const unsigned short rdb_port = 18773;
//static const char * rdb_user = "RETHINKDB_USERNAME";
//static const char * rdb_pass = "RETHINKDB_PASSWORD";

//Define a CTransport API ReqlService (ie CTTarget) C style struct to initiate a RethinkDB TLS connection with a CTransport API CTConnection
static const char* rdb_server = "rdb.3rd-gen.net";
static const unsigned short rdb_port = 28015;
static const char* rdb_user = "admin";
static const char* rdb_pass = "3rdgen.rdb.4.$";

//Define SSL Certificate for ReqlService construction:
//	a)  For xplatform MBEDTLS support: The SSL certificate can be defined in a buffer OR a file AS a .cer in PEM format
//	b)  For Win32 SCHANNEL support:	   The SSL certificate must be pre-installed in a trusted root store on the system using MS .cer format
//  c)  For Darwin SecureTransport:	   The SSL certificate must be defined in a .der file with ... format

//const char * certFile = "../Keys/MSComposeSSLCertificate.cer";
const char * rdb_cert = "RETHINKDB_CERT_PATH_OR_STR\0";

/***
 * Start CXTransport C++ API Example
 ***/
using namespace CoreTransport;

//Define pointers to keep reference to our secure HTTPS and RethinkDB connections, respectively
CXConnection * _httpCXConn;
CXConnection * _reqlCXConn;

int _cxURLConnectionClosure(CTError *err, CXConnection * conn)
{
	if (err->id == CTSuccess && conn) 
	{
		_httpCXConn = conn;
		/*
		if (CTSocketCreateEventQueue(&(conn->connection()->socketContext)) < 0)
		{
			printf("ReqlSocketCreateEventQueue failed\n");
			err->id = (int)conn->connection()->event_queue;
			//goto CONN_CALLBACK;
		}
		*/
	}
	else { assert(1 == 0); } //process errors

	return err->id;
}

int _cxReQLConnectionClosure(ReqlError *err, CXConnection * conn)
{
	if( err->id == CTSuccess && conn ){ _reqlCXConn = conn; }
	else {} //process errors	
	return err->id;
}

void SendReqlQuery()
{
	//Send the ReQL Query to find some URLs to download
	//Demonstrate issue of query and callback lambda using CXReQL API
	auto queryCallback = [&] (ReqlError * error, std::shared_ptr<CXCursor> &cxCursor) {

		//The cursor's file mapping was closed so the file could be truncated to the size read from network, 
		//but the curor's file handle itself is still open/valid, so we just need to remap the cursor's file for reading
		CTCursorMapFileR(cxCursor->_cursor);
		printf("Lambda callback response: %.*s\n", cxCursor->_cursor->file.size, (char*)(cxCursor->_cursor->file.buffer) + sizeof(ReqlQueryMessageHeader)); return;  
	};	
	CXReQL.db("MastryDB").table("Paintings").run(_reqlCXConn, NULL, queryCallback);
}

static int httpsRequestCount = 0;
void SendHTTPRequest()
{
	//file path to open
	char filepath[1024] = "https\0";

	_itoa(httpsRequestCount, filepath + 5, 10);

	strcat(filepath, ".txt");
	httpsRequestCount++;
	
	//Create a CXTransport API C++ CXURLRequest
	//std::shared_ptr<CXURLRequest> getRequest = CXURL.GET("/index.html", filepath);
	std::shared_ptr<CXURLRequest> getRequest = CXURL.GET("/index.html", filepath);

	getRequest->setValueForHTTPHeaderField("Host", "www.example.com:80");
	//Add some HTTP headers to the CXURLRequest
	getRequest->setValueForHTTPHeaderField("Accept", "*/*");

	
	//Define the response callback to return response buffers using CXTransport CXCursor object returned via a Lambda Closure
	auto requestCallback = [&] (CTError * error, std::shared_ptr<CXCursor> &cxCursor) {

		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorMapFileR(cursor);
		printf("SendHTTPRequest response header:  %.*s\n", cxCursor->_cursor->headerLength, cxCursor->_cursor->requestBuffer);//%.*s\n", cursor->_cursor.length - sizeof(ReqlQueryMessageHeader), (char*)(cursor->_cursor.header) + sizeof(ReqlQueryMessageHeader)); return;  
		printf("SendHTTPRequest response body:  %.*s\n", cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));
	};

	//Pass the CXConnection and the lambda to populate the request buffer and asynchronously send it on the CXConnection
	//The lambda will be executed so the code calling the request can interact with the asynchronous response buffers
	getRequest->send(_httpCXConn, requestCallback);
	
}

void SendTLSUpgradeRequest()
{
	//file path to open
	char filepath[1024] = "https\0";

	_itoa(httpsRequestCount, filepath + 5, 10);

	strcat(filepath, ".txt");
	httpsRequestCount++;

	//Create a CXTransport API C++ CXURLRequest
	//std::shared_ptr<CXURLRequest> getRequest = CXURL.GET("/index.html", filepath);
	std::shared_ptr<CXURLRequest> optionsRequest = CXURL.OPTIONS("*", filepath);

	optionsRequest->setValueForHTTPHeaderField("Host", "www.example.com");
	optionsRequest->setValueForHTTPHeaderField("Upgrade", "TLS/1.0");
	optionsRequest->setValueForHTTPHeaderField("Connection", "Upgrade");

	//Add some HTTP headers to the CXURLRequest
	//getRequest->setValueForHTTPHeaderField("Accept:", "*/*");


	//Define the response callback to return response buffers using CXTransport CXCursor object returned via a Lambda Closure
	auto requestCallback = [&](CTError* error, std::shared_ptr<CXCursor>& cxCursor) {

		//CTCursorMapFileR(cursor);
		printf("SendTLSUpgradeRequest response header:  %.*s\n", cxCursor->_cursor->headerLength, cxCursor->_cursor->requestBuffer);//%.*s\n", cursor->_cursor.length - sizeof(ReqlQueryMessageHeader), (char*)(cursor->_cursor.header) + sizeof(ReqlQueryMessageHeader)); return;  
		printf("SendTLSUpgradeRequest response body:  %.*s\n", cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);
		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));

		//SendHTTPRequest();
	};

	//Pass the CXConnection and the lambda to populate the request buffer and asynchronously send it on the CXConnection
	//The lambda will be executed so the code calling the request can interact with the asynchronous response buffers
	optionsRequest->send(_httpCXConn, requestCallback);
}


char BasicCharacterTestPrefab[65536] = "{\
	\"vtype\" : \"Prefab\",\
	\"name\" : \"BasicCharacterTest\",\
	\"components\" : [2,[  ]],\
	\"subentities\" :\
	[2,[\
		{\
			\"name\" : \"DefaultSkybox\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Environment/DefaultSkybox\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 3.850143, 104.200722, 0.0 ]]\
			},\
			\"overrides\" :\
			{\
				\"name\" : \"\",\
				\"components\" :\
				[2,[\
					{\
						\"vcomponent\" : \"EnvironmentProbe\",\
						\"lightingStr\" : \"Cubemaps/quarry_03_8k_irrad\",\
						\"lightingCubemap\" : \"Cubemaps/quarry_03_8k_irrad\",\
						\"reflectionStr\" : \"Cubemaps/quarry_03_8k_rad\",\
						\"reflectionCubemap\" : \"Cubemaps/quarry_03_8k_rad\",\
						\"albedoCubemap\" : \"Cubemaps/quarry_03_8k\",\
						\"albedoStr\" : \"Cubemaps/quarry_03_8k\"\
					},\
					{\
						\"vcomponent\" : \"PostProcess\",\
						\"hdr\" :\
						{\
							\"gamma\" : 1.0,\
							\"exposure\" : 1.0,\
							\"sunDirection\" : [2,[ -0.09195, -0.126558, -0.987688, 0.0 ]],\
							\"brightness\" : 1.0,\
							\"sunColor\" : [2,[ 5.0, 5.0, 5.0, 1.0 ]],\
							\"contrast\" : 1.01,\
							\"iblIntensity\" : 0.0\
						},\
						\"fog\" :\
						{\
							\"color\" : [2,[ 0.07446, 0.117437, 0.147368, 1.0 ]],\
							\"densityStart\" : 50000.0,\
							\"densityFalloff\" : 10000.0\
						},\
						\"bloom\" :\
						{\
							\"threshold\" : 10.0\
						},\
						\"ssao\" :\
						{\
							\"intensity\" : 1.0\
						}\
					}\
				]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 4000.0, -4000.0, 0.0 ]],\
				\"scale\" : [2,[ 8.0, 8.0, 1.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 10000.0, -2000.0, 0.0 ]],\
				\"scale\" : [2,[ 1.0, 2.0, 1.0 ]]\
			}\
		},\
		{\
			\"name\" : \"PlayerSpawner\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Environment/BasicSpawner\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 1000.0, 647.419373, 0.0 ]]\
			},\
			\"overrides\" :\
			{\
				\"components\" :\
				[2,[\
					{\
						\"vcomponent\" : \"BasicSpawner\",\
						\"spawnpointsTagName\" : \"PlayerSpawns\"\
					}\
				]]\
			}\
		},\
		{\
			\"name\" : \"GunSpawner\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Environment/GunSpawner\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 8000.0, 0.0, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 90.000008 ]]\
			}\
		},\
		{\
			\"name\" : \"GunSpawner2\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Environment/GunSpawner\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 8000.0, 2000.0, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 90.000008 ]]\
			}\
		},\
		{\
			\"name\" : \"Player_FP\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Player/Player_FP\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ -2400.0, 2000.0, 0.0 ]]\
			},\
			\"overrides\" :\
			{\
				\"subentities\" : [2,[  ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_3\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 12000.0, 4000.0, 0.0 ]],\
				\"scale\" : [2,[ 1.0, 8.0, 6.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_14\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 12000.0, 8000.0, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 90.000008 ]],\
				\"scale\" : [2,[ 1.000004, 3.0, 3.000001 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_7\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 12000.035156, 12000.012695, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 90.000008 ]],\
				\"scale\" : [2,[ 1.000004, 8.000031, 6.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_ramp_10x20x10_3\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_ramp_10x20x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 12000.0, 0.0, 1000.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, -179.999954 ]],\
				\"scale\" : [2,[ 2.0, 1.0, 1.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_6\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 11000.0, -3000.0, 1000.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_2\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 9000.0, -4000.0, 1000.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_3\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 10000.0, -4000.0, 1000.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_13\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 4000.0, 4000.0, 0.0 ]],\
				\"scale\" : [2,[ 8.0, 8.0, 1.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_6\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 3999.972656, 11999.999023, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 89.999962 ]],\
				\"scale\" : [2,[ 1.000004, 8.000033, 6.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_2\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 12000.0, -4000.0, 0.0 ]],\
				\"scale\" : [2,[ 1.0, 8.0, 6.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_11\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 12000.0, 1999.994507, 3000.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 89.999962 ]],\
				\"scale\" : [2,[ 6.0, 3.0, 1.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_15\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ -4000.0, -4000.0, 0.0 ]],\
				\"scale\" : [2,[ 8.0, 8.0, 1.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_4\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 4000.027344, -3999.993408, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, -90.000008 ]],\
				\"scale\" : [2,[ 1.000002, 8.000013, 6.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_8\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ -4000.009766, 11999.996094, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, -179.999954 ]],\
				\"scale\" : [2,[ 1.000002, 8.000013, 6.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_5\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 11000.0, -4000.0, 1000.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_1\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 9000.0, -3000.0, 1000.0 ]],\
				\"scale\" : [2,[ 1.0, 1.0, 10.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_ramp_10x20x10_4\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_ramp_10x20x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 12000.0, 2000.0, 2000.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 179.999969 ]],\
				\"scale\" : [2,[ 2.0, 1.000002, 1.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_ramp_10x20x10\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_ramp_10x20x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 9000.0, -3000.0, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 90.000008 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_4\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 10000.0, -3000.0, 1000.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_ramp_10x20x10_1\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_ramp_10x20x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 9000.0, -4000.0, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 90.000008 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_5\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ -4000.0, -4000.0, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, -90.000008 ]],\
				\"scale\" : [2,[ 1.000002, 8.000014, 6.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_1\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 10000.0, 0.0, 0.0 ]],\
				\"scale\" : [2,[ 20.0, 2.0, 2.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_wall_01x10x10_9\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_wall_01x10x10\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ -4000.0, 4000.0, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 179.999969 ]],\
				\"scale\" : [2,[ 1.000001, 8.000008, 6.0 ]]\
			}\
		},\
		{\
			\"name\" : \"greybox_floor_10x10x01_14\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Greybox/greybox_floor_10x10x01\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ -4000.0, 4000.0, 0.0 ]],\
				\"scale\" : [2,[ 8.0, 8.0, 1.0 ]]\
			}\
		},\
		{\
			\"name\" : \"BasicSpawnPoint\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Environment/BasicSpawnPoint\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 10000.0, -3000.0, 1000.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, 135.0 ]]\
			},\
			\"overrides\" :\
			{\
				\"components\" :\
				[2,[\
					{\
						\"vcomponent\" : \"BasicSpawnPoint\",\
						\"spawnerTagName\" : \"PlayerSpawns\"\
					}\
				]]\
			}\
		},\
		{\
			\"name\" : \"BasicSpawnPoint_1\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Environment/BasicSpawnPoint\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 11000.0, 7000.0, 3000.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, -179.999985 ]]\
			},\
			\"overrides\" :\
			{\
				\"name\" : \"\",\
				\"components\" :\
				[2,[\
					{\
						\"vcomponent\" : \"BasicSpawnPoint\",\
						\"spawnerTagName\" : \"PlayerSpawns\"\
					}\
				]]\
			}\
		},\
		{\
			\"name\" : \"BasicSpawnPoint_2\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Environment/BasicSpawnPoint\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 11000.0, 5000.0, 0.0 ]],\
				\"rotation\" : [2,[ 0.0, 0.0, -179.999985 ]]\
			},\
			\"overrides\" :\
			{\
				\"name\" : \"\",\
				\"components\" :\
				[2,[\
					{\
						\"vcomponent\" : \"BasicSpawnPoint\",\
						\"spawnerTagName\" : \"PlayerSpawns\"\
					}\
				]]\
			}\
		},\
		{\
			\"name\" : \"BasicSpawner\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"Prefabs/Environment/BasicSpawner\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 6417.604492, 0.0, 0.0 ]]\
			},\
			\"overrides\" :\
			{\
				\"name\" : \"\",\
				\"components\" :\
				[2,[\
					{\
						\"vcomponent\" : \"BasicSpawner\",\
						\"autoPossessOnPlayerJoin\" : false,\
						\"prefab\" : \"Prefabs/Weapons/BasicRailgun\",\
						\"spawnOnGameBegin\" : true,\
						\"spawnOnPlayerJoin\" : false\
					}\
				]]\
			}\
		},\
		{\
			\"name\" : \"ENV_CAR_A\",\
			\"vtype\" : \"Prefab\",\
			\"prefabpath\" : \"EnvironmentArt/Vehicles/ENV_CAR_A\",\
			\"transform\" :\
			{\
				\"translation\" : [2,[ 5190.835449, 7649.76123, 0.0 ]]\
			}\
		}\
	]]\
}";

void SaveGameStateQuery()
{
	//Send the ReQL Query to find some URLs to download
	//Demonstrate issue of query and callback lambda using CXReQL API
	auto queryCallback = [&](ReqlError* error, std::shared_ptr<CXCursor>& cxCursor) {
		CTCursorCloseMappingWithSize((cxCursor->_cursor), cxCursor->_cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
		printf("SaveGameState response:  \n\n%.*s\n\n", cxCursor->_cursor->contentLength, cxCursor->_cursor->requestBuffer);
		CTCursorCloseFile((cxCursor->_cursor));
	};
	CXReQL.db("GameState").table("Scene").insert(BasicCharacterTestPrefab).run(_reqlCXConn, NULL, queryCallback);
}

const char* sampleForwardUpdate = "{\"MovementInput\" : {\"Forward\" : [2,[ -0.09195, -0.126558, -0.987688, 0.0 ]]}}";
//const char* testUpdate = "{\"MovementInput\" : {\"Forward\" : [2,[ -0.09195, -0.126558, -0.987688, 0.0 ]]}}";

static void SendForwardUpdateQuery()// CXReQLQueryClosure queryCallback, CXConnection* conn)
{

	ReqlQueryDocumentType queryDoc;

	Value UserProfileObject(kObjectType);
	Value MovementInputObject(kObjectType);
	Value ForwardVectorMakeArray(kArrayType);
	Value ForwardVectorArray(kArrayType);

	ForwardVectorArray.PushBack(Value(0.5f), queryDoc.GetAllocator()).PushBack(Value(0.5f), queryDoc.GetAllocator()).PushBack(Value(0.5f), queryDoc.GetAllocator()).PushBack(Value(1.f), queryDoc.GetAllocator());
	ForwardVectorMakeArray.PushBack(Value(REQL_MAKE_ARRAY), queryDoc.GetAllocator()).PushBack( ForwardVectorArray, queryDoc.GetAllocator());

	MovementInputObject.AddMember("Forward", ForwardVectorMakeArray, queryDoc.GetAllocator());
	UserProfileObject.AddMember("MovementInput", MovementInputObject, queryDoc.GetAllocator());
	
	auto queryCallback = [&](ReqlError* error, std::shared_ptr<CXCursor>& cxCursor) {
		printf("SendForwardUpdateQuery response:  \n\n%.*s\n\n", cxCursor->_cursor->contentLength, cxCursor->_cursor->requestBuffer);
		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));
	};
	CXReQL.db("Accounts").table("UserProfiles").get("joe@third-gen.com").update(&UserProfileObject).run(_reqlCXConn, NULL, queryCallback);
}

template<typename CXReQLQueryClosure>
static void CreateUserProfileChangefeedQuery(const char* userEmail, CXReQLQueryClosure queryCallback, CXConnection* conn)
{

	ReqlQueryDocumentType queryDoc;
	Value changeOptions(kObjectType);
	changeOptions.AddMember("squash", Value(true), queryDoc.GetAllocator());
	CXReQL.db("Accounts").table("UserProfiles").get(userEmail).changes(&changeOptions).run(conn, NULL, queryCallback);

}

template<typename CXReQLQueryClosure>
static void CreateTransientsChangefeedQuery(CXReQLQueryClosure queryCallback, CXConnection* conn)
{
	ReqlQueryDocumentType queryDoc;
	Value changeOptions(kObjectType);
	changeOptions.AddMember("squash", Value(true), queryDoc.GetAllocator());
	CXReQL.db("GameState").table("Transients").changes(&changeOptions).run(conn, NULL, queryCallback);
}

static void StartUserProfileChangefeedUpdate()
{
	char* userEmail = "joe@third-gen.com";// (char*)results[0]["Email"].GetString();

	auto CreateUserChangefeedQueryCallback = [&](ReqlError* error, std::shared_ptr<CXCursor> cxCursor) {

		//Info(L"CreateUserProfileChangefeedQuery response:  \n\n%.*s\n\n", cxCursor->_cursor.contentLength, cxCursor->_cursor.file.buffer);
		fprintf(stderr, "CreateUserChangefeedQuery response:  \n\n%.*s\n\n", (int)cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);



		ReqlQueryDocumentType queryObject;
		//queryObject.SetObject();

		// holds the values I retrieve from the json document
		cxCursor->_cursor->file.buffer[cxCursor->_cursor->contentLength] = '\0';
		queryObject.Parse((char*)cxCursor->_cursor->file.buffer);

		//if (queryObject.Parse().HasParseError())
		//	printf( "\nERROR: encountered a JSON parsing error.\n" );

		// Get the nested object that contains the elements I want.
		// In my case, the nested object in my json document was results
		// and the values I was after were identified as "t"
		rapidjson::Value& results = queryObject["r"];
		assert(results.IsArray());

		if (results.Size() > 0 && results[0].HasMember("new_val"))
		{
			assert(results[0]["new_val"]["MovementInput"]["Forward"].IsArray());
			assert(results[0]["new_val"]["MovementInput"]["Forward"].Size() == 4);
			rapidjson::Value& forwardVectorArray = results[0]["new_val"]["MovementInput"]["Forward"];

			float x, y, z, delta;
			x = forwardVectorArray[0].GetFloat();
			y = forwardVectorArray[1].GetFloat();
			z = forwardVectorArray[2].GetFloat();
			delta = forwardVectorArray[3].GetFloat();
		
			fprintf(stderr, "Forward Vector [%f,%f,%f,%f] \n\n", x, y, z, delta);
		
		}

		CXReQLQuery continueQuery;
		continueQuery.Continue(cxCursor, cxCursor->callback);
		//CXReQL.Continue( cxCursor, )

		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));

	};

	CreateUserProfileChangefeedQuery(userEmail, CreateUserChangefeedQueryCallback, _reqlCXConn);
}

static void StartTransientsChangefeedUpdate()
{
	//Create the Changfeed on the GameState->Transients tables
	auto CreateTransientsChangefeedQueryCallback = [&](ReqlError* error, std::shared_ptr<CXCursor> cxCursor)
	{

		//Info(L"CreatTransientChangefeedQuery response:  \n\n%.*s\n\n", cxCursor->_cursor.contentLength, cxCursor->_cursor.file.buffer);
		fprintf(stderr, "CreatTransientChangefeedQuery response:  \n\n%.*s\n\n", (int)cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);


		ReqlQueryDocumentType queryObject;
		//queryObject.SetObject();

		// holds the values I retrieve from the json document
		cxCursor->_cursor->file.buffer[cxCursor->_cursor->contentLength] = '\0';
		queryObject.Parse((char*)cxCursor->_cursor->file.buffer);

		//if (queryObject.Parse().HasParseError())
		//	printf( "\nERROR: encountered a JSON parsing error.\n" );

		rapidjson::Value& results = queryObject["r"];
		assert(results.IsArray());

		for (unsigned int transientIndex = 0; transientIndex < results.Size(); transientIndex++)
		{
			if (results[transientIndex].HasMember("new_val") && results[transientIndex]["new_val"].HasMember("type"))
			{
				if (strcmp(results[transientIndex]["new_val"]["type"].GetString(), "PlayerInput") == 0)
				{
					assert(results[transientIndex]["new_val"]["Orientation"].IsArray());
					assert(results[transientIndex]["new_val"]["Forward"].IsArray());
					assert(results[transientIndex]["new_val"]["Right"].IsArray());

					assert(results[transientIndex]["new_val"]["Forward"].Size() == 4);
					assert(results[transientIndex]["new_val"]["Right"].Size() == 4);
					assert(results[transientIndex]["new_val"]["Orientation"].Size() == 4);

					rapidjson::Value& quatVectorArray = results[transientIndex]["new_val"]["Orientation"];
					rapidjson::Value& forwardVectorArray = results[transientIndex]["new_val"]["Forward"];
					rapidjson::Value& rightVectorArray = results[transientIndex]["new_val"]["Right"];
					//rapidjson::Value& launchVectorArray = results[transientIndex]["new_val"]["Projectile"];
					//rapidjson::Value& yawVal = results[0]["new_val"]["Yaw"];
					//rapidjson::Value& pitchVal = results[0]["new_val"]["Pitch"];
					rapidjson::Value& jumpVal = results[transientIndex]["new_val"]["Jump"];

					float qx, qy, qz, qw;
					qx = quatVectorArray[0].GetFloat();
					qy = quatVectorArray[1].GetFloat();
					qz = quatVectorArray[2].GetFloat();
					qw = quatVectorArray[3].GetFloat();

					float fx, fy, fz, fdelta;
					fx = forwardVectorArray[0].GetFloat();
					fy = forwardVectorArray[1].GetFloat();
					fz = forwardVectorArray[2].GetFloat();
					fdelta = forwardVectorArray[3].GetFloat();

					float rx, ry, rz, rdelta;
					rx = rightVectorArray[0].GetFloat();
					ry = rightVectorArray[1].GetFloat();
					rz = rightVectorArray[2].GetFloat();
					rdelta = rightVectorArray[3].GetFloat();

					//float yaw, pitch;
					//yaw = yawVal.GetFloat();
					//pitch = pitchVal.GetFloat();

					bool jump = jumpVal.GetBool();

					fprintf(stderr, "Quat Vector    [%f,%f,%f,%f] \n", qx, qy, qz, qw);
					fprintf(stderr, "Forward Vector [%f,%f,%f,%f] \n", fx, fy, fz, fdelta);
					fprintf(stderr, "Right Vector   [%f,%f,%f,%f] \n", rx, ry, rz, rdelta);

					//fprintf(stderr, "Yaw (%f) \n\n", yaw);
					//fprintf(stderr, "Pitch (%f) \n", pitch);
					fprintf(stderr, "Jump State (%d) \n", jump);

					/*
					FQuat quatVector = FQuat(qx, qy, qz, qw);
					FVector4f forwardVector = FVector4f(fx, fy, fz, fdelta);
					FVector4f rightVector = FVector4f(rx, ry, rz, rdelta);
					//FVector launchVector = FVector(lx, ly, lz);

					//schedule update back to game thread
					AsyncTask(ENamedThreads::GameThread, [&, gCharacter, quatVector, forwardVector, rightVector, jump]()
						{
							gCharacter->SetMovementInput(quatVector, forwardVector, rightVector, jump);
						});
					*/

				}
				else if (strcmp(results[transientIndex]["new_val"]["type"].GetString(), "Bullet") == 0)
				{
					assert(results[transientIndex]["new_val"]["direction"].IsArray());
					assert(results[transientIndex]["new_val"]["direction"].Size() == 4);

					rapidjson::Value& launchVectorArray = results[transientIndex]["new_val"]["direction"];
					rapidjson::Value& launchTimestamp = results[transientIndex]["new_val"]["timestamp"];

					float lx, ly, lz;
					double ltimestamp;
					lx = launchVectorArray[0].GetFloat();
					ly = launchVectorArray[1].GetFloat();
					lz = launchVectorArray[2].GetFloat();
					ltimestamp = launchTimestamp.GetDouble();
					uint64_t utimestamp = ltimestamp; //i hate this
					fprintf(stderr, "Launch Vector    [%f,%f,%f,%g] \n", lx, ly, lz, ltimestamp);

					/*
					FVector launchVector = FVector(lx, ly, lz);

					//schedule update back to game thread
					AsyncTask(ENamedThreads::GameThread, [&, gCharacter, launchVector, utimestamp]()
						{
							gCharacter->LaunchProjectile(launchVector, utimestamp);
						});
					*/
				}
				else
					assert(1 == 0); //unknown transient type
			}
			else if (results[transientIndex].HasMember("new_val"))
				assert(results[transientIndex]["new_val"].HasMember("type"));
		}


		CXReQLQuery continueQuery;
		continueQuery.Continue(cxCursor, cxCursor->callback);

		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));
	};


	auto CreateUserProfileChangefeedQueryCallback = [&](ReqlError* error, std::shared_ptr<CXCursor> cxCursor) {

		//Info(L"CreateUserProfileChangefeedQuery response:  \n\n%.*s\n\n", cxCursor->_cursor.contentLength, cxCursor->_cursor.file.buffer);
		fprintf(stderr, "CreateUserProfileChangefeedQuery response:  \n\n%.*s\n\n", (int)cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);


		ReqlQueryDocumentType queryObject;
		//queryObject.SetObject();

		// holds the values I retrieve from the json document
		cxCursor->_cursor->file.buffer[cxCursor->_cursor->contentLength] = '\0';
		queryObject.Parse((char*)cxCursor->_cursor->file.buffer);

		//if (queryObject.Parse().HasParseError())
		//	printf( "\nERROR: encountered a JSON parsing error.\n" );

		rapidjson::Value& results = queryObject["r"];
		assert(results.IsArray());

		if (results.Size() > 0 && results[0].HasMember("new_val"))
		{
			assert(results[0]["new_val"]["Orientation"].IsArray());
			assert(results[0]["new_val"]["Forward"].IsArray());
			assert(results[0]["new_val"]["Right"].IsArray());

			assert(results[0]["new_val"]["Forward"].Size() == 4);
			assert(results[0]["new_val"]["Right"].Size() == 4);
			assert(results[0]["new_val"]["Orientation"].Size() == 4);

			rapidjson::Value& quatVectorArray = results[0]["new_val"]["Orientation"];
			rapidjson::Value& forwardVectorArray = results[0]["new_val"]["Forward"];
			rapidjson::Value& rightVectorArray = results[0]["new_val"]["Right"];
			//rapidjson::Value& launchVectorArray = results[0]["new_val"]["Projectile"];
			//rapidjson::Value& yawVal = results[0]["new_val"]["Yaw"];
			//rapidjson::Value& pitchVal = results[0]["new_val"]["Pitch"];
			rapidjson::Value& jumpVal = results[0]["new_val"]["Jump"];

			float qx, qy, qz, qw;
			qx = quatVectorArray[0].GetFloat();
			qy = quatVectorArray[1].GetFloat();
			qz = quatVectorArray[2].GetFloat();
			qw = quatVectorArray[3].GetFloat();

			float fx, fy, fz, fdelta;
			fx = forwardVectorArray[0].GetFloat();
			fy = forwardVectorArray[1].GetFloat();
			fz = forwardVectorArray[2].GetFloat();
			fdelta = forwardVectorArray[3].GetFloat();

			float rx, ry, rz, rdelta;
			rx = rightVectorArray[0].GetFloat();
			ry = rightVectorArray[1].GetFloat();
			rz = rightVectorArray[2].GetFloat();
			rdelta = rightVectorArray[3].GetFloat();

			//float yaw, pitch;
			//yaw = yawVal.GetFloat();
			//pitch = pitchVal.GetFloat();

			bool jump = jumpVal.GetBool();

			fprintf(stderr, "Quat Vector    [%f,%f,%f,%f] \n", qx, qy, qz, qw);
			fprintf(stderr, "Forward Vector [%f,%f,%f,%f] \n", fx, fy, fz, fdelta);
			fprintf(stderr, "Right Vector   [%f,%f,%f,%f] \n", rx, ry, rz, rdelta);

			//fprintf(stderr, "Yaw (%f) \n\n", yaw);
			//fprintf(stderr, "Pitch (%f) \n", pitch);
			fprintf(stderr, "Jump State (%d) \n", jump);

			/*
			FQuat quatVector = FQuat(qx, qy, qz, qw);
			FVector4f forwardVector = FVector4f(fx, fy, fz, fdelta);
			FVector4f rightVector = FVector4f(rx, ry, rz, rdelta);
			//FVector launchVector = FVector(lx, ly, lz);


			// We schedule back to the main thread and pass in our params
			AsyncTask(ENamedThreads::GameThread, [&, gCharacter, quatVector, forwardVector, rightVector, jump]()
				{
					gCharacter->SetMovementInput(quatVector, forwardVector, rightVector, jump);
					//gCharacter->SetMovementInput(forwardVector, rightVector, yaw, pitch);

					//gCharacter->LaunchProjectile(launchVector, ltimestamp.i);

				});
			*/
		}


		CXReQLQuery continueQuery;
		continueQuery.Continue(cxCursor, cxCursor->callback);
		//CXReQL.Continue( cxCursor, )

		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));

	};

	char* userEmail = "joe@third-gen.com";// (char*)results[0]["Email"].GetString();
	CreateUserProfileChangefeedQuery(userEmail, CreateUserProfileChangefeedQueryCallback, _reqlCXConn);
	CreateTransientsChangefeedQuery(CreateTransientsChangefeedQueryCallback, _reqlCXConn);
}


template<typename CXReQLQueryClosure>
static void SendProjectileUpdateQuery(float x, float y, float z, int64_t timestamp, CXReQLQueryClosure queryCallback, CXConnection* conn)
{
	ReqlQueryDocumentType queryDoc;

	Value UserProfileObject(kObjectType);
	//Value MovementInputObject(kObjectType);
	Value LaunchVectorMakeArray(kArrayType);
	Value LaunchVectorArray(kArrayType);

	LaunchVectorArray.PushBack(Value(x), queryDoc.GetAllocator()).PushBack(Value(y), queryDoc.GetAllocator()).PushBack(Value(z), queryDoc.GetAllocator()).PushBack(Value(timestamp), queryDoc.GetAllocator());
	LaunchVectorMakeArray.PushBack(Value(REQL_MAKE_ARRAY), queryDoc.GetAllocator()).PushBack(LaunchVectorArray, queryDoc.GetAllocator());

	UserProfileObject.AddMember("Projectile", LaunchVectorMakeArray, queryDoc.GetAllocator());
	//UserProfileObject.AddMember("MovementInput", MovementInputObject, queryDoc.GetAllocator());

	CXReQL.db("Accounts").table("UserProfiles").get("joe@third-gen.com").update(&UserProfileObject).run(_reqlCXConn, NULL, queryCallback);
	//CXReQL.db("Accounts").table("UserProfiles").get("joe@third-gen.com").update((char*)sampleForwardUpdate).run(conn, NULL, queryCallback);
}


void SendProjectileUpdate(float x, float y, float z, int64_t timestamp)
{
	//Send the ReQL Query to find some URLs to download
	//Demonstrate issue of query and callback lambda using CXReQL API
	auto queryCallback = [&](ReqlError* error, std::shared_ptr<CXCursor>& cxCursor) {

		printf("SendProjectileUpdate response:  \n\n%.*s\n\n", cxCursor->_cursor->contentLength, cxCursor->_cursor->requestBuffer);
		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));
	};

	SendProjectileUpdateQuery(x, y, z, timestamp, queryCallback, _reqlCXConn);
}


void CreateUserQuery(const char* Email, const char* Username, const char* Password, CXConnection* conn)
{
	//const char* Email = "joem@vitalxp.io";
	//const char* Username = "VitalDev";
	//const char* Password = "vital.pw.4.dev";
	ReqlQueryDocumentType queryDoc;

	Value UserObject(kObjectType);
	Value UserProfileObject(kObjectType);
	UserObject.AddMember("Email", StringRef(Email), queryDoc.GetAllocator());
	UserObject.AddMember("Username", StringRef(Username), queryDoc.GetAllocator());
	UserObject.AddMember("Password", StringRef(Password), queryDoc.GetAllocator());
	UserObject.AddMember("Verified", Value(false), queryDoc.GetAllocator());

	UserProfileObject.AddMember("Email", StringRef(Email), queryDoc.GetAllocator());
	UserProfileObject.AddMember("Username", StringRef(Username), queryDoc.GetAllocator());


	//Send the ReQL Query to find some URLs to download
	//Demonstrate issue of query and callback lambda using CXReQL API
	auto queryCallback = [&](ReqlError* error, std::shared_ptr<CXCursor>& cxCursor) {
		
		printf("CreateUserQuery response:  \n\n%.*s\n\n", cxCursor->_cursor->contentLength, cxCursor->_cursor->requestBuffer);
		CTCursorCloseMappingWithSize((cxCursor->_cursor), cxCursor->_cursor->contentLength); //overlappedResponse->buf - cursor->file.buffer);
		CTCursorCloseFile((cxCursor->_cursor));
	};


	//[43, [[78, [[15, [[14, ["Accounts"]], "Users"] ], "joem@vitalxp.io"], { "index":"Email" }]] ],
	CXReQLQuery* getAllUserByEmailCountQuery = &(CXReQL.dbQuery("Accounts").table("Users").getAll(Email, "Email").count());

	//[43, [[78, [[15, [[14, ["Accounts"]], "UserProfiles"] ], "VitalDev"], { "index":"Username" }]] ],
	CXReQLQuery* getAllUserProfilesByUsernameCountQuery = &(CXReQL.dbQuery("Accounts").table("UserProfiles").getAll(Username, "Username").count());

	//[16, [[15, [[14, ["Accounts"]], "UserProfiles"] ], "joem@vitalxp.io"] ]
	CXReQLQuery* getUserProfileQuery = &(CXReQL.dbQuery("Accounts").table("UserProfiles").get(Email));




	//Value vars0(kArrayType);
	//Value vars1(kArrayType);
	//Value vars2(kArrayType);

	Value vars012(kArrayType); 	//[0, 1, 2] used for first do->func response
	Value vars3(kArrayType);	//[3]		 used for the second do->func response
	Value vars4(kArrayType);	//[4]		 used for the second do->func response

	Value datum012(kArrayType); //[2, [0, 1, 2]],
	Value datum3(kArrayType); 	//[2, [3]],
	Value datum4(kArrayType); 	//[2, [4]],
	//Value VARS_ARRAY(kArrayType);
	Value doFuncArgs(kArrayType);

	Value result0(kArrayType);
	Value result1(kArrayType);
	Value result2(kArrayType);
	Value result3(kArrayType);
	Value result4(kArrayType);

	//[0, 1, 2]
	vars012.PushBack(Value(0), queryDoc.GetAllocator())\
		.PushBack(Value(1), queryDoc.GetAllocator())\
		.PushBack(Value(2), queryDoc.GetAllocator());

	vars3.PushBack(Value(3), queryDoc.GetAllocator()); 	//[3]
	vars4.PushBack(Value(4), queryDoc.GetAllocator()); 	//[4]

	//[2, [0, 1, 2]],
	datum012.PushBack(Value(REQL_MAKE_ARRAY), queryDoc.GetAllocator()).PushBack(vars012, queryDoc.GetAllocator());
	//[2, [3]],
	datum3.PushBack(Value(REQL_MAKE_ARRAY), queryDoc.GetAllocator()).PushBack(vars3, queryDoc.GetAllocator());
	//[2, [4]],
	datum4.PushBack(Value(REQL_MAKE_ARRAY), queryDoc.GetAllocator()).PushBack(vars4, queryDoc.GetAllocator());


	//[10, [0]],
	result0.PushBack(Value(REQL_VAR), queryDoc.GetAllocator()).PushBack(Value(kArrayType).PushBack(Value(0), queryDoc.GetAllocator()), queryDoc.GetAllocator());
	result1.PushBack(Value(REQL_VAR), queryDoc.GetAllocator()).PushBack(Value(kArrayType).PushBack(Value(1), queryDoc.GetAllocator()), queryDoc.GetAllocator());
	result3.PushBack(Value(REQL_VAR), queryDoc.GetAllocator()).PushBack(Value(kArrayType).PushBack(Value(3), queryDoc.GetAllocator()), queryDoc.GetAllocator());
	result4.PushBack(Value(REQL_VAR), queryDoc.GetAllocator()).PushBack(Value(kArrayType).PushBack(Value(4), queryDoc.GetAllocator()), queryDoc.GetAllocator());

	//[10, [0, 1, 2]] ]
	//VARS_ARRAY.PushBack(Value(REQL_VAR), queryDoc.GetAllocator()).PushBack(vars012Copy, queryDoc.GetAllocator());

	//[67, [[67, [[17, [[10, [0]], 0] ], [17, [[10, [1]], 0] ]] ], [17, [[10, [2]], null] ]] ] ,
	int var0EqualValue = 0;
	int var1EqualValue = 0;
	CXReQLQuery* var1EqualQuery = &(CXReQL.eq(1, &var1EqualValue));
	CXReQLQuery* var2EqualQuery = &(CXReQL.eq(2, NULL));
	CXReQLQuery* doFuncBranchIFQuery = &CXReQL.eq(0, &var0EqualValue). bland (var1EqualQuery). bland (var2EqualQuery);// . and (UsernameProfileCount.eq(0)). and (EmailProfile.eq(null))

	//sub do->func->branch
	//{
		//[56, [[15, [[14, ["Accounts"]], "Users"] ], { "Email":"joem@vitalxp.io","Username" : "VitalDev","Password" : "vital.pw.4.dev","Verified" : false }] ]
	CXReQLQuery* insertUserQuery = &(CXReQL.dbQuery("Accounts").table("Users").insert(&UserObject));
	CXReQLQuery* insertUserProfileQuery = &(CXReQL.dbQuery("Accounts").table("UserProfiles").insert(&UserProfileObject));

	//[17, [[170, [[10, [3]], "inserted"] ], 1] ] ,
	int var3EqualValue = 1;
	CXReQLQuery* doInsertUserFuncBranchIFQuery = &CXReQL.eq(3, "inserted", &var3EqualValue);// . and (UsernameProfileCount.eq(0)). and (EmailProfile.eq(null))

	//[2, [[10, [3]], [10, [4]]] ]
	Value successJson(kArrayType);
	Value successJsonArray(kArrayType);
	successJsonArray.PushBack(result3, queryDoc.GetAllocator()).PushBack(result4, queryDoc.GetAllocator());
	successJson.PushBack(Value(REQL_MAKE_ARRAY), queryDoc.GetAllocator())\
		.PushBack(successJsonArray, queryDoc.GetAllocator());



	//[2, [4]], [2, [[10, [3]], [10, [4]]] ]
	Value doInsertUserProfileFuncArgs(kArrayType);
	doInsertUserProfileFuncArgs.PushBack(datum4, queryDoc.GetAllocator()).PushBack(successJson, queryDoc.GetAllocator());
	CXReQLQuery* doInsertUserProfileFuncQuery = &(CXReQL.funcQuery(&doInsertUserProfileFuncArgs));

	/*
	//Serialize to buffer
	char queryBuffer[VT_REQL_QUERY_STATIC_BUFF_SIZE];
	memset(queryBuffer, 0, VT_REQL_QUERY_STATIC_BUFF_SIZE);
	char* queryMsgBuffer = &(queryBuffer[0]);
	VTReQLString parseDomToJsonString(&queryMsgBuffer, VT_REQL_QUERY_STATIC_BUFF_SIZE);
	rapidjson::Writer<VTReQLString, UTF8<> > writer(parseDomToJsonString);
	doInsertUserProfileFuncQuery->getQueryArray()->Accept(writer);
	printf("doInsertUserProfileFuncQuery: %s", queryBuffer);
	*/

	Value doInsertUserProfileQueryArgs(kArrayType);
	doInsertUserProfileQueryArgs.PushBack(*doInsertUserProfileFuncQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(*insertUserProfileQuery->getQueryArray(), queryDoc.GetAllocator());
	CXReQLQuery* doInsertUserProfileQuery = &(CXReQL.doQuery(&doInsertUserProfileQueryArgs));


	//[2, [[10, [3]], { "errors":1,"inserted" : 0 }] ]
	Value doInsertUserFuncBranchElseJson(kArrayType);
	Value doInsertUserErrorJsonArray(kArrayType);
	Value doInsertUserErrorJson(kObjectType);
	doInsertUserErrorJson.AddMember("errors", Value(1), queryDoc.GetAllocator());
	doInsertUserErrorJson.AddMember("inserted", Value(0), queryDoc.GetAllocator());
	doInsertUserErrorJsonArray.PushBack(result3, queryDoc.GetAllocator())\
		.PushBack(doInsertUserErrorJson, queryDoc.GetAllocator());
	doInsertUserFuncBranchElseJson.PushBack(Value(REQL_MAKE_ARRAY), queryDoc.GetAllocator())\
		.PushBack(doInsertUserErrorJsonArray, queryDoc.GetAllocator());

	Value doInsertUserFuncBranchArgs(kArrayType);
	doInsertUserFuncBranchArgs.PushBack(*doInsertUserFuncBranchIFQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(*doInsertUserProfileQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(doInsertUserFuncBranchElseJson, queryDoc.GetAllocator());

	CXReQLQuery* doInsertUserFuncBranchQuery = &(CXReQL.branchQuery(&doInsertUserFuncBranchArgs));


	Value doInsertUserFuncArgs(kArrayType);
	doInsertUserFuncArgs.PushBack(datum3, queryDoc.GetAllocator()).PushBack(*doInsertUserFuncBranchQuery->getQueryArray(), queryDoc.GetAllocator());

	CXReQLQuery* doInsertUserFuncQuery = &(CXReQL.funcQuery(&doInsertUserFuncArgs));

	//Put the list of 2n+1 commands to be passed to VTReQL.do in a json framework array
	Value doInsertUserQueryArgs(kArrayType);
	doInsertUserQueryArgs.PushBack(*doInsertUserFuncQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(*insertUserQuery->getQueryArray(), queryDoc.GetAllocator());
	//}

	//The entire sub do->func->branch is the THEN statement of the first branch
	CXReQLQuery* doInsertUserQuery = &(CXReQL.doQuery(&doInsertUserQueryArgs));

	//[2, [{"errors": [10, [0]] , "inserted" : 0}, { "errors": [10,[1]] ,"inserted" : 0 }]]
	Value doFuncBranchElseJson(kArrayType);
	Value errorJsonArray(kArrayType);
	Value emailErrorJson(kObjectType);
	Value usernameErrorJson(kObjectType);
	emailErrorJson.AddMember("errors", result0, queryDoc.GetAllocator());
	emailErrorJson.AddMember("inserted", Value(0), queryDoc.GetAllocator());
	usernameErrorJson.AddMember("errors", result1, queryDoc.GetAllocator());
	usernameErrorJson.AddMember("inserted", Value(0), queryDoc.GetAllocator());
	errorJsonArray.PushBack(emailErrorJson, queryDoc.GetAllocator()).PushBack(usernameErrorJson, queryDoc.GetAllocator());
	doFuncBranchElseJson.PushBack(Value(REQL_MAKE_ARRAY), queryDoc.GetAllocator())\
		.PushBack(errorJsonArray, queryDoc.GetAllocator());

	Value doFuncBranchArgs(kArrayType);
	doFuncBranchArgs.PushBack(*doFuncBranchIFQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(*doInsertUserQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(doFuncBranchElseJson, queryDoc.GetAllocator());

	CXReQLQuery* doFuncBranchQuery = &(CXReQL.branchQuery(&doFuncBranchArgs));


	doFuncArgs.PushBack(datum012, queryDoc.GetAllocator()).PushBack(*doFuncBranchQuery->getQueryArray(), queryDoc.GetAllocator());

	//The VTReQL.funcQuery member function allocates VTReQLQuery object memory with new, so must be manually deleted
	CXReQLQuery* doFuncQuery = &(CXReQL.funcQuery(&doFuncArgs));

	//The VTReQL.dbQuery member function allocates VTReQLQuery object memory with new, so must be manually deleted
	//VTReQLQuery* getQuery = &(VTReQL.dbQuery("Accounts").table("UserProfiles").get("joem@vitalxp.io"));





	//Put the list of 2n+1 commands to be passed to VTReQL.do in a json framework array
	Value doQueryArgs(kArrayType);
	doQueryArgs.PushBack(*doFuncQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(*getAllUserByEmailCountQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(*getAllUserProfilesByUsernameCountQuery->getQueryArray(), queryDoc.GetAllocator())\
		.PushBack(*getUserProfileQuery->getQueryArray(), queryDoc.GetAllocator());

	//The VTReQL.dbQuery macro allocates VTReQLQuery object memory with new manged by a shared_ptr, so does not need manual cleanup
	//VTReQL.db("Accounts").table("UserProfiles").get("joem@vitalxp.io").run(_reqlConn, NULL, queryCallback);
	CXReQL.do(&doQueryArgs).run(conn, NULL, queryCallback);

	//delete non-shared pointer intermediate queries

	//delete inner do-func-branch queries
	delete insertUserQuery;
	delete insertUserProfileQuery;
	delete doInsertUserFuncBranchIFQuery;
	delete doInsertUserProfileFuncQuery;
	delete doInsertUserProfileQuery;
	delete doInsertUserFuncBranchQuery;
	delete doInsertUserFuncQuery;
	delete doInsertUserQuery;

	//cleanup outer do-func-branch queries;
	delete var1EqualQuery;
	delete var2EqualQuery;
	delete doFuncBranchIFQuery;
	delete doFuncBranchQuery;
	delete doFuncQuery;

	delete getAllUserByEmailCountQuery;
	delete getAllUserProfilesByUsernameCountQuery;
	delete getUserProfileQuery;

}

void LoadGameStateQuery()
{
	//Send the ReQL Query to find some URLs to download
	//Demonstrate issue of query and callback lambda using CXReQL API
	auto queryCallback = [&](ReqlError* error, std::shared_ptr<CXCursor>& cxCursor) {

		//CTCursorCloseMappingWithSize(&(cxCursor->_cursor), cxCursor->_cursor.contentLength); //overlappedResponse->buf - cursor->file.buffer);
		printf("LoadGameState response:  \n\n%.*s\n\n", cxCursor->_cursor->contentLength, cxCursor->_cursor->file.buffer);
		//CTCursorCloseFile(&(cxCursor->_cursor));
	};
	CXReQL.db("GameState").table("Scene").get("BasicCharacterTest").run(_reqlCXConn, NULL, queryCallback);
}


//Define crossplatform keyboard event loop handler
bool getconchar(KEY_EVENT_RECORD* krec)
{
#ifdef _WIN32
	DWORD cc;
	INPUT_RECORD irec;
	KEY_EVENT_RECORD* eventRecPtr;

	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

	if (h == NULL)
	{
		return false; // console not found
	}

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
			krec.uChar.AsciiChar = c;
			return true;
		}
	}
	//else	
	//	printf("\n!kbhit\n");


#endif
	return false; //future ????
}

static const char* user_email = "joe@third-gen.com";
static const char* user_password = "this_is_a_password$";
static const char* user_name = "3rdGenJoe";


void SystemKeyboardEventLoop()
{
#ifndef _WIN32	
	//user linux termios to set terminal mode from line read to char read
	set_conio_terminal_mode();
#endif
	printf("\nStarting SystemKeyboardEventLoop...\n");
	int64_t timestamp = 12345678912345;

	KEY_EVENT_RECORD key;
	for (;;)
	{
		memset(&key, 0, sizeof(KEY_EVENT_RECORD));
		getconchar(&key);

		if (key.uChar.AsciiChar == 's')
			StartTransientsChangefeedUpdate();
		if (key.uChar.AsciiChar == 'g')
			LoadGameStateQuery();
			//SendHTTPRequest();
		if (key.uChar.AsciiChar == 'i')
			SaveGameStateQuery();
		//if (key.uChar.AsciiChar == 'u')
		//	SendTLSUpgradeRequest();
			
			//CreateUserQuery(user_email, user_name, user_password, _reqlCXConn);
		else if (key.uChar.AsciiChar == 'q')
			break;
	}
}

#define HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS 1024
CTConnection HAPPYEYEBALLS_CONNECTION_POOL[HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS];

#define CT_MAX_INFLIGHT_CURSORS 1024
CTCursor _httpCursor[CT_MAX_INFLIGHT_CURSORS];
//CTCursor _reqlCursor;

int main(int argc, char **argv) 
{	
	//Static Definitions
	int i;

	int64_t time = ct_system_utc();
	printf("time = %lld", time);

	HANDLE cxThread, txThread, rxThread;
	CTKernelQueue cxQueue, txQueue, rxQueue;

	ULONG_PTR dwCompletionKey = (ULONG_PTR)NULL;

	CTTarget httpTarget = {0};
	CTTarget reqlService = {0};
	char* caPath = NULL;// (char*)certStr;//"C:\\Development\\git\\CoreTransport\\bin\\Keys\\ComposeSSLCertficate.der";// .. / Keys / ComposeSSLCertificate.der";// MSComposeSSLCertificate.cer";

	//These are only relevant for custom DNS resolution, which has not been implemented yet for WIN32 platforms
	//char * resolvConfPath = "../Keys/resolv.conf";
	//char * nsswitchConfPath = "../Keys/nsswitch.conf";

	//Connection & Cursor Pool Initialization
	CTCreateConnectionPool(&(HAPPYEYEBALLS_CONNECTION_POOL[0]), HAPPYEYEBALLS_MAX_INFLIGHT_CONNECTIONS);
	CTCreateCursorPool(&(_httpCursor[0]), CT_MAX_INFLIGHT_CURSORS);

	//Thread Pool Initialization
	cxQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	txQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);
	rxQueue = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, dwCompletionKey, 0);

	for (i = 0; i < 1; ++i)
	{
		//On Darwin Platforms we utilize a thread pool implementation through GCD that listens for available data on the socket
		//and provides us with a dispatch_queue on a dedicated pool thread for reading
		//On Win32 platforms we create a thread/pool of threads to associate an io completion port
		//When Win32 sees that data is available for read it will read for us and provide the buffers to us on the completion port thread/pool

		cxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CX_Dequeue_Connect, cxQueue, 0, NULL);
		txThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CX_Dequeue_Encrypt_Send, txQueue, 0, NULL);
		rxThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CX_Dequeue_Recv_Decrypt, rxQueue, 0, NULL);
	}


	//Define https connection target
	//We aren't doing any authentication via http
	httpTarget.url.host = (char*)http_server;
	httpTarget.url.port = http_port;
	httpTarget.proxy.host = (char*)proxy_server;
	httpTarget.proxy.port = proxy_port;
	httpTarget.ssl.ca = NULL;//(char*)caPath;
	httpTarget.ssl.method = CTSSL_TLS_1_2;
	//httpTarget.dns.resconf = (char*)resolvConfPath;
	//httpTarget.dns.nssconf = (char*)nsswitchConfPath;
	httpTarget.txQueue = txQueue;
	httpTarget.rxQueue = rxQueue;
	httpTarget.cxQueue = cxQueue;

	//Define RethinkDB service connection target
	//We are doing SCRAM authentication over TLS required by RethinkDB
	reqlService.url.host = (char*)rdb_server;
	reqlService.url.port = rdb_port;
	reqlService.proxy.host = (char*)proxy_server;
	reqlService.proxy.port = proxy_port;
	reqlService.user = (char*)rdb_user;
	reqlService.password = (char*)rdb_pass;
	reqlService.ssl.ca = (char*)caPath;
	reqlService.ssl.method = CTSSL_TLS_1_2;
	reqlService.txQueue = txQueue;
	reqlService.rxQueue = rxQueue;
	reqlService.cxQueue = cxQueue;

	//Use CXTransport CXURL C++ API to connect to our HTTPS target server
	//CXURL.connect(&httpTarget, _cxURLConnectionClosure);

	//User CXTransport CXReQL C++ API to connect to our RethinkDB service
	CXReQL.connect(&reqlService, _cxReQLConnectionClosure);

	//Keep the app running using platform defined run loop
	SystemKeyboardEventLoop();

	//Deleting CXConnection objects will
	//Clean up socket connections
	//delete _httpCXConn;
	delete _reqlCXConn;	
	
	//Clean  Up Auth Memory
	ca_scram_cleanup();

	//Clean Up SSL Memory
	CTSSLCleanup();

	//TO DO:  Shutdown threads

	//Cleanup Thread Handles
	if (cxThread && cxThread != INVALID_HANDLE_VALUE)
		CloseHandle(cxThread);

	if (txThread && txThread != INVALID_HANDLE_VALUE)
		CloseHandle(txThread);

	if (rxThread && rxThread != INVALID_HANDLE_VALUE)
		CloseHandle(rxThread);


	return 0;
}
