#ifndef	RANDOM_H
#define RANDOM_H
#include <stdlib.h>
void	randomize(void);
int		gen_random(void);
inline int random(int max) {
	if ( max <= 0 ) {
		return 0;
	}
	return static_cast<int>(gen_random()%max);
}

#endif	// RANDOM_H
