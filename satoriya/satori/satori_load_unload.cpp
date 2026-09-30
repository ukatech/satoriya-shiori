#ifdef _MSC_VER 

	// マルチモニタ関連
	//#define WINVER 0x0500
	#include	<windows.h>
	#include	<multimon.h>
	#define SM_CXVIRTUALSCREEN      78
	#define SM_CYVIRTUALSCREEN      79

#endif	// _MSC_VER


#include	"satori.h"

#include	<fstream>
#include	<cassert>
#include      <locale.h>

#include	"../_/Utilities.h"
#include     "../_/random.h"

#include "posix_utils.h"
#include "charset.h"

#ifdef POSIX
#include <unistd.h>
#endif

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


//---------------------------------------------------------------------------
#ifndef POSIX
BOOL CALLBACK MonitorEnumFunc(HMONITOR hMonitor,HDC hdc,LPRECT rect,LPARAM lParam) {

    MONITORINFOEX MonitorInfoEx;
    MonitorInfoEx.cbSize=sizeof(MonitorInfoEx);

	static BOOL (WINAPI* pGetMonitorInfo)(HMONITOR,LPMONITORINFO) = NULL;
	if ( ! pGetMonitorInfo ) {
		(FARPROC&)pGetMonitorInfo = ::GetProcAddress(::GetModuleHandle(L"user32.dll"), "GetMonitorInfoW");
	}

	if ( pGetMonitorInfo==NULL )
		return	FALSE;
	if ( !(*pGetMonitorInfo)(hMonitor,&MonitorInfoEx) ) {
		GetSender().sender() << L"'GetMonitorInfo' was failed." << std::endl;
        return FALSE;
    }


	RECT* pRect = ((RECT*)lParam);

	if ( MonitorInfoEx.dwFlags & MONITORINFOF_PRIMARY ) {
		pRect[1] = *rect;
		GetSender().sender() << L"モニタ: " << MonitorInfoEx.szDevice << L" / (" << 
			rect->left << L"," << rect->top << L"," << rect->right << L"," << rect->bottom << L") / primary" << std::endl;
	}
	else {
		GetSender().sender() << L"モニタ: " << MonitorInfoEx.szDevice << L" / (" << 
			rect->left << L"," << rect->top << L"," << rect->right << L"," << rect->bottom << L") / extra" << std::endl;
	}

	RECT&	max_screen_rect = pRect[0];
	if ( rect->left < max_screen_rect.left )
		max_screen_rect.left = rect->left;
	if ( rect->top < max_screen_rect.top )
		max_screen_rect.top = rect->top;
	if ( rect->right > max_screen_rect.right )
		max_screen_rect.right = rect->right;
	if ( rect->bottom > max_screen_rect.bottom )
		max_screen_rect.bottom = rect->bottom;

    return TRUE;
}
#endif

//---------------------------------------------------------------------------

#ifdef	_DEBUG
#ifdef _WINDOWS

class DummyDbgSetFlagClass {
public:
	DummyDbgSetFlagClass(void) {
		int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
		tmpFlag |= _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
		tmpFlag &= ~_CRTDBG_CHECK_CRT_DF;
		_CrtSetDbgFlag( tmpFlag );
	}
};
DummyDbgSetFlagClass dummy;

#endif // _WINDOWS
#endif // _DEBUG


wstring	Satori::getversionlist(const wstring& iBaseFolder)
{
	return L"SHIORI/3.0\1SAORI/1.0";
}


bool	Satori::load(const wstring& iBaseFolder)
{
	GetSender().next_event();

	setlocale(LC_ALL, "Japanese");

	mBaseFolder = iBaseFolder;
	GetSender().sender() << L"■SATORI::Load on " << mBaseFolder << L"" << std::endl;

#if POSIX
	// 「/」で終わっていなければ付ける。
	if (!mBaseFolder.empty() && mBaseFolder[mBaseFolder.size() - 1] != L'/') {
	    mBaseFolder += L'/';
	}
#else
	// 「\」で終わっていなければ付ける。
	if (!mBaseFolder.empty() && mBaseFolder[mBaseFolder.size() - 1] != L'\\') {
	    mBaseFolder += L'\\';
	}
#endif


#ifdef	_MSC_VER
	// 本体のあるフォルダをサーチ
	{
		TCHAR	buf[MAX_PATH+1];
		buf[0] = L'\0';
		::GetModuleFileName(NULL, buf, MAX_PATH);
		buf[MAX_PATH] = L'\0';	// 長すぎて切り詰められたときは終端が付かない
		wchar_t*	p = FindFinalChar(buf, DIR_CHAR);
		if ( p==NULL )
			mExeFolder = L"";
		else {
			*(++p) = L'\0';
			mExeFolder = buf;
		}
	}
	GetSender().sender() << L"本体の所在: " << mExeFolder << L"" << std::endl;
#endif // _MSC_VER

	// メンバ初期化
	InitMembers();

#ifdef	_MSC_VER
	// システムの設定を読んでおく
    OSVERSIONINFO	ovi;
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionEx(&ovi);
	wstring	os;
	if ( ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
		if ( ovi.dwMinorVersion == 0 ) { mOSType=SATORI_OS_WIN95; os=L"Windows 95"; }
		else if ( ovi.dwMinorVersion == 10 ) { mOSType=SATORI_OS_WIN98; os=L"Windows 98"; }
		else if ( ovi.dwMinorVersion == 90 ) { mOSType=SATORI_OS_WINME; os=L"Windows Me"; }
		else { mOSType = SATORI_OS_UNDEFINED; os=L"undefined"; }
	} else {
		if ( ovi.dwMinorVersion == 0 ) {
			if ( ovi.dwMajorVersion == 4 ) { mOSType=SATORI_OS_WINNT; os=L"Windows NT"; }
			else if ( ovi.dwMajorVersion == 5 ) { mOSType=SATORI_OS_WIN2K; os=L"Windows 2000"; }
		}
		else { mOSType = SATORI_OS_WINXP; os=L"Windows XP or later"; }
	}
	GetSender().sender() << L"ＯＳ種別: " << os << std::endl;
	if ( mOSType==SATORI_OS_WIN95 ) {
		is_single_monitor = true;
	} else {
		BOOL (WINAPI* pEnumDisplayMonitors)(HDC,LPRECT,MONITORENUMPROC,LPARAM);
		(FARPROC&)pEnumDisplayMonitors = ::GetProcAddress(::GetModuleHandle(L"user32.dll"), "EnumDisplayMonitors");
		if ( pEnumDisplayMonitors==NULL ) {
			is_single_monitor = true;
		}
		else {
			RECT rectData[2];
			memset(rectData,0,sizeof(rectData));
			(*pEnumDisplayMonitors)(NULL,NULL,(MONITORENUMPROC)MonitorEnumFunc,(LPARAM)(rectData));

			max_screen_rect = rectData[0];
			desktop_rect = rectData[1];

			RECT*	rect;
			rect = &desktop_rect;
			GetSender().sender() << L"プライマリデスクトップ: (" << 
				rect->left << L"," << rect->top << L"," << rect->right << L"," << rect->bottom << L")" << std::endl;
			rect = &max_screen_rect;
			GetSender().sender() << L"仮想デスクトップ: (" << 
				rect->left << L"," << rect->top << L"," << rect->right << L"," << rect->bottom << L")" << std::endl;
			is_single_monitor = ( ::EqualRect(&max_screen_rect, &desktop_rect)!=FALSE );
			GetSender().sender() << (is_single_monitor ? 
				L"モニタは一つだけと判断、見切れ判定を呼び出し元に任せます。" : 
				L"複数のモニタが接続されていると判断、見切れ判定は里々が行います。") << std::endl;
		}
	}
#endif // _MSC_VER

	//config
	// is_utf8_* が真ならUTF-8として読む。偽なら文字コードを自動判定する。
	{
		strmap config;
		strmap_from_file(config, mBaseFolder+L"satori_bootconf.txt", L",");

		strmap::const_iterator j;

		is_utf8_dic = false;
		is_utf8_replace = false;
		is_utf8_savedata = false;
		is_utf8_charactersini = false;

		j = config.find(L"is_utf8_all");
		if ( j != config.end() && j->second.size()>0 ) {
			if ( stobool(j->second.c_str()) ) {
				is_utf8_dic = true;
				is_utf8_replace = true;
				is_utf8_savedata = true;
				is_utf8_charactersini = true;
			}
		}

		j = config.find(L"is_utf8_dic");
		if ( j != config.end() && j->second.size()>0 ) {
			is_utf8_dic = stobool(j->second.c_str());
		}

		j = config.find(L"is_utf8_replace");
		if ( j != config.end() && j->second.size()>0 ) {
			is_utf8_replace = stobool(j->second.c_str());
		}

		j = config.find(L"is_utf8_savedata");
		if ( j != config.end() && j->second.size()>0 ) {
			is_utf8_savedata = stobool(j->second.c_str());
		}

		j = config.find(L"is_utf8_charactersini");
		if ( j != config.end() && j->second.size()>0 ) {
			is_utf8_charactersini = stobool(j->second.c_str());
		}
	}

	// 置換辞書読み取り
	strmap_from_file(replace_before_dic, mBaseFolder+L"replace.txt", L"\t", L"#", is_utf8_replace ? CS_UTF8 : CS_NULL);
	strmap_from_file(replace_after_dic, mBaseFolder+L"replace_after.txt", L"\t", L"#", is_utf8_replace ? CS_UTF8 : CS_NULL);

	// キャラデータ読み込み
	mCharacters.load(mBaseFolder + L"characters.ini", is_utf8_charactersini ? CS_UTF8 : CS_NULL);
	for ( inimap::iterator i=mCharacters.begin() ; i!=mCharacters.end() ; ++i ) {

		const strmap& m = i->second;
		strmap::const_iterator j;

		// 置換辞書に追加
		j = m.find(L"popular-name");
		if ( j != m.end() && j->second.size()>0 ) 
			replace_before_dic[j->second + L"："] = wstring(INTERNAL_MARK_STR) + INTERNAL_MARK_SCOPE_STR + zen2han(i->first) + INTERNAL_MARK_STR; //INTERNAL_MARK SCOPE(数値) INTERNAL_MARK はあとで変換
		j = m.find(L"initial-letter");
		if ( j != m.end() && j->second.size()>0 ) 
			replace_before_dic[j->second + L"："] = wstring(INTERNAL_MARK_STR) + INTERNAL_MARK_SCOPE_STR + zen2han(i->first) + INTERNAL_MARK_STR; //INTERNAL_MARK SCOPE(数値) INTERNAL_MARK はあとで変換

		j = m.find(L"base-surface");
		if ( j != m.end() && j->second.size()>0 )
			system_variable_operation( wstring(L"サーフェス加算値") + i->first, j->second);
	}

	//for ( strmap::const_iterator j=replace_before_dic.begin() ; j!=replace_before_dic.end() ; ++j )
	//	cout << j->first << ": " << j->second << endl;

	// ランダマイズ
	randomize();


	//------------------------------------------

	// コンフィグ読み込み
	LoadDictionary(mBaseFolder + L"satori_conf.txt", false, is_utf8_dic);

	// 変数初期化実行
	GetSentence(L"初期化");	

	// SAORI読み込み
	Family<Word>* f = words.get_family(L"SAORI");

	mShioriPlugins->load(mBaseFolder);
	
	if ( f != NULL )
	{
		std::vector<const Word*> els;
		f->get_elements_pointers(els);

		for (std::vector<const Word*>::const_iterator i=els.begin(); i!=els.end() ; ++i)
		{
			if ( (*i)->size()>0 && !mShioriPlugins->load_a_plugin(**i) )
			{
				GetSender().sender() << L"SAORI読み込み中にエラーが発生: " << **i << std::endl;
			}
		}

	}
	mShioriPlugins->load_default_entry();

	talks.clear();
	words.clear();

	// 辞書拡張子・接頭辞
	if ( variables.find(L"辞書拡張子") != variables.end() ) {
		dic_load_ext = variables[L"辞書拡張子"];
	}
	if ( variables.find(L"辞書接頭辞") != variables.end() ) {
		dic_load_prefix = variables[L"辞書接頭辞"];
	}

	//------------------------------------------

	// セーブデータ読み込み
	//bool oldConf = fEncodeSavedata;

	bool loadResult = LoadDictionary(mBaseFolder + L"satori_savedata." + (fEncodeSavedata?L"sat":dic_load_ext.c_str()), false, is_utf8_savedata);
	GetSentence(L"セーブデータ");
	bool execResult = talks.get_family(L"セーブデータ") != NULL;

	if ( ! loadResult || ! execResult ) {
		loadResult = LoadDictionary(mBaseFolder + L"satori_savebackup." + (fEncodeSavedata?L"sat":dic_load_ext.c_str()), false, is_utf8_savedata);
		GetSentence(L"セーブデータ");
		execResult = talks.get_family(L"セーブデータ") != NULL;

		if ( ! loadResult || ! execResult ) {
			load_savedata_status = L"失敗";
		}
		else {
			load_savedata_status = L"バックアップ";
		}
	}
	else {
		load_savedata_status = L"正常";
	}

	talks.clear();
	
	reload_flag = false;

	if ( variables.find(L"ゴースト起動時間累計秒") != variables.end() ) {
		sec_count_total = zen2ul(variables[L"ゴースト起動時間累計秒"]);
	}
	else if ( variables.find(L"ゴースト起動時間累計ミリ秒") != variables.end() ) {
		sec_count_total = zen2ul(variables[L"ゴースト起動時間累計ミリ秒"]) / 1000;
	}
	else {
		sec_count_total = zen2ul(variables[L"ゴースト起動時間累計(ms)"]) / 1000;
	}
	variables[L"起動回数"] = itos( zen2int(variables[L"起動回数"])+1 );


	// 「単語の追加」で登録された単語を覚えておく
	const std::map< wstring, Family<Word> >& m = words.compatible();
	for (std::map< wstring, Family<Word> >::const_iterator it = m.begin() ; it != m.end() ; ++it )
	{
		std::vector<const Word*> v;
		it->second.get_elements_pointers(v);
		for (std::vector<const Word*>::const_iterator itx = v.begin() ; itx < v.end() ; ++itx ) {
			mAppendedWords[it->first].push_back(**itx);
		}
	}

	//------------------------------------------

	// 指定フォルダの辞書を読み込み
	int loadcount = 0;
	strvec::iterator i = dic_folder.begin();
	if ( i==dic_folder.end() ) {
		loadcount += LoadDicFolder(mBaseFolder);	// ルートフォルダの辞書
	} else {
		for ( ; i!=dic_folder.end() ; ++i )
			loadcount += LoadDicFolder(mBaseFolder + *i + DIR_CHAR);	// サブフォルダの辞書
	}

	is_dic_loaded = loadcount != 0;

	//------------------------------------------

	secure_flag = true;

	system_variable_operation(L"単語群「＊」の重複回避", L"有効、トーク中");
	system_variable_operation(L"文「＊」の重複回避", L"有効");
	//system_variable_operation("単語群「季節の食べ物」の重複回避", "有効、トーク中");

	GetSentence(L"OnSatoriLoad");
	on_loaded_script = GetSentence(L"OnSatoriBoot");
	diet_script(on_loaded_script);

	GetSender().sender() << L"loaded." << std::endl;

	GetSender().flush();
	return	true;
}


//---------------------------------------------------------------------------
// セーブデータは常にUTF-8で書く。暗号化はUTF-8のバイト列に対して行う。
#define	ENCODE(x)	(fEncodeSavedata ? encode(encode( WtoUTF8(x) )) : WtoUTF8(x) )

#ifdef POSIX
#  include <time.h>
#endif
bool	Satori::Save(bool isOnUnload) {
	GetSender().next_event();
	
	if ( isOnUnload ) {
		secure_flag = true;
		(void)GetSentence(L"OnSatoriUnload");
	}

	strmap savemap = variables;

	// メンバ変数を里々変数化
	for (std::map<int, wstring>::iterator it=reserved_talk.begin(); it!=reserved_talk.end() ; ++it)
		savemap[wstring(L"次から")+itos(it->first)+L"回目のトーク"] = it->second;

	// 起動時間累計を設定
	savemap[L"ゴースト起動時間累計秒"] =
	    uitos(posix_get_current_sec() - sec_count_at_load + sec_count_total,L"%lu");
	// (互換用)
	savemap[L"ゴースト起動時間累計ミリ秒"] =
	    uitos((posix_get_current_sec() - sec_count_at_load + sec_count_total)*1000,L"%lu");
	savemap[L"ゴースト起動時間累計(ms)"] =
	    uitos((posix_get_current_sec() - sec_count_at_load + sec_count_total)*1000,L"%lu");

	wstring	theFullPath = mBaseFolder + L"satori_savedata.tmp";

	std::string	out;
	bool	temp = GetSender().is_validated();
	GetSender().validate();
	GetSender().sender() << L"saving " << theFullPath << L"... " ;
	GetSender().validate(temp);

	wstring	line = L"＊セーブデータ";
	wstring  data;

	out += ENCODE(line);
	out += FILE_NEWLINE;
	for (strmap::const_iterator it= savemap.begin() ; it!= savemap.end() ; ++it) {
		wstring	key = zen2han(it->first);
		if ( key[0]==L'S' && aredigits(key.c_str()+1) ) {
			continue;
		}
		if ( key == L"今回は喋らない" || key == L"今回は会話時サーフェス戻し" || key == L"今回は会話時サーフィス戻し" || key == L"今回は自動アンカー" || key == L"今回は自動改行挿入" ) {
			continue;
		}
		if ( key.size()>const_strlen(L"タイマ") && compare_tail(key, L"タイマ") ) { //＄タイマ変数はセーブしない＝有効
			wstring	timer_name = strip_head_tail(key, L"", L"タイマ"); //「タイマ」を消す
			
			strintmap::const_iterator tm = timer_sec.find(timer_name);
			if ( tm != timer_sec.end() ) {
				if ( fDontSaveTimerValue ) {
					continue;
				}
			}

			if ( tm != timer_sec.end() && tm->second < 1 ) { continue; } //タイムアウト済なのでスキップ
		}

		data = it->second;
		
		replace(data,L"φ",L"φφ");
		replace(data,L"（",L"φ（");
		replace(data,L"）",L"φ）");
		m_escaper.unescape_for_dic(data);

		wstring	line = wstring(L"＄")+it->first+L"\t"+data; // 変数を保存
		out += ENCODE(line);
		out += FILE_NEWLINE;
	}

	for (std::map<wstring, std::vector<Word> >::const_iterator i=mAppendedWords.begin() ; i!=mAppendedWords.end() ; ++i )
	{
		if ( ! i->second.empty() ) {
			out += FILE_NEWLINE;
			out += ENCODE( wstring(L"＠") + i->first );
			out += FILE_NEWLINE;
			for (std::vector<Word>::const_iterator j=i->second.begin() ; j!=i->second.end() ; ++j )
			{
				data = *j;

				replace(data,L"φ",L"φφ");
				replace(data,L"（",L"φ（");
				replace(data,L"）",L"φ）");
				m_escaper.unescape_for_dic(data);

				out += ENCODE( data );
				out += FILE_NEWLINE;
			}
		}
	}

	if ( !bytes_to_file(out, theFullPath) )
	{
		GetSender().sender() << L"failed." << std::endl;
		return	false;
	}

	GetSender().sender() << L"ok." << std::endl;

	//バックアップ
	wstring	realFullPath = mBaseFolder + L"satori_savedata." + (fEncodeSavedata?L"sat":dic_load_ext.c_str());
	wstring	realFullPathBackup = mBaseFolder + L"satori_savebackup." + (fEncodeSavedata?L"sat":dic_load_ext.c_str());
#ifdef POSIX
	unlink(WtoUTF8(realFullPathBackup).c_str());
	rename(WtoUTF8(realFullPath).c_str(),WtoUTF8(realFullPathBackup).c_str());
	rename(WtoUTF8(theFullPath).c_str(),WtoUTF8(realFullPath).c_str());
#else
	::DeleteFile(realFullPathBackup.c_str());
	::MoveFile(realFullPath.c_str(),realFullPathBackup.c_str());
	::MoveFile(theFullPath.c_str(),realFullPath.c_str());
#endif

	//いらないほうを消す
	wstring	delFullPath = mBaseFolder + L"satori_savedata." + (fEncodeSavedata?dic_load_ext.c_str():L"sat");
	wstring	delFullPathBackup = mBaseFolder + L"satori_savebackup." + (fEncodeSavedata?dic_load_ext.c_str():L"sat");
#ifdef POSIX
	unlink(WtoUTF8(delFullPath).c_str());
	unlink(WtoUTF8(delFullPathBackup).c_str());
#else
	::DeleteFile(delFullPath.c_str());
	::DeleteFile(delFullPathBackup.c_str());
#endif

	return	true;
}

//---------------------------------------------------------------------------
bool	Satori::unload() {
	GetSender().next_event();

	// ファイルに保存
	if ( is_dic_loaded ) {
		this->Save(true);
	}
	is_dic_loaded = false;

	// プラグイン解放
	mShioriPlugins->unload();

	GetSender().sender() << L"■SATORI::Unload ---------------------" << std::endl;
	GetSender().flush();

	return	true;
}


