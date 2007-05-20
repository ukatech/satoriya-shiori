#ifndef	BASE_H
#define	BASE_H

#include	<stddef.h>
#include	<assert.h>
#include	"Exception.h"
//#include	"Memory.h"


void*	operator new( size_t );
void	operator delete( void* );
void	walk();
/*#ifdef	_OSTREAM_
void	walk( ostream& =cout );
#endif	_OSTREAM_*/

typedef	unsigned char	uchar;
typedef	unsigned short	ushort;
typedef	unsigned long	ulong;
typedef	unsigned int	uint;
typedef	char*	pchar;
typedef	short*	pshort;
typedef	long*	plong;
typedef	int*	pint;
typedef	signed char		int8;
typedef	signed short	int16;
typedef	signed long		int32;
typedef	signed __int64	int64;
typedef	unsigned char		uint8;
typedef	unsigned short		uint16;
typedef	unsigned long		uint32;
typedef	unsigned __int64	uint64;
typedef	uchar	byte;
typedef	ushort	word;
typedef	ulong	dword;
typedef	byte*	pbyte;
typedef	word*	pword;
typedef	dword*	pdword;

/*
void	Report( const char* format, ... );
void	MesBox( const char* format, ... );
void	DbgStr( const char* format, ... );
void	WinText( const char* format, ... );
void	Aleat( const char* format, ... );
void	Error( const char* format, ... );
void	FatalError( const char* format, ... );
*/

#endif	//	BASE_H
