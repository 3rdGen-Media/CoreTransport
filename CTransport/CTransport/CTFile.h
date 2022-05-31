//
//  ct_file.h
//  CTransort
//
//  Created by Joe Moulton on 2/9/19.
//  Copyright © 2019 Abstract Embedded. All rights reserved.
//

#ifndef CTFILE_H
#define CTFILE_H


//mmap
#include <stdint.h>
#include <string.h>
#include <stdlib.h> //malloc
#include <stdio.h>
#include <errno.h>
//#include <unistd.h>
//#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef _WIN32
//darwin system file open
#include <sys/mman.h>
#include <sys/fcntl.h>
//#include <sys/fadvise.h>
#include <unistd.h>
#else
#include "CTSystem.h"
//#include <winnt.h>
#include <io.h>
#endif

//lseeko

#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64 //define off_t to be 64 bits
#include <sys/types.h>

#include <errno.h>


#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef O_TEXT
#define O_TEXT 0
#endif



#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

//on Darwin file I/O is 64 bit by default
#ifdef __APPLE__
#  define off64_t off_t
#  define fopen64 fopen
#endif

#ifdef _WIN32
#define open _open

#ifndef PROT_READ
#define PROT_READ PAGE_READONLY
#define PROT_WRITE PAGE_READWRITE
#define PROT_READWRITE PAGE_READWRITE
#endif


#ifndef MAP_SHARED
#define MAP_SHARED 0
#define MAP_NORESERVE 0
#endif

#endif

typedef struct CTFile
{
	char *  buffer;
	unsigned long size;
	int fd;
#ifdef _WIN32
	HANDLE  hFile;
	HANDLE  mFile;
#endif
	char * path;
}CTFile;


// to do move this to ct_system.h,c
CTRANSPORT_API CTRANSPORT_INLINE unsigned long ct_system_allocation_granularity();

CTRANSPORT_API CTRANSPORT_INLINE const char* ct_file_name(char *filepath);
CTRANSPORT_API CTRANSPORT_INLINE const char* ct_file_extension(const char *fileNameOrPath); 

//open file for mmap reading
CTRANSPORT_API CTRANSPORT_INLINE int ct_file_open(const char * filepath);

//ope file for mmap writing
CTRANSPORT_API CTRANSPORT_INLINE int ct_file_create_w(const char * filepath);
CTRANSPORT_API CTRANSPORT_INLINE off_t ct_file_truncate(int fd, off_t size);

CTRANSPORT_API CTRANSPORT_INLINE void ct_file_close(int fileDescriptor);
CTRANSPORT_API CTRANSPORT_INLINE off_t ct_file_size(int fileDescriptor);
CTRANSPORT_API CTRANSPORT_INLINE void ct_file_allocate_storage(int fileDescriptor, off_t size);
CTRANSPORT_API CTRANSPORT_INLINE void*  ct_file_map_to_buffer( char ** buffer, off_t filesize, unsigned long filePrivelege, unsigned long mapOptions, int fileDescriptor, off_t offset);
CTRANSPORT_API CTRANSPORT_INLINE void ct_file_unmap(int fileDescriptor, char * fbuffer);

//Win32 UTC .1 microsecond precision
CTRANSPORT_API CTRANSPORT_INLINE uint64_t ct_system_utc();

#endif /* cr_file_h */
