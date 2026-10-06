
// 乱数生成部
// 今のところMT

#include "mt19937ar.h"

#ifdef POSIX
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

void randomize(void)
{
	unsigned int dwSeed;

#ifdef POSIX
	// tv_usec だけだと 0～999999 の100万通りしかないので、秒・単調な時計・プロセスIDも混ぜる
	struct timeval tv;
	gettimeofday(&tv,nullptr);
	dwSeed = static_cast<unsigned int>(tv.tv_sec) * 1000003U ^ static_cast<unsigned int>(tv.tv_usec);

	struct timespec ts;
	if ( clock_gettime(CLOCK_MONOTONIC, &ts) == 0 ) {
		dwSeed ^= static_cast<unsigned int>(ts.tv_nsec) * 2654435761U;
		dwSeed ^= static_cast<unsigned int>(ts.tv_sec) << 8;
	}
	dwSeed ^= static_cast<unsigned int>(getpid()) << 16;
#else
	dwSeed = ::GetTickCount();
#endif

	init_genrand(dwSeed);
}

int	gen_random(void) {
	return genrand_int31();
}
