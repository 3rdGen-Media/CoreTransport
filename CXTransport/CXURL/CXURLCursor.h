#pragma once
#ifndef CXURLCURSOR_H
#define CXURLCURSOR_H


namespace CoreTransport
{
	class CXURL_API CXURLCursor : public CXCursor
	{
		public:
			//CXCursor();
			CXURLCursor(CTConnection * conn) : CXCursor(conn) {}
			CXURLCursor(CTConnection * conn, const char * filepath) : CXCursor(conn, filepath) {}
			~CXURLCursor(void){}
			char * ProcessResponseHeader(char * buffer, unsigned long bufferLength);

		private:
	};
}

#endif