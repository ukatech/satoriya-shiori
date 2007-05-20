#ifndef	MEMORY_H
#define	MEMORY_H

class Memory {
public:
	Memory( size_t=0 );
	~Memory();
	void*	Alloc( size_t );
	void	Free();
	operator unsigned char*();
	operator char*();
	operator void*();
	void*	Ptr();
	size_t	Size();
private:
	unsigned char*	mPtr;
	size_t	mSize;
};

#endif	// MEMORY_H
