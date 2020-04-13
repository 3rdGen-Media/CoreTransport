#include "stdafx.h"
#include "../ctReQL.h"

#ifdef _WIN32
#include <share.h>
#endif

REQL_API REQL_INLINE unsigned long ReqlPageSize()
{
#ifdef _WIN32
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);     // Initialize the structure.
	printf(TEXT("ReqlPageSize() = %d.\n"), systemInfo.dwPageSize);
	return (unsigned long)systemInfo.dwPageSize;
#elif defined(__APPLE__)

#endif
}

int ReqlFileOpen( const char * filepath)
{
#ifdef _WIN32
	int fileDescriptor = 0;
	_sopen_s(&fileDescriptor, filepath, O_RDONLY, _SH_DENYRW, _S_IREAD);
#else
	int fileDescriptor = open(filepath, O_RDONLY, 0700);
#endif
	//check if fd is valid
	if( fileDescriptor < 0 )
	{
		printf("\nUnable to open file: %s\n", filepath);
		printf("errno is %d and fd is %d", errno, fileDescriptor);
	}

	/*
	if( fcntl( fileDescriptor, F_NOCACHE ) < 0 )
	{

	}
	*/

	return fileDescriptor;
}

void ReqlFileClose(int fileDescriptor)
{
	_close(fileDescriptor);
	return;
}

off_t ReqlFileSize(int fileDescriptor)
{
	off_t filesize = 0;
	filesize = _lseek(fileDescriptor, 0, SEEK_END);
	_lseek(fileDescriptor, 0, SEEK_SET);
	//rewind(fileDescriptor);
	return filesize;
}