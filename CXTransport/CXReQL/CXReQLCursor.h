#pragma once
#ifndef CXREQLCURSOR_H
#define CXREQLCURSOR_H


namespace CoreTransport
{
	class CXREQL_API CXReQLCursor : public CXCursor
	{
		public:
			CXReQLCursor(CTConnection * conn) : CXCursor(conn) {}
			CXReQLCursor(CTConnection * conn, const char * filepath) : CXCursor(conn, filepath) {}
			~CXReQLCursor(void){}
			char * ProcessResponseHeader(char * buffer, unsigned long bufferLength);

		private:
	};
}

#endif