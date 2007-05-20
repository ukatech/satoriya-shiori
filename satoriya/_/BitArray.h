// BitArray  2000/06/21  v1.0

#include	"Assert.h"

class	BitArray {

	const int		mBits;		// 論理上の使用可能サイズ
	const int		mBytes;		// 確保してあるバイトサイズ
	unsigned char*	mBuffer;	// 確保してあるバッファ

public:
	BitArray( int iBits ) :
		mBits(iBits),
		mBytes((iBits+7)/8)	// 8bit align
	{
		ASSERT( iBits > 0 );
		mBuffer = new unsigned char[mBytes];
		memset( mBuffer, 0, mBytes );
	}
	~BitArray() {
		delete [] mBuffer;
	}
	
	operator unsigned char*() { return mBuffer; }
	int		BitSize() { return mBits; }
	int		ByteSize() { return mBytes; }

	void	Fill(bool f) { memset( mBuffer, f?0xff:0, mBytes );	}

	// 1bit単位でアクセス
	
	void	On( int n ) {
		ASSERT( n>=0 && n<mBits );
		mBuffer[n/8] |= (1<<(n%8));
	}
	void	Off( int n ) {
		ASSERT( n>=0 && n<mBits );
		mBuffer[n/8] &= ~(1<<(n%8));
	}
	void	Set( int n, int value ) {
		value ? On(n) : Off(n);
	}
	int		Get( int n ) {
		ASSERT( n>=0 && n<mBits );
		return	(( mBuffer[n/8] & (1<<(n%8)) ) != 0 );
	}

	// 指定範囲にアクセス
	
	void	Set( int iStart, int iBits, unsigned int iValue ) {
		const int theEnd = iStart+iBits-1;

		// ビット指定は範囲内か？
		ASSERT( iStart>=0 && iBits>=1 && iBits<=32 && theEnd<mBits ); 

		for ( int n=theEnd ; n>=iStart ; n-- ) {
			Set( n, iValue&1 );
			iValue >>= 1;
		}		
	}

	unsigned int Get( int iStart, int iBits ) {
		const int theEnd = iStart+iBits-1;

		 // ビット指定は範囲内か？
		ASSERT( iStart>=0 && iBits>=1 && iBits<=32 && theEnd<mBits );

		unsigned int theValue=0;
		for ( int n=iStart ; n<=theEnd ; n++ ) {
			theValue <<= 1;
			if ( Get(n) )
				theValue++;
		}
		return	theValue;
	}
};

