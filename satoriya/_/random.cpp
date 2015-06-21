
// óêêîê∂ê¨ïî
// ç°ÇÃÇ∆Ç±ÇÎMT

#include "mt19937ar.h"

#ifdef POSIX
#include <sys/time.h>
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
	struct timeval tv;
	gettimeofday(&tv,nullptr);
	dwSeed = tv.tv_usec;
#else
	dwSeed = ::GetTickCount();
#endif

	init_genrand(dwSeed);
}

int	gen_random(void) {
	return genrand_int31();
}
