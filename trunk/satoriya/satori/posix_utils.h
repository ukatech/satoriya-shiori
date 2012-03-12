/*
  POSIX環境で必要になるマクロやインライン関数類。
*/
#ifndef _POSIX_UTILS_H_INCLUDED_
#define _POSIX_UTILS_H_INCLUDED_

#ifdef POSIX
#include <time.h>

inline time_t posix_get_current_sec() {
    struct timespec ts;
#ifdef CLOCK_UPTIME
	clock_gettime(CLOCK_UPTIME, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return ts.tv_sec;
}

#else  //POSIX

inline time_t posix_get_current_sec() {
    return ::GetTickCount() / 1000;
}

#endif //POSIX

#endif //_POSIX_UTILS_H_INCLUDED_
