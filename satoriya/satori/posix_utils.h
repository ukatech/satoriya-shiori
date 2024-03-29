/*
  POSIX環境で必要になるマクロやインライン関数類。
*/
#ifndef _POSIX_UTILS_H_INCLUDED_
#define _POSIX_UTILS_H_INCLUDED_

#ifdef POSIX
#include <time.h>
#include <sys/time.h>

inline unsigned long posix_get_current_tick() {
	struct timeval ts;
//	struct timespec {time_t tv_sec; long tv_nsec;} ts;
//    struct timespec ts;
#ifdef CLOCK_UPTIME
	clock_gettime(CLOCK_UPTIME, &ts);
#else
	gettimeofday(&ts, NULL);
//	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return (ts.tv_sec * 1000) + (ts.tv_usec/1000/1000);
//	return (ts.tv_sec * 1000) + (ts.tv_nsec/1000/1000);
}

#else  //POSIX

inline unsigned long posix_get_current_tick() {
    return ::GetTickCount();
}

#endif //POSIX

inline unsigned long posix_get_current_sec() {
#ifdef POSIX
	struct timeval ts;
//	struct timespec {time_t tv_sec; long tv_nsec;} ts;
//    struct timespec ts;
#ifdef CLOCK_UPTIME
	clock_gettime(CLOCK_UPTIME, &ts);
#else
	gettimeofday(&ts, NULL);
//	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return ts.tv_sec;
#else  //POSIX
	typedef unsigned __int64 (WINAPI *DefGetTickCount64)();

	static const DefGetTickCount64 pGetTickCount64 = (DefGetTickCount64)::GetProcAddress(::GetModuleHandleA("kernel32"),"GetTickCount64");

	if ( pGetTickCount64 ) {
		return (unsigned long)(pGetTickCount64() / 1000);
	}
	else {
		return ::GetTickCount() / 1000;
	}
#endif //POSIX
}

#endif //_POSIX_UTILS_H_INCLUDED_
