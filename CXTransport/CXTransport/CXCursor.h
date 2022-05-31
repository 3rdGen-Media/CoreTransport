#pragma once
#ifndef CXCURSOR_H
#define CXCURSOR_H

#include <map>
#include <functional>
#include <iostream>
//A Custom String class to provide char* buffer to rapidjson::writer
class CXRapidJsonString
{
public:
	typedef char Ch;
	CXRapidJsonString(char** buffer, unsigned long bufSize) : _buffer(buffer) { _bufferStart = *_buffer; _bufSize = bufSize; }
	void Put(char c) { **_buffer = c; (*_buffer)++; }
	void Clear() { memset(_bufferStart, 0, _bufSize); }
	void Flush() { return; }
	size_t Size() const { return *_buffer - _bufferStart; }
private:
	char* _bufferStart;
	char** _buffer;
	unsigned long _bufSize;
};

namespace CoreTransport
{
	typedef class CXConnection;



	class CXTRANSPORT_API CXCursor
	{
		public:
			//CXCursor();
			CXCursor(CXConnection* conn);
			CXCursor(CXConnection * conn, const char * filepath);

			~CXCursor(void);
			CTCursor * _cursor;
			//unsigned long _headerLength;
			//char	_requestBuffer[CX_REQUEST_BUFFER_SIZE];

			CXConnection* connection();

			virtual uint64_t Continue() { fprintf(stderr, "CXCursor::continue should be overidden!!!"); return 0;  }
			virtual char* ProcessResponseHeader(CTCursor * cursor, char * buffer, unsigned long bufferLength){fprintf(stderr, "CXCursor::ProcessResponseHeaders should be overidden!!!"); return NULL; }

			std::function<void(CTError* error, std::shared_ptr<CXCursor> cxCursor)> callback;
		protected:


		private:
			void createResponseBuffers(const char * filepath);

			CXConnection* _cxConnection;

	};
}

#endif