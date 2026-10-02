#ifdef POSIX
#  include      "Utilities.h"
#  include      <iostream>
#  include      <stdlib.h>
#  include      <sys/types.h>
#  include      <sys/stat.h>
#  include      <dlfcn.h>
#else
#  include	<windows.h>
#endif
#include	<assert.h>
#include	"../_/stltool.h"
#include	"../_/Sender.h"
#include	"shiori_plugin.h"
#include	"console_application.h"
#include	"ssu.h"
#include	<sstream>
using std::wstring;


//////////DEBUG/////////////////////////
#include "warning.h"
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

static std::vector<wstring> posix_dll_search_path;
static bool posix_dll_search_path_is_ready = false;
static wstring posix_search_fallback_dll(const wstring& dllfile) {
    // dllfileは探したいファイルDLL名。パス区切り文字は/。
    // 代替ライブラリが見付かればその絶対パスを、
    // 見付けられなければ空文字列を返す。
    
    if (!posix_dll_search_path_is_ready) {
	// SAORI_FALLBACK_PATHを見る。
	const char* cstr_path = getenv("SAORI_FALLBACK_PATH");
	if (cstr_path != NULL) {
	    split(UTF8toW(cstr_path), L":", posix_dll_search_path);
	}
	posix_dll_search_path_is_ready = true;
    }

    wstring::size_type pos_slash = dllfile.rfind(L'/');
    wstring fname(
	dllfile.begin() + (pos_slash == wstring::npos ? 0 : pos_slash),
	dllfile.end());

    for (std::vector<wstring>::const_iterator ite = posix_dll_search_path.begin();
	 ite != posix_dll_search_path.end(); ite++ ) {
	wstring fpath = *ite + L'/' + fname;
	struct stat sb;
	if (stat(WtoUTF8(fpath).c_str(), &sb) == 0) {
	    // 代替ライブラリが存在するようだ。これ以上のチェックは省略。
	    return fpath;
	}
    }
    return wstring();
}
#endif

bool ShioriPlugins::load(const wstring& iBaseFolder)
{
	mBaseFolder = iBaseFolder;

	// mBaseFolderがスラッシュで終わっていなければ、付ける。
	if ( false == mBaseFolder.empty() && mBaseFolder[mBaseFolder.length() - 1] != DIR_CHAR)
	{
	    mBaseFolder += DIR_CHAR;
	}
	return true;
}

bool ShioriPlugins::load_a_plugin(const wstring& iPluginLine)
{
	strvec	vec;
	split(iPluginLine, L",", vec);	// カンマ区切りで分割
	if ( vec.size()<2 || vec[0].size()==0 ) {	// 呼び出し名と相対パスが必須
		GetSender().errsender() << iPluginLine + L": 設定ファイルの書式が正しくありません。" << satori::endl;
		return	false;
	}
	if ( mCallData.find(vec[0]) != mCallData.end() ) {
		GetSender().errsender() << vec[0] + L": 同じ呼び出し名が複数定義されています。" << satori::endl;
		return	false;
	}

	// フォルダ区切りを環境に応じて方に統一
	wstring filename = unify_dir_char(vec[1]);
	wstring fullpath = unify_dir_char(mBaseFolder + filename);

	if ( mDllData.find(fullpath) != mDllData.end() ) 
	{
		// 既に登録済みなら参照を増やす
		mDllData[fullpath].mRefCount += 1;
	}
	else 
	{
		// 未登録なら読み込む

		//SSU Direct Call
#ifdef POSIX
		if ( compare_tail(fullpath, L"\\ssu") || compare_tail(fullpath, L"/ssu") || compare_tail(fullpath, L"\\ssu.dll") || compare_tail(fullpath, L"/ssu.dll") )
#else
		if ( compare_tail(fullpath, L"\\ssu.dll") || compare_tail(fullpath, L"/ssu.dll") )
#endif
		{
			// プラグインDLLをロード
			mDllData[fullpath].mRefCount=1;
			mDllData[fullpath].m_pSaoriClient=new ssu();
			{
				FILE*	ssu_fp = w_fopen(fullpath, L"rb");
				if ( ssu_fp != NULL ) { fclose(ssu_fp); }
				else { mDllData[fullpath].mIsInternal = true; }
			}
		}
		else {
#ifndef POSIX
			// ネットワーク更新時、SAORI.dllが上書きできずdl2として保存される問題に暫定対処
			if ( compare_tail(fullpath, L".dll") )
			{
				wstring	dl2 = fullpath;
				dl2[dl2.size()-1]=L'2';
				FILE*	fp = w_fopen(dl2, L"rb");
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
			FILE*	fp = w_fopen(fullpath, L"rb");
			if ( fp == NULL )
			{
#ifdef POSIX
				GetSender().errsender() << fullpath + L": failed to open" << satori::endl;
#else
				GetSender().errsender() << fullpath + L": プラグインが存在しません。" << satori::endl;
#endif
				return	false;
			}
			fclose(fp);

			// ファイル名・フォルダ名・拡張子を分離
			const wchar_t*	lastyen = wcsrchr(fullpath.c_str(), DIR_CHAR);
			const wchar_t*	lastdot = wcsrchr(fullpath.c_str(), L'.');
			// 拡張子が無く、フォルダ名に . があるときは lastdot が lastyen より前になる（ファイル名を切り出せない）
			if ( lastyen==NULL || lastdot==NULL || lastdot<lastyen )
			{
				return false;
			}

			wstring  dll_full_path(fullpath);
			wstring	foldername(fullpath.c_str(), lastyen - fullpath.c_str());
			wstring	filename(lastyen+1, wcslen(lastyen)-wcslen(lastdot)-1);
			wstring	extention(lastdot+1);

#ifdef POSIX
			// 環境変数 SAORI_FALLBACK_ALWAYS が定義されていて、且つ
			// 空でも"0"でもなければ、このdllファイルを開いてみる事は
			// 初めからやらない。そうでなければ、試しにdlopenしてみる。
			const char* env_fallback_always = getenv("SAORI_FALLBACK_ALWAYS");
			bool fallback_always = false;
			if (env_fallback_always != NULL) {
				wstring str_fallback_always = UTF8toW(env_fallback_always);
				if (str_fallback_always.length() > 0 &&
				str_fallback_always != L"0") {
				fallback_always = true;
				}
			}
			bool do_fallback = true;
			if (!fallback_always) {
				void* handle = dlopen(WtoUTF8(fullpath).c_str(), RTLD_LAZY);
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
				if (handle != NULL) { dlclose(handle); }
			}
			if (do_fallback) {
				// 代替ライブラリを探す。
				wstring fallback_lib = posix_search_fallback_dll(filename+L"."+extention);
				if (fallback_lib.length() == 0) {
				// 無い。
				const char* cstr_path = getenv("SAORI_FALLBACK_PATH");
				wstring fallback_path =
					(cstr_path == NULL ?
					 wstring(L"(environment variable `SAORI_FALLBACK_PATH' is empty)") : UTF8toW(cstr_path));
				
				GetSender().errsender() << (
					fullpath+L": This is not usable in this platform.\n"+
					L"Fallback library `"+filename+L"."+extention+L"' doesn't exist: "+fallback_path) << satori::endl;
				
				mDllData.erase(fullpath);

				return false;
				}
				else {
				std::wcerr << L"SAORI: using " << fallback_lib << L" instead of " << fullpath << std::endl;
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
			if ( compare_tail(fullpath, L".dll") )
#endif
			{
				// プラグインDLLをロード
				mDllData[fullpath].mRefCount=1;
				mDllData[fullpath].m_pSaoriClient=new SaoriClient();
				extern const wchar_t* gSatoriName;

				if ( mDllData[fullpath].m_pSaoriClient->load(gSatoriName, L"UTF-8", foldername+DIR_CHAR, dll_full_path) )
				{
					// バージョン確認
					wstring ver = mDllData[fullpath].m_pSaoriClient->get_version(L"Local");
					if ( ver != L"SAORI/1.0" )
					{
						GetSender().errsender() << fullpath + L": SAORI/1.xのdllではありません。GET Versionの戻り値が未対応のものでした。(" + ver + L")" << satori::endl;
					}
					else if ( ! mDllData[fullpath].m_pSaoriClient->version_reply_has_charset() ) {
						// YAYA as SAORI の古いayasaori.aymは、GET Versionの応答にCharsetを返さず、
						// 次の最初のEXECUTEだけ文字コードの設定が合わず文字化けする。
						// 応答にCharsetが無い・空のときは、何もしないEXECUTEを1回送って読み捨てる。
						std::vector<wstring>	dummy_args(1, L"");
						wstring	dummy_result;
						std::vector<wstring>	dummy_values;
						mDllData[fullpath].m_pSaoriClient->request(dummy_args, true, dummy_result, dummy_values);
					}
				}
				else {
					GetSender().errsender() << fullpath + L": SAORI/1.xのdllの読み込みに失敗しました。" << satori::endl;
				}

			}
		}
	}

	// 呼び出し名mapに登録
	mCallData[vec[0]].mDllPath = fullpath;
#ifdef POSIX
	mCallData[vec[0]].mIsBasic = false; // 拡張子では判断できないので、とりあえずSaori Basicのサポートは無し…
#else
	mCallData[vec[0]].mIsBasic = !compare_tail(fullpath, L".dll");
#endif
	strvec::const_iterator j=vec.begin();
	for ( j+=2 ; j!=vec.end() ; ++j )
		mCallData[vec[0]].mPreDefinedArguments.push_back(*j);

	return	true;
}

void	ShioriPlugins::load_default_entry()
{
	std::vector<wstring> funclist;
	get_ssu_funclist(funclist);

	for (std::vector<wstring>::const_iterator i = funclist.begin(); i != funclist.end() ; ++i ) {
		if ( mCallData.find(*i) == mCallData.end() ) {
			wstring func_line = *i;
			func_line += L",saori/ssu.dll,";
			func_line += *i;

			load_a_plugin(func_line);
		}
	}
}

void	ShioriPlugins::unload()
{
	for ( std::map<wstring, CallData>::iterator i=mCallData.begin() ; i!=mCallData.end() ; ++i ) {
		if ( i->second.mIsBasic )
			continue;
		DllData&	ddat = mDllData[i->second.mDllPath];
		if ( --ddat.mRefCount == 0 ) {
			ddat.m_pSaoriClient->unload();
			mDllData.erase(i->second.mDllPath);
		}
	}
	mCallData.clear();
}

wstring	ShioriPlugins::request(const wstring& iCallName, const strvec& iArguments, strvec& oResults, const wstring& iSecurityLevel) {

	if ( mCallData.find(iCallName) == mCallData.end() ) {
		GetSender().errsender() << iCallName + L": この呼び出し名は定義されていません。" << satori::endl;
		return	L"";
	}
	CallData&	theCallData = mCallData[iCallName];

	if ( theCallData.mIsBasic ) 
	{
		// SAORI-basicの呼び出し

		// さおべーはexternal呼び出しを判別する能力が無いので（環境変数等で渡せばいい？）
		// この時点で全て切ります。-universalはリクエスト時にsecurity levelヘッダを渡し、SAORI側で判別していただきます。
		if ( iSecurityLevel != L"local" && iSecurityLevel != L"Local" )
			return	L""; 

		wstring	theCommandLine = theCallData.mDllPath;
		strvec::const_iterator i;
		for ( i=theCallData.mPreDefinedArguments.begin() ; i!=theCallData.mPreDefinedArguments.end() ; ++i )
			theCommandLine += L" "+ *i;
		for ( i=iArguments.begin() ; i!=iArguments.end() ; ++i )
			theCommandLine += L" "+ *i;

		wstring out;
		wstring r = call_console_application(
			theCommandLine,
			get_folder_name(theCallData.mDllPath).c_str(),
			out);
		if ( r != L"" )
		{
			// エラー
			GetSender().errsender() << iCallName + L": " + r << satori::endl;
			return L"";
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

		std::vector<wstring> req;
		req.insert(req.end(), theCallData.mPreDefinedArguments.begin(), theCallData.mPreDefinedArguments.end());
		req.insert(req.end(), iArguments.begin(), iArguments.end());

		//---------------------
		// リクエスト実行

		assert( mDllData.find(theCallData.mDllPath) != mDllData.end() );

		wstring result;
		const wstring dll_label = mDllData[ theCallData.mDllPath ].mIsInternal ? wstring(L"(internal ssu)") : theCallData.mDllPath;
		int return_code = mDllData[ theCallData.mDllPath ].m_pSaoriClient->request(
			 req,
			 ( iSecurityLevel == L"local" || iSecurityLevel == L"Local" ),
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
			GetSender().errsender() << dll_label + L" - " + iCallName + L" : 400 Bad Request / 呼び出しの不備" << satori::endl;
			break;
		case 500:
			GetSender().errsender() << dll_label + L" - " + iCallName + L" : 500 Internal Server Error / saori内でのエラー" << satori::endl;
			break;
		default:
			GetSender().errsender() << dll_label + L" - " + iCallName + L" : " + itos(return_code) + L"? / 定義されていないステータスを返しました。" << satori::endl;
			break;
		}

		return	result;
	}
}


