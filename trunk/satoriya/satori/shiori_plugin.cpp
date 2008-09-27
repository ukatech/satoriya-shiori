#ifdef POSIX
#  include      "Utilities.h"
#  include      <iostream>
#  include      <stdlib.h>
#  include      <sys/types.h>
#  include      <sys/stat.h>
#  include      <dlfcn.h>
#else
#  include	<windows.h>
#  include	<mbctype.h>
#endif
#include	<assert.h>
#include	"../_/stltool.h"
#include	"../_/Sender.h"
#include	"shiori_plugin.h"
#include	"console_application.h"
#include	<sstream>
using namespace std;

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


#ifdef POSIX
// dllサーチパス関連。
// POSIX上では、ある特定の場所にDLLと同名のライブラリを置く事でSAORIに対応する。
// DLLと同名とは云っても、それはシンボリックリンクであるべきで、例えば次のようにである。
// % pwd
// /home/foo/.saori
// % ls -l
// -rwxr-xr-x x foo bar xxxxx 1 1 00:00 libssu.so
// lrwxr-xr-x x foo bar xxxxx 1 1 00:00 ssu.dll -> libssu.so
//
// パスは環境変数 SAORI_FALLBACK_PATH から取得する。これはコロン区切りの絶対パスである。
static vector<string> posix_dll_search_path;
static bool posix_dll_search_path_is_ready = false;
static string posix_search_fallback_dll(const string& dllfile) {
    // dllfileは探したいファイルDLL名。パス区切り文字は/。
    // 代替ライブラリが見付かればその絶対パスを、
    // 見付けられなければ空文字列を返す。
    
    if (!posix_dll_search_path_is_ready) {
	// SAORI_FALLBACK_PATHを見る。
	char* cstr_path = getenv("SAORI_FALLBACK_PATH");
	if (cstr_path != NULL) {
	    split(cstr_path, ":", posix_dll_search_path);
	}
	posix_dll_search_path_is_ready = true;
    }

    string::size_type pos_slash = dllfile.rfind('/');
    string fname(
	dllfile.begin() + (pos_slash == string::npos ? 0 : pos_slash),
	dllfile.end());

    for (vector<string>::const_iterator ite = posix_dll_search_path.begin();
	 ite != posix_dll_search_path.end(); ite++ ) {
	string fpath = *ite + '/' + fname;
	struct stat sb;
	if (stat(fpath.c_str(), &sb) == 0) {
	    // 代替ライブラリが存在するようだ。これ以上のチェックは省略。
	    return fpath;
	}
    }
    return string();
}
#endif

bool ShioriPlugins::load(const string& iBaseFolder)
{
	mBaseFolder = iBaseFolder;

	// mBaseFolderがスラッシュで終わっていなければ、付ける。
	if ( false == mBaseFolder.empty() && mBaseFolder[mBaseFolder.length() - 1] != DIR_CHAR)
	{
	    mBaseFolder += DIR_CHAR;
	}
	return true;
}

bool ShioriPlugins::load_a_plugin(const string& iPluginLine)
{
	strvec	vec;
	split(iPluginLine, ",", vec);	// カンマ区切りで分割
	if ( vec.size()<2 || vec[0].size()==0 ) {	// 呼び出し名と相対パスが必須
		errsender << iPluginLine + ": 設定ファイルの書式が正しくありません。" << std::endl;
		return	false;
	}
	if ( mCallData.find(vec[0]) != mCallData.end() ) {
		errsender << vec[0] + ": 同じ呼び出し名が複数定義されています。" << std::endl;
		return	false;
	}

	// フォルダ区切りを環境に応じて方に統一
	string filename = unify_dir_char(vec[1]);
	string fullpath = unify_dir_char(mBaseFolder + filename);

	if ( mDllData.find(fullpath) != mDllData.end() ) 
	{
		// 既に登録済みなら参照を増やす
		mDllData[fullpath].mRefCount += 1;
	}
	else 
	{
		// 未登録なら読み込む

#ifndef POSIX
		// ネットワーク更新時、SAORI.dllが上書きできずdl2として保存される問題に暫定対処
		if ( compare_tail(fullpath, ".dll") )
		{
			string	dl2 = fullpath;
			dl2[dl2.size()-1]='2';
			FILE*	fp = fopen(dl2.c_str(), "rb");
			if ( fp != NULL )
			{
				fclose(fp);
				
				// .dllを消して.dl2を.dllにリネームする。
				::DeleteFile(fullpath.c_str());
				::MoveFile(dl2.c_str(), fullpath.c_str());
			}
		}
#endif
		
		// ファイルの存在を確認
		FILE*	fp = fopen(fullpath.c_str(), "rb");
		if ( fp == NULL )
		{
#ifdef POSIX
            errsender << fullpath + ": failed to open" << std::endl;
#else
			errsender << fullpath + ": プラグインが存在しません。" << std::endl;
#endif
			return	false;
		}
		fclose(fp);

		// ファイル名・フォルダ名・拡張子を分離
		const char*	lastyen = NULL;
		const char*	lastdot = NULL;
		const char*	p = fullpath.c_str();
		while (*p != '\0') {
			if (*p == DIR_CHAR)
				lastyen=p;
			else if (*p == '.')
				lastdot=p;
			p += _ismbblead(*p) ? 2 : 1; 
		}
		if ( lastyen==NULL || lastdot==NULL )
		{
			return false;
		}

		string  dll_full_path(fullpath);
		string	foldername(fullpath.c_str(), lastyen - fullpath.c_str());
		string	filename(lastyen+1, strlen(lastyen)-strlen(lastdot)-1);
		string	extention(lastdot+1);

#ifdef POSIX
		// 環境変数 SAORI_FALLBACK_ALWAYS が定義されていて、且つ
		// 空でも"0"でもなければ、このdllファイルを開いてみる事は
		// 初めからやらない。そうでなければ、試しにdlopenしてみる。
		char* env_fallback_always = getenv("SAORI_FALLBACK_ALWAYS");
		bool fallback_always = false;
		if (env_fallback_always != NULL) {
		    string str_fallback_always(env_fallback_always);
		    if (str_fallback_always.length() > 0 &&
			str_fallback_always != "0") {
			fallback_always = true;
		    }
		}
		bool do_fallback = true;
		if (!fallback_always) {
		    void* handle = dlopen(fullpath.c_str(), RTLD_LAZY);
		    if (handle != NULL) {
			// load, unload, requestを取出してみる。
			void* sym_load = dlsym(handle, "load");
			void* sym_unload = dlsym(handle, "unload");
			void* sym_request = dlsym(handle, "request");
			if (sym_load != NULL && sym_unload != NULL && sym_request != NULL) {
			    // なんと正常に読めてしまった。実験目的で作った環境だろうか。
			    do_fallback = false;
			}
		    }
		    dlclose(handle);
		}
		if (do_fallback) {
		    // 代替ライブラリを探す。
		    string fallback_lib = posix_search_fallback_dll(filename+"."+extention);
		    if (fallback_lib.length() == 0) {
			// 無い。
			char* cstr_path = getenv("SAORI_FALLBACK_PATH");
			string fallback_path =
			    (cstr_path == NULL ?
			     "(environment variable `SAORI_FALLBACK_PATH' is empty)" : cstr_path);
			
			errsender <<
			    fullpath+": This is not usable in this platform.\n"+
			    "Fallback library `"+filename+"."+extention+"' doesn't exist: "+fallback_path) << std::endl;
			
			mDllData.erase(fullpath);

			return false;
		    }
		    else {
			cerr << "SAORI: using " << fallback_lib << " instead of " << fullpath << endl;
		    }

		    // 参照カウントに使うため、fullpathは書換えない。
		    // dll_full_pathのみ。
		    dll_full_path = fallback_lib;
		}
#endif

		// POSIX環境では、ライブラリ名に*.dll以外も許す。
#ifdef POSIX
		if ( 1 )
#else
		if ( compare_tail(fullpath, ".dll") )
#endif
		{
			// プラグインDLLをロード
			mDllData[fullpath].mRefCount=1;
			extern const char* gSatoriName;
			if ( !(mDllData[fullpath].mSaoriClient.load(gSatoriName, "Shift_JIS", foldername+DIR_CHAR, dll_full_path)) )
			{
				mDllData.erase(fullpath);
				return	false;
			}

			// バージョン確認
			string ver = mDllData[fullpath].mSaoriClient.get_version("Local");
			if ( ver != "SAORI/1.0" )
			{
				errsender << fullpath + ": SAORI/1.0のdllではありません。GET Versionの戻り値が未対応のものでした。(" + ver + ")" << std::endl;
				mDllData.erase(fullpath);
				return	false;
			}
		}
	}

	// 呼び出し名mapに登録
	mCallData[vec[0]].mDllPath = fullpath;
#ifdef POSIX
	mCallData[vec[0]].mIsBasic = false; // 拡張子では判断できないので、とりあえずSaori Basicのサポートは無し…
#else
	mCallData[vec[0]].mIsBasic = !compare_tail(fullpath, ".dll");
#endif
	strvec::const_iterator j=vec.begin();
	for ( j+=2 ; j!=vec.end() ; ++j )
		mCallData[vec[0]].mPreDefinedArguments.push_back(*j);

	return	true;
}

void	ShioriPlugins::unload()
{
	for ( map<string, CallData>::iterator i=mCallData.begin() ; i!=mCallData.end() ; ++i ) {
		if ( i->second.mIsBasic )
			continue;
		DllData&	ddat = mDllData[i->second.mDllPath];
		if ( --ddat.mRefCount == 0 ) {
			ddat.mSaoriClient.unload();
			mDllData.erase(i->second.mDllPath);
		}
	}
	mCallData.clear();
}

string	ShioriPlugins::request(const string& iCallName, const strvec& iArguments, strvec& oResults, const string& iSecurityLevel) {

	if ( mCallData.find(iCallName) == mCallData.end() ) {
		errsender << iCallName + ": この呼び出し名は定義されていません。" << std::endl;
		return	"";
	}
	CallData&	theCallData = mCallData[iCallName];

	if ( theCallData.mIsBasic ) 
	{
		// SAORI-basicの呼び出し

		// さおべーはexternal呼び出しを判別する能力が無いので（環境変数等で渡せばいい？）
		// この時点で全て切ります。-universalはリクエスト時にsecurity levelヘッダを渡し、SAORI側で判別していただきます。
		if ( iSecurityLevel != "local" && iSecurityLevel != "Local" )
			return	""; 

		string	theCommandLine = theCallData.mDllPath;
		strvec::const_iterator i;
		for ( i=theCallData.mPreDefinedArguments.begin() ; i!=theCallData.mPreDefinedArguments.end() ; ++i )
			theCommandLine += " "+ *i;
		for ( i=iArguments.begin() ; i!=iArguments.end() ; ++i )
			theCommandLine += " "+ *i;

		string out;
		string r = call_console_application(
			theCommandLine,
			get_folder_name(theCallData.mDllPath).c_str(),
			out);
		if ( r != "" )
		{
			// エラー
			errsender << iCallName + ": " + r << std::endl;
			return "";
		}
		else
		{
			// 正常終了
			return out;
		}
	}
	else 
	{
		// SAORI-universalの呼び出し

		//---------------------
		// リクエスト作成

		vector<string> req;
		req.insert(req.end(), theCallData.mPreDefinedArguments.begin(), theCallData.mPreDefinedArguments.end());
		req.insert(req.end(), iArguments.begin(), iArguments.end());

		//---------------------
		// リクエスト実行

		assert( mDllData.find(theCallData.mDllPath) != mDllData.end() );

		string result;
		int return_code = mDllData[ theCallData.mDllPath ].mSaoriClient.request(
			 req,
			 ( iSecurityLevel == "local" || iSecurityLevel == "Local" ),
			 result,
			 oResults);

		//---------------------
		// 返答に対処

		switch (return_code)
		{
		case 200:
		case 204:
			break;
		case 400:
			errsender << theCallData.mDllPath + " - " + iCallName + " : 400 Bad Request / 呼び出しの不備" << std::endl;
			break;
		case 500:
			errsender << theCallData.mDllPath + " - " + iCallName + " : 500 Internal Server Error / saori内でのエラー" << std::endl;
			break;
		default:
			errsender << theCallData.mDllPath + " - " + iCallName + " : " + itos(return_code) + "? / 定義されていないステータスを返しました。" << std::endl;
			break;
		}

		return	result;
	}
}


