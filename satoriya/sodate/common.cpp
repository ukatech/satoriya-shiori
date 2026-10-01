#include	"common.h"



// 除外するファイル名
set<wstring>		deny_filename_set;
// 対象ファイル一覧
list<wstring>	files;

map<wstring, set<wstring> >	files_on_dir;	// dirname : filenames

// 環境設定
strmap	conf;
// 作業フォルダ
wstring	base_folder;


void	console(const wstring& str) {
	HANDLE	h = ::GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD	written = 0;
	if ( ::WriteConsoleW(h, str.c_str(), str.size(), &written, NULL) )
		return;
	// コンソールでなければ（リダイレクト等）既定のコードページで出力
	cout << WtoACP(str) << flush;
}

void	error(const wstring& str) {
	console(L"\nエラー：" + str + L"\n");

	::MessageBox(NULL, str.c_str(), L"error - sodate", MB_OK);
}
