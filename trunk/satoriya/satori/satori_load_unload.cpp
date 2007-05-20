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
#include	<ctime>	// for randomize

#include	"../_/Utilities.h"

#ifdef POSIX
#include "posix_utils.h"
#include <unistd.h>
#endif

//////////DEBUG/////////////////////////
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

	BOOL (WINAPI* pGetMonitorInfo)(HMONITOR,LPMONITORINFO);
	(FARPROC&)pGetMonitorInfo = ::GetProcAddress(::LoadLibrary("user32.dll"), "GetMonitorInfoA");
	if ( pGetMonitorInfo==NULL )
		return	FALSE;
	if ( !(*pGetMonitorInfo)(hMonitor,&MonitorInfoEx) ) {
		sender << "'GetMonitorInfo' was failed." << endl;
        return FALSE;
    }

	sender << "モニタ: " << MonitorInfoEx.szDevice << " / (" << 
		rect->left << "," << rect->top << "," << rect->right << "," << rect->bottom << ") / " <<
		((MonitorInfoEx.dwFlags==MONITORINFOF_PRIMARY) ? "primary" : "extra") << endl;

	RECT&	max_screen_rect = *((RECT*)lParam);
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

bool	Satori::load(const string& iBaseFolder)
{
	Sender::initialize();

	mBaseFolder = iBaseFolder;
	sender << "■SATORI::Load on " << mBaseFolder << "" << endl;

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
	sender << "本体の所在: " << mExeFolder << "" << endl;
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
		if ( ovi.dwMinorVersion == 0 ) { mOSType=WIN95; os="Windows 95"; }
		else if ( ovi.dwMinorVersion == 10 ) { mOSType=WIN98; os="Windows 98"; }
		else if ( ovi.dwMinorVersion == 90 ) { mOSType=WINME; os="Windows Me"; }
		else { mOSType = UNDEFINED; os="undefined"; }
	} else {
		if ( ovi.dwMinorVersion == 0 ) {
			if ( ovi.dwMajorVersion == 4 ) { mOSType=WINNT; os="Windows NT"; }
			else if ( ovi.dwMajorVersion == 5 ) { mOSType=WIN2K; os="Windows 2000"; }
		}
		else { mOSType = WINXP; os="Windows XP or later"; }
	}
	sender << "ＯＳ種別: " << os << endl;
	if ( mOSType==WIN95 ) {
		is_single_monitor = true;
	} else {
		BOOL (WINAPI* pEnumDisplayMonitors)(HDC,LPRECT,MONITORENUMPROC,LPARAM);
		(FARPROC&)pEnumDisplayMonitors = ::GetProcAddress(::LoadLibrary("user32.dll"), "EnumDisplayMonitors");
		if ( pEnumDisplayMonitors==NULL ) {
			is_single_monitor = true;
		}
		else {
			(*pEnumDisplayMonitors)(NULL,NULL,(MONITORENUMPROC)MonitorEnumFunc,(LPARAM)(&max_screen_rect));
			::GetWindowRect(::GetDesktopWindow(), &desktop_rect);
			RECT*	rect;
			rect = &desktop_rect;
			sender << "プライマリデスクトップ: (" << 
				rect->left << "," << rect->top << "," << rect->right << "," << rect->bottom << ")" << endl;
			rect = &max_screen_rect;
			sender << "仮想デスクトップ: (" << 
				rect->left << "," << rect->top << "," << rect->right << "," << rect->bottom << ")" << endl;
			is_single_monitor = ( ::EqualRect(&max_screen_rect, &desktop_rect)!=FALSE );
			sender << (is_single_monitor ? 
				"モニタは一つだけと判断、見切れ判定を呼び出し元に任せます。" : 
				"複数のモニタが接続されていると判断、見切れ判定は里々が行います。") << endl;
		}
	}
#endif // _MSC_VER

	// 置換辞書読み取り
	strmap_from_file(replace_before_dic, mBaseFolder+"replace.txt", "\t");
	strmap_from_file(replace_after_dic, mBaseFolder+"replace_after.txt", "\t");

	// キャラデータ読み込み
	mCharacters.load(mBaseFolder + "characters.ini");
	for ( inimap::const_iterator i=mCharacters.begin() ; i!=mCharacters.end() ; ++i ) {
		const strmap& m = i->second;
		strmap::const_iterator j;

		// 置換辞書に追加
		j = m.find("popular-name");
		if ( j != m.end() && j->second.size()>0 ) 
			replace_before_dic[j->second + "："] = string("\xff\x01") + zen2han(i->first); //0xff0x01はあとで変換
		j = m.find("initial-letter");
		if ( j != m.end() && j->second.size()>0 ) 
			replace_before_dic[j->second + "："] = string("\xff\x01") + zen2han(i->first); //0xff0x01はあとで変換

		j = m.find("base-surface");
		if ( j != m.end() && j->second.size()>0 )
			system_variable_operation( string("サーフェス加算値") + i->first, j->second);
	}

	//for ( strmap::const_iterator j=replace_before_dic.begin() ; j!=replace_before_dic.end() ; ++j )
	//	cout << j->first << ": " << j->second << endl;

	// ランダマイズ
	randomize(time(NULL));


	//------------------------------------------

	// コンフィグ読み込み
	LoadDictionary(mBaseFolder + "satori_conf.txt", false);

	// 変数初期化実行
	GetSentence("初期化");	

	// SAORI読み込み
	Family<Word>* f = words.get_family("SAORI");
	if ( f != NULL )
	{
		list<const Word*> els;
		f->get_elements_pointers(els);

		mShioriPlugins.load(mBaseFolder);
		for ( list<const Word*>::const_iterator i=els.begin(); i!=els.end() ; ++i)
		{
			if ( (*i)->size()>0 && !mShioriPlugins.load_a_plugin(**i) )
			{
				sender << "SAORI読み込み中にエラーが発生: " << **i << endl;
			}
		}
	}
	talks.clear();
	words.clear();

	//------------------------------------------

	// セーブデータ読み込み
	bool oldConf = fEncodeSavedata;

	bool loadResult = LoadDictionary(mBaseFolder + "satori_savedata." + (fEncodeSavedata?"sat":"txt"), false);
	GetSentence("セーブデータ");
	bool execResult = talks.get_family("セーブデータ") != NULL;

	if ( ! loadResult || ! execResult ) {
		loadResult = LoadDictionary(mBaseFolder + "satori_savebackup." + (fEncodeSavedata?"sat":"txt"), false);
		GetSentence("セーブデータ");
		execResult = talks.get_family("セーブデータ") != NULL;
	}

	talks.clear();
	
	reload_flag = false;

	tick_count_total = stoi(variables["ゴースト起動時間累計(ms)"]);
	variables["起動回数"] = itos( stoi(variables["起動回数"])+1 );

	// 「単語の追加」で登録された単語を覚えておく
	const map< string, Family<Word> >& m = words.compatible();
	for ( map< string, Family<Word> >::const_iterator it = m.begin() ; it != m.end() ; ++it )
	{
		vector<const Word*> v;
		it->second.get_elements_pointers(v);
		mAppendedWords[it->first] = v;
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

	sender << "loaded." << endl;
	return	true;
}


//---------------------------------------------------------------------------
#define	ENCODE(x)	(fEncodeSavedata ? encode(encode(x)) : (x))

#ifdef POSIX
#  include <time.h>
#endif
bool	Satori::Save(bool isOnUnload) {

	// メンバ変数を里々変数化
	for (map<int, string>::iterator it=reserved_talk.begin(); it!=reserved_talk.end() ; ++it)
		variables[string("次から")+itos(it->first)+"回目のトーク"] = it->second;
	// 起動時間累計を設定
#ifdef POSIX
	variables["ゴースト起動時間累計(ms)"] =
	    itos(posix_get_current_millis() - tick_count_at_load + tick_count_total);
#else
	variables["ゴースト起動時間累計(ms)"] = itos( ::GetTickCount() - tick_count_at_load + tick_count_total );
#endif

	if ( isOnUnload ) {
		secure_flag = true;
		(void)GetSentence("OnSatoriUnload");
	}

	string	theFullPath = mBaseFolder + "satori_savedata.tmp";

	ofstream	out(theFullPath.c_str());
	bool	temp = Sender::is_validated();
	Sender::validate();
	sender << "saving " << theFullPath << "... " ;
	Sender::validate(temp);
	if ( !out.is_open() )
	{
		sender << "failed." << endl;
		return	false;
	}
	string	line = "＊セーブデータ";
	out << ENCODE(line) << endl;
	for (strmap::const_iterator it=variables.begin() ; it!=variables.end() ; ++it) {
		string	zen2han(string str);
		string	str = zen2han(it->first);
		if ( str[0]=='S' && aredigits(str.c_str()+1) )
			continue;
		string	line = string("＄")+it->first+"\t"+it->second; // 変数を保存
		out << ENCODE(line) << endl;
	}

	for ( map<string, vector<const Word*> >::const_iterator i=mAppendedWords.begin() ; i!=mAppendedWords.end() ; ++i )
	{
		out << endl << ENCODE( string("＠") + i->first ) << endl;
		for ( vector<const Word*>::const_iterator j=i->second.begin() ; j!=i->second.end() ; ++j )
		{
			out << ENCODE(**j) << endl;
		}
	}

	out.flush();
	out.close();

	sender << "ok." << endl;

	//バックアップ
	string	realFullPath = mBaseFolder + "satori_savedata." + (fEncodeSavedata?"sat":"txt");
	string	realFullPathBackup = mBaseFolder + "satori_savebackup." + (fEncodeSavedata?"sat":"txt");
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
	string	delFullPath = mBaseFolder + "satori_savedata." + (fEncodeSavedata?"txt":"sat");
	string	delFullPathBackup = mBaseFolder + "satori_savebackup." + (fEncodeSavedata?"txt":"sat");
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

	// ファイルに保存
	if ( is_dic_loaded ) {
		this->Save(true);
	}
	is_dic_loaded = false;

	// プラグイン解放
	mShioriPlugins.unload();

	sender << "■SATORI::Unload ---------------------" << endl;
	return	true;
}

