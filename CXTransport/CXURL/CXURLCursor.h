#pragma once
#ifndef CXURLCURSOR_H
#define CXURLCURSOR_H


namespace CoreTransport
{
	class CXURL_API CXURLCursor : public CXCursor
	{
		public:
			//CXCursor();
			CXURLCursor(CXConnection * conn) : CXCursor(conn) {}
			CXURLCursor(CXConnection * conn, const char * filepath) : CXCursor(conn, filepath) {}
			~CXURLCursor(void){}
			char * ProcessResponseHeader(CTCursor * cursor, char * buffer, unsigned long bufferLength);

		private:
	};
}

#endif