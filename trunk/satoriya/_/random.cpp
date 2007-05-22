
// —”¶¬•”
// ¡‚Ì‚Æ‚±‚ëMT

#include "mt19937ar.h"

void randomize(unsigned int dwSeed)
{
	init_genrand(dwSeed);
}

int	random(void) {
	return genrand_int31();
}
