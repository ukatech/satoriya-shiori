#include	"Base.h"
#include	<malloc.h>
#include	<memory.h>

#ifndef	_DEBUG // --------------------------------------------------------

void*	operator new( size_t iSize ) {
	void*	thePtr = malloc( iSize );
	if ( thePtr == NULL )
		throw Exception("メモリが足りません。");
	return	thePtr;
}

void	operator delete( void* iPtr ) {
	free( iPtr );
}

void	walk() {
}

#else	_DEBUG // --------------------------------------------------------

#include	<ctype.h>	// for isgraph

// 後述するテーブルの大きさ＝最大メモリブロック数
static const int gTableSize=1024*64;	// 64k * 4 * 3 = 768kb
// 確保したメモリブロックのアドレスを格納するテーブル
static	void*	gTable[gTableSize];	// 256kb
static	unsigned long	gBlockSize[gTableSize];	// 256kb
static	unsigned long	gAllocCount[gTableSize];	// 256kb
// ポインタに対するハッシュ関数
static unsigned short	PointerHash( void* iPtr ) {
//	return	(unsigned short)(  (((long)iPtr)>>16) ^ ((long)iPtr)  );
	return	(unsigned short)( ((unsigned long)iPtr)>>4 );
}
static int TableSearch( int iStart, void* iFindValue ) {
	for ( int i=iStart ; i<gTableSize ; i++ )
		if ( gTable[i] == iFindValue )
			return	i;
	for ( i=0 ; i<iStart ; i++ )
		if ( gTable[i] == iFindValue )
			return	i;
	return	-1;
}

void*	operator new( size_t iSize ) {
	static	int	theAllocCount=0;

	void*	thePtr = malloc( iSize );
	memset( thePtr, 0xac, iSize );	// 確保したメモリを埋めておく
	if ( thePtr == NULL )
		throw Exception("メモリが足りません。");

	// 格納位置を得る
	int	i = TableSearch( PointerHash(thePtr), NULL );
	if ( i == -1 )
		throw Exception("メモリブロック格納テーブルが一杯です。");
	// 確保したメモリのアドレスを保持しておく
	gTable[i] = thePtr;
	gBlockSize[i] = iSize;
	gAllocCount[i] = theAllocCount++;
	return	thePtr;
}

void	operator delete( void* iPtr ) {
	if ( iPtr == NULL )
		return;

	// 引数のメモリがテーブル内に存在するか？
	int	i = TableSearch( PointerHash(iPtr), iPtr );
	if ( i == -1 )
		throw Exception("確保された記録のないメモリを解放しようとしました。");
	// 解放処理
	free( gTable[i] );
	gTable[i] = NULL;
	gBlockSize[i] = 0;
}

#include	<stdio.h>
//void	walk( ostream& out ) {
void	walk() {
	printf("--------------------------\n");
	printf("No.   addres       size\n");
	printf("--------------------------\n");

	int	n=0;
	for ( int i=0 ; i<gTableSize ; i++ ) {
		if ( gTable[i] == NULL )
			continue;
		printf("%5d 0x%08p : ", gAllocCount[i], gTable[i] );
		/*if ( gBlockSize[i] > 10000 ) 
			printf( "%uK", gBlockSize[i]/1000 );
		else*/
		printf( "%u", gBlockSize[i] );

		/*printf( "   ");
		int	max = 8<gBlockSize[i] ? 8 : gBlockSize[i];
		for ( int j=0 ; j<max ; j++ )
			//if ( !isgraph( ((char*)gTable[i])[j]) )
				printf( "%c", ((char*)gTable[i])[j] );*/
		printf("\n");
		n++;
	}

	printf("--------------------------\n");
	printf("            %5d block[s]\n", n );
	printf("--------------------------\n");
}

#endif	// _DEBUG --------------------------------------------------------


