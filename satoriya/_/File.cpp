#include	<windows.h>
#include	<assert.h>
#include	<stdio.h>	// for sprintf
#include	"File.h"

File::File() : mHandle(INVALID_HANDLE_VALUE) {
}

File::~File() {
	Close();
}

bool	File::Open( const char* iFileName, OpenFlag iOpenFlag ) {
	assert(iFileName != NULL);
	assert(iOpenFlag == READ || iOpenFlag == WRITE);

	Close();
	if ( iOpenFlag==READ ) {
		mHandle = ::CreateFile( iFileName, 
			GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	}
	else {
		mHandle = ::CreateFile( iFileName, 
			GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	}
	if ( mHandle == INVALID_HANDLE_VALUE ) {
		return	false;
	}

	return	true;
}
void	File::Close() {
	if ( mHandle != INVALID_HANDLE_VALUE ) {
		::CloseHandle(mHandle);
		mHandle = INVALID_HANDLE_VALUE;
	}
}

bool	File::Read( void* oBuffer, unsigned long iLength ) {
	assert(oBuffer != NULL);

	DWORD	theRead;
	if ( !::ReadFile( mHandle, oBuffer, iLength, &theRead, NULL ) )
		return	false;
	if ( theRead < iLength )
		return	false;
	return	true;
}

bool	File::Write( const void* iBuffer, unsigned long iLength ) {
	assert(iBuffer != NULL);
	if ( iLength == 0)
		return	true;
	
	DWORD	theWritten;
	BOOL	theFlag = ::WriteFile( mHandle, iBuffer, iLength, &theWritten, NULL );
	return theFlag ? true : false;
}

bool	File::SetPosition( long iPosition, MoveMethod iMoveMethod ) {
	assert( iMoveMethod==BEGIN || iMoveMethod==CURRENT || iMoveMethod==END );
	assert( !( iMoveMethod==BEGIN && iPosition<0 ) );
	assert( !( iMoveMethod==END && iPosition>0 ) );
	
	DWORD	dwMoveMethod;
	if ( iMoveMethod == BEGIN )
		dwMoveMethod = FILE_BEGIN;
	else if ( iMoveMethod == BEGIN )
		dwMoveMethod = FILE_END;
	else
		dwMoveMethod = FILE_CURRENT;
	

	if ( mHandle == INVALID_HANDLE_VALUE )
		return	false;
	return	::SetFilePointer( mHandle, iPosition, NULL, dwMoveMethod ) != 0xFFFFFFFF;
}

bool	File::GetPosition( unsigned long* oPosition ) {
	assert( oPosition != NULL );

	*oPosition = ::SetFilePointer( mHandle, 0, NULL, FILE_CURRENT );
	return	( *oPosition != 0xffffffff );
}

bool	File::GetSize( unsigned long* oSizeLow, unsigned long* oSizeHigh ) {
	assert( oSizeLow != NULL );
	if ( mHandle == INVALID_HANDLE_VALUE )
		return	false;
		
	*oSizeLow = ::GetFileSize( mHandle, oSizeHigh );
	return	( *oSizeLow != 0xffffffff );
}


#if 0
//----------------------------------------------------------------------
// シェル関数利用によるファイル操作
//		参考 : C MAGAZINE 2000年7月号 p133

static BOOL 
FileOperation(
	UINT iFunc, LPCSTR iFrom, LPCSTR iTo,
	FILEOP_FLAGS iFlags, LPCSTR iProgressTitle )
{
	SHFILEOPSTRUCT	fos;
	fos.hwnd = NULL;
	fos.wFunc = iFunc;
	fos.pFrom = iFrom;
	fos.pTo = iTo;
	fos.fFlags = iFlags | FOF_FILESONLY;
	fos.fAnyOperationsAborted = FALSE;
	fos.hNameMappings = NULL;
	fos.lpszProgressTitle = iProgressTitle;
	return ( ::SHFileOperation(&fos) == 0 );
}

BOOL	Move( LPCTSTR iTo, LPCTSTR iFrom ) {
	return FileOperation( FO_MOVE, iFrom, iTo, 
		FOF_ALLOWUNDO|FOF_SIMPLEPROGRESS, NULL );
}
BOOL	Copy( LPCTSTR iTo, LPCTSTR iFrom ) {
	return FileOperation( FO_COPY, iFrom, iTo, 
		FOF_ALLOWUNDO|FOF_RENAMEONCOLLISION, NULL );
}
BOOL	Delete( LPCTSTR iFileName ) {
	return FileOperation( FO_DELETE, iFileName, NULL, 
		FOF_ALLOWUNDO|FOF_SIMPLEPROGRESS, NULL );
}
BOOL	Rename( LPCTSTR iTo, LPCTSTR iFrom ) {
	return FileOperation( FO_RENAME, iFrom, iTo, 
		FOF_ALLOWUNDO|FOF_RENAMEONCOLLISION, NULL );
}
#endif
// ファイル・フォルダの存在有無を問い合わせ
bool	isExist(const char* iPath) {
	assert(iPath != NULL);
	WIN32_FIND_DATA	fdFOUND;
	HANDLE hFIND = ::FindFirstFile( iPath, &fdFOUND );
	if ( hFIND == INVALID_HANDLE_VALUE )
		return	false;
	::FindClose(hFIND);
	return	true;
}
bool	isExistFile(const char* iPath) {
	assert(iPath != NULL);
	HANDLE hFile = ::CreateFile( iPath, 0, 
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
		return false;
	::CloseHandle(hFile);
	return true;
}
bool	isExistFolder(const char* iPath) {
	assert(iPath != NULL);
	WIN32_FIND_DATA	fdFOUND;
	HANDLE hFIND = ::FindFirstFile( iPath, &fdFOUND );
	if ( hFIND == INVALID_HANDLE_VALUE )
		return	false;
	::FindClose(hFIND);
	return (( fdFOUND.dwFileAttributes
		& FILE_ATTRIBUTE_DIRECTORY ) !=0 );
}


// ファイルの大きさを取得
BOOL	GetFileSize( LPCSTR szFileName, DWORD* pdwSize ) {
	assert( szFileName != NULL );
	assert( pdwSize != NULL );

	HANDLE hFile = ::CreateFile( szFileName, 0, 
		FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE ) {
		*pdwSize = 0;
		return FALSE;
	}
	*pdwSize = ::GetFileSize( hFile, NULL );
	::CloseHandle( hFile );
	if ( *pdwSize == 0xffffffff ) {
		*pdwSize = 0;
		return FALSE;
	}
	return TRUE;
}

// 文字列中から指定の1byte文字が最後に出現する位置を返す。
#include	<mbctype.h>	// for _ismbblead()
template<class T>
inline T*	FindFinalChar(T* start, T c) {
	T* last=NULL;
	for (T* p=start; *p ; p+=_ismbblead(*p)?2:1)
		if (*p==c)
			last=p;
	return	last;
}

// バス名からフォルダ名と拡張子を削除する。
void	ToOnlyFileName( LPSTR buf ) {
	LPSTR	ptr;
	// フォルダ名
	if ( (ptr = FindFinalChar( buf, '\\' )) != NULL )
		::lstrcpy( buf, ptr+1 );
	// 拡張子
	if ( (ptr = FindFinalChar( buf, '.' )) != NULL )
		*ptr = '\0';
}

// パス文字列の拡張子を変更する
void	SetExtention( LPTSTR szPath, LPCTSTR szNewExtention ) {
	LPTSTR	ext = FindFinalChar( szPath, '.' );
	if ( ext == NULL ) {
		ext = szPath + ::lstrlen(szPath);
		sprintf( ext, ".%s", szNewExtention );
	} else
		strcpy( ext+1, szNewExtention );
}

// パス文字列の拡張子部を取得する
LPCTSTR	GetExtention( LPCTSTR szPath ) {
	LPCTSTR	ext = FindFinalChar( szPath, '.' );
	if ( ext == NULL )
		return	NULL;
	else
		return	ext+1;
}

LPTSTR	GetExtention( LPTSTR szPath ) { 
	return const_cast<LPTSTR>( GetExtention( static_cast<LPCTSTR>(szPath) ) ); 
}
// 拡張子を判定
bool	isExtention(LPCTSTR iPath, LPCTSTR iExtention) {
	return ( lstrcmpi( GetExtention(iPath), iExtention ) == 0 );
}


// パス文字列のファイル名部分を取得する。
// 文字列中に \ 記号がない場合は、与えられた文字列をそのまま返す。
LPCTSTR	GetFileName( LPCTSTR szPath ) {
	LPCTSTR	ext = FindFinalChar( szPath, '\\' );
	return ( ext == NULL ) ? szPath : ext+1 ;
}
LPTSTR	GetFileName( LPTSTR szPath ) {
	return const_cast<LPTSTR>( GetFileName( static_cast<LPCTSTR>(szPath) ) ); 
}


#if 0
// コモンダイアログを使用し、ファイル名をユーザーに選択させる。
bool	GetOpenFileName(LPTSTR oFileName, HWND iParentWindow, LPCTSTR iFormat, LPCTSTR iTitle) {
	assert(oFileName != NULL);
	::lstrcpy(oFileName, TEXT(""));
	OPENFILENAME	of = {
		sizeof(OPENFILENAME), iParentWindow, NULL,
		iFormat, NULL, 0, 0,
		oFileName, MAX_PATH,
		NULL, 0, NULL,
		iTitle,
		OFN_HIDEREADONLY|OFN_LONGNAMES|OFN_FILEMUSTEXIST, 
		0, 0, NULL, 0, NULL, NULL };
	return	(::GetOpenFileName(&of) != FALSE);
}

bool	GetSaveFileName(LPTSTR oFileName, HWND iParentWindow, LPCTSTR iFormat, LPCTSTR iTitle) {
	assert(oFileName != NULL);
	::lstrcpy(oFileName, TEXT(""));
	OPENFILENAME	of = {
		sizeof(OPENFILENAME), iParentWindow, NULL,
		iFormat, NULL, 0, 0,
		oFileName, MAX_PATH,
		NULL, 0, NULL,
		iTitle,
		OFN_HIDEREADONLY|OFN_LONGNAMES, 
		0, 0, NULL, 0, NULL, NULL };
	return	(::GetSaveFileName(&of) != FALSE);
}
#endif

// フォルダを作成
bool	MakeFolder(const char* iFolderName) {
	if ( ::CreateDirectory(iFolderName, NULL) )
		return	true;
	else if ( ::GetLastError()==ERROR_ALREADY_EXISTS )
		return	true;
	else
		return	false;
}

// 複数階層に渡りフォルダを作成
bool	MakeFolder_Nest(const char* iFolderName) {
	int		theLength = strlen(iFolderName);
	char*	theFolderName = new char[theLength+1];
	memset(theFolderName, '\0', theLength+1);

	for (int i=0 ; i<theLength ; i++) {
		if ( iFolderName[i] == '\\' )
			if ( !MakeFolder(theFolderName) ) {
				delete [] theFolderName;
				return	false;
			}
		theFolderName[i] = iFolderName[i];
	}
	delete [] theFolderName;
	return	MakeFolder(iFolderName);
}

// ファイルの最終更新日時を取得
bool	GetLastWriteTime(LPCSTR iFileName, SYSTEMTIME& oSystemTime) {
	HANDLE	theFile = ::CreateFile( iFileName, 
		GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( theFile==INVALID_HANDLE_VALUE )
		return	false;

	BY_HANDLE_FILE_INFORMATION	theInfo;
	::GetFileInformationByHandle(theFile, &theInfo);
	::CloseHandle(theFile);

	FILETIME	FileTime;
	::FileTimeToLocalFileTime(&(theInfo.ftLastWriteTime), &FileTime);
	::FileTimeToSystemTime(&FileTime, &oSystemTime);
	return	true;
}


/*----------------------------------------------------------------------
	SYSTEMTIMEを比較。
	返値が正ならば前者、負ならば後者のほうが新しいファイル。
----------------------------------------------------------------------*/
int	CompareTime(const SYSTEMTIME& stL, const SYSTEMTIME& stR) {
	if ( stL.wYear > stR.wYear )	return	1;
	else if ( stL.wYear < stR.wYear )	return	-1;
	if ( stL.wMonth > stR.wMonth )	return	1;
	else if ( stL.wMonth < stR.wMonth )	return	-1;
	if ( stL.wDay > stR.wDay )	return	1;
	else if ( stL.wDay < stR.wDay )	return	-1;
	if ( stL.wHour > stR.wHour )	return	1;
	else if ( stL.wHour < stR.wHour )	return	-1;
	if ( stL.wMinute > stR.wMinute )	return	1;
	else if ( stL.wMinute < stR.wMinute )	return	-1;
	if ( stL.wSecond > stR.wSecond )	return	1;
	else if ( stL.wSecond < stR.wSecond )	return	-1;
	if ( stL.wMilliseconds > stR.wMilliseconds )	return	1;
	else if ( stL.wMilliseconds < stR.wMilliseconds )	return	-1;
	return	0;
}

/*----------------------------------------------------------------------
	ファイルの更新日時を比較。
	返値が正ならば前者、負ならば後者のほうが新しいファイル。
----------------------------------------------------------------------*/
int	CompareTime(LPCSTR szL, LPCSTR szR) {
	assert(szL!=NULL && szR!=NULL);

	SYSTEMTIME	stL, stR;
	BOOL		fexistL, fexistR;

	// 更新日付を得る。
	fexistL = GetLastWriteTime(szL, stL);
	fexistR	= GetLastWriteTime(szR, stR);
	// 存在しないファイルは「古い」と見なす。
	if ( fexistL ) {
		if ( !fexistR)
			return	1;
	} else {
		if ( fexistR )
			return	-1;
		else
			return	0;	// どっちもありゃしねぇ
	}

	// 最終更新日付を比較
	return	CompareTime(stL, stR);
}
