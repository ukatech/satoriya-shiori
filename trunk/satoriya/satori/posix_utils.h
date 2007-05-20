/*
  POSIX環境で必要になるマクロやインライン関数類。
*/
#if !defined(_POSIX_UTILS_H_INCLUDED_) && defined(POSIX)
#define _POSIX_UTILS_H_INCLUDED_
#include <sys/time.h>

inline long posix_get_current_millis() {
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    // tv_usecはエポックからのマイクロ秒では*ない*!
    // tv_secからの追加マイクロ秒である。
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

#endif
