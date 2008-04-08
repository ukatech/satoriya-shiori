
#include	<windows.h>
#include	"../_/stltool.h"
#include	"../_/utilities.h"
#include	"../_/File.h"
#include	"../_/FileLister.h"
#include	"../_/FTP.h"
#include	"../_/MD5.h"

#include	<map>
#include	<list>
#include	<string>
#include	<vector>
#include	<fstream>
using namespace std;


void	error(const string& str);

// 除外するファイル名
extern	set<string>		deny_filename_set;

// 対象ファイル一覧
extern	list<string>	files;
// filesをフォルダ別に分離格納
extern	map<string, set<string> >	files_on_dir;	// dirname : filenames

// 環境設定
extern	strmap	conf;
// 作業フォルダ
extern	string	base_folder;

static const char byte_value_1[2] = {1,0};
static const char byte_value_2[2] = {2,0};



#include	"Sender.h"


