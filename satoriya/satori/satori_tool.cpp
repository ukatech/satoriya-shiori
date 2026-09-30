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

			if ( unkakko_t1 == reference_or_empty(1) ) {
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

// 文章の中で （ を見つけた場合、pが （ の次の位置まで進められた上でこれが実行される。
// pはこの内部で ） の次の位置まで進められる。
// 返値はカッコの解釈結果。
// 括弧の入れ子の深さの上限
static const int	KAKKO_MAX_DEPTH = 1000;

// 深さを数える（例外で抜けても戻るようにデストラクタで減らす）
class KakkoDepthCounter
{
	int& m_depth;
public:
	KakkoDepthCounter(int& i_depth) : m_depth(i_depth) { ++m_depth; }
	~KakkoDepthCounter() { --m_depth; }
};

// 呼び出し総数の予算を1つ使う。使い切っていたらtrueを返す。
bool	Satori::use_call_budget()
{
	if ( m_total_call_limit <= 0 ) {
		return	false;
	}
	if ( m_total_call_count < m_total_call_limit ) {
		++m_total_call_count;
		return	false;
	}
	if ( !m_total_call_reported ) {
		m_total_call_reported = true;
		GetSender().sender() << L"呼び出し総数超過（" << m_total_call_limit << L"回）：以降の呼び出しと繰り返しを打ち切ります" << std::endl;
	}
	return	true;
}

wstring	Satori::KakkoSection(const wchar_t*& p,bool for_calc,bool for_non_talk)
{
	KakkoDepthCounter depth_counter(m_kakko_depth);
	if ( m_kakko_depth > KAKKO_MAX_DEPTH ) {
		// これ以上は展開しない。（ だけを返すので、呼び出し側は続きを普通の文字として読み進める。
		if ( m_kakko_depth == KAKKO_MAX_DEPTH + 1 ) {
			GetSender().sender() << L"括弧の入れ子が深すぎます（" << KAKKO_MAX_DEPTH << L"段まで）。" << std::endl;
		}
		return	wstring(L"（");
	}

	if ( for_calc ) {
		for_non_talk = true;
	}

	// when などの特殊形式は、引数を展開せずに呼ぶ
	wstring	result;
	if ( CallSpecialFunction(p, result, for_calc, for_non_talk) ) {
		return	result;
	}

	wstring	kakko_str;
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
	if ( Call(kakko_str, result, for_calc, for_non_talk) )
		return	result;
	if ( for_calc )
		return	wstring(L"０");
	else
		return	wstring(L"（") + kakko_str + L"）";
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
	// スコープ切り換え時などのシステム変数は、この後の system_variable_operation(key, "") で戻る。
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
	
	variables.erase(key);
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



