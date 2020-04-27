#pragma once
#ifndef CXCURSOR_H
#define CXCURSOR_H


namespace CoreTransport
{
	class CXTRANSPORT_API CXCursor
	{
		public:
			//CXCursor();
			CXCursor(CTConnection * conn);
			CXCursor(CTConnection * conn, const char * filepath);

			~CXCursor(void);
			CTCursor _cursor;
			//unsigned long _headerLength;
			//char	_requestBuffer[CX_REQUEST_BUFFER_SIZE];

			virtual char* ProcessResponseHeader(char * buffer, unsigned long bufferLength){printf("CXCursor::ProcessResponseHeaders should be overidden!!!"); return NULL; }

		protected:


		private:
			void createResponseBuffers(const char * filepath);


	};
}

#endif