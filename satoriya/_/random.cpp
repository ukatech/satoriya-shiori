
// —”¶¬•”
// ¡‚Ì‚Æ‚±‚ëMT

#include "mt19937ar.h"

#ifdef POSIX
#include <sys/time.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

void randomize(void)
{
	unsigned int dwSeed;

#ifdef POSIX
	struct timeval tv;
	gettimeofday(&tv,NULL);
	dwSeed = tv.tv_usec;
#else
	dwSeed = ::GetTickCount();
#endif

	init_genrand(dwSeed);
}

int	gen_random(void) {
	return genrand_int31();
}
