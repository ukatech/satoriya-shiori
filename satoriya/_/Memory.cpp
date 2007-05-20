#include	<assert.h>
#include	<stdio.h>	// for NULL
#include	"Memory.h"

Memory::Memory( size_t iSize ) : 
	mPtr(NULL),
	mSize(0)
{
	assert(this != NULL);
	if ( iSize > 0 )
		Alloc(iSize);
}
Memory::~Memory() {
	assert(this != NULL);
	Free();
}
void*	Memory::Alloc( size_t iSize ) { 
	assert(this != NULL);
	Free();
	mPtr = new unsigned char[iSize];
	if ( mPtr != NULL )
		mSize = iSize;
	return	mPtr;
}
void	Memory::Free() { 
	assert(this != NULL);
	delete [] mPtr; 
	mPtr = NULL;
	mSize = 0;
}
Memory::operator unsigned char*() {
	assert(this != NULL);
	return mPtr; 
}
Memory::operator char*() {
	assert(this != NULL);
	return (char*)mPtr; 
}
Memory::operator void*() {
	assert(this != NULL);
	return mPtr; 
}
void*	Memory::Ptr() { 
	assert(this != NULL);
	return mPtr; 
}
size_t	Memory::Size() {
	assert(this != NULL);
	return mSize; 
}
