#include	"satori.h"
#include	"../_/Utilities.h"


#include	<fstream>
#include	<cassert>
#include <algorithm>

#ifdef POSIX
#  include <iostream>
#include <sys/stat.h>
using std::min;
using std::max;
#endif

#include "random.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


#ifndef POSIX
// ファイルの最終更新日時を取得
bool	GetLastWriteTime(LPCWSTR iFileName, SYSTEMTIME& oSystemTime) {
	HANDLE	theFile = ::CreateFile( iFileName, 
		GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( theFile==INVALID_HANDLE_VALUE )
		return	false;
	
	BY_HANDLE_FILE_INFORMATION	theInfo;
	::GetFileInformationByHandle(theFile, &theInfo);
	::CloseHandle(theFile);
	
	FILETIME	FileTime;
	::FileTimeToLocalFileTime(&(theInfo.ftLastWriteTime), &FileTime);
	::FileTimeToSystemTime(&FileTime, &oSystemTime);
	return	true;
}
#endif

//----------------------------------------------------------------------
//	ファイルが存在するかチェック
//----------------------------------------------------------------------
#ifdef POSIX
bool FileExist(const wstring& f)
{
    struct stat s;
    return ::stat(WtoUTF8(f).c_str(), &s) == 0;
}
#else
bool FileExist(const wstring& f)
{
	return ::GetFileAttributes(f.c_str()) != 0xFFFFFFFFU;
}
#endif

#if 0
//----------------------------------------------------------------------
//	ファイルの更新日時を比較。
//	返値が正ならば前者、負ならば後者のほうが新しいファイル。
//----------------------------------------------------------------------
#ifdef POSIX
#include <sys/types.h>
#include <sys/stat.h>
int CompareTime(const wstring& file1, const wstring& file2) {
    // file1の方が新しければ1、同じなら0、古ければ-1。
    struct stat s1, s2;
    int r1 = ::stat(file1.c_str(), &s1);
    int r2 = ::stat(file2.c_str(), &s2);
    if (r1 == 0) {
		if (r2 != 0) {
			return 1;
		}
    }
    else {
		if (r2 == 0) {
			return -1;
		}
		else {
			return 0;
		}
    }
    if (s1.st_mtime > s2.st_mtime) {
		return 1;
    }
    else if (s1.st_mtime < s2.st_mtime) {
		return -1;
    }
    else {
		return 0;
    }
}
#else
int	CompareTime(LPCSTR szL, LPCSTR szR) {
	assert(szL!=NULL && szR!=NULL);
	
	SYSTEMTIME	stL, stR;
	BOOL		fexistL, fexistR;
	
	// 更新日付を得る。
	fexistL = GetLastWriteTime(szL, stL);
	fexistR	= GetLastWriteTime(szR, stR);
	// 存在しないファイルは「古い」と見なす。
	if ( fexistL ) {
		if ( !fexistR)
			return	1;
	} else {
		if ( fexistR )
			return	-1;
		else
			return	0;	// どっちもありゃしねぇ
	}
	
	// 最終更新日付を比較
	if ( stL.wYear > stR.wYear )	return	1;
	else if ( stL.wYear < stR.wYear )	return	-1;
	if ( stL.wMonth > stR.wMonth )	return	1;
	else if ( stL.wMonth < stR.wMonth )	return	-1;
	if ( stL.wDay > stR.wDay )	return	1;
	else if ( stL.wDay < stR.wDay )	return	-1;
	if ( stL.wHour > stR.wHour )	return	1;
	else if ( stL.wHour < stR.wHour )	return	-1;
	if ( stL.wMinute > stR.wMinute )	return	1;
	else if ( stL.wMinute < stR.wMinute )	return	-1;
	if ( stL.wSecond > stR.wSecond )	return	1;
	else if ( stL.wSecond < stR.wSecond )	return	-1;
	if ( stL.wMilliseconds > stR.wMilliseconds )	return	1;
	else if ( stL.wMilliseconds < stR.wMilliseconds )	return	-1;
	// 制作日時の完全一致
	return	0;
}
#endif
#endif

wstring	Satori::GetWord(const wstring& name) {
	return L"いぬ";
}

void Satori::surface_restore_string_addfunc(wstring &str, std::map<int, int>::const_iterator &i)
{
	if ( i->first >= 2 ) {
		if ( ! mIsMateria ) {
			str += wstring() + L"\\p[" + itos(i->first) + L"]\\s[" + itos(i->second) + L"]";
		}
	}
	else {
		str += wstring() + L"\\" + itos(i->first) + L"\\s[" + itos(i->second) + L"]";
	}
}

wstring	Satori::surface_restore_string()
{
	enum SurfaceRestoreMode srestore = surface_restore_at_talk_onetime;
	if ( srestore == SR_INVALID ) {
		srestore = surface_restore_at_talk;
	}
	
	// そもそも必要なし、の場合
	// invalid比較はただの保険（ありえない）
	if ( srestore == SR_NONE || srestore == SR_INVALID ) {	
		return	L"\\1";
	}
	
	wstring	str=L"";
	
	if ( srestore == SR_FORCE ) {	
		for (std::map<int, int>::const_iterator i=default_surface.begin() ; i!=default_surface.end() ; ++i ) {
			if ( surface_changed_before_speak.size() ) {
				std::map<int,bool>::const_iterator found = surface_changed_before_speak.find(i->first);
				if ( found == surface_changed_before_speak.end() || found->second ) {
					surface_restore_string_addfunc(str,i);
				}
			}
			else {
				surface_restore_string_addfunc(str,i);
			}
		}
	}
	else {
		for (std::map<int, int>::const_iterator i=default_surface.begin() ; i!=default_surface.end() ; ++i ) {
			if ( surface_changed_before_speak.find(i->first) == surface_changed_before_speak.end() ) {
				surface_restore_string_addfunc(str,i);
			}
		}
	}
	
	surface_changed_before_speak.clear();
	return	str;
}


// ある名前により指定される「全ての」URL及び付帯情報、のリスト
bool	Satori::GetURLList(const wstring& name, wstring& result)
{
	std::list<const Talk*> tg;
	talks.select_all(name,*this,tg);
	
	const wstring sep_1 = L"\1";
	const wstring sep_2 = L"\2";
	
	for (std::list<const Talk*>::iterator it = tg.begin() ; it != tg.end() ; ++it )
	{
		const Talk& vec = **it;
		if ( vec.size() < 1 )
			continue;
		wstring	menu = UnKakko(vec[0].c_str());
		wstring	url = (vec.size()<2) ? (L"") : (UnKakko(vec[1].c_str()));
		wstring	banner = (vec.size()<3) ? (L"") : (UnKakko(vec[2].c_str()));
		
		int	len = menu.size()+url.size()+banner.size()+3;
		result.reserve(result.size() + len + 1);
		
		result += menu;
		result += sep_1;
		result += url;
		result += sep_1;
		result += banner;
		result += sep_2;
	}
	return	true;
}

// ある名前により指定されるURL中の指定サイトのスクリプトを取得
bool	Satori::GetRecommendsiteSentence(const wstring& name, wstring& result)
{
	std::list<const Talk*> tg;
	talks.select_all(name,*this,tg);
	wstring unkakko_t1;
	
	for (std::list<const Talk*>::iterator it = tg.begin() ; it != tg.end() ; ++it )
	{
		const Talk& t = **it;
		if ( t.size() >= 4 )
		{
			unkakko_t1 = UnKakko(t[1].c_str(),false,true);

			if ( unkakko_t1 == mReferences[1] ) {
				result = SentenceToSakuraScriptExec( Talk(t.begin()+3, t.end()) );
				return	true;
			}
		}
	}
	return	false;
}

strmap*	Satori::find_ghost_info(wstring name) {
	std::vector<strmap>::iterator i=ghosts_info.begin();
	for ( ; i!=ghosts_info.end() ; ++i )
		if ( (*i)[L"name"] == name )
			return	&(*i);
		return	NULL;
}


bool Satori::calc_argument(const wstring &iExpression, int &oResult, bool for_non_talk)
{
	wstring exp = UnKakko(iExpression.c_str(), true, for_non_talk);
	if ( !calc(exp) ){
		return false;
	}
	oResult = zen2int(exp);
	return true;
}

wstring	Satori::special_call(
							 const wstring& iCallName,
							 const strvec& iArgv,
							 bool for_calc,
							 bool for_non_talk,
							 bool iIsSecure)
{
	if ( iCallName == L"when" ) {
		int result = 0;
		if ( iArgv.size() < 2 || 3 < iArgv.size() ) {
			return L"引数の個数が正しくありません。";
		}
		if ( !calc_argument(iArgv[0], result, for_non_talk) ) return L"' 式が計算不能です。";
		if ( result != 0 ) {
			return	UnKakko(iArgv[1].c_str(), for_calc, for_non_talk);	// 真
		}
		else if ( iArgv.size()==3 ) {
			return	UnKakko(iArgv[2].c_str(), for_calc, for_non_talk);	// 偽
		}
		else {
			return	L"";	// 偽でelseなし
		}
	}
	
	if (iCallName == L"whenlist") {
		if (iArgv.size() < 2) {
			return L"引数の個数が正しくありません。";
		}
		const wstring lhs = UnKakko(iArgv[0].c_str(), for_calc, for_non_talk);
		size_t max = iArgv.size();
		for (size_t i = 1; i < max; i += 2) {
			if (i == max - 1) {
				return UnKakko(iArgv[i].c_str(), for_calc, for_non_talk);
			}
			wstring exp = lhs + UnKakko(iArgv[i].c_str(), for_calc, for_non_talk);
			int result = 0;
			if (!calc_argument(exp, result, for_non_talk)) {
				return L"' whenlistの" + itos((i - 1) / 2 + 1) + L"番目、式" + exp + L"は計算不能でした。";
			}
			if (result != 0){
				return	UnKakko(iArgv[i + 1].c_str(), for_calc, for_non_talk);
			}
		}
		return L"";
	}
	
	if ( iCallName == L"times" ) {
		int count = 0;
		int max = 0;
		int body = 0;
		wstring ret=L"";
		mLoopCounters.push(L"0");
		try{
			if ( iArgv.size() == 2 ){
				if ( !calc_argument(iArgv[0], max, for_non_talk) ) throw(L"' 式が計算不能です。");
				count = 0;
				mLoopCounters.top() = itos(count);
				body = 1;
			}
			else if( iArgv.size() == 3 ){
				if ( !calc_argument(iArgv[1], count, for_non_talk) ) throw(L"' 式が計算不能です。");
				mLoopCounters.top() = itos(count);
				if ( !calc_argument(iArgv[0], max, for_non_talk) ) throw(L"' 式が計算不能です。");
				max += count;
				body = 2;
			}
			else{
				throw(L"引数の個数が正しくありません。");
			}
			for(int i=count; i<max; i++){
				mLoopCounters.top() = itos(i);
				ret += UnKakko(iArgv[body].c_str(), for_calc, for_non_talk);
			}
		}
		catch( const wchar_t *str ){
			ret = str;
		}
		mLoopCounters.pop();
		return ret;
	}
	
	if ( iCallName == L"while" ) {
		int count = 0;
		int result = 0;
		int expression;
		int body;
		wstring ret=L"";
		mLoopCounters.push(L"0");
		try {
			if ( iArgv.size() == 2 ){
				expression = 0;
				count = 0;
				body = 1;
			}
			else if( iArgv.size() == 3 ){
				expression = 0;
				if ( !calc_argument(iArgv[1], count, for_non_talk) ) throw(L"' 式が計算不能です。");
				body = 2;
			}
			else{
				throw(L"引数の個数が正しくありません。");
			}
			for(int i=count; i<INT_MAX; i++){
				mLoopCounters.top() = itos(i);
				if ( !calc_argument(iArgv[expression], result, for_non_talk) ) throw(L"' 式が計算不能です。");
				if ( result == 0 ) {
					break;
				}
				ret += UnKakko(iArgv[body].c_str(), for_calc, for_non_talk);
			}
		}
		catch(const wchar_t * str){
			ret = str;
		}
		mLoopCounters.pop();
		return ret;
	}
	
	if ( iCallName == L"for" ) {
		int start = 0;
		int end = 0;
		int step = 0;
		int body;
		wstring ret=L"";
		mLoopCounters.push(L"0");
		try{
			if ( iArgv.size() == 3 ){
				if ( !calc_argument(iArgv[0], start, for_non_talk) ) throw(L"' 式が計算不能です。");
				mLoopCounters.top() = itos(start);
				if ( !calc_argument(iArgv[1], end, for_non_talk) ) throw(L"' 式が計算不能です。");
				step = 1;
				body = 2;
			}
			else if ( iArgv.size() == 4 ) {
				if ( !calc_argument(iArgv[0], start, for_non_talk) ) throw(L"' 式が計算不能です。");
				mLoopCounters.top() = itos(start);
				if ( !calc_argument(iArgv[1], end, for_non_talk) ) throw(L"' 式が計算不能です。");
				if ( !calc_argument(iArgv[2], step, for_non_talk) ) throw(L"' 式が計算不能です。");
				body = 3;
			}
			else {
				throw(L"引数の個数が正しくありません。");
			}
			if ( step == 0 ) {
				throw(L"forの増分に0が指定されました。");
			}
			step = abs(step);
			if ( start <= end ) {
				for(int i=start; i<=end; i+=step) {
					mLoopCounters.top() = itos(i);
					ret += UnKakko(iArgv[body].c_str(), for_calc, for_non_talk);
				}
			}
			else {
				for(int i=start; end<=i; i-=step) {
					mLoopCounters.top() = itos(i);
					ret += UnKakko(iArgv[body].c_str(), for_calc, for_non_talk);
				}
			}
		}
		catch(const wchar_t *str){
			ret = str;
		}
		mLoopCounters.pop();
		return ret;
	}
	
	assert(0);
	return L"";
}



// 文章の中で （ を見つけた場合、pが （ の次の位置まで進められた上でこれが実行される。
// pはこの内部で ） の次の位置まで進められる。
// 返値はカッコの解釈結果。
wstring	Satori::KakkoSection(const wchar_t*& p,bool for_calc,bool for_non_talk)
{
	wstring	thePluginName = L"";
	wstring  theDelimiter = L"";
	bool specialFlag = false;
	const wchar_t *pp=0;
	strvec	theArguments;
	wstring	kakko_str;
	
	if ( for_calc ) {
		for_non_talk = true;
	}
	for (std::set<wstring>::iterator it = special_commands.begin(); it != special_commands.end(); ++it) {
		if ( wcsncmp(it->c_str(), p, it->size()) == 0 ) {
			pp = p + it->size();
			const wstring c = next_a_chr(pp).str();
			//引数がない場合はスペシャルフォームにする必要はない。
			if ( mDelimiters.find(c) != mDelimiters.end() ){
				specialFlag = true;
				theDelimiter = c;
				thePluginName = it->c_str();
				break;
			}
		}
	}
	
	if( specialFlag ) {
		assert(pp);
		int level = 0;
		const wchar_t *p_start = pp;
		while( true ){
			if ( *pp == L'\0' ){
				return wstring(L"（"); // 閉じカッコが無かった
			}
			a_chr c = next_a_chr(pp);
			if ( c == L"（" ) {
				level++;
			}
			if ( c == L"）" ) {
				level--;
			}
			if ( level < 0 ) {
				theArguments.push_back( wstring(p_start, pp-p_start-c.size()) );
				break;
			}
			if ( level == 0 ) {
				if ( c == theDelimiter ) {
					theArguments.push_back( wstring(p_start, pp-p_start-c.size()) );
					p_start = (wchar_t *)pp;
				}
			}
		}
		p = pp;
		return special_call(thePluginName, theArguments, for_calc, for_non_talk, secure_flag);
	}
	else {
		while (true) {
			if ( p[0] == L'\0' )
				return	wstring(L"（") + kakko_str;	// 閉じカッコが無かった
			
			a_chr c = next_a_chr(p);
			if ( c==L"）" )
				break;
			else if ( c==L"（" ) {
				append_grow(kakko_str, KakkoSection(p,false,for_non_talk)); //内側の括弧は0に置き換えしない
			}
			else
				kakko_str += c;
		}	
		wstring	result;
		if ( Call(kakko_str, result, for_calc, for_non_talk) )
			return	result;
		if ( for_calc )
			return	wstring(L"０");
		else
			return	wstring(L"（") + kakko_str + L"）";
	}
}

wstring	Satori::UnKakko(const wchar_t* p,bool for_calc,bool for_non_talk)
{
	assert(p!=NULL);
	wstring	result;

	while ( p[0] != L'\0' ) {
		a_chr c=next_a_chr(p);

		if ( c == L"（" ) {
			append_grow(result, KakkoSection(p,for_calc,for_non_talk));
		}
		else {
			result += c;
		}

		if ( m_kakko_size_limit > 0 && result.size() > m_kakko_size_limit ) {
			GetSender().sender() << L"括弧展開サイズ超過：" << result << std::endl;
			return result;
		}
	}
	return	result;
}

void	Satori::erase_var(const wstring& key)
{
	if ( key == L"スコープ切り換え時" ) {
		append_at_scope_change = L"";
	}
	else if ( key == L"さくらスクリプトによるスコープ切り換え時" ) {
		append_at_scope_change_with_sakura_script = L"";
	}
	else if ( key == L"スクリプトの一番頭" ) {
		header_script = L"";
	}
	else if ( key == L"トーク開始時" ) {
		append_at_talk_start = L"";
	}
	else if ( key == L"トーク終了時" ) {
		append_at_talk_end = L"";
	}
	else if ( key == L"選択肢開始時" ) {
		append_at_choice_start = L"";
	}
	else if ( key == L"選択肢終了時" ) {
		append_at_choice_end = L"";
	}
	else {
		int ref;
		wchar_t firstChar;
		
		if ( IsArrayValue(key,ref,firstChar) ) {
			if ( firstChar==L'R' ) {
				// Event通知時の引数取得
				if (ref>=0 && ref<mReferences.size()) {
					mReferences[ref] = L"";
					if ( ref == (mReferences.size()-1) ) {
						mReferences.pop_back();
					}
				}
			}
			else if ( firstChar==L'H' ) {
				// 過去の置き換え履歴を参照
				if ( kakko_replace_history.empty() ) { return; }
				
				strvec&	khr = kakko_replace_history.top();
				
				ref -= 1; //いっこまえでないと履歴にならん！
				
				if ( ref>=0 && ref < khr.size() ) {
					khr[ref] = L"";
					if ( ref == (khr.size()-1) ) {
						khr.pop_back();
					}
				}
			}
			else if ( firstChar==L'A' ) {
				if ( mCallStack.empty() ) { return; }
				
				// callによる呼び出しの引数を参照S
				strvec&	v = mCallStack.top();
				if ( ref >= 0 && ref < v.size() ) {
					v[ref] = L"";
					if ( ref == (v.size()-1) ) {
						v.pop_back();
					}
				}
			}
			else if ( firstChar==L'S' ) {
				// SAORIなどコール時の結果処理
				if (ref>=0 && ref<mKakkoCallResults.size()) {
					mKakkoCallResults[ref] = L"";
					if ( ref == (mKakkoCallResults.size()-1) ) {
						mKakkoCallResults.pop_back();
					}
				}
			}
			//else if ( firstChar=='C' ) {
			//}
		}
	}
	
	variables.erase(key);
}

// システム変数
// return = 0(処理なし) / 1(処理した) / -1(処理したけど変数設定してはだめ)

int	Satori::system_variable_operation(wstring key, wstring value, wstring* result)
{
	int r = system_variable_operation_real(key,value,result);
	if ( r < 0 ) {
		variables.erase(key); //念の為
	}
	return r;
}

int	Satori::system_variable_operation_real(wstring key, wstring value, wstring* result)
{
	// mapにしようよ。
	
	if ( key == L"喋り間隔" ) {
		talk_interval = zen2int(value);
		if ( talk_interval<3 ) talk_interval=0; // 3未満は喋らない
		
		// 喋りカウント初期化
		int	dist = static_cast<int>(talk_interval*(talk_interval_random/100.0));
		talk_interval_count = ( dist==0 ) ? talk_interval : (talk_interval-dist)+(random(dist*2));
		
		return 1; //実行＋変数設定
	}
	
	if ( key == L"喋り間隔誤差" ) {
		talk_interval_random = zen2int(value);
		if ( talk_interval_random>100 ) talk_interval_random=100;
		if ( talk_interval_random<0 ) talk_interval_random=0;
		
		// 喋りカウント初期化
		int	dist = int(talk_interval*(talk_interval_random/100.0));
		talk_interval_count = ( dist==0 ) ? talk_interval : 
		(talk_interval-dist)+(random(dist*2));
		
		return 1; //実行＋変数設定
	}
	
	if ( key ==L"見切れてても喋る" ) {
		is_call_ontalk_at_mikire= (value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if ( key == L"今回は喋らない" ) {
		return_empty=(value==L"有効");
		return true;
	}
	
	if ( key == L"スクリプトの一番頭" ) {
		header_script = value;
		return 1; //実行＋変数設定
	}
	
	if ( key == L"呼び出し回数制限" ) {
		m_nest_limit = zen2int(value);
		if ( m_nest_limit < 0 ) { m_nest_limit = 0; }
		return 1; //実行＋変数設定
	}

	if ( key == L"括弧展開サイズ制限" ) {
		m_kakko_size_limit = zen2int(value);
		if ( m_kakko_size_limit < 0 ) { m_kakko_size_limit = 0; }
		return 1; //実行＋変数設定
	}
	
	if ( key == L"ジャンプ回数制限" ) {
		m_jump_limit = zen2int(value);
		if ( m_jump_limit < 0 ) { m_jump_limit = 0; }
		return 1; //実行＋変数設定
	}
	
	if ( key == L"スコープ切り換え時" ) {
		append_at_scope_change = zen2han(value);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"さくらスクリプトによるスコープ切り換え時" ) {
		append_at_scope_change_with_sakura_script = zen2han(value);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"トーク開始時" ) {
		append_at_talk_start = zen2han(value);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"トーク終了時" ) {
		append_at_talk_end = zen2han(value);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"選択肢開始時" ) {
		append_at_choice_start = zen2han(value);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"選択肢終了時" ) {
		append_at_choice_end = zen2han(value);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"会話時サーフェス戻し" || key == L"会話時サーフィス戻し" ) {
		if ( value == L"有効" ) {
			surface_restore_at_talk = SR_NORMAL;
		}
		else if ( value == L"強制" ) {
			surface_restore_at_talk = SR_FORCE;
		}
		else {
			surface_restore_at_talk = SR_NONE;
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"今回は会話時サーフェス戻し" || key == L"今回は会話時サーフィス戻し" ) {
		if ( value == L"有効" ) {
			surface_restore_at_talk_onetime = SR_NORMAL;
		}
		else if ( value == L"強制" ) {
			surface_restore_at_talk_onetime = SR_FORCE;
		}
		else {
			surface_restore_at_talk_onetime = SR_NONE;
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"自動アンカー" ) {
		if ( value == L"有効" ) {
			auto_anchor_enable = true;
			auto_anchor_enable_onetime = auto_anchor_enable;
		}
		else {
			auto_anchor_enable = false;
			auto_anchor_enable_onetime = auto_anchor_enable;
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"今回は自動アンカー" ) {
		if ( value == L"有効" ) {
			auto_anchor_enable_onetime = true;
		}
		else {
			auto_anchor_enable_onetime = false;
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"自動改行挿入" ) {
		if ( value == L"有効" ) {
			auto_newline_enable = true;
			auto_newline_enable_onetime = auto_newline_enable;
		}
		else {
			auto_newline_enable = false;
			auto_newline_enable_onetime = auto_newline_enable;
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"今回は自動改行挿入" ) {
		if ( value == L"有効" ) {
			auto_newline_enable_onetime = true;
		}
		else {
			auto_newline_enable_onetime = false;
		}
		return 1; //実行＋変数設定
	}
	
	if ( compare_head(key,  L"サーフェス加算値") && aredigits(key.c_str() + const_strlen(L"サーフェス加算値")) ) {
		int n = zen2int(key.c_str() + const_strlen(L"サーフェス加算値"));
		surface_add_value[n]= zen2int(value);
		
		variables[wstring()+L"デフォルトサーフェス"+itos(n)] = value;
		next_default_surface[n] = zen2int(value);
		if ( !is_speaked_anybody() )
			default_surface[n]=next_default_surface[n];
		return 1; //実行＋変数設定
	}
	
	if ( compare_head(key,  L"デフォルトサーフェス") && aredigits(key.c_str() + const_strlen(L"デフォルトサーフェス")) ) {
		int n = zen2int(key.c_str() + const_strlen(L"デフォルトサーフェス"));
		next_default_surface[n]= zen2int(value);
		if ( !is_speaked_anybody() )
			default_surface[n]=next_default_surface[n];
		return 1; //実行＋変数設定
	}
	
	if ( compare_head(key,  L"BalloonOffset") && aredigits(key.c_str() + const_strlen(L"BalloonOffset")) ) {
		int n = stoi_internal(key.c_str() + const_strlen(L"BalloonOffset"));
		BalloonOffset[n] = value;
		validBalloonOffset[n] = true;
		return 1; //実行＋変数設定
	}
	
	if ( key == L"トーク中のなでられ反応") {
		insert_nade_talk_at_other_talk= (value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if (key == L"なでられ時実行イベント") {
		if (value == L"なでられ時の反応") {
			bool_of_action_when_ghost_is_stroked = true;
			variables[L"なでられ時実行イベント"] = L"なでられ時の反応";
		}
		else /* if ( value == "デフォルト" ) */ {
			bool_of_action_when_ghost_is_stroked = false;
			variables[L"なでられ時実行イベント"] = L"デフォルト";
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"なでられ持続秒数") {
		nade_valid_time_initializer = zen2int(value);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"なでられ反応回数") {
		nade_sensitivity = zen2int(value);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"デバッグ" ) {
		fDebugMode = (value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if ( key == L"Log" ) {
		GetSender().validate(value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if ( key == L"RequestLog" ) {
		fRequestLog = (value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if ( key == L"OperationLog" ) {
		fOperationLog = (value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if ( key == L"ResponseLog" ) {
		fResponseLog = (value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if ( key == L"自動挿入ウェイトの倍率" || key == L"自動挿入ウエイトの倍率" ) {
		rate_of_auto_insert_wait= zen2int(value);
		rate_of_auto_insert_wait = min(1000, max(0, rate_of_auto_insert_wait));
		variables[L"自動挿入ウェイトの倍率"] = int2zen(rate_of_auto_insert_wait);
		return 1; //実行＋変数設定
	}
	
	if ( key == L"自動挿入ウェイトタイプ" || key == L"自動挿入ウエイトタイプ"  ) {
		if ( value == L"一般" ) {
			type_of_auto_insert_wait = 2;
			variables[L"自動挿入ウェイトタイプ"] = L"一般";
		}
		else if ( value == L"無効" ) {
			type_of_auto_insert_wait = 0;
			variables[L"自動挿入ウェイトタイプ"] = L"無効";
		}
		else /* if ( value == "里々" ) */ {
			type_of_auto_insert_wait = 1;
			variables[L"自動挿入ウェイトタイプ"] = L"里々";
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"コミュニケートの検索方法"  ) {
		if ( value == L"合計文字数" ) {
			type_of_communicate_search = COMSEARCH_LENGTH;
			variables[L"コミュニケートの検索方法"] = L"合計文字数";
		}
		else /* if ( value == "里々" ) */ {
			type_of_communicate_search = COMSEARCH_DEFAULT;
			variables[L"コミュニケートの検索方法"] = L"里々";
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"辞書フォルダ" ) {
		strvec	words;
		split(value, L",",dic_folder);
		reload_flag=true;
		return 1; //実行＋変数設定
	}
	
	if ( key == L"セーブデータ暗号化" ) {
		fEncodeSavedata = (value==L"有効");
		return 1; //実行＋変数設定
	}

	if ( key == L"タイマ変数はセーブしない" ) {
		fDontSaveTimerValue = (value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if ( compare_head(key,L"単語群「") && compare_tail(key,L"」の重複回避") ) {
		words.setOC( strip_head_tail(key, L"単語群「", L"」の重複回避"), value );
		return -1; //◆実行：変数設定しない
	}
	
	if ( compare_head(key,L"文「") && compare_tail(key,L"」の重複回避") ) {
		talks.setOC( strip_head_tail(key, L"文「", L"」の重複回避"), value );
		return -1; //◆実行：変数設定しない
	}
	
	if ( key == L"セーブデータ読み込み" ) {
		load_savedata_status = value;
		return -1; //◆実行：変数設定しない
	}
	
	if ( key == L"次のトーク" ) {
		int	count=1;
		while ( reserved_talk.find(count) != reserved_talk.end() )
			++count;
		reserved_talk[count] = value;
		GetSender().sender() << L"次回のランダムトークが「" << value << L"」に予約されました。" << std::endl;
		return -1; //◆実行：変数設定しない
	}
	
	if ( compare_head(key,L"次から") && compare_tail(key,L"回目のトーク") ) {
		int	count = zen2int( strip_head_tail(key, L"次から", L"回目のトーク") );
		if ( count<=0 ) {
			GetSender().sender() << L"トーク予約、設定値がヘンです。" << std::endl;
		}
		else {
			while ( reserved_talk.find(count) != reserved_talk.end() )
				++count;
			reserved_talk[count] = value;
			GetSender().sender() << count << L"回後のランダムトークが「" << value << L"」に予約されました。" << std::endl;
		}
		return -1; //◆実行：変数設定しない
	}

	if ( key==L"トーク予約のキャンセル" ) {
		if ( value==L"＊" ) {
			reserved_talk.clear();
		}
		else {
			for (std::map<int, wstring>::iterator it=reserved_talk.begin(); it!=reserved_talk.end() ; ) {
				if ( value == it->second ) {
					reserved_talk.erase(it++);
				}
				else {
					++it;
				}
			}
		}
		return -1; //◆実行：変数設定しない
	}
	
	if ( key == L"SAORI引数の計算" ) {
		if (value==L"有効") {
			mSaoriArgumentCalcMode = SACM_ON;
		}
		else if (value==L"無効") {
			mSaoriArgumentCalcMode = SACM_OFF;
		}
		else {
			mSaoriArgumentCalcMode = SACM_AUTO;
		}
		return 1; //実行＋変数設定
	}
	
	if ( key == L"辞書リロード" && value==L"実行") {
		reload_flag=true;
		return -1; //◆実行：変数設定しない
	}
	
	if ( key == L"れしば送信") {
		variables.erase(key);
		GetSender().reinit(value==L"有効");
		return 1; //実行＋変数設定
	}
	
	if ( key == L"手動セーブ" && value==L"実行") {
		if ( is_dic_loaded ) {
			this->Save();
		}
		return -1; //◆実行：変数設定しない
	}
	
	if ( key == L"自動セーブ間隔" ) {
		mAutoSaveInterval = zen2int(value);
		mAutoSaveCurrentCount = mAutoSaveInterval;
		if ( mAutoSaveInterval > 0 )
			GetSender().sender() << L""  << itos(mAutoSaveInterval) << L"秒間隔で自動セーブを行います。" << std::endl;
		else
			GetSender().sender() << L"自動セーブは行いません。" << std::endl;
		return 1; //実行＋変数設定
	}
	
	if ( key == L"教わること" ) {
		teach_genre=value;
		if ( result != NULL )
			*result += L"\\![open,teachbox]";
		return -1; //◆実行：変数設定しない
	}
	
	if ( key == L"全タイマ解除" && value==L"実行") {
		for (strintmap::const_iterator i=timer_sec.begin();i!=timer_sec.end();++i) {
			variables.erase(i->first + L"タイマ");
		}
		timer_sec.clear();
		return -1; //◆実行：変数設定しない
	}
	
	if ( key.size()>const_strlen(L"タイマ") && compare_tail(key, L"タイマ") ) {
		wstring	timer_name = strip_head_tail(key, L"", L"タイマ"); //「タイマ」を消す

		int sec = zen2int(value);
		if ( sec < 1 ) {
			if ( timer_sec.find(timer_name)!=timer_sec.end() ) {
				timer_sec.erase(timer_name);
				variables.erase(key);
				GetSender().sender() << L"タイマ「"  << timer_name << L"」の予約がキャンセルされました。" << std::endl;
			}
			else {
				GetSender().sender() << L"タイマ「"  << timer_name << L"」は元から予約されていません。" << std::endl;
			}
			return -1; //◆実行：変数設定しない
		}
		else {
			timer_sec[timer_name] = sec;
			GetSender().sender() << L"タイマ「"  << timer_name << L"」が" << sec << L"秒後に予約されました。" << std::endl;
			return 1; //実行＋変数設定
		}
	}
	
	if ( key == L"引数区切り追加" && value.size()>0 ) {
		mDelimiters.insert(value);
		return -1; //◆実行：変数設定しない
	}
	
	if ( key == L"引数区切り削除" && value.size()>0 ) {
		mDelimiters.erase(value);
		return -1; //◆実行：変数設定しない
	}
	
	if ( compare_head(key, L"Value") && aredigits(key.c_str() + 5) )
	{
		if(value!=L""){
			mResponseMap[wstring()+L"Reference"+key.substr(5)] = value;
		}else{
			mResponseMap.erase(wstring()+L"Reference"+key.substr(5));
		}
		return -1; //◆実行：変数設定しない
	}
	
	if ( compare_head(key,L"返信ヘッダ「") && compare_tail(key,L"」") ) {
		if(value!=L""){
			mResponseMap[strip_head_tail(key, L"返信ヘッダ「", L"」")] = value;
		}else{
			mResponseMap.erase(strip_head_tail(key, L"返信ヘッダ「", L"」"));
		}
		return -1; //◆実行：変数設定しない
	}
	
	if (key == L"NOTIFYの自動保存" && value.size() > 0){
		is_save_notify = (value == L"有効");
		return 1; //実行＋変数設定
	}
	
	if (key == L"れしばログ一時保存件数" && value.size() > 0)
	{
		GetSender().set_delay_save_count(zen2int(value));
		return 1; //実行＋変数設定
	}

	if (key == L"外部から実行可能なイベントの接頭辞") {
		split(value, L"、,", allow_external_event_prefixes);
		return 1; //実行+変数設定
	}
	
	return	0; //実行しない
}


bool	Satori::calculate(const wstring& iExpression, wstring& oResult) {
	
	oResult = UnKakko(iExpression.c_str(),true);
	
	bool r = calc(oResult);
	if ( !r ) {
#ifdef POSIX
		GetSender().errsender() <<
			L"error on Satori::calculate" << std::endl <<
			L"Error in expression: " << iExpression << satori::endl;
#else
		// もうちょっと抽象化を……
		GetSender().errsender() << wstring() + L"式が計算不能です。\n" + iExpression << satori::endl;
#endif
	}
	return	r;
}



