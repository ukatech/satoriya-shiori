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
#include	"satori.h"
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

#ifndef POSIX

// SAORI over DirectSSTP の互換モジュール（本体が内蔵するSAORI DLLの互換）。internal_ で始まるものは別に判定する
static const wchar_t* const direct_sstp_compat_modules[] = {
	L"houchi", L"clipcopy", L"textcopy", L"saori_ip", L"saori_cpuid",
	L"mciaudio", L"cpuusage", L"httpc", L"string", L"hanayu",
};

// DLLの実体が無く、名前が本体の内蔵モジュールのものなら、そのモジュール名（小文字、.dllなし）を返す。
// 実体があるときは、これまでどおりDLLとして読む。
static bool find_direct_sstp_module(const wstring& i_fullpath, wstring& o_module)
{
	if ( ! compare_tail_nocase(i_fullpath, L".dll") ) {
		return false;
	}
	const wchar_t* lastyen = wcsrchr(i_fullpath.c_str(), DIR_CHAR);
	wstring name = (lastyen != NULL) ? (lastyen + 1) : i_fullpath.c_str();
	name.erase(name.size() - const_strlen(L".dll"));
	for ( wstring::iterator i = name.begin() ; i != name.end() ; ++i ) {
		*i = towlower(*i);
	}

	bool is_module = compare_head(name, L"internal_");
	for ( size_t n = 0 ; ! is_module && n < sizeof(direct_sstp_compat_modules)/sizeof(direct_sstp_compat_modules[0]) ; ++n ) {
		is_module = (name == direct_sstp_compat_modules[n]);
	}
	if ( ! is_module ) {
		return false;
	}

	FILE*	fp = w_fopen(i_fullpath, L"rb");
	if ( fp != NULL ) {
		fclose(fp);
		return false;
	}
	o_module = name;
	return true;
}

// SaoriDirectClient から本体へ要求を送る
static bool send_saori_direct_sstp(void* i_context, const std::string& i_request, std::string& o_response)
{
	return static_cast<Satori*>(i_context)->SendSaoriDirectSSTP(i_request, o_response);
}

#endif // POSIX

// ssu の正規表現のサブルーチン呼び出しの名前を、（名前）として展開する
static bool resolve_ssu_regex_name(const wstring& i_name, wstring& o_result, void* i_context)
{
	return static_cast<Satori*>(i_context)->Call(i_name, o_result, false, true);
}

bool ShioriPlugins::load(const wstring& iBaseFolder)
{
	mBaseFolder = iBaseFolder;
	set_ssu_name_resolver(resolve_ssu_regex_name, pSatori);

	// mBaseFolderがスラッシュで終わっていなければ、付ける。
	if ( false == mBaseFolder.empty() && mBaseFolder[mBaseFolder.length() - 1] != DIR_CHAR)
	{
	    mBaseFolder += DIR_CHAR;
	}
#ifndef POSIX
	// 読み込み直したら、本体の内蔵モジュールは問い合わせ直す
	mInternalModulesState = IMS_UNKNOWN;
	mInternalModules.clear();
#endif
	return true;
}

#ifndef POSIX

// 本体に internal_system の ListModules を送り、内蔵モジュールの一覧を得る。
void	ShioriPlugins::fetch_internal_modules()
{
	if ( ! pSatori->CanSendSaoriDirectSSTP() ) {
		// 本体の窓がまだ分からない。次の呼び出しで再び試す
		return;
	}

	SaoriDirectClient client(L"internal_system", send_saori_direct_sstp, pSatori);
	const std::vector<wstring> args(1, L"ListModules");
	wstring result;
	std::vector<wstring> values;
	const int code = client.request(args, true, result, values);

	if ( code == 200 || code == 204 ) {
		if ( values.empty() && ! result.empty() ) {
			split(result, L",", values);
		}
		for ( std::vector<wstring>::const_iterator i = values.begin() ; i != values.end() ; ++i ) {
			wstring name = *i;
			for ( wstring::iterator c = name.begin() ; c != name.end() ; ++c ) {
				*c = towlower(*c);
			}
			mInternalModules.insert(name);
		}
		mInternalModulesState = IMS_LISTED;
	}
	else if ( code == 400 && client.error_description() == L"unknown module" ) {
		// SAORI over DirectSSTP には対応しているが、internal_system が無い本体
		mInternalModulesState = IMS_ANY;
	}
	else {
		GetSender().sender() << L"SAORI over DirectSSTP : 本体の内蔵モジュールを使えません。(" << client.error_description() << L")" << std::endl;
		mInternalModulesState = IMS_NONE;
	}
}

// iCallNameが本体の内蔵モジュール（internal_*）の名前なら、＠SAORIに
// 「iCallName,saori/<小文字の名前>.dll」と書いたのと同じように登録してtrueを返す。
bool	ShioriPlugins::register_internal_module(const wstring& iCallName)
{
	if ( ! compare_head_nocase(iCallName, L"internal_") ) {
		return false;
	}
	// モジュール名に使える文字だけ（ファイル名・＠SAORIの行として扱えないものを除く）
	wstring name = iCallName;
	for ( wstring::iterator c = name.begin() ; c != name.end() ; ++c ) {
		if ( !( (*c >= L'A' && *c <= L'Z') || (*c >= L'a' && *c <= L'z') || (*c >= L'0' && *c <= L'9') || *c == L'_' ) ) {
			return false;
		}
		*c = towlower(*c);
	}

	if ( mInternalModulesState == IMS_UNKNOWN ) {
		fetch_internal_modules();
	}
	switch ( mInternalModulesState ) {
	case IMS_LISTED:
		if ( mInternalModules.find(name) == mInternalModules.end() ) {
			return false;
		}
		break;
	case IMS_ANY:
		break;
	default:
		return false;
	}

	if ( ! load_a_plugin(iCallName + L",saori/" + name + L".dll") ) {
		return false;
	}
	GetSender().sender() << L"SAORI over DirectSSTP : " << iCallName << L" を本体の内蔵モジュールとして登録しました。" << std::endl;
	return mCallData.find(iCallName) != mCallData.end();
}

#endif // POSIX

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
#ifndef POSIX
	wstring direct_module;
#endif

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
		if ( compare_tail_nocase(fullpath, L"\\ssu.dll") || compare_tail_nocase(fullpath, L"/ssu.dll") )
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
#ifndef POSIX
		else if ( find_direct_sstp_module(fullpath, direct_module) ) {
			// SAORI over DirectSSTP：本体が内蔵するモジュール。呼び出しのたびに本体へ要求を送る
			mDllData[fullpath].mRefCount=1;
			mDllData[fullpath].m_pSaoriClient=new SaoriDirectClient(direct_module, send_saori_direct_sstp, pSatori);
			mDllData[fullpath].mDirectModule = direct_module;
		}
#endif
		else {
#ifndef POSIX
			// ネットワーク更新時、SAORI.dllが上書きできずdl2として保存される問題に暫定対処
			if ( compare_tail_nocase(fullpath, L".dll") )
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
			if ( compare_tail_nocase(fullpath, L".dll") )
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
					else if ( mDllData[fullpath].m_pSaoriClient->version_reply_has_charset() ) {
						// GET Versionの応答のCharsetが、送った値と違うときは、SAORIが対応する文字コードを知らせている。
						// 要求のCharsetを見ずに常にShift_JISで読み書きするSAORI（葉梨のhanasi.dllなど）に
						// UTF-8で送ると文字化けするので、応答の文字コードに合わせて送る。
						CharactorSet reply_cs = CharsetFromName(mDllData[fullpath].m_pSaoriClient->version_reply_charset());
						if ( reply_cs != CS_NULL
						  && reply_cs != CharsetFromName(mDllData[fullpath].m_pSaoriClient->charset()) ) {
							mDllData[fullpath].m_pSaoriClient->set_charset(CharsetName(reply_cs));
						}
					}
					else {
						// GET Versionの応答にCharsetを返さないSAORIは、Charsetヘッダを見ずに
						// Shift_JISとして読むもの（華和梨のSAORIモードなど）があるので、master版と同じくShift_JISで送る。
						mDllData[fullpath].m_pSaoriClient->set_charset(L"Shift_JIS");

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
	mCallData[vec[0]].mIsBasic = !compare_tail_nocase(fullpath, L".dll");
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
	set_ssu_name_resolver(NULL, NULL);
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

		// パスに空白があっても別のファイルと取り違えないよう、引用符で囲む
		wstring	theCommandLine = L"\"" + theCallData.mDllPath + L"\"";
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
		const DllData& dll_data = mDllData[ theCallData.mDllPath ];
		wstring dll_label = theCallData.mDllPath;
		if ( dll_data.mIsInternal ) {
			dll_label = L"(internal ssu)";
		}
		else if ( ! dll_data.mDirectModule.empty() ) {
			dll_label = L"(DirectSSTP " + dll_data.mDirectModule + L")";
		}
		// 正規表現の（名前）の解決先はプロセスで1つなので、呼ぶたびにこの里々を指す（POSIX は複数の実体を持てる）
		set_ssu_name_resolver(resolve_ssu_regex_name, pSatori);
		int return_code = mDllData[ theCallData.mDllPath ].m_pSaoriClient->request(
			 req,
			 ( iSecurityLevel == L"local" || iSecurityLevel == L"Local" ),
			 result,
			 oResults);

		//---------------------
		// 返答に対処

		// ErrorDescriptionがあれば、ログに添える（SAORI over DirectSSTPの内蔵モジュールなど）
		const wstring error_description = mDllData[ theCallData.mDllPath ].m_pSaoriClient->error_description();
		const wstring error_suffix = error_description.empty() ? wstring() : (L" (" + error_description + L")");

		switch (return_code)
		{
		case 200:
		case 204:
			break;
		case 400:
			GetSender().errsender() << dll_label + L" - " + iCallName + L" : 400 Bad Request / 呼び出しの不備" + error_suffix << satori::endl;
			break;
		case 500:
			GetSender().errsender() << dll_label + L" - " + iCallName + L" : 500 Internal Server Error / saori内でのエラー" + error_suffix << satori::endl;
			break;
		default:
			GetSender().errsender() << dll_label + L" - " + iCallName + L" : " + itos(return_code) + L"? / 定義されていないステータスを返しました。" << satori::endl;
			break;
		}

		return	result;
	}
}


