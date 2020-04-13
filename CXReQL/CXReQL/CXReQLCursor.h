#pragma once
#ifndef CXREQL_CURSOR_H
#define CXREQL_CURSOR_H

namespace CoreTransport
{
	class CXReQLCursor
	{
	public:
		CXReQLCursor(ReqlCursor cursor);
		~CXReQLCursor(void);

		void continueQuery(void * options);
		ReqlCursor _cursor;

	
	protected:


	private:
	};



}

#endif