#ifndef	FILELISTER_H
#define	FILELISTER_H

#include	<windows.h>

// 指定パスのファイルを列挙する。
class	FileLister {
public:
	FileLister();
	~FileLister();

	// 列挙パスを指定。ワイルドカード使用可。
	BOOL	Open( LPCSTR iFindPath );
	// 次のファイルを列挙。
	BOOL	Next();

	// 現在のファイル名を取得。
	LPCSTR	Name();
	// 現在のファイルはフォルダか？
	BOOL	isFolder();
	// 現在のファイル名は . のみで構成されている？
	BOOL	isDots();
	// 指定の拡張子か？
	BOOL	isExt( LPCSTR iExtention );
	// 現在のファイルの詳細データを取得。
	const WIN32_FIND_DATA*	Data();

	// 列挙の終了処理を行う。
	void	Close();

public:
	TCHAR	mPath[MAX_PATH];	// 列挙対象パス
	HANDLE	mHandle;			// 列挙ハンドル
	WIN32_FIND_DATA	mData;	// 列挙したファイルの情報
};


#include	<string>
#include	<list>
using namespace std;

/*
// ファイル情報をリストアップ
bool	ListFile(string iFindPath, const vector<WIN32_FIND_DATA>& oVector );
// ファイル名をリストアップ
bool	ListFile(string iFindPath, const vector<string>& oVector );
// フォルダ名をリストアップ、iDotsFolderは .及び..を含めるか否か
bool	ListFolder(string iFindPath, const vector<WIN32_FIND_DATA>& oVector, bool iDotsFolder=false );
*/

inline bool list_files(string i_folder, list<WIN32_FIND_DATA>& o_files)
{
	FileLister lister;
	if ( lister.Open((i_folder+"\\*.*").c_str()) == FALSE )
		return false;

	while ( lister.Next() ) 
	{
		o_files.push_back( *(lister.Data()) );
	} 
	
	return true;
}

inline list<WIN32_FIND_DATA> list_files(string i_folder)
{
	list<WIN32_FIND_DATA> data;
	list_files(i_folder, data);
	return data;
}



/*
void	do_folder(const string& iFolder, strvec& oVector) {

	vector<WIN32_FIND_DATA>	datas;

	FileLister	lister;
	if ( !lister.Open((iFolder+"\\*.*").c_str()) )
		return;
	while ( lister.Next() )
		datas.push_back( *lister.Data() );
	lister.Close();

	for (vector<WIN32_FIND_DATA>::iterator i=datas.begin(); i!=datas.end() ; ++i ) {
		if ( strcmp(i->cFileName, ".")==0 )
			continue;
		if ( strcmp(i->cFileName, "..")==0 )
			continue;
		string	path = iFolder + DIR_CHAR + i->cFileName;
		if ( i->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY )
			do_folder(path, oVector);
		else
			oVector.push_back(path);
	}
}
*/

#endif	//	FILELISTER_H
