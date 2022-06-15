#ifndef CTCOROUTINE_H
#define CTCOROUTINE_H

#ifndef _WIN32
#include "dill/libdill.h"
//dill coroutine handles are actually coroutines bundles that are store as ints
#define CTRoutine int
#define CTRoutineClose hclose
#else
//map coroutine to an empty macro on platforms that don't support it
#ifndef coroutine
#define coroutine
#define go(routine) routine
#define yield()
#define CTRoutine int
#define CTRoutineClose
#endif
#endif


#endif