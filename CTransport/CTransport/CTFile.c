//
//  cr_file.c
//  CRViewer
//
//  Created by Joe Moulton on 2/9/19.
//  Copyright � 2019 Abstract Embedded. All rights reserved.
//

#include "../CTransport.h"

#ifndef _WIN32
#include <assert.h>
#endif

CTRANSPORT_API CTRANSPORT_INLINE unsigned long ct_system_allocation_granularity()
{
#ifdef _WIN32
	SYSTEM_INFO  systemInfo;
	GetSystemInfo(&systemInfo);     // Initialize the structure.
	//printf (TEXT("This computer has allocation granularity %d.\n"), systemInfo.dwAllocationGranularity);
	return (unsigned long)systemInfo.dwAllocationGranularity/2;
#else
	return (unsigned long)(4096UL * 16UL) / 2UL;
#endif
}

CTRANSPORT_API CTRANSPORT_INLINE const char* ct_file_name(char *filepath)
{
	//char * filename;
	char *slash = filepath, *next;
	//iterate through all the backslash or forward slashes in the string
	//to find the last one 
    while ((next = strpbrk(slash + 1, "\\/"))) slash = next;
    //advance beyond the final slash by one index
	if (filepath != slash) slash++;
	//strncpy(*p, (const char*)pf, strlen(pf));//strdup(pf, slash - pf);
    //filename = slash;//strdup(slash);  //duplicate the substring in pf starting at index slash

	return slash;

}


CTRANSPORT_API CTRANSPORT_INLINE const char* ct_file_extension(const char *fileNameOrPath)
{
    const char *dot = strrchr(fileNameOrPath, '.');
    if(!dot || dot == fileNameOrPath) return "";
    return dot + 1;
}


CTRANSPORT_API CTRANSPORT_INLINE int ct_file_open(const char * filepath)
{
    int fileDescriptor = open(filepath, O_RDONLY, 0700);
    
    //check if the file descriptor is valid
    if (fileDescriptor < 0) {
        
        printf("\nUnable to open file: %s\n", filepath);
        printf("errno is %d and fd is %d",errno,fileDescriptor);
        
    }
    
    //printf("\nfile size = %lld\n", filesize);
    
    //disabling disk caching will allow zero performance reads
    //According to John Carmack:  I am pleased to report that fcntl( fd, F_NOCACHE ) works exactly as desired on iOS � I always worry about behavior of historic unix flags on Apple OSs. Using
    //this and page aligned target memory will bypass the file cache and give very repeatable performance ranging from the page fault bandwidth with 32k reads up to 30 mb/s for one meg reads (22
    //mb/s for the old iPod). This is fractionally faster than straight reads due to the zero copy, but the important point is that it won�t evict any other buffer data that may have better
    //temporal locality.
    
#ifdef __APPLE__
    //this takes care preventing caching, but we also need to ensure that "target memory" is page aligned
    if ( fcntl( fileDescriptor, F_NOCACHE ) < 0 )
    {
        printf("\nF_NOCACHE failed!\n");
    }
#endif
    return fileDescriptor;

}

CTRANSPORT_API CTRANSPORT_INLINE int ct_file_create_w(char * filepath)
{
    int fileDescriptor;// = open(filepath, O_RDWR | O_APPEND | O_CREAT | O_TRUNC | O_TEXT | O_SEQUENTIAL, 0700);
#ifdef _WIN32

	HANDLE fh;
	DWORD dwErr;

	printf("ct_file_create_w::filepath = %s\n", filepath);
	fh = CreateFileA(filepath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if( fh == INVALID_HANDLE_VALUE )
	{
        printf("\ncr_file_create_w::CreateFileA failed: %s\n", filepath);
		
		dwErr = GetLastError();
		if (dwErr > 0) {
			printf("ncr_file_create_w::CreateFileA Error Code: %lu\n", dwErr);
			fileDescriptor = (int)dwErr;
		}
		
	}
	
	//asociate int fd with WIN32 file HANDLE
	fileDescriptor = _open_osfhandle((intptr_t)fh, O_RDWR | O_APPEND );

	//check if the file descriptor is valid
    if (fileDescriptor < 0) {
        
        printf("\nUnable to open osfhandle: %s\n", filepath);
        printf("errno is %d and fd is %d",errno,fileDescriptor);
		fileDescriptor = errno;
        
    }
#else
	fileDescriptor = open(filepath, O_RDWR | O_APPEND | O_CREAT | O_TRUNC | O_TEXT /*| O_SEQUENTIAL*/, 0700);
	//check if the file descriptor is valid
	if (fileDescriptor < 0) {

		printf("\nUnable to open file: %s\n", filepath);
		printf("errno is %d and fd is %d", errno, fileDescriptor);
		fileDescriptor = errno;
	}

	//printf("\nfile size = %lld\n", filesize);

	//disabling disk caching will allow zero performance reads
	//According to John Carmack:  I am pleased to report that fcntl( fd, F_NOCACHE ) works exactly as desired on iOS � I always worry about behavior of historic unix flags on Apple OSs. Using
	//this and page aligned target memory will bypass the file cache and give very repeatable performance ranging from the page fault bandwidth with 32k reads up to 30 mb/s for one meg reads (22
	//mb/s for the old iPod). This is fractionally faster than straight reads due to the zero copy, but the important point is that it won�t evict any other buffer data that may have better
	//temporal locality.

	//this takes care preventing caching, but we also need to ensure that "target memory" is page aligned
	//if (fcntl(fileDescriptor, F_NOCACHE) < 0)
	//{
	//	printf("\nF_NOCACHE failed!\n");
	//}
#endif
    return fileDescriptor;

}


CTRANSPORT_API CTRANSPORT_INLINE off_t ct_file_truncate(int fd, off_t size)
{
	off_t outSz = size;
#ifdef _WIN32
	DWORD dwErr = 0;
	HANDLE hFile = (HANDLE)_get_osfhandle( fd );
	SetFilePointer(hFile, size, 0, FILE_BEGIN);
	SetEndOfFile(hFile);

	dwErr = GetLastError();
	if (dwErr > 0) {
		printf("ct_file_truncate::Error Code: %lu\n", dwErr );
		outSz = (off_t)dwErr;
	}
#else
	
#endif
	return outSz;
}


CTRANSPORT_API CTRANSPORT_INLINE void ct_file_unmap(int fileDescriptor, char * fbuffer)
{
#ifdef _WIN32
	//check if this is a memory mapped file
	HANDLE mFile;
	char handleStr[sizeof(HANDLE)+1] = "\0";
	HANDLE hFile = (HANDLE)_get_osfhandle( fileDescriptor );
	memcpy(handleStr, &(hFile), sizeof(HANDLE));
	handleStr[sizeof(HANDLE)] = '\0';

	//First. assume the file has been mapped for reading and writing
	mFile = OpenFileMapping( FILE_MAP_ALL_ACCESS, 0, handleStr );
	if( mFile != INVALID_HANDLE_VALUE )
	{
		//Unmap the file backing buffer
		if (fbuffer)
		  UnmapViewOfFile(fbuffer);

		//close the file mapping
		CloseHandle(mFile);
	}
	else //otherwise just assume it has been mapped for reading
	{
		mFile = OpenFileMapping( FILE_MAP_READ, 0, handleStr );
		if( mFile != INVALID_HANDLE_VALUE)
		{
			//Unmap the file backing buffer
			if (fbuffer)
			  UnmapViewOfFile(fbuffer);

			//close the file mapping
			CloseHandle(mFile);
		}
	}
#else

#endif
    //close(fileDescriptor);
    return;
}


CTRANSPORT_API CTRANSPORT_INLINE void ct_file_close(int fileDescriptor)
{
#ifdef _WIN32
	//check if this is a memory mapped file
	HANDLE hFile = (HANDLE)_get_osfhandle( fileDescriptor );
	if( hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	else //we didn't create the file with a WIN32 HANDLE, just a descriptor
#else

#endif
		close(fileDescriptor);
    return;
}

CTRANSPORT_API CTRANSPORT_INLINE off_t ct_file_size(int fileDescriptor)
{
    off_t filesize = 0;
    filesize =lseek(fileDescriptor, 0, SEEK_END);
    lseek(fileDescriptor, 0, SEEK_SET);
    return filesize;
}

CTRANSPORT_API CTRANSPORT_INLINE void ct_file_allocate_storage(int fileDescriptor, off_t size)
{
    //first we need to preallocate a contiguous range of memory for the file
    //if this fails we won't have enough space to reliably map a file for zero copy lookup
#ifdef __APPLE__
    fstore_t fst;
    fst.fst_flags = F_ALLOCATECONTIG;
    fst.fst_posmode = F_PEOFPOSMODE;
    fst.fst_offset = 0;
    fst.fst_length = size;
    fst.fst_bytesalloc = 0;
    
    if( fcntl( fileDescriptor, F_ALLOCATECONTIG, &fst ) < 0 )
    {
        printf("F_PREALLOCATED failed!");
    }
#else
	assert(1==0);
#endif
    return;
}



//to do change file size to an unsigned long long specifier
CTRANSPORT_API CTRANSPORT_INLINE void*  ct_file_map_to_buffer( char ** buffer, off_t filesize, unsigned long filePrivelege, unsigned long mapOptions, int fileDescriptor, off_t offset)
{
#ifdef _WIN32

	HANDLE mFile; //mapped file handle
	DWORD dwMaximumSizeHigh, dwMaximumSizeLow;


	char handleStr[sizeof(HANDLE)+1] = "\0";
	HANDLE hFile = (HANDLE)_get_osfhandle( fileDescriptor );
	
	memcpy(handleStr, &(hFile), sizeof(HANDLE));
	handleStr[sizeof(HANDLE)] = '\0';
	
	dwMaximumSizeHigh = (unsigned long long)filesize >> 32;
	dwMaximumSizeLow = filesize & 0xffffffffu;
	// try to allocate and map our space
    if ( !(mFile = CreateFileMappingA(hFile, NULL, filePrivelege, dwMaximumSizeHigh, dwMaximumSizeLow, handleStr))  )//||
          //!MapViewOfFileEx(buffer->mapping, FILE_MAP_ALL_ACCESS, 0, 0, ring_size, (char *)desired_addr + ring_size))
    {
		// something went wrong - clean up
		printf("cr_file_map_to_buffer failed:  OS Virtual Mapping failed");
		if( !(*buffer = (char *)MapViewOfFileEx(mFile, FILE_MAP_ALL_ACCESS, 0, 0, filesize, NULL)) )
		{
			fprintf(stderr, "cr_file_map_to_buffer failed:  OS Virtual Mapping failed2 ");
	    }

    }
    else // success!
	{
	  printf("cr_file_map_to_buffer Success:  OS Virtual Mapping succeeded");
	}
    return (void*)*buffer;

#else
    return (void*)mmap(*buffer, (size_t)(filesize), filePrivelege, mapOptions, fileDescriptor, offset);
#endif
}

#ifdef __WIN32
CTRANSPORT_API CTRANSPORT_INLINE uint64_t ct_system_utc()
{
	const uint64_t OA_ZERO_TICKS = 94353120000000000; //12/30/1899 12:00am in ticks
	//const uint64_t TICKS_PER_DAY = 864000000000;      //ticks per day

	FILETIME ft = { 0 };
	GetSystemTimePreciseAsFileTime(&ft);

	ULARGE_INTEGER dt; //needed to avoid alignment faults
	dt.LowPart = ft.dwLowDateTime;
	dt.HighPart = ft.dwHighDateTime;

	return (dt.QuadPart - OA_ZERO_TICKS);
}
#endif

