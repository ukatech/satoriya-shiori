#include	"FileLister.h"
#include	<assert.h>

//------------------------------------------------------------------------
FileLister::FileLister() {
	::ZeroMemory( &mPath, sizeof(mPath) );
	mHandle = INVALID_HANDLE_VALUE;
	::ZeroMemory( &mData, sizeof(mData) );
};
//------------------------------------------------------------------------
FileLister::~FileLister() {
	Close();
}

//------------------------------------------------------------------------
// 列挙パスを指定。ワイルドカード使用可。
BOOL	FileLister::Open( LPCSTR iFindPath ) {
	assert(iFindPath != NULL);
	Close();
	::lstrcpy( mPath, iFindPath );
	return	TRUE;
}

//------------------------------------------------------------------------
// 次のファイルを列挙。
BOOL	FileLister::Next() {

	if ( mPath[0] == '\0' )
		return	FALSE;

	if ( mHandle == INVALID_HANDLE_VALUE ) {
		mHandle = ::FindFirstFile( mPath, &mData );
		if ( mHandle == INVALID_HANDLE_VALUE )
			return	FALSE;
	}
	else {
		if ( !::FindNextFile( mHandle, &mData ) ) {
			mHandle = INVALID_HANDLE_VALUE;
			return	FALSE;
		}
	}
	return	TRUE;
}

//------------------------------------------------------------------------
// 現在のファイル名を取得。
LPCSTR	FileLister::Name() {
	return	mData.cFileName;
}

//------------------------------------------------------------------------
// 現在のファイルの詳細データを取得。
const WIN32_FIND_DATA*	FileLister::Data() {
	return	&mData;
}

//------------------------------------------------------------------------
// 現在のファイルはフォルダか？
BOOL	FileLister::isFolder() {
	return ( mData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY );
}

//------------------------------------------------------------------------
// 現在のファイル名は . のみで構成されている？
BOOL	FileLister::isDots() {
	char*	p = mData.cFileName;
	if (*p != '.') 
		return FALSE;
	for ( ; *p != '\0' ; p++ )
		if ( *p != '.' )
			return	FALSE;
	return	TRUE;
}

//------------------------------------------------------------------------
// 指定の拡張子か？
BOOL	FileLister::isExt( LPCSTR iExtention ) {
	assert(iExtention != NULL);

	int	len = strlen(mData.cFileName);
	char*	p = mData.cFileName;
	for ( int i=len-1 ; i>=0 ; i-- )
		if ( p[i] == '.' )
			break;

	if ( i>=0 )
		return ( stricmp( iExtention, p+i+1 ) == 0 );// 見つけたとき
	else
		return ( iExtention[0] == '\0' );			// 無かったとき
}

//------------------------------------------------------------------------
// 列挙の終了処理を行う。
void	FileLister::Close() {
	if ( mHandle != INVALID_HANDLE_VALUE ) {
		::FindClose(mHandle);
		mHandle=INVALID_HANDLE_VALUE;
	}
}

/*




		void	FindPath( LPCSTR iFindPath );


	// 検索パスを指定する。ワイルドカード使用可。
	void	Open( LPCSTR );
	// 見つかったファイルを順次取得する。
	LPCSTR	GetFoundFileName();
	// 
	void	Close( LPCSTR );
	
	 {
	}
	}
	void	Reset
	

};


	FileFinder	ff;
	ff.First("c:\*.*");
	do {
		GetFoundData().cFileName;
	} while ( ff.Next() );
	ff.Close();
	
	FileFinder ff("c:\\*.*");
	while ( ff.FindNext() != NULL ) {
		(LPCSTR)ff.Now()
	}

class	FileFinder {
	HANDLE			m_hFind;	// 検索ハンドル
	WIN32_FIND_DATA	m_fdFound;	// 見つかったファイルの情報
public:
	FileFinder() {
		m_hFind = INVALID_HANDLE_VALUE;
		::ZeroMemory( &m_fdFound, sizeof(WIN32_FIND_DATA) );
	}
	~FileFinder() {
		Close();
	}

	BOOL	First( LPCSTR szPath ) {
		Close();
		m_hFind = ::FindFirstFile( szPath, &m_fdFound );
		if ( m_hFind == INVALID_HANDLE_VALUE )
			return	FALSE;
	}

	BOOL	Next() {
		return	::FindNextFile( m_hFind, &m_fdFound );
	}

	BOOL	Close( LPCSTR szPath ) {
		if ( m_hFind != INVALID_HANDLE_VALUE ) {
			::FindClose(m_hFind);
			m_hFind = INVALID_HANDLE_VALUE;
		}
	}

	const WIN32_FIND_DATA& GetFoundData() {
		return	&m_fdFound;
	}
};
*/