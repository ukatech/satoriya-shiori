/*
  POSIX環境で必要になるマクロやインライン関数類。
*/
#ifndef _POSIX_UTILS_H_INCLUDED_
#define _POSIX_UTILS_H_INCLUDED_

#ifdef POSIX
#include <time.h>

inline time_t posix_get_current_tick() {
    struct timespec ts;
#ifdef CLOCK_UPTIME
	clock_gettime(CLOCK_UPTIME, &ts);
#else
	clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
	return (ts.tv_sec * 1000) + (ts.tv_nsec/1000/1000);
}

#else  //POSIX

inline time_t posix_get_current_tick() {
    return ::GetTickCount();
}

#endif //POSIX

inline time_t posix_get_current_sec() {
    return posix_get_current_tick() / 1000;
}

#endif //_POSIX_UTILS_H_INCLUDED_
