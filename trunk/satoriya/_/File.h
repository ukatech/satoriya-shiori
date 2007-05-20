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
	bool	Open( const char* iFileName, OpenFlag iOpenFlag=READ );
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
bool	GetLastWriteTime(LPCSTR iFileName, SYSTEMTIME& oSystemTime);
//	ファイルの更新日時を比較。
//	返値が正ならば前者、負ならば後者のほうが新しいファイル。
int		CompareTime(LPCSTR iLeft, LPCSTR iRight);
//	SYSTEMTIMEを比較。
//	返値が正ならば前者、負ならば後者のほうが新しいファイル。
int	CompareTime(const SYSTEMTIME& stL, const SYSTEMTIME& stR);

// ファイルの大きさを取得
BOOL	GetFileSize( LPCSTR szFileName, DWORD* pdwSize );

// ファイル・フォルダの存在有無を問い合わせ
bool	isExist(const char* iPath);
bool	isExistFile(const char* iPath);
bool	isExistFolder(const char* iPath);

//----------------------------------------------------------
// ファイル名・パス

// バス名からフォルダ名と拡張子を削除する。
void	ToOnlyFileName( LPSTR buf );

// パス文字列の拡張子を変更する
void	SetExtention( LPTSTR szPath, LPCTSTR szNewExtention );
// パス文字列の拡張子部を取得する
LPCTSTR	GetExtention( LPCTSTR szPath );
LPTSTR	GetExtention( LPTSTR szPath );
// 拡張子を判定する
bool	isExtention(LPCTSTR iPath, LPCTSTR iExtention);

// パス文字列のファイル名部分を取得する。
// 文字列中に \ 記号がない場合は、与えられた文字列をそのまま返す。
LPCTSTR	GetFileName( LPCTSTR szPath );
LPTSTR	GetFileName( LPTSTR szPath );


// コモンダイアログを使用し、ファイル名をユーザーに選択させる。
bool	GetOpenFileName(
	LPTSTR	oFileName,	// MAX_PATH長が必要。
	HWND	iParentWindow=NULL,
	LPCTSTR	iFormat="全てのファイル (*.*)\0*.*\0",
	LPCTSTR iTitle="ファイルを開く" );
bool	GetSaveFileName(
	LPTSTR	oFileName,	// MAX_PATH長が必要。
	HWND	iParentWindow=NULL,
	LPCTSTR	iFormat="全てのファイル (*.*)\0*.*\0",
	LPCTSTR	iTitle="ファイルを保存する" );

//----------------------------------------------------------
// ファイル・フォルダ構造

// フォルダを作成。成功／既存ならtrue。
bool	MakeFolder(const char* iFolderName);
// 複数階層に渡りフォルダを作成。
bool	MakeFolder_Nest(const char* iFolderName);


#endif	// FILE_H
