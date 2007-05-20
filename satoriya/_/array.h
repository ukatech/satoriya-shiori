#ifndef	ARRAY_H
#define	ARRAY_H

#if 0  // --------------------------------------------------------

	固定長配列コンテナ array<class T>

	2000/08　しょはん
	2001/02　だい２はん

#endif // --------------------------------------------------------

#include	<assert.h>

template<class T>
class array {
public:
	// 要素数を指定して配列を作成
	array(int iSize=0);
	// 配列を破棄
	~array();
	// コピーコンストラクタ
	array( const array& iArray );
	// 代入演算子
	array&	operator = ( const array& iArray );	
	// 配列を伸縮
	void	resize(int);
	
	// 配列が正しく確保されたか？
	operator bool() const { return (mArray != NULL); }

	// 要素の参照
	T&	operator[]( int iPosition ) const {
		assert(iPosition >= 0);
		assert(iPosition < size());
		return	mArray[iPosition];
	}
	// 要素を検索、添え字を返す。見つからなければ -1。
	int	find( const T& iT, int iStartPosition=0 );

	// 要素数を取得
	int		size() const { return mSize; }
	operator int() const { return size(); }
	bool 	empty() const { return size() == 0; }

	// 配列をクリア
	void	clear() { resize(0); }

	// 各要素に引数値を代入
	void	fill( const T& iT ) {
		for ( int i=0 ; i<mSize ; i++ )
			(*this)[i] = iT;
	}

	// 各要素を関数に渡す。
	// 関数は Function( T& ); で呼び出される。
	// 返値は利用されない。
	template<class Function>
	void	each( Function iFunction ) {
		for ( int i=0 ; i<mSize ; i++ )
			iFunction( (*this)[i] );
	}

private:
	T*	mArray;
	int	mSize;
};

#ifdef	_OSTREAM_
template<class T>
inline ostream& operator<<( ostream& stream, const array<T>& ary ) {
	for ( int i=0 ; i<ary.size() ; i++ ) {
		stream << (ary[i]);
		if ( i < ary.size()-1 )
			stream << ", ";
	}
	return	stream;
}
#endif

// --------------------------------------------------------

// 要素数を指定して配列を作成
template<class T>
array<T>::array<T>( int iSize ) : mSize(iSize) {
	assert(iSize >= 0);
	mArray = new T[mSize];
}

// 配列を破棄
template<class T>
array<T>::~array() {
	delete [] mArray;
}

// コピーコンストラクタ
template<class T>
array<T>::array<T>( const array<T>& iArray ) : mSize(iArray.mSize) {
	mArray = new T[mSize];
	for ( int i=0 ; i<mSize ; i++ )
		mArray[i] = iArray[i];
}

// 代入演算子
template<class T>
array<T>&	array<T>::operator = ( const array<T>& iArray ) {
	if ( &iArray == this )
		return	*this;
	mSize = iArray.mSize;
	delete [] mArray;
	mArray = new T[mSize];
	for ( int i=0 ; i<mSize ; i++ )
		mArray[i] = iArray[i];
	return	*this;
}

// 配列長の伸縮
template<class T>
void	array<T>::resize( int iSize ) {
	T*	thearray = new T[iSize];
	int	theCopyLen = ( mSize > iSize ) ? iSize : mSize;
	for ( int i=0 ; i<theCopyLen ; i++ )
		thearray[i] = mArray[i];
	delete [] mArray;
	mArray = thearray;
	mSize = iSize;
}

// 要素を検索し、引数と最初に一致した時点での添え字を返す。見つからなければ -1。
template<class T>
int	array<T>::find( const T& iT, int iStartPosition ) {
	assert(iStartPosition < mSize);
	for ( int i=iStartPosition ; i<mSize ; i++ )
		if ( mArray[i] == iT )
			return	i;
	return	-1;
}


#endif	//	ARRAY_H
