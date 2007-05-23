#ifndef	RANDOM_H
#define RANDOM_H
#include <stdlib.h>
//void	randomize(unsigned int seed);
//int		random(void);
inline int random(int max) {
	if ( max <= 0 ) {
		return 0;
	}
	return static_cast<int>(random()%max);
}

#endif	// RANDOM_H
