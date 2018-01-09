/*
 * 2017-10-30
 * mem structure def of windows
 */

#include "qemu/osdep.h"

#ifndef _DWORD
#define _DWORD
typedef uint32_t DWORD;
#endif

#ifndef _DWORDLONG
#define _DWORDLONG
typedef uint64_t DWORDLONG;
#endif

#ifndef _WSIZE_T
#define _WSIZE_T
typedef uint32_t SIZE_T;
#endif

typedef struct MemoryStatus MemoryStatus;
typedef struct MemoryStatusEx MemoryStatusEx;

struct MemoryStatus {
    DWORD  dwLength;
    DWORD  dwMemoryLoad;
    SIZE_T dwTotalPhys;
    SIZE_T dwAvailPhys;
    SIZE_T dwTotalPageFile;
    SIZE_T dwAvailPageFile;
    SIZE_T dwTotalVirtual;
    SIZE_T dwAvailVirtual;
};

struct MemoryStatusEx {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORDLONG ullTotalPhys;
    DWORDLONG ullAvailPhys;
    DWORDLONG ullTotalPageFile;
    DWORDLONG ullAvailPageFile;
    DWORDLONG ullTotalVirtual;
    DWORDLONG ullAvailVirtual;
    DWORDLONG ullAvailExtendedVirtual;
};
