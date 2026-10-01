#ifndef	FILE_H
#define	FILE_H

//----------------------------------------------------------
// ファイルハンドルのラッパークラス
#define	NULL	0
class File {
public:
	File();
	~File();

	enum OpenFlag { READ, WRITE };
	bool	Open( const wchar_t* iFileName, OpenFlag iOpenFlag=READ );
	void	Close();
	
	bool	Read( void* oBuffer, unsigned long iLength );
	bool	Write( const void* iBuffer, unsigned long iLength );

	enum MoveMethod { BEGIN, CURRENT, END };
	bool	SetPosition( long iPosition, MoveMethod iMoveMethod=BEGIN );
	bool	GetPosition( unsigned long* oPosition );
	bool	GetSize( unsigned long* oSizeLow, unsigned long* oSizeHigh=NULL );

private:
	void*	mHandle;	// typedef void* HANDLE;
};

#include	<windows.h>

//----------------------------------------------------------
// ファイル操作（Shell～APIを使用）

BOOL	Move( LPCTSTR iTo, LPCTSTR iFrom );
BOOL	Copy( LPCTSTR iTo, LPCTSTR iFrom );
BOOL	Delete( LPCTSTR iFileName );
BOOL	Rename( LPCTSTR iTo, LPCTSTR iFrom );


// ファイルの最終更新日時を取得
bool	GetLastWriteTime(LPCWSTR iFileName, SYSTEMTIME& oSystemTime);
//	ファイルの更新日時を比較。
//	返値が正ならば前者、負ならば後者のほうが新しいファイル。
int		CompareTime(LPCWSTR iLeft, LPCWSTR iRight);
//	SYSTEMTIMEを比較。
//	返値が正ならば前者、負ならば後者のほうが新しいファイル。
int	CompareTime(const SYSTEMTIME& stL, const SYSTEMTIME& stR);

// ファイルの大きさを取得
BOOL	GetFileSize( LPCWSTR szFileName, DWORD* pdwSize );

// ファイル・フォルダの存在有無を問い合わせ
bool	isExist(const wchar_t* iPath);
bool	isExistFile(const wchar_t* iPath);
bool	isExistFolder(const wchar_t* iPath);

//----------------------------------------------------------
// ファイル名・パス

// バス名からフォルダ名と拡張子を削除する。
void	ToOnlyFileName( LPWSTR buf );

// パス文字列の拡張子を変更する
void	SetExtention( LPWSTR szPath, LPCWSTR szNewExtention );
// パス文字列の拡張子部を取得する
LPCWSTR	GetExtention( LPCWSTR szPath );
LPWSTR	GetExtention( LPWSTR szPath );
// 拡張子を判定する
bool	isExtention(LPCWSTR iPath, LPCWSTR iExtention);

// パス文字列のファイル名部分を取得する。
// 文字列中に \ 記号がない場合は、与えられた文字列をそのまま返す。
LPCWSTR	GetFileName( LPCWSTR szPath );
LPWSTR	GetFileName( LPWSTR szPath );


// コモンダイアログを使用し、ファイル名をユーザーに選択させる。
bool	GetOpenFileName(
	LPWSTR	oFileName,	// MAX_PATH長が必要。
	HWND	iParentWindow=NULL,
	LPCWSTR	iFormat=L"全てのファイル (*.*)\0*.*\0",
	LPCWSTR iTitle=L"ファイルを開く" );
bool	GetSaveFileName(
	LPWSTR	oFileName,	// MAX_PATH長が必要。
	HWND	iParentWindow=NULL,
	LPCWSTR	iFormat=L"全てのファイル (*.*)\0*.*\0",
	LPCWSTR	iTitle=L"ファイルを保存する" );

//----------------------------------------------------------
// ファイル・フォルダ構造

// フォルダを作成。成功／既存ならtrue。
bool	MakeFolder(const wchar_t* iFolderName);
// 複数階層に渡りフォルダを作成。
bool	MakeFolder_Nest(const wchar_t* iFolderName);


#endif	// FILE_H
