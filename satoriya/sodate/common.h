
#include	<windows.h>
#include	"../_/stltool.h"
#include	"../_/utilities.h"
#include	"../_/File.h"
#include	"../_/FileLister.h"
#include	"../_/FTP.h"
#include	"../_/MD5.h"
#include	"../_/Sender.h"

#include	<map>
#include	<list>
#include	<string>
#include	<vector>
#include	<fstream>
using namespace std;


void	error(const wstring& str);
// コンソールに出力
void	console(const wstring& str);

// 除外するファイル名
extern	set<wstring>		deny_filename_set;

// 対象ファイル一覧
extern	list<wstring>	files;
// filesをフォルダ別に分離格納
extern	map<wstring, set<wstring> >	files_on_dir;	// dirname : filenames

// 環境設定
extern	strmap	conf;
// 作業フォルダ
extern	wstring	base_folder;

static const wchar_t byte_value_1[2] = {1,0};
static const wchar_t byte_value_2[2] = {2,0};

