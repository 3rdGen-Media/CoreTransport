
#ifndef CTURL_H
#define CTURL_H


typedef enum CTPROTCOL
{
	//TCP PROTCOLS
	HTTP,
	HTTPS,
	REQL,
	SOCKS4,
	SOCKS4A,
	SOCKS5
}CTPROTOCOL;

typedef struct CTURL //Size:  22 or 26; Alignment: 16 bytes
{
    //TO DO:  replace this with sockaddr_storage
    //Pre-resolved socket address input or storage for async resolve
    //struct sockaddr_in      addr;           //16 bytes
    struct sockaddr_storage addr;
//#endif
    //Server host address string
    char*                   host;           //4 or 8 bytes
    unsigned short          port;           //2 bytes
}CTURL;

//TO DO:  These are HTTP specific (not URL)
typedef enum URLRequestType
{
    URL_GET				= 1,			//1
    URL_PUT				= 2,		   //2
    URL_POST			= 3,          //3
    URL_DELETE			= 4,		 //4
    URL_CONNECT         = 5,        //5
    URL_OPTIONS         = 6        //6
}URLRequestType;


#endif