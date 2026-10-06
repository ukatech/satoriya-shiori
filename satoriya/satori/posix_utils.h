/*
  POSIX環境で必要になるマクロやインライン関数類。
*/
#ifndef _POSIX_UTILS_H_INCLUDED_
#define _POSIX_UTILS_H_INCLUDED_

#ifdef POSIX
#include <time.h>
#include <sys/time.h>

// 経過ミリ秒（GetTickCount 相当）。壁時計は時刻合わせで戻ることがあるので、単調な時計を使う。
// 差を取って使うこと（符号なしの引き算なので、一周しても差は合う）
inline unsigned long posix_get_current_tick() {
	struct timespec ts;
	if ( clock_gettime(CLOCK_MONOTONIC, &ts) == 0 ) {
		return static_cast<unsigned long>(ts.tv_sec) * 1000UL + static_cast<unsigned long>(ts.tv_nsec / 1000000);
	}
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return static_cast<unsigned long>(tv.tv_sec) * 1000UL + static_cast<unsigned long>(tv.tv_usec / 1000);
}

#else  //POSIX

inline unsigned long posix_get_current_tick() {
    return ::GetTickCount();
}

#endif //POSIX

// OS を起動してからの経過秒（Windows の GetTickCount64 / 1000 と同じ意味）
inline unsigned long posix_get_current_sec() {
#ifdef POSIX
	struct timespec ts;
#ifdef CLOCK_BOOTTIME
	// Linux。サスペンド中も数える
	if ( clock_gettime(CLOCK_BOOTTIME, &ts) == 0 ) {
		return static_cast<unsigned long>(ts.tv_sec);
	}
#endif
	// macOS などは CLOCK_MONOTONIC が起動からの時間
	if ( clock_gettime(CLOCK_MONOTONIC, &ts) == 0 ) {
		return static_cast<unsigned long>(ts.tv_sec);
	}
	return static_cast<unsigned long>(time(NULL));	// どちらも使えない環境だけ。起動時間としては正しくない
#else  //POSIX
	typedef unsigned __int64 (WINAPI *DefGetTickCount64)();

	static const DefGetTickCount64 pGetTickCount64 = (DefGetTickCount64)::GetProcAddress(::GetModuleHandle(L"kernel32"),"GetTickCount64");

	if ( pGetTickCount64 ) {
		return (unsigned long)(pGetTickCount64() / 1000);
	}
	else {
		return ::GetTickCount() / 1000;
	}
#endif //POSIX
}

#endif //_POSIX_UTILS_H_INCLUDED_
