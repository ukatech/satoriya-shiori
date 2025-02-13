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
		(FARPROC&)pGetMonitorInfo = ::GetProcAddress(::GetModuleHandle("user32.dll"), "GetMonitorInfoA");
	}

	if ( pGetMonitorInfo==NULL )
		return	FALSE;
	if ( !(*pGetMonitorInfo)(hMonitor,&MonitorInfoEx) ) {
		GetSender().sender() << "'GetMonitorInfo' was failed." << std::endl;
        return FALSE;
    }


	RECT* pRect = ((RECT*)lParam);

	if ( MonitorInfoEx.dwFlags & MONITORINFOF_PRIMARY ) {
		pRect[1] = *rect;
		GetSender().sender() << "モニタ: " << MonitorInfoEx.szDevice << " / (" << 
			rect->left << "," << rect->top << "," << rect->right << "," << rect->bottom << ") / primary" << std::endl;
	}
	else {
		GetSender().sender() << "モニタ: " << MonitorInfoEx.szDevice << " / (" << 
			rect->left << "," << rect->top << "," << rect->right << "," << rect->bottom << ") / extra" << std::endl;
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


string	Satori::getversionlist(const string& iBaseFolder)
{
	return "SHIORI/3.0\1SAORI/1.0";
}


bool	Satori::load(const string& iBaseFolder)
{
	GetSender().next_event();

	setlocale(LC_ALL, "Japanese");
#ifdef _WINDOWS
	_setmbcp(_MB_CP_LOCALE);
#endif

	mBaseFolder = iBaseFolder;
	GetSender().sender() << "■SATORI::Load on " << mBaseFolder << "" << std::endl;

#if POSIX
	// 「/」で終わっていなければ付ける。
	if (mBaseFolder[mBaseFolder.size() - 1] != '/') {
	    mBaseFolder += '/';
	}
#else
	// 「\」で終わっていなければ付ける。
	if (mBaseFolder[mBaseFolder.size() - 1] != '\\') {
	    mBaseFolder += '\\';
	}
#endif


#ifdef	_MSC_VER
	// 本体のあるフォルダをサーチ
	{
		TCHAR	buf[MAX_PATH+1];
		::GetModuleFileName(NULL, buf, MAX_PATH);
		char*	p = FindFinalChar(buf, DIR_CHAR);
		if ( p==NULL )
			mExeFolder = "";
		else {
			*(++p) = '\0';
			mExeFolder = buf;
		}
	}
	GetSender().sender() << "本体の所在: " << mExeFolder << "" << std::endl;
#endif // _MSC_VER

	// メンバ初期化
	InitMembers();

#ifdef	_MSC_VER
	// システムの設定を読んでおく
    OSVERSIONINFO	ovi;
    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	::GetVersionEx(&ovi);
	string	os;
	if ( ovi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) {
		if ( ovi.dwMinorVersion == 0 ) { mOSType=SATORI_OS_WIN95; os="Windows 95"; }
		else if ( ovi.dwMinorVersion == 10 ) { mOSType=SATORI_OS_WIN98; os="Windows 98"; }
		else if ( ovi.dwMinorVersion == 90 ) { mOSType=SATORI_OS_WINME; os="Windows Me"; }
		else { mOSType = SATORI_OS_UNDEFINED; os="undefined"; }
	} else {
		if ( ovi.dwMinorVersion == 0 ) {
			if ( ovi.dwMajorVersion == 4 ) { mOSType=SATORI_OS_WINNT; os="Windows NT"; }
			else if ( ovi.dwMajorVersion == 5 ) { mOSType=SATORI_OS_WIN2K; os="Windows 2000"; }
		}
		else { mOSType = SATORI_OS_WINXP; os="Windows XP or later"; }
	}
	GetSender().sender() << "ＯＳ種別: " << os << std::endl;
	if ( mOSType==SATORI_OS_WIN95 ) {
		is_single_monitor = true;
	} else {
		BOOL (WINAPI* pEnumDisplayMonitors)(HDC,LPRECT,MONITORENUMPROC,LPARAM);
		(FARPROC&)pEnumDisplayMonitors = ::GetProcAddress(::GetModuleHandle("user32.dll"), "EnumDisplayMonitors");
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
			GetSender().sender() << "プライマリデスクトップ: (" << 
				rect->left << "," << rect->top << "," << rect->right << "," << rect->bottom << ")" << std::endl;
			rect = &max_screen_rect;
			GetSender().sender() << "仮想デスクトップ: (" << 
				rect->left << "," << rect->top << "," << rect->right << "," << rect->bottom << ")" << std::endl;
			is_single_monitor = ( ::EqualRect(&max_screen_rect, &desktop_rect)!=FALSE );
			GetSender().sender() << (is_single_monitor ? 
				"モニタは一つだけと判断、見切れ判定を呼び出し元に任せます。" : 
				"複数のモニタが接続されていると判断、見切れ判定は里々が行います。") << std::endl;
		}
	}
#endif // _MSC_VER

	//config
	{
		strmap config;
		strmap_from_file(config, mBaseFolder+"satori_bootconf.txt", ",");

		strmap::const_iterator j;

		is_utf8_dic = false;
		is_utf8_replace = false;
		is_utf8_savedata = false;
		is_utf8_charactersini = false;

		j = config.find("is_utf8_all");
		if ( j != config.end() && j->second.size()>0 ) {
			if ( stobool(j->second.c_str()) ) {
				is_utf8_dic = true;
				is_utf8_replace = true;
				is_utf8_savedata = true;
				is_utf8_charactersini = true;
			}
		}

		j = config.find("is_utf8_dic");
		if ( j != config.end() && j->second.size()>0 ) {
			is_utf8_dic = stobool(j->second.c_str());
		}

		j = config.find("is_utf8_replace");
		if ( j != config.end() && j->second.size()>0 ) {
			is_utf8_replace = stobool(j->second.c_str());
		}

		j = config.find("is_utf8_savedata");
		if ( j != config.end() && j->second.size()>0 ) {
			is_utf8_savedata = stobool(j->second.c_str());
		}

		j = config.find("is_utf8_charactersini");
		if ( j != config.end() && j->second.size()>0 ) {
			is_utf8_charactersini = stobool(j->second.c_str());
		}
	}

	// 置換辞書読み取り
	strmap_from_file(replace_before_dic, mBaseFolder+"replace.txt", "\t");
	strmap_from_file(replace_after_dic, mBaseFolder+"replace_after.txt", "\t");

	if ( is_utf8_replace ) {
		convert_utf8_to_sjis_strmap(replace_before_dic);
		convert_utf8_to_sjis_strmap(replace_after_dic);
	}

	// キャラデータ読み込み
	mCharacters.load(mBaseFolder + "characters.ini");
	for ( inimap::iterator i=mCharacters.begin() ; i!=mCharacters.end() ; ++i ) {

		if ( is_utf8_charactersini ) {
			convert_utf8_to_sjis_strmap(i->second);
		}

		const strmap& m = i->second;
		strmap::const_iterator j;

		// 置換辞書に追加
		j = m.find("popular-name");
		if ( j != m.end() && j->second.size()>0 ) 
			replace_before_dic[j->second + "："] = string("\xff\x01") + zen2han(i->first) + "\xff"; //0xff0x01(数値)0x01はあとで変換
		j = m.find("initial-letter");
		if ( j != m.end() && j->second.size()>0 ) 
			replace_before_dic[j->second + "："] = string("\xff\x01") + zen2han(i->first) + "\xff"; //0xff0x01(数値)0x01はあとで変換

		j = m.find("base-surface");
		if ( j != m.end() && j->second.size()>0 )
			system_variable_operation( string("サーフェス加算値") + i->first, j->second);
	}

	//for ( strmap::const_iterator j=replace_before_dic.begin() ; j!=replace_before_dic.end() ; ++j )
	//	cout << j->first << ": " << j->second << endl;

	// ランダマイズ
	randomize();


	//------------------------------------------

	// コンフィグ読み込み
	LoadDictionary(mBaseFolder + "satori_conf.txt", false, is_utf8_dic);

	// 変数初期化実行
	GetSentence("初期化");	

	// SAORI読み込み
	Family<Word>* f = words.get_family("SAORI");

	mShioriPlugins->load(mBaseFolder);
	
	if ( f != NULL )
	{
		std::vector<const Word*> els;
		f->get_elements_pointers(els);

		for (std::vector<const Word*>::const_iterator i=els.begin(); i!=els.end() ; ++i)
		{
			if ( (*i)->size()>0 && !mShioriPlugins->load_a_plugin(**i) )
			{
				GetSender().sender() << "SAORI読み込み中にエラーが発生: " << **i << std::endl;
			}
		}

	}
	mShioriPlugins->load_default_entry();

	talks.clear();
	words.clear();

	// 辞書拡張子・接頭辞
	if ( variables.find("辞書拡張子") != variables.end() ) {
		dic_load_ext = variables["辞書拡張子"];
	}
	if ( variables.find("辞書接頭辞") != variables.end() ) {
		dic_load_prefix = variables["辞書接頭辞"];
	}

	//------------------------------------------

	// セーブデータ読み込み
	//bool oldConf = fEncodeSavedata;

	bool loadResult = LoadDictionary(mBaseFolder + "satori_savedata." + (fEncodeSavedata?"sat":dic_load_ext.c_str()), false, is_utf8_savedata);
	GetSentence("セーブデータ");
	bool execResult = talks.get_family("セーブデータ") != NULL;

	if ( ! loadResult || ! execResult ) {
		loadResult = LoadDictionary(mBaseFolder + "satori_savebackup." + (fEncodeSavedata?"sat":dic_load_ext.c_str()), false, is_utf8_savedata);
		GetSentence("セーブデータ");
		execResult = talks.get_family("セーブデータ") != NULL;

		if ( ! loadResult || ! execResult ) {
			load_savedata_status = "失敗";
		}
		else {
			load_savedata_status = "バックアップ";
		}
	}
	else {
		load_savedata_status = "正常";
	}

	talks.clear();
	
	reload_flag = false;

	if ( variables.find("ゴースト起動時間累計秒") != variables.end() ) {
		sec_count_total = zen2ul(variables["ゴースト起動時間累計秒"]);
	}
	else if ( variables.find("ゴースト起動時間累計ミリ秒") != variables.end() ) {
		sec_count_total = zen2ul(variables["ゴースト起動時間累計ミリ秒"]) / 1000;
	}
	else {
		sec_count_total = zen2ul(variables["ゴースト起動時間累計(ms)"]) / 1000;
	}
	variables["起動回数"] = itos( zen2int(variables["起動回数"])+1 );


	// 「単語の追加」で登録された単語を覚えておく
	const std::map< string, Family<Word> >& m = words.compatible();
	for (std::map< string, Family<Word> >::const_iterator it = m.begin() ; it != m.end() ; ++it )
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

	system_variable_operation("単語群「＊」の重複回避", "有効、トーク中");
	system_variable_operation("文「＊」の重複回避", "有効");
	//system_variable_operation("単語群「季節の食べ物」の重複回避", "有効、トーク中");

	GetSentence("OnSatoriLoad");
	on_loaded_script = GetSentence("OnSatoriBoot");
	diet_script(on_loaded_script);

	GetSender().sender() << "loaded." << std::endl;

	GetSender().flush();
	return	true;
}


//---------------------------------------------------------------------------
#define	ENCODE(x)	(fEncodeSavedata ? encode(encode( (is_utf8_savedata ? SJIStoUTF8(x) : x) )) : (is_utf8_savedata ? SJIStoUTF8(x) : x) )

#ifdef POSIX
#  include <time.h>
#endif
bool	Satori::Save(bool isOnUnload) {
	GetSender().next_event();
	
	if ( isOnUnload ) {
		secure_flag = true;
		(void)GetSentence("OnSatoriUnload");
	}

	strmap savemap = variables;

	// メンバ変数を里々変数化
	for (std::map<int, string>::iterator it=reserved_talk.begin(); it!=reserved_talk.end() ; ++it)
		savemap[string("次から")+itos(it->first)+"回目のトーク"] = it->second;

	// 起動時間累計を設定
	savemap["ゴースト起動時間累計秒"] =
	    uitos(posix_get_current_sec() - sec_count_at_load + sec_count_total,"%lu");
	// (互換用)
	savemap["ゴースト起動時間累計ミリ秒"] =
	    uitos((posix_get_current_sec() - sec_count_at_load + sec_count_total)*1000,"%lu");
	savemap["ゴースト起動時間累計(ms)"] =
	    uitos((posix_get_current_sec() - sec_count_at_load + sec_count_total)*1000,"%lu");

	string	theFullPath = mBaseFolder + "satori_savedata.tmp";

	std::ofstream	out(theFullPath.c_str());
	bool	temp = GetSender().is_validated();
	GetSender().validate();
	GetSender().sender() << "saving " << theFullPath << "... " ;
	GetSender().validate(temp);
	if ( !out.is_open() )
	{
		GetSender().sender() << "failed." << std::endl;
		return	false;
	}

	string	line = "＊セーブデータ";
	string  data;

	out << ENCODE(line) << std::endl;
	for (strmap::const_iterator it= savemap.begin() ; it!= savemap.end() ; ++it) {
		string	key = zen2han(it->first);
		if ( key[0]=='S' && aredigits(key.c_str()+1) ) {
			continue;
		}
		if ( key == "今回は喋らない" || key == "今回は会話時サーフェス戻し" || key == "今回は会話時サーフィス戻し" || key == "今回は自動アンカー" || key == "今回は自動改行挿入" ) {
			continue;
		}
		if ( key.size()>6 && compare_tail(key, "タイマ") ) { //＄タイマ変数はセーブしない＝有効
			string	timer_name(key.c_str(), strlen(key.c_str())-6); //「タイマ」を消す
			
			strintmap::const_iterator tm = timer_sec.find(timer_name);
			if ( tm != timer_sec.end() ) {
				if ( fDontSaveTimerValue ) {
					continue;
				}
			}

			if ( tm->second < 1 ) { continue; } //タイムアウト済なのでスキップ
		}

		data = it->second;
		
		replace(data,"φ","φφ");
		replace(data,"（","φ（");
		replace(data,"）","φ）");
		m_escaper.unescape_for_dic(data);

		string	line = string("＄")+it->first+"\t"+data; // 変数を保存
		out << ENCODE(line) << std::endl;
	}

	for (std::map<string, std::vector<Word> >::const_iterator i=mAppendedWords.begin() ; i!=mAppendedWords.end() ; ++i )
	{
		if ( ! i->second.empty() ) {
			out << std::endl << ENCODE( string("＠") + i->first ) << std::endl;
			for (std::vector<Word>::const_iterator j=i->second.begin() ; j!=i->second.end() ; ++j )
			{
				data = *j;

				replace(data,"φ","φφ");
				replace(data,"（","φ（");
				replace(data,"）","φ）");
				m_escaper.unescape_for_dic(data);

				out << ENCODE( data ) << std::endl;
			}
		}
	}

	out.flush();
	out.close();

	GetSender().sender() << "ok." << std::endl;

	//バックアップ
	string	realFullPath = mBaseFolder + "satori_savedata." + (fEncodeSavedata?"sat":dic_load_ext.c_str());
	string	realFullPathBackup = mBaseFolder + "satori_savebackup." + (fEncodeSavedata?"sat":dic_load_ext.c_str());
#ifdef POSIX
	unlink(realFullPathBackup.c_str());
	rename(realFullPath.c_str(),realFullPathBackup.c_str());
	rename(theFullPath.c_str(),realFullPath.c_str());
#else
	::DeleteFile(realFullPathBackup.c_str());
	::MoveFile(realFullPath.c_str(),realFullPathBackup.c_str());
	::MoveFile(theFullPath.c_str(),realFullPath.c_str());
#endif

	//いらないほうを消す
	string	delFullPath = mBaseFolder + "satori_savedata." + (fEncodeSavedata?dic_load_ext.c_str():"sat");
	string	delFullPathBackup = mBaseFolder + "satori_savebackup." + (fEncodeSavedata?dic_load_ext.c_str():"sat");
#ifdef POSIX
	unlink(delFullPath.c_str());
	unlink(delFullPathBackup.c_str());
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

	GetSender().sender() << "■SATORI::Unload ---------------------" << std::endl;
	GetSender().flush();

	return	true;
}


