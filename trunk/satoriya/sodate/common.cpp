#include	"common.h"



// 除外するファイル名
set<string>		deny_filename_set;
// 対象ファイル一覧
list<string>	files;

map<string, set<string> >	files_on_dir;	// dirname : filenames

// 環境設定
strmap	conf;
// 作業フォルダ
string	base_folder;


void	error(const string& str) {
	cerr << "\nエラー：" << str << "\n";

	::MessageBox(NULL, str.c_str(), "error - sodate", MB_OK);
}
