#pragma once
#ifndef CXREQLCURSOR_H
#define CXREQLCURSOR_H


namespace CoreTransport
{
	class CXREQL_API CXReQLCursor : public CXCursor
	{
		public:
			CXReQLCursor(CXConnection * conn) : CXCursor(conn) {}
			CXReQLCursor(CXConnection * conn, const char * filepath) : CXCursor(conn, filepath) {}
			~CXReQLCursor(void){}

			uint64_t Continue();
			char * ProcessResponseHeader(CTCursor * cursor, char * buffer, unsigned long bufferLength);

		private:
	};
}

#endif