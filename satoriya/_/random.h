#ifndef	RANDOM_H
#define RANDOM_H

void	randomize(unsigned int seed);
int		random();
inline int random(int max) {
	if ( max <= 0 ) {
		return 0;
	}
	return random()%max;
}

#endif	// RANDOM_H
