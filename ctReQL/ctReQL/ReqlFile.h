#ifndef ReqlFile_h
#define ReqlFile_h

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <stdio.h>
#include <errno.h>

//Utilities (between Connection and Send/Recv)
REQL_API REQL_INLINE unsigned long ReqlPageSize();

#endif