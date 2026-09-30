//---------------------------------------------------------------------------
//	内蔵関数・内蔵変数・システム変数
//
//	名前と処理の対応はこのファイルの表にまとめてある。
//	新しいものを足すときは、処理を書いて表に1行追加し、satori.h に宣言を足す。
//
//	・内蔵関数	（名前、引数…）で呼ぶもの。function_table()
//	・内蔵変数	（名前）で値を返すもの。GetBuiltinValue()
//	・システム変数	＄名前＝値 で設定すると動作が変わるもの。system_variable_operation_real()
//---------------------------------------------------------------------------
#include	"satori.h"
#include	"../_/Utilities.h"
#include	"posix_utils.h"
#include	<time.h>
#ifndef POSIX
#include	<tlhelp32.h>
#else
#include <climits>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

struct shm_t {
	uint32_t size;
	sem_t sem;
	char buf[PATH_MAX];
};

const int BUFFER_SIZE = 1024;
#endif // !POSIX
#include	<sstream>
#include	<algorithm>

#ifdef POSIX
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


//===========================================================================
//	表の型
//===========================================================================

// 内蔵関数。iArgvは引数、for_calc / for_non_talk は特殊形式の引数の展開に使う。
typedef wstring (Satori::*FunctionHandler)(const strvec& iArgv, bool for_calc, bool for_non_talk);

struct SatoriFunction {
	FunctionHandler	handler;
	bool	is_special;		// 特殊形式：引数を展開せずに渡し、必要なものだけ展開する
	bool	need_secure;	// SecurityLevel: local のときだけ実行できる
};

struct SatoriFunctionTable {
	std::map<wstring, SatoriFunction>	functions;
	// 特殊形式の名前（KakkoSectionで（の直後と比べる）
	std::vector<const std::pair<const wstring, SatoriFunction>*>	specials;
};

// 内蔵変数。iNameは名前全体、iArgは名前から前後の決まった部分を除いたもの。
// 値があればoResultに入れてtrueを返す。falseなら次の候補を探す。
typedef bool (Satori::*NameHandler)(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);

struct BuiltinName {
	const wchar_t*	name;
	NameHandler	handler;
	int	param;
};

struct BuiltinNamePattern {
	const wchar_t*	head;	// 名前の先頭（L""なら問わない）
	const wchar_t*	tail;	// 名前の末尾（L""なら問わない）
	bool	use_hankaku;	// 全角を半角にした名前と比べる
	NameHandler	handler;
	int	param;
};

// システム変数。iKeyは変数名全体、iArgは変数名から前後の決まった部分を除いたもの。
// 返値は system_variable_operation と同じ（SYSVAR_NONE なら次の候補を探す）。
typedef int (Satori::*SystemVariableHandler)(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);

struct SystemVariable {
	const wchar_t*	name;
	SystemVariableHandler	handler;	// 設定時の処理
	bool Satori::*	flag;	// handlerが無いとき：「有効」なら真にするフラグ
	wstring Satori::*	text;	// handlerもflagも無いとき：全角を半角にして入れる文字列
};

struct SystemVariablePattern {
	const wchar_t*	head;
	const wchar_t*	tail;
	SystemVariableHandler	handler;
};

// 名前が head で始まり tail で終わるか（L""は問わない）
static bool	match_head_tail(const wstring& iName, const wchar_t* head, const wchar_t* tail)
{
	if ( *head && !compare_head(iName, head) ) { return false; }
	if ( *tail && !compare_tail(iName, tail) ) { return false; }
	return true;
}


//===========================================================================
//	内蔵関数
//===========================================================================

const SatoriFunctionTable&	Satori::function_table()
{
	static SatoriFunctionTable	table;
	if ( table.functions.empty() ) {
		static const struct {
			const wchar_t*	name;
			SatoriFunction	func;
		} entries[] = {
			// 名前					処理								特殊形式	localのみ
			{ L"バイト値",			{ &Satori::func_byte_value,			false,	false } },
			{ L"nop",				{ &Satori::func_nop,				false,	false } },
			{ L"合成単語群",		{ &Satori::func_synthesized_words,	false,	false } },
			{ L"文の数",			{ &Satori::func_talk_count,			false,	false } },
			{ L"set",				{ &Satori::func_set,				false,	true } },
			{ L"loop",				{ &Satori::func_loop,				false,	true } },
			{ L"sync",				{ &Satori::func_sync,				false,	true } },
			{ L"remember",			{ &Satori::func_remember,			false,	true } },
			{ L"変数の一括削除",	{ &Satori::func_erase_variables,	false,	true } },
			{ L"変数の一括コピー",	{ &Satori::func_copy_variables,		false,	true } },
			{ L"変数の列挙",		{ &Satori::func_list_variables,		false,	false } },
			{ L"split_to",			{ &Satori::func_split_to,			false,	true } },
			{ L"call",				{ &Satori::func_call,				false,	true } },
			{ L"vncall",			{ &Satori::func_vncall,				false,	true } },
			{ L"equal",				{ &Satori::func_equal,				false,	true } },
			{ L"get_property",		{ &Satori::func_get_property,		false,	true } },
			{ L"set_property",		{ &Satori::func_set_property,		false,	true } },
			{ L"load_saori",		{ &Satori::func_load_saori,			false,	true } },
			{ L"単語の追加",		{ &Satori::func_add_word,			false,	true } },
			{ L"追加単語の削除",	{ &Satori::func_remove_added_word,	false,	true } },
			{ L"追加単語の全削除",	{ &Satori::func_remove_all_added_words,	false,	true } },
			{ L"when",				{ &Satori::func_when,				true,	false } },
			{ L"whenlist",			{ &Satori::func_whenlist,			true,	false } },
			{ L"times",				{ &Satori::func_times,				true,	false } },
			{ L"while",				{ &Satori::func_while,				true,	false } },
			{ L"for",				{ &Satori::func_for,				true,	false } },
		};
		for ( int i=0 ; i<sizeof(entries)/sizeof(entries[0]) ; ++i ) {
			table.functions[entries[i].name] = entries[i].func;
		}
		// 名前順（std::setだったころと同じ順）に並べる
		for ( std::map<wstring, SatoriFunction>::const_iterator it=table.functions.begin() ; it!=table.functions.end() ; ++it ) {
			if ( it->second.is_special ) {
				table.specials.push_back(&*it);
			}
		}
	}
	return	table;
}

// 繰り返しの結果が括弧展開サイズ制限を超えたら、警告してtrueを返す（呼び出し側は繰り返しを打ち切る）
bool	Satori::loop_result_too_large(const wstring& iResult)
{
	if ( m_kakko_size_limit > 0 && iResult.size() > static_cast<wstring::size_type>(m_kakko_size_limit) ) {
		GetSender().sender() << L"繰り返しの結果が大きすぎるので、打ち切りました：" << iResult.size() << L"文字" << std::endl;
		return	true;
	}
	return	false;
}

// 特殊形式の引数を分ける。pは最初の引数の先頭。
// 対応する）で終われば、pをその次へ進めてtrueを返す。
// 文字列の終わりに達したら、残りを最後の引数にしてfalseを返す。
static bool	split_special_arguments(const wchar_t*& p, const wstring& iDelimiter, strvec& oArguments)
{
	int level = 0;
	const wchar_t *p_start = p;
	while ( true ) {
		if ( *p == L'\0' ) {
			oArguments.push_back( wstring(p_start, p-p_start) );
			return false;
		}
		a_chr c = next_a_chr(p);
		if ( c == L"（" ) {
			level++;
		}
		if ( c == L"）" ) {
			level--;
		}
		if ( level < 0 ) {
			oArguments.push_back( wstring(p_start, p-p_start-c.size()) );
			return true;
		}
		if ( level == 0 ) {
			if ( c == iDelimiter ) {
				oArguments.push_back( wstring(p_start, p-p_start-c.size()) );
				p_start = p;
			}
		}
	}
}

// 文章中の（の直後pが「特殊形式の名前＋区切り」なら呼び出して結果をoResultに入れ、trueを返す。
// pは）の次まで進める。）が無ければ oResult は（ で、pは進めない。
bool	Satori::CallSpecialFunction(const wchar_t*& p, wstring& oResult, bool for_calc, bool for_non_talk)
{
	const SatoriFunctionTable& table = function_table();

	const SatoriFunction* func = NULL;
	wstring delimiter;
	const wchar_t *pp = NULL;
	for ( int i=0 ; i<table.specials.size() ; ++i ) {
		const wstring& name = table.specials[i]->first;
		if ( wcsncmp(name.c_str(), p, name.size()) == 0 ) {
			pp = p + name.size();
			const wstring c = next_a_chr(pp).str();
			//引数がない場合はスペシャルフォームにする必要はない。
			if ( mDelimiters.find(c) != mDelimiters.end() ) {
				func = &(table.specials[i]->second);
				delimiter = c;
				break;
			}
		}
	}
	if ( func == NULL ) {
		return false;
	}

	strvec	theArguments;
	if ( !split_special_arguments(pp, delimiter, theArguments) ) {
		oResult = L"（";	// 閉じカッコが無かった
		return true;
	}
	p = pp;
	oResult = (this->*(func->handler))(theArguments, for_calc, for_non_talk);
	return true;
}

// iNameがSAORIか内蔵関数の呼び出しなら呼んで結果をoResultに入れ、trueを返す。
// 名前の後ろに区切りがあれば、そこから後ろが引数。
// use_arg_callstackがtrueのとき（call / vncall から）は、iName全体が名前で、引数はmCallStackの先頭。
bool	Satori::CallFunction(const wstring& iName, wstring& oResult, bool use_arg_callstack)
{
	wstring	thePluginName;
	const SatoriFunction* func = NULL;
	std::set<wstring>::const_iterator theDelimiter = mDelimiters.end();
	const wchar_t* p = NULL;	// 区切りの位置

	// 呼び出すものを探す。SAORIは内蔵関数より優先。
	if ( mShioriPlugins->find(iName) ) {
		thePluginName = iName;
	}
	else if ( use_arg_callstack ) {
		func = find_function(iName);
		if ( func == NULL ) {
			return false;
		}
		thePluginName = iName;
	}
	else {
		for (std::set<wstring>::const_iterator i = mDelimiters.begin(); i != mDelimiters.end(); ++i) {
			p = strstr_hz(iName.c_str(), i->c_str());
			if (p == NULL)
				continue;
			wstring	str(iName.c_str(), p - iName.c_str());
			if ( mShioriPlugins->find(str) ) {
				thePluginName = str;
				theDelimiter = i;
				break;
			}
			func = find_function(str);
			if ( func != NULL ) {
				thePluginName = str;
				theDelimiter = i;
				break;
			}
		}
		if ( theDelimiter == mDelimiters.end() ) {
			return false;
		}
	}

	// 引数を作る
	strvec	theArguments;
	if (use_arg_callstack && !mCallStack.empty())
	{
		//call / vncall用の処理。
		//SPECIAL CALL の動作は保証しません
		theArguments = mCallStack.top();
	}
	else if (p != NULL && theDelimiter != mDelimiters.end())
	{
		if ( func != NULL && func->is_special ) {
			next_a_chr(p);
			split_special_arguments(p, *theDelimiter, theArguments);
		}
		else {
			while (true)
			{
				p += theDelimiter->size();
				const wchar_t* pdlmt = strstr_hz(p, theDelimiter->c_str());
				if (pdlmt == NULL) {
					theArguments.push_back(p);
					break;
				}
				theArguments.push_back(wstring(p, pdlmt - p));
				p = pdlmt;
			}

			if (mSaoriArgumentCalcMode != SACM_OFF) {
				for (strvec::iterator i = theArguments.begin(); i != theArguments.end(); ++i) {
					if (i->size() == 0)
						continue;
					if (mSaoriArgumentCalcMode == SACM_AUTO) {
						int	c = zen2han(*i).at(0);
						if (c != L'+' && c != L'-' && !(c >= L'0' && c <= L'9'))
							continue;
					}

					wstring	exp = *i;
					if (calc(exp, true)) {
						if (func == NULL && aredigits(zen2han(exp))) {
							*i = zen2han(exp);
						}
						else {
							*i = exp;
						}
					}
				}
			}
		}
	}

	// 引数渡して返値を取得、と。
	if ( func == NULL ) {
		for ( strvec::iterator i=theArguments.begin() ; i!=theArguments.end() ; ++i ) {
			m_escaper.unescape(*i);
		}
		oResult = mShioriPlugins->request(thePluginName, theArguments, mKakkoCallResults, secure_flag ? L"Local" : L"External" );
	}
	else if ( func->is_special ) {
		oResult = (this->*(func->handler))(theArguments, false, true);
	}
	else if ( func->need_secure && !secure_flag ) {
		GetSender().sender() << L"local/Localでないので蹴りました: " << thePluginName << std::endl;
		oResult = L"";
	}
	else {
		oResult = (this->*(func->handler))(theArguments, false, false);
	}
	oResult = UnKakko(oResult.c_str());	// 返値を再度カッコ展開
	return true;
}

const SatoriFunction*	Satori::find_function(const wstring& iName)
{
	const std::map<wstring, SatoriFunction>& functions = function_table().functions;
	std::map<wstring, SatoriFunction>::const_iterator it = functions.find(iName);
	return ( it == functions.end() ) ? NULL : &(it->second);
}

//---------------------------------------------------------------------------
// 変数・呼び出し

wstring	Satori::func_set(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size()==2 ) {
		wstring	result, key=iArgv[0], value=iArgv[1];

		SubstVariable(key,value,result,false);

		return	result;
	}
	return	L"";
}

wstring	Satori::func_loop(const strvec& iArgv, bool, bool)
{
	int	init=1, max=0, step=1, arg_size=iArgv.size();
	if ( arg_size==2 ) {
		max=zen2int(iArgv[1]);
	}
	else if ( arg_size==3 ) {
		init=zen2int(iArgv[1]);
		max=zen2int(iArgv[2]);
	}
	else if ( arg_size==4 ) {
		init=zen2int(iArgv[1]);
		max=zen2int(iArgv[2]);
		step=zen2int(iArgv[3]);
	}
	else
		return	L"";
	wstring	name=iArgv[0];
	wstring	ret,temp;

	if ( step==0 )
		return	L"";
	else if ( step>0 ) {
		if ( init>max )
			return	L"";
		for (int i=init ; ; ) {
			variables[name+L"カウンタ"] = itos(i);
			if ( !Call(name, temp) )
				return	L"";
			ret += temp;
			if ( loop_result_too_large(ret) )
				break;
			// i+=step が int を越えないように、残りの幅と比べる
			if ( static_cast<unsigned int>(step) > static_cast<unsigned int>(max) - static_cast<unsigned int>(i) )
				break;
			i += step;
		}
	}
	else {
		if ( init<max )
			return	L"";
		// 減らす幅。INT_MINは符号を反転できないので INT_MAX にする
		const unsigned int down = ( step==INT_MIN ) ? static_cast<unsigned int>(INT_MAX) : static_cast<unsigned int>(-step);
		for (int i=init ; ; ) {
			variables[name+L"カウンタ"] = itos(i);
			if ( !Call(name, temp) )
				return	L"";
			ret += temp;
			if ( loop_result_too_large(ret) )
				break;
			if ( down > static_cast<unsigned int>(i) - static_cast<unsigned int>(max) )
				break;
			i -= static_cast<int>(down);
		}
	}
	variables.erase(name+L"カウンタ");
	return	ret;
}

wstring	Satori::func_call(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size() >= 1 ) {
		mCallStack.push( strvec() );
		strvec&	v = mCallStack.top();
		for ( int i=1 ; i<iArgv.size() ; ++i )
			v.push_back( iArgv[i] );
		wstring	r;
		Call(iArgv[0],r,false,false,true);
		mCallStack.pop();
		return	r;
	}
	return	L"";
}

wstring	Satori::func_vncall(const strvec& iArgv, bool, bool)
{
	if (iArgv.size() >= 1) {
		strvec	v;
		for (int i = 1; i < iArgv.size(); ++i){
			wstring	r;
			Call(iArgv[i], r);
			v.push_back(r);
		}

		//pushのタイミングはcallの直前。先にpushすると、引数の評価中にA0を取ったときに狂う。
		mCallStack.push(v);
		wstring	r;
		Call(iArgv[0], r, false, false, true);
		mCallStack.pop();
		return	r;
	}
	return	L"";
}

wstring	Satori::func_equal(const strvec& iArgv, bool, bool)
{
	if (iArgv.size() == 2) {
		const wstring &lhs = iArgv[0], &rhs = iArgv[1];
		return itos(lhs == rhs);
	}
	return	L"";
}

wstring	Satori::func_nop(const strvec&, bool, bool)
{
	return	L"";
}

wstring	Satori::func_sync(const strvec& iArgv, bool, bool)
{
	wstring	str = L"\\![raise,OnDirectSaoriCall";
	if ( !iArgv.empty() ) {
		wstring	arg;
		combine(arg, iArgv, L",");
		str += L",";
		str += arg;
	}
	str += L"]";
	return	str;
}

wstring	Satori::func_remember(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size() == 1 ) {
		int	n = zen2int(iArgv[0]);
		if ( mResponseHistory.size() > n ) {
			return	mResponseHistory[n];
		}
	}
	return	L"";
}

//---------------------------------------------------------------------------
// 変数の一括操作

// 名前が iPrefix で始まる変数の名前を、名前順に oNames に入れる
static void	variable_names_with_prefix(const strmap& iVariables, const wstring& iPrefix, strvec& oNames)
{
	for ( strmap::const_iterator it=iVariables.lower_bound(iPrefix) ; it!=iVariables.end() ; ++it ) {
		if ( !compare_head(it->first, iPrefix) ) {
			break;
		}
		oNames.push_back(it->first);
	}
}

// （変数の一括削除、接頭辞）＄名前＝（空）と同じように消す。消した個数を返す。
wstring	Satori::func_erase_variables(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size()<1 || iArgv[0].empty() ) {
		return	L"0";	// 空の接頭辞で全部消す事故を防ぐ
	}
	strvec	names;
	variable_names_with_prefix(variables, iArgv[0], names);
	for ( strvec::const_iterator it=names.begin() ; it!=names.end() ; ++it ) {
		wstring	value, result;
		SubstVariable(*it, value, result, false);
	}
	return	itos(names.size());
}

// （変数の一括コピー、元の接頭辞、先の接頭辞）元の接頭辞を先の接頭辞に付け替えた名前へ代入する。コピーした個数を返す。
wstring	Satori::func_copy_variables(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size()<2 || iArgv[0].empty() || iArgv[0]==iArgv[1] ) {
		return	L"0";
	}
	const wstring&	from = iArgv[0];
	const wstring&	to = iArgv[1];

	// 先に全部集めてから代入する（先の接頭辞が元の接頭辞で始まるときに、コピーした変数を再びコピーしないように）
	strvec	names, values;
	variable_names_with_prefix(variables, from, names);
	strvec::const_iterator it;
	for ( it=names.begin() ; it!=names.end() ; ++it ) {
		values.push_back(variables[*it]);
	}
	for ( int i=0 ; i<names.size() ; ++i ) {
		wstring	key = to + names[i].substr(from.size());
		wstring	result;
		SubstVariable(key, values[i], result, false);
	}
	return	itos(names.size());
}

// （変数の列挙、接頭辞[、区切り]）名前が接頭辞で始まる変数の名前を、名前順に区切りでつないで返す。
wstring	Satori::func_list_variables(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size()<1 ) {
		return	L"";
	}
	const wstring	delimiter = ( iArgv.size()>=2 ) ? iArgv[1] : wstring(L",");
	strvec	names;
	variable_names_with_prefix(variables, iArgv[0], names);
	wstring	result;
	for ( strvec::const_iterator it=names.begin() ; it!=names.end() ; ++it ) {
		if ( it!=names.begin() ) {
			result += delimiter;
		}
		result += *it;
	}
	return	result;
}

// （split_to、接頭辞、文字列[、区切り文字[、最大個数[、空要素を残す]]]）
// ssuのsplitと同じように分けて、接頭辞0、接頭辞1…と接頭辞の数に入れる。S0などは変えない。
wstring	Satori::func_split_to(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size()<2 || iArgv[0].empty() ) {
		return	L"";
	}
	const wstring&	prefix = iArgv[0];
	int	ref;
	wchar_t	firstChar;
	if ( IsArrayValue(prefix+L"0", ref, firstChar) ) {
		// S0やA0は変数ではないので、接頭辞には使えない
		GetSender().errsender() << L"split_to: 接頭辞「" << prefix << L"」は使えません（" << prefix << L"0 がS0などと同じ扱いになるため）。" << satori::endl;
		return	L"";
	}

	strvec	vec;
	if ( iArgv.size()==2 ) {
		split(iArgv[1], vec);
	}
	else {
		int max_words = 0;
		if ( iArgv.size() > 3 ) {
			max_words = zen2int(iArgv[3]);
		}
		bool split_one = false;
		if ( iArgv.size() > 4 ) {
			split_one = zen2int(iArgv[4]) != 0;
		}
		split(iArgv[1].c_str(), iArgv[2].c_str(), vec, max_words, split_one);
	}

	// 前回の結果が今回より多かったら、余った分を消す
	const wstring	count_name = prefix + L"の数";
	strmap::const_iterator	old = variables.find(count_name);
	if ( old != variables.end() ) {
		// 変数の値は利用者が書き換えられるので、巨大な値で延々と回らないよう添字の上限で止める
		int	old_count = zen2int(old->second);
		if ( old_count > MAX_ARRAY_INDEX ) {
			old_count = MAX_ARRAY_INDEX;
		}
		for ( int k=vec.size() ; k<old_count ; ++k ) {
			variables.erase(prefix + itos(k));
		}
	}

	for ( int i=0 ; i<vec.size() ; ++i ) {
		variables[prefix + itos(i)] = vec[i];
	}
	variables[count_name] = itos(vec.size());
	GetSender().sender() << L"split_to: " << prefix << L"0～ に " << itos(vec.size()) << L"個" << std::endl;
	return	L"";
}

//---------------------------------------------------------------------------
// 単語・文字

wstring	Satori::func_byte_value(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size() ) {
		// Unicodeのコードポイントとして扱う
		unsigned long cp = zen2ul(iArgv[0]);
		wstring r;
		if ( cp >= 0xD800 && cp <= 0xDFFF ) {
			return L"";	// サロゲート単体は文字ではない
		}
		if ( sizeof(wchar_t) == 2 && cp >= 0x10000 && cp <= 0x10FFFF ) {
			cp -= 0x10000;
			r += (wchar_t)(0xD800 + (cp >> 10));
			r += (wchar_t)(0xDC00 + (cp & 0x3FF));
		}
		else {
			r += (wchar_t)cp;
		}
		return r;
	}
	else {
		GetSender().sender() << L"error: 'バイト値' : 引数が不正です。" << std::endl;
		return L"";
	}
}

wstring	Satori::func_synthesized_words(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size() ) {
		std::vector<const Word*> vt;
		for ( strvec::const_iterator it = iArgv.begin() ; it != iArgv.end() ; ++it ) {
			words.select_all(*it,*this,vt);
		}
		if ( vt.size() ) {
			return *(vt[random(vt.size())]);
		}
		else {
			return L"";
		}
	}
	else {
		GetSender().sender() << L"error: '合成単語群' : 引数が不正です。" << std::endl;
		return L"";
	}
}

// （文の数、名前）は（文「名前」の数）と同じ
wstring	Satori::func_talk_count(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size() == 1 ) {
		wstring	r;
		var_talk_count(iArgv[0], iArgv[0], 0, r);
		return	r;
	}
	GetSender().sender() << L"error: '文の数' : 引数が不正です。" << std::endl;
	return	L"";
}

wstring	Satori::func_add_word(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size() == 2 )
	{
		Family<Word>* f = words.get_family(iArgv[0]);
		if ( f == NULL || false == f->is_exist_element(iArgv[1]) )
		{
			mAppendedWords[ iArgv[0] ].push_back( words.add_element(iArgv[0],iArgv[1],Condition()) );
			GetSender().sender() << L"単語群「" << iArgv[0] << L"」に単語「" << iArgv[1] << L"」が追加されました。" << std::endl;
		}
		else
		{
			GetSender().sender() << L"単語群「" << iArgv[0] << L"」に単語「" << iArgv[1] << L"」は既に存在します。" << std::endl;
		}
	}
	else {
		GetSender().sender() << L"error: '単語の追加' : 引数が不正です。" << std::endl;
	}
	return	L"";
}

wstring	Satori::func_remove_added_word(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size() == 2 )
	{
		Family<Word>* f = words.get_family(iArgv[0]);
		if ( f && f->is_exist_element(iArgv[1]) ) { //すでに存在し…
			std::map<wstring, std::vector<Word> >::iterator it = mAppendedWords.find(iArgv[0]);
			if ( it != mAppendedWords.end() ) { //しかも「単語の追加」で追加したもので…
				std::vector<Word> &setword = it->second;

				std::vector<Word>::iterator itrm = std::remove(setword.begin(),setword.end(),iArgv[1]);
				if ( itrm != setword.end() ) {
					setword.erase(itrm,setword.end());
					f->delete_element(iArgv[1]);

					if ( setword.empty() ) {
						mAppendedWords.erase(it);
					}
					GetSender().sender() << L"単語群「" << iArgv[0] << L"」の単語「" << iArgv[1] << L"」が削除されました。" << std::endl;
					if ( f->empty() ) {
						words.erase(iArgv[0]);
					}
				}
			}
		}
	}
	else {
		GetSender().sender() << L"error: '追加単語の削除' : 引数が不正です。" << std::endl;
	}
	return	L"";
}

wstring	Satori::func_remove_all_added_words(const strvec& iArgv, bool, bool)
{
	if ( iArgv.size() == 1 )
	{
		Family<Word>* f = words.get_family(iArgv[0]);
		if ( f ) { //すでに存在し…
			std::map<wstring, std::vector<Word> >::iterator it = mAppendedWords.find(iArgv[0]);
			if ( it != mAppendedWords.end() ) { //しかも「単語の追加」で追加したもので…
				std::vector<Word> &setword = it->second;
				for ( std::vector<Word>::const_iterator its = setword.begin(); its != setword.end() ; ++its ) {
					f->delete_element(*its);
				}
				mAppendedWords.erase(it);

				GetSender().sender() << L"単語群「" << iArgv[0] << L"」に追加された単語は全て削除されました。" << std::endl;
			}
			if ( f->empty() ) {
				words.erase(iArgv[0]);
			}
		}
	}
	else {
		GetSender().sender() << L"error: '単語の削除' : 引数が不正です。" << std::endl;
	}
	return	L"";
}

//---------------------------------------------------------------------------
// 本体との連携

//get_property関数用のハンドラと結果格納
#ifdef POSIX

static std::string SendDataUsingUnixSocket(const std::string &path, const std::string &request, bool has_header) {
	sockaddr_un addr = {};
	if (path.length() >= sizeof(addr.sun_path)) {
		return "";
	}
	int soc = socket(AF_UNIX, SOCK_STREAM, 0);
	if (soc == -1) {
		return "";
	}
	addr.sun_family = AF_UNIX;
	// null-terminatedも書き込ませる
	strncpy(addr.sun_path, path.c_str(), path.length() + 1);
	if (connect(soc, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
		return "";
	}
	if (send(soc, request.data(), request.size(), 0) != request.size()) {
		close(soc);
		return "";
	}
	shutdown(soc, SHUT_WR);
	char buffer[BUFFER_SIZE] = {};
	std::string data;
	uint32_t remain = 0;
	if (has_header) {
		if (read(soc, buffer, sizeof(uint32_t)) != sizeof(uint32_t)) {
			close(soc);
			return "";
		}
		remain = *reinterpret_cast<uint32_t *>(buffer);
		data.reserve(remain);
	}
	while (true) {
		int ret = read(soc, buffer, BUFFER_SIZE);
		if (ret == -1) {
			close(soc);
			return "";
		}
		if (ret == 0) {
			close(soc);
			break;
		}
		if (!has_header || remain > ret) {
			data.append(buffer, ret);
		}
		else {
			data.append(buffer, remain);
		}
		remain -= ret;
	}
	return data;
}

static bool SendDirectSSTP(const void* targetHWnd, const std::wstring &sendText, std::wstring &result)
{
    result = L"";
    shm_t *shm;
    int fd = shm_open("/ninix", O_RDWR, 0);
    if (fd == -1) {
        return false;
    }
    shm = static_cast<shm_t *>(mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    if (shm == MAP_FAILED) {
        return false;
    }
    if (sem_wait(&shm->sem) == -1) {
        return false;
    }
    std::string path(shm->buf, shm->size);
    if (sem_post(&shm->sem) == -1) {
        return false;
    }
    std::string data = SendDataUsingUnixSocket(path + "ninix", "GetFMO\r\n", true);
    if (data.empty()) {
        return false;
    }
    int target = reinterpret_cast<long>(targetHWnd);
    std::istringstream iss(data);
    std::string uuid;
    while (true) {
        if (!iss) {
            return false;
        }
        std::string tmp;
        int hwnd;
        std::getline(iss, tmp);
        std::istringstream line(tmp);
        std::getline(iss, uuid, '.');
        std::getline(iss, tmp, '\x01');
        if (tmp != "hwnd") {
            continue;
        }
        std::getline(iss, tmp);
        if (target == atoi(tmp.c_str())) {
            break;
        }
    }
    std::string request = "EXECUTE SSTP/1.1\r\nCharset: UTF-8\r\n" + WtoUTF8(sendText) + "Sender: Satori\r\n\r\n";
    data = SendDataUsingUnixSocket(path + uuid, request, false);
    std::wstring response = MBtoW(data, CharsetFromName(UTF8toW(find_charset_header(data))));
    std::wstring header = cut_token(response, CRLF);
    cut_token(header, L" ");
    if (header == L"200 OK")
    {
        //1行文読み捨て
        cut_token(response, CRLF);
        result = cut_token(response, CRLF);
        return true;
    }
    else {
        return false;
    }
}

#else

static std::wstring execute_result;
static bool execute_succeeded;

static LRESULT CALLBACK GetPropertyHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_COPYDATA)
	{
		const COPYDATASTRUCT* cds = (const COPYDATASTRUCT*)lparam;
		std::string recv_bytes((const char*)cds->lpData, cds->cbData);
		wstring recv_str = MBtoW(recv_bytes, CharsetFromName(UTF8toW(find_charset_header(recv_bytes))));

		wstring header = cut_token(recv_str, CRLF);
		cut_token(header, L" ");
		if (header == L"200 OK")
		{
			//1行文読み捨て
			cut_token(recv_str, CRLF);
			execute_result = cut_token(recv_str, CRLF);
			execute_succeeded = true;
		}
	}
	return CallWindowProc(DefWindowProc, hwnd, message, wparam, lparam);
}

static bool SendDirectSSTP(const void* targetHWnd, std::wstring sendText, std::wstring &result)
{
	execute_result = L"";
	execute_succeeded = false;

	//結果受信用ウインドウ作成: リソースの仕様を局所化してみたけどオーバーヘッドがでかい場合はSHIORIの初期化周辺に絡めるといいのかも
	const wchar_t* windowname = L"satori_get_property";

	WNDCLASSEX windowClass;
	ZeroMemory(&windowClass,sizeof(windowClass));

	windowClass.cbSize = sizeof(windowClass);
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.lpszClassName = windowname;
	windowClass.lpfnWndProc = ::GetPropertyHandler;

	::RegisterClassEx(&windowClass);

	HWND propertyWindow = ::CreateWindow(windowname, windowname, 0, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

	std::string sendData = "EXECUTE SSTP/1.1\r\nCharset: UTF-8\r\n" + WtoUTF8(sendText) + "Sender: Satori\r\n\r\n";

	//メッセージ転送
	COPYDATASTRUCT cds;
	cds.dwData = 9801;
	cds.cbData = sendData.size();
	cds.lpData = malloc(cds.cbData);
	memcpy(cds.lpData, sendData.c_str(), cds.cbData);

	/*LRESULT res =*/ ::SendMessage((HWND)targetHWnd, WM_COPYDATA, (WPARAM)propertyWindow, (LPARAM)&cds);

	//リソースの開放
	free(cds.lpData);

	::DestroyWindow(propertyWindow);
	::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

	result = execute_result;
	return execute_succeeded;
}
#endif

wstring	Satori::func_get_property(const strvec& iArgv, bool, bool)
{
	if (iArgv.size() >= 1)
	{
		const void* targetHWnd = characters_hwnd[0];
		std::wostringstream ost;
		ost << L"Command: GetProperty\r\nReference0: " << iArgv[0] << L"\r\n";
		std::wstring sendData = ost.str();

		std::wstring result;
		if (SendDirectSSTP(targetHWnd, sendData, result)) {
			return result;
		}
		return (iArgv.size() >= 2) ? iArgv[1] : L"";
	}
	return	L"";
}

wstring	Satori::func_set_property(const strvec& iArgv, bool, bool)
{
	if (iArgv.size() >= 2)
	{
		const void* targetHWnd = characters_hwnd[0];
		std::wostringstream ost;
		ost << L"Command: SetProperty\r\nReference0: " << iArgv[0] << L"\r\nReference1: " << iArgv[1] << L"\r\n";
		std::wstring sendData = ost.str();

		std::wstring result;
		SendDirectSSTP(targetHWnd, sendData, result);
		return result;
	}
	return	L"";
}

wstring	Satori::func_load_saori(const strvec& iArgv, bool, bool)
{
	if (iArgv.size() >= 2)
	{
		wstring load_line = iArgv[0];
		for (int i = 1; i < iArgv.size(); ++i)
			load_line += L"," + iArgv[i];

		mShioriPlugins->load_a_plugin(load_line);
	}
	return	L"";
}

//---------------------------------------------------------------------------
// 特殊形式（条件・繰り返し）

wstring	Satori::func_when(const strvec& iArgv, bool for_calc, bool for_non_talk)
{
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

wstring	Satori::func_whenlist(const strvec& iArgv, bool for_calc, bool for_non_talk)
{
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

wstring	Satori::func_times(const strvec& iArgv, bool for_calc, bool for_non_talk)
{
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
			// max += count が int を越えないように（越えるときは上限にする）
			if ( count > 0 && max > INT_MAX - count ) {
				max = INT_MAX;
			}
			else {
				max += count;
			}
			body = 2;
		}
		else{
			throw(L"引数の個数が正しくありません。");
		}
		for(int i=count; i<max; i++){
			if ( use_call_budget() ) {
				break;
			}
			mLoopCounters.top() = itos(i);
			ret += UnKakko(iArgv[body].c_str(), for_calc, for_non_talk);
			if ( loop_result_too_large(ret) ) {
				break;
			}
		}
	}
	catch( const wchar_t *str ){
		ret = str;
	}
	mLoopCounters.pop();
	return ret;
}

wstring	Satori::func_while(const strvec& iArgv, bool for_calc, bool for_non_talk)
{
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
			if ( use_call_budget() ) {
				break;
			}
			mLoopCounters.top() = itos(i);
			if ( !calc_argument(iArgv[expression], result, for_non_talk) ) throw(L"' 式が計算不能です。");
			if ( result == 0 ) {
				break;
			}
			ret += UnKakko(iArgv[body].c_str(), for_calc, for_non_talk);
			if ( loop_result_too_large(ret) ) {
				break;
			}
		}
	}
	catch(const wchar_t * str){
		ret = str;
	}
	mLoopCounters.pop();
	return ret;
}

wstring	Satori::func_for(const strvec& iArgv, bool for_calc, bool for_non_talk)
{
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
		// 増分の大きさ。INT_MINは符号を反転できないので INT_MAX にする
		const unsigned int width = ( step==INT_MIN ) ? static_cast<unsigned int>(INT_MAX) : static_cast<unsigned int>( step<0 ? -step : step );
		if ( start <= end ) {
			for(int i=start; ; ) {
				if ( use_call_budget() ) {
					break;
				}
				mLoopCounters.top() = itos(i);
				ret += UnKakko(iArgv[body].c_str(), for_calc, for_non_talk);
				if ( loop_result_too_large(ret) ) {
					break;
				}
				// i+=step が int を越えないように、残りの幅と比べる
				if ( width > static_cast<unsigned int>(end) - static_cast<unsigned int>(i) ) {
					break;
				}
				i += static_cast<int>(width);
			}
		}
		else {
			for(int i=start; ; ) {
				if ( use_call_budget() ) {
					break;
				}
				mLoopCounters.top() = itos(i);
				ret += UnKakko(iArgv[body].c_str(), for_calc, for_non_talk);
				if ( loop_result_too_large(ret) ) {
					break;
				}
				if ( width > static_cast<unsigned int>(i) - static_cast<unsigned int>(end) ) {
					break;
				}
				i -= static_cast<int>(width);
			}
		}
	}
	catch(const wchar_t *str){
		ret = str;
	}
	mLoopCounters.pop();
	return ret;
}


//===========================================================================
//	内蔵変数
//===========================================================================

// iNameが内蔵変数なら値をoResultに入れ、trueを返す。
// 単語群・文・変数に同じ名前が無いときだけ呼ばれる。
bool	Satori::GetBuiltinValue(const wstring& iName, wstring& oResult)
{
	// 名前が完全に一致するもの
	static const BuiltinName names[] = {
		{ L"里々のバージョン",		&Satori::var_version,		0 },
		{ L"里々のライセンス",		&Satori::var_license,		0 },

		{ L"現在年",				&Satori::var_current_time,	TIME_YEAR },
		{ L"現在月",				&Satori::var_current_time,	TIME_MONTH },
		{ L"現在日",				&Satori::var_current_time,	TIME_DAY },
		{ L"現在時",				&Satori::var_current_time,	TIME_HOUR },
		{ L"現在分",				&Satori::var_current_time,	TIME_MINUTE },
		{ L"現在秒",				&Satori::var_current_time,	TIME_SECOND },
		{ L"現在曜日",				&Satori::var_current_time,	TIME_WEEKDAY },

		{ L"起動時",				&Satori::var_uptime,		UPTIME_FROM_LOAD | UPTIME_HOUR },
		{ L"起動分",				&Satori::var_uptime,		UPTIME_FROM_LOAD | UPTIME_MINUTE },
		{ L"起動秒",				&Satori::var_uptime,		UPTIME_FROM_LOAD | UPTIME_SECOND },
		{ L"単純起動時",			&Satori::var_uptime,		UPTIME_FROM_LOAD | UPTIME_TOTAL_HOURS },
		{ L"単純起動分",			&Satori::var_uptime,		UPTIME_FROM_LOAD | UPTIME_TOTAL_MINUTES },
		{ L"単純起動秒",			&Satori::var_uptime,		UPTIME_FROM_LOAD | UPTIME_TOTAL_SECONDS },
		{ L"OS起動時",				&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_HOUR },
		{ L"OS起動分",				&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_MINUTE },
		{ L"OS起動秒",				&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_SECOND },
		{ L"単純OS起動時",			&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_TOTAL_HOURS },
		{ L"単純OS起動分",			&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_TOTAL_MINUTES },
		{ L"単純OS起動秒",			&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_TOTAL_SECONDS },
		{ L"ＯＳ起動時",			&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_HOUR },
		{ L"ＯＳ起動分",			&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_MINUTE },
		{ L"ＯＳ起動秒",			&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_SECOND },
		{ L"単純ＯＳ起動時",		&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_TOTAL_HOURS },
		{ L"単純ＯＳ起動分",		&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_TOTAL_MINUTES },
		{ L"単純ＯＳ起動秒",		&Satori::var_uptime,		UPTIME_FROM_OS | UPTIME_TOTAL_SECONDS },
		{ L"累計時",				&Satori::var_uptime,		UPTIME_TOTAL | UPTIME_HOUR },
		{ L"累計分",				&Satori::var_uptime,		UPTIME_TOTAL | UPTIME_MINUTE },
		{ L"累計秒",				&Satori::var_uptime,		UPTIME_TOTAL | UPTIME_SECOND },
		{ L"単純累計時",			&Satori::var_uptime,		UPTIME_TOTAL | UPTIME_TOTAL_HOURS },
		{ L"単純累計分",			&Satori::var_uptime,		UPTIME_TOTAL | UPTIME_TOTAL_MINUTES },
		{ L"単純累計秒",			&Satori::var_uptime,		UPTIME_TOTAL | UPTIME_TOTAL_SECONDS },
		{ L"最終トークからの経過秒",	&Satori::var_seconds_from_last_talk,	0 },

		{ L"隣で起動しているゴースト",	&Satori::var_neighbor_ghost,	0 },
		{ L"起動しているゴースト数",	&Satori::var_ghost_count,		0 },

		{ L"セーブデータ読み込み",	&Satori::var_savedata_status,	0 },
		{ L"次のトーク",			&Satori::var_next_talk,			0 },
		{ L"予約トーク数",			&Satori::var_reserved_talk_count,	0 },
		{ L"イベント名",			&Satori::var_event_name,		0 },
		{ L"直前の選択肢名",		&Satori::var_last_choice_name,	0 },
		{ L"本体の所在",			&Satori::var_exe_folder,		0 },
	};

	// 全角を半角にした名前が完全に一致するもの
	static const BuiltinName hankaku_names[] = {
		{ L"Aの数",		&Satori::var_argument_count,	L'A' },
		{ L"Rの数",		&Satori::var_argument_count,	L'R' },
		{ L"Sの数",		&Satori::var_argument_count,	L'S' },
		{ L"time_t",	&Satori::var_time_t,			0 },
		{ L"pwd",		&Satori::var_base_folder,		0 },
	};

	// 名前の先頭・末尾で決まるもの。上から順に調べる。
	static const BuiltinNamePattern patterns[] = {
		// 先頭				末尾						半角	処理
		{ L"乱数",			L"",						false,	&Satori::var_random,			0 },
		{ L"サーフェス",	L"",						false,	&Satori::var_surface,			0 },
		{ L"前回終了時サーフェス",	L"",				false,	&Satori::var_last_exiting_surface,	0 },
		{ L"ウィンドウハンドル",	L"",				false,	&Satori::var_window_handle,		0 },
		{ L"isempty",		L"",						false,	&Satori::var_isempty,			0 },
		{ L"文「",			L"」の存在",				false,	&Satori::var_talk_exists,		0 },
		{ L"文「",			L"」の数",					false,	&Satori::var_talk_count,		0 },
		{ L"単語群「",		L"」の存在",				false,	&Satori::var_word_exists,		0 },
		{ L"単語群「",		L"」の数",					false,	&Satori::var_word_count,		0 },
		{ L"単語群「",		L"」の重複回避枯渇",		false,	&Satori::var_word_used_all,		0 },
		{ L"変数「",		L"」の存在",				false,	&Satori::var_variable,			VARIABLE_EXISTS },
		{ L"変数「",		L"」か０",					false,	&Satori::var_variable,			VARIABLE_OR_ZERO },
		{ L"変数「",		L"」か空文字列",			false,	&Satori::var_variable,			VARIABLE_OR_EMPTY },
		{ L"導入済みゴースト「",		L"」の存在",	false,	&Satori::var_installed,			INSTALLED_GHOST },
		{ L"導入済みシェル「",			L"」の存在",	false,	&Satori::var_installed,			INSTALLED_SHELL },
		{ L"導入済みバルーン「",		L"」の存在",	false,	&Satori::var_installed,			INSTALLED_BALLOON },
		{ L"導入済みヘッドライセンサ「",	L"」の存在",	false,	&Satori::var_installed,		INSTALLED_HEADLINE },
		{ L"導入済みフォント「",		L"」の存在",	false,	&Satori::var_installed,			INSTALLED_FONT },
		{ L"導入済みプラグイン「",		L"」の存在",	false,	&Satori::var_installed,			INSTALLED_PLUGIN },
		{ L"導入済みプラグイン「",		L"」のID",		false,	&Satori::var_plugin_id,			0 },
		{ L"使ってるぞグラフ「",	L"」の本体側の名前",	false,	&Satori::var_rate_of_use_graph,	GRAPH_SAKURA_NAME },
		{ L"使ってるぞグラフ「",	L"」の相方側の名前",	false,	&Satori::var_rate_of_use_graph,	GRAPH_KERO_NAME },
		{ L"使ってるぞグラフ「",	L"」の起動回数",	false,	&Satori::var_rate_of_use_graph,	GRAPH_BOOT_COUNT },
		{ L"使ってるぞグラフ「",	L"」の単純累計分",	false,	&Satori::var_rate_of_use_graph,	GRAPH_BOOT_MINUTES },
		{ L"使ってるぞグラフ「",	L"」の起動割合",	false,	&Satori::var_rate_of_use_graph,	GRAPH_BOOT_PERCENT },
		{ L"使ってるぞグラフ「",	L"」の状態",		false,	&Satori::var_rate_of_use_graph,	GRAPH_STATUS },
		{ L"ウインドウ「",	L"」の存在",				false,	&Satori::var_window_exists,		0 },
		{ L"「",			L"」を含むウインドウの存在",	false,	&Satori::var_window_exists,	1 },
		{ L"プロセス「",	L"」の存在",				false,	&Satori::var_process_exists,	0 },
		{ L"「",			L"」を含むプロセスの存在",	false,	&Satori::var_process_exists,	1 },
		{ L"起動中ゴースト「",	L"」の存在",			false,	&Satori::var_running_ghost_exists,	0 },
		{ L"",				L"の存在",					false,	&Satori::var_ghost_exists,		0 },
		{ L"",				L"のサーフェス",			false,	&Satori::var_ghost_surface,		0 },
		{ L"FMO",			L"",						true,	&Satori::var_fmo,				0 },
		{ L"count",			L"",						true,	&Satori::var_count,				0 },
		{ L"次から",		L"回目のトーク",			false,	&Satori::var_reserved_talk,		0 },
		{ L"トーク「",		L"」の予約有無",			false,	&Satori::var_is_talk_reserved,	0 },
	};

	static std::map<wstring, const BuiltinName*>	name_map, hankaku_name_map;
	if ( name_map.empty() ) {
		for ( int i=0 ; i<sizeof(names)/sizeof(names[0]) ; ++i ) {
			name_map[names[i].name] = &names[i];
		}
		for ( int j=0 ; j<sizeof(hankaku_names)/sizeof(hankaku_names[0]) ; ++j ) {
			hankaku_name_map[hankaku_names[j].name] = &hankaku_names[j];
		}
	}

	oResult = L"";

	// ここにある名前は、下のどのパターンにも当てはまらないので先に調べてよい。
	std::map<wstring, const BuiltinName*>::const_iterator found = name_map.find(iName);
	if ( found != name_map.end() ) {
		return	(this->*(found->second->handler))(iName, iName, found->second->param, oResult);
	}

	const wstring hankaku = zen2han(iName);

	// 数字はサーフェス切り替え
	if ( aredigits(hankaku) || (hankaku.c_str()[0]==L'-' && aredigits(hankaku.c_str()+1)) ) {
		int	s = stoi_internal(hankaku);
		oResult = wstring(INTERNAL_MARK_STR) + INTERNAL_MARK_SURFACE_STR + itos(s) + INTERNAL_MARK_STR; //内部特殊表現に一旦変換して、後でサーフェス加算処理をする
		return	true;
	}

	found = hankaku_name_map.find(hankaku);
	if ( found != hankaku_name_map.end() ) {
		return	(this->*(found->second->handler))(hankaku, hankaku, found->second->param, oResult);
	}

	for ( int i=0 ; i<sizeof(patterns)/sizeof(patterns[0]) ; ++i ) {
		const BuiltinNamePattern& pt = patterns[i];
		const wstring& name = pt.use_hankaku ? hankaku : iName;
		if ( !match_head_tail(name, pt.head, pt.tail) ) {
			continue;
		}
		if ( (this->*(pt.handler))(name, strip_head_tail(name, pt.head, pt.tail), pt.param, oResult) ) {
			return	true;
		}
	}

	// リクエストのヘッダ
	strmap::const_iterator header = mRequestMap.find(iName);
	if ( header != mRequestMap.end() ) {
		oResult = header->second;
		return	true;
	}

	if ( iName == L"全変数列挙" ) {
		if (fDebugMode && secure_flag) {
			for (strmap::const_iterator i = variables.begin(); i != variables.end(); i++){
				oResult += wstring(L"＄") + i->first + L"\t" + i->second + L"\\n";
			}
		}
		return	true;
	}

	return	false;
}

//---------------------------------------------------------------------------
// 里々・時刻

bool	Satori::var_version(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = gSatoriVersion;
	return	true;
}

bool	Satori::var_license(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = gSatoriLicense;
	replace(oResult, L"\n", L"\\n");
	return	true;
}

bool	Satori::var_current_time(const wstring&, const wstring&, int iParam, wstring& oResult)
{
	static const wchar_t* const ary[7]={L"日",L"月",L"火",L"水",L"木",L"金",L"土"};
#ifdef POSIX
	time_t st = time(NULL);
	struct tm* st_tm = localtime(&st);
	switch ( iParam ) {
	case TIME_YEAR:		oResult = int2zen(st_tm->tm_year + 1900); break;
	case TIME_MONTH:	oResult = int2zen(st_tm->tm_mon + 1); break;
	case TIME_DAY:		oResult = int2zen(st_tm->tm_mday); break;
	case TIME_HOUR:		oResult = int2zen(st_tm->tm_hour); break;
	case TIME_MINUTE:	oResult = int2zen(st_tm->tm_min); break;
	case TIME_SECOND:	oResult = int2zen(st_tm->tm_sec); break;
	case TIME_WEEKDAY:	oResult = (st_tm->tm_wday >= 0 && st_tm->tm_wday < 7) ? ary[st_tm->tm_wday] : L"？"; break;
	}
#else
	SYSTEMTIME st; ::GetLocalTime(&st);
	switch ( iParam ) {
	case TIME_YEAR:		oResult = int2zen(st.wYear); break;
	case TIME_MONTH:	oResult = int2zen(st.wMonth); break;
	case TIME_DAY:		oResult = int2zen(st.wDay); break;
	case TIME_HOUR:		oResult = int2zen(st.wHour); break;
	case TIME_MINUTE:	oResult = int2zen(st.wMinute); break;
	case TIME_SECOND:	oResult = int2zen(st.wSecond); break;
	case TIME_WEEKDAY:	oResult = ( st.wDayOfWeek >= 0 && st.wDayOfWeek < 7 ) ? ary[st.wDayOfWeek] : L"？"; break;
	}
#endif
	return	true;
}

// iParamは UPTIME_FROM_* と UPTIME_* の組み合わせ
bool	Satori::var_uptime(const wstring&, const wstring&, int iParam, wstring& oResult)
{
	const int kind = iParam & UPTIME_KIND_MASK;

	if ( (iParam & UPTIME_FROM_MASK) == UPTIME_TOTAL ) {
		// 累計
		unsigned long sec = posix_get_current_sec() - sec_count_at_load + sec_count_total;
		unsigned long hour = sec / 60 / 60;
		unsigned long minute = (sec - hour * 60 * 60) / 60;
		switch ( kind ) {
		case UPTIME_HOUR:			oResult = ul2zen(hour); break;
		case UPTIME_MINUTE:			oResult = ul2zen(minute); break;
		case UPTIME_SECOND:			oResult = ul2zen(sec - hour * 60 * 60 - minute * 60); break;
		case UPTIME_TOTAL_HOURS:	oResult = ul2zen(sec / 60 / 60); break;
		case UPTIME_TOTAL_MINUTES:	oResult = ul2zen(sec / 60); break;
		case UPTIME_TOTAL_SECONDS:	oResult = ul2zen(sec); break;
		}
	}
	else {
		// 起動・OS起動
		time_t sec = posix_get_current_sec();
		if ( (iParam & UPTIME_FROM_MASK) == UPTIME_FROM_LOAD ) {
			sec -= sec_count_at_load;
		}
		time_t hour = sec / 60 / 60;
		time_t minute = (sec - hour * 60 * 60) / 60;
		switch ( kind ) {
		case UPTIME_HOUR:			oResult = int2zen(hour); break;
		case UPTIME_MINUTE:			oResult = int2zen(minute); break;
		case UPTIME_SECOND:			oResult = int2zen(sec - hour * 60 * 60 - minute * 60); break;
		case UPTIME_TOTAL_HOURS:	oResult = int2zen(sec / 60 / 60); break;
		case UPTIME_TOTAL_MINUTES:	oResult = int2zen(sec / 60); break;
		case UPTIME_TOTAL_SECONDS:	oResult = int2zen(sec); break;
		}
	}
	return	true;
}

bool	Satori::var_time_t(const wstring&, const wstring&, int, wstring& oResult)
{
	time_t tm;
	time(&tm);
	oResult = int2zen(tm);
	return	true;
}

bool	Satori::var_seconds_from_last_talk(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = int2zen(second_from_last_talk);
	return	true;
}

// 乱数m～n
bool	Satori::var_random(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	if ( iArg.size() < 2 ) {
		return	false;
	}
	strvec	vec;
	// 区切りは FULLWIDTH TILDE(U+FF5E, CP932の0x8160) と WAVE DASH(U+301C) の両方を受け付ける
	if ( split( iArg.c_str(), L"\xFF5E\x301C", vec ) != 2 ) {
		oResult = L"※　乱数の指定が変です　※";
	}
	else {
		wstring vec0 = zen2han(vec[0]);
		int	bottom = stoi_internal(vec0);
		int	top = zen2int(vec[1]);
		if ( bottom > top )
			Swap(&bottom, &top);

		if ( vec0 != vec[0] ) {
			if ( bottom == top )
				oResult = int2zen(top);
			else
				oResult = int2zen( random(top-bottom+1) + bottom );
		}
		else {
			if ( bottom == top )
				oResult = itos(top);
			else
				oResult = itos( random(top-bottom+1) + bottom );
		}
	}
	return	true;
}

// isemptyの直後に1文字だけあれば1
bool	Satori::var_isempty(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	if ( iArg.empty() ) {
		return	false;
	}
	const wchar_t* p = iArg.c_str();
	get_a_chr(p);
	oResult = (*p==L'\0') ? L"1" : L"0";
	return	true;
}

// A・R・Sの数
bool	Satori::var_argument_count(const wstring&, const wstring&, int iParam, wstring& oResult)
{
	switch ( iParam ) {
	case L'A':
		oResult = mCallStack.empty() ? wstring(L"0") : itos(mCallStack.top().size());
		break;
	case L'R':
		oResult = itos(mReferences.size());
		break;
	case L'S':
		oResult = itos(mKakkoCallResults.size());
		break;
	}
	return	true;
}

//---------------------------------------------------------------------------
// サーフェス・ゴースト

bool	Satori::var_surface(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	if ( !aredigits(iArg) ) {
		return	false;
	}
	oResult = itos(cur_surface[ zen2int(iArg) ]);
	return	true;
}

bool	Satori::var_last_exiting_surface(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	if ( iArg.empty() ) {
		return	false;
	}
	oResult = itos(last_talk_exiting_surface[ zen2int(iArg) ]);
	return	true;
}

bool	Satori::var_window_handle(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	if ( iArg.empty() ) {
		return	false;
	}
	std::map<int,void*>::const_iterator found = characters_hwnd.find(zen2int(iArg));
	if ( found != characters_hwnd.end() ) {
		// NOTE: sizeof(void *) == sizeof(long)
#ifdef POSIX
		oResult = uitos((unsigned long)found->second);
#else
		oResult = uitos((unsigned int)found->second);
#endif // POSIX
	}
	return	true;
}

bool	Satori::var_neighbor_ghost(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = ( otherghostname.size()>=1 ) ? *otherghostname.begin() : L""; //自分自身はotherghostnameには含まない
	return	true;
}

bool	Satori::var_ghost_count(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = int2zen(otherghostname.size()+1); //自分自身はotherghostnameには含まないので +1
	return	true;
}

bool	Satori::var_running_ghost_exists(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	oResult = otherghostname.count(iArg) ? L"1" : L"0"; //自分自身はotherghostnameには含まない
	return	true;
}

// ゴースト名の存在（名前の先頭が本体側か相方側の名前）
bool	Satori::var_ghost_exists(const wstring& iName, const wstring&, int, wstring& oResult)
{
	updateGhostsInfo();	// ゴースト情報を更新
	std::vector<strmap>::iterator i=ghosts_info.begin();
	for ( ; i!=ghosts_info.end() ; ++i )
		if ( compare_head(iName, (*i)[L"name"]) )
			break;
		else if ( compare_head(iName, (*i)[L"keroname"]) )
			break;
	oResult = ( i==ghosts_info.end() ) ? L"0" : L"1";
	return	true;
}

// ゴースト名のサーフェス
bool	Satori::var_ghost_surface(const wstring& iName, const wstring&, int, wstring& oResult)
{
	updateGhostsInfo();	// ゴースト情報を更新
	std::vector<strmap>::iterator i=ghosts_info.begin();
	for ( ; i!=ghosts_info.end() ; ++i )
		if ( compare_head(iName, (*i)[L"name"]) ) {
			oResult = (*i)[L"sakura.surface"];
			break;
		} else if ( compare_head(iName, (*i)[L"keroname"]) ) {
			oResult = (*i)[L"kero.surface"];
			break;
		}

	if ( i==ghosts_info.end() ) {
		oResult = L"-1";
	}
	return	true;
}

// FMOn項目名（全角は半角にしてある）
bool	Satori::var_fmo(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	if ( iArg.size() < 2 ) {
		return	false;
	}
	const wchar_t* p = iArg.c_str();
	unsigned int digit = 0;
	while ( p[digit] >= L'0' && p[digit] <= L'9' ) { ++digit; }
	if ( digit == 0 ) {
		return	false;
	}

	updateGhostsInfo();	// ゴースト情報を更新
	unsigned int index = wcstoul(iArg.c_str(),NULL,10);
	if ( index < ghosts_info.size() ) {
		strmap&	m=ghosts_info[index];
		wstring	value(iArg.c_str()+digit);
		if ( m.find(value) != m.end() ) {
			oResult = m[value];
		}
	}
	return	true;
}

//---------------------------------------------------------------------------
// 辞書・変数

bool	Satori::var_talk_exists(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	oResult = talks.is_exist(iArg) ? L"1" : L"0";
	return	true;
}

bool	Satori::var_talk_count(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	Family<Talk>* f = talks.get_family(iArg);
	oResult = f ? itos(f->size_of_element()) : wstring(L"0");
	return	true;
}

bool	Satori::var_word_exists(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	oResult = words.is_exist(iArg) ? L"1" : L"0";
	return	true;
}

bool	Satori::var_word_count(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	Family<Word>* f = words.get_family(iArg);
	oResult = int2zen( f ? f->size_of_element() : 0 );
	return	true;
}

bool	Satori::var_word_used_all(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	Family<Word>* f = words.get_family(iArg);
	oResult = ( f && f->is_OC_used_all(*this) ) ? L"1" : L"0";
	return	true;
}

bool	Satori::var_variable(const wstring&, const wstring& iArg, int iParam, wstring& oResult)
{
	bool isSysValue;
	wstring *v = GetValue(iArg,isSysValue); //こっちはシステム変数かどうかどっちでもいい
	switch ( iParam ) {
	case VARIABLE_EXISTS:	oResult = v ? L"1" : L"0"; break;
	case VARIABLE_OR_ZERO:	oResult = v ? *v : L"０"; break;
	case VARIABLE_OR_EMPTY:	oResult = v ? *v : L""; break;
	}
	return	true;
}

// countの後ろの名前で辞書の統計を返す
bool	Satori::var_count(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	int r = count_func(iArg);
	if ( r < 0 ) {
		return	false;
	}
	oResult = int2zen(r);
	return	true;
}

//countコール・getaistate互換用
int Satori::count_func(const wstring &name)
{
	if ( name==L"Words" ) { return words.size_of_family(); }
	else if ( name==L"Variable" ) { return variables.size(); }
	else if ( name==L"Anchor" ) { return anchors.size(); }
	else if ( name==L"Talk" ) { return talks.size_of_element(); }
	else if ( name==L"Word" ) { return words.size_of_element(); }
	else if ( name==L"NoNameTalk" )
	{
		Family<Talk>* f = talks.get_family(L"");
		return ( f==0 ) ? 0 : f->size_of_element();
	}
	else if ( name==L"EventTalk" )
	{
		int	n=0;
		for ( std::map< wstring, Family<Talk> >::const_iterator it = talks.compatible().begin() ; it != talks.compatible().end() ; ++it ) {
			if ( compare_head(it->first, L"On") ) {
				n += it->second.size_of_element();
			}
		}
		return n;
	}
	else if ( name==L"OtherTalk" )
	{
		int	n=0;
		for ( std::map< wstring, Family<Talk> >::const_iterator it = talks.compatible().begin() ; it != talks.compatible().end() ; ++it ) {
			if ( !compare_head(it->first, L"On") && !it->first.empty() ) {
				n += it->second.size_of_element();
			}
		}
		return n;
	}
	else if ( name==L"Line" )
	{
		int	n=0;
		for ( std::map< wstring, Family<Talk> >::const_iterator it = talks.compatible().begin() ; it != talks.compatible().end() ; ++it )
		{
			std::vector<const Talk*> v;
			it->second.get_elements_pointers(v);
			for ( std::vector<const Talk*>::const_iterator el_it = v.begin() ; el_it != v.end() ; ++el_it )
			{
				n += (*el_it)->size();
			}
		}
		for ( std::map< wstring, Family<Word> >::const_iterator it = words.compatible().begin() ; it != words.compatible().end() ; ++it )
		{
			n += it->second.size_of_element();
		}
		return n;
	}
	else if ( name==L"Parenthesis" )
	{
		int	n=0;
		for ( std::map< wstring, Family<Talk> >::const_iterator it = talks.compatible().begin() ; it != talks.compatible().end() ; ++it )
		{
			std::vector<const Talk*> v;
			it->second.get_elements_pointers(v);
			for ( std::vector<const Talk*>::const_iterator el_it = v.begin() ; el_it != v.end() ; ++el_it )
			{
				for ( Talk::const_iterator tk_it = (*el_it)->begin() ; tk_it != (*el_it)->end() ; ++tk_it )
				{
					n += count(*tk_it, L"（");
				}
			}
		}
		for ( std::map< wstring, Family<Word> >::const_iterator it = words.compatible().begin() ; it != words.compatible().end() ; ++it )
		{
			std::vector<const Word*> v;
			it->second.get_elements_pointers(v);
			for ( std::vector<const Word*>::const_iterator el_it = v.begin() ; el_it != v.end() ; ++el_it )
			{
				n += count(**el_it, L"（");
			}
		}
		return n;
	}
	return -1;
}

//---------------------------------------------------------------------------
// トーク予約・リクエスト

bool	Satori::var_savedata_status(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = load_savedata_status;
	return	true;
}

bool	Satori::var_next_talk(const wstring&, const wstring&, int, wstring& oResult)
{
	std::map<int,wstring>::const_iterator it = reserved_talk.find(1);
	if ( it != reserved_talk.end() )
		oResult = it->second;
	return	true;
}

// 次からn回目のトーク
bool	Satori::var_reserved_talk(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	std::map<int,wstring>::const_iterator it = reserved_talk.find(zen2int(iArg));
	if ( it != reserved_talk.end() ) {
		oResult = it->second;
	}
	return	true;
}

bool	Satori::var_is_talk_reserved(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	oResult = L"0";
	for (std::map<int, wstring>::const_iterator it=reserved_talk.begin(); it!=reserved_talk.end() ; ++it) {
		if ( iArg == it->second ) {
			oResult = L"1";
			break;
		}
	}
	return	true;
}

bool	Satori::var_reserved_talk_count(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = int2zen( reserved_talk.size() );
	return	true;
}

bool	Satori::var_event_name(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = mRequestID;
	return	true;
}

bool	Satori::var_last_choice_name(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = last_choice_name;
	return	true;
}

bool	Satori::var_base_folder(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = mBaseFolder;
	return	true;
}

bool	Satori::var_exe_folder(const wstring&, const wstring&, int, wstring& oResult)
{
	oResult = mExeFolder;
	return	true;
}

//---------------------------------------------------------------------------
// NOTIFYで受け取った情報

bool	Satori::var_installed(const wstring&, const wstring& iArg, int iParam, wstring& oResult)
{
	bool exists = false;
	switch ( iParam ) {
	case INSTALLED_GHOST:		exists = installed_ghost_name.count(iArg) != 0; break;
	case INSTALLED_SHELL:		exists = installed_shell_name.count(iArg) != 0; break;
	case INSTALLED_BALLOON:		exists = installed_balloon_name.count(iArg) != 0; break;
	case INSTALLED_HEADLINE:	exists = installed_headline_name.count(iArg) != 0; break;
	case INSTALLED_FONT:		exists = installed_font_name.count(iArg) != 0; break;
	case INSTALLED_PLUGIN:		exists = installed_plugin.count(iArg) != 0; break;
	}
	oResult = exists ? L"1" : L"0";
	return	true;
}

bool	Satori::var_plugin_id(const wstring&, const wstring& iArg, int, wstring& oResult)
{
	std::map<wstring, PluginInfo>::const_iterator it = installed_plugin.find(iArg);
	if ( it != installed_plugin.end() ) {
		oResult = it->second.plugin_id;
	}
	return	true;
}

bool	Satori::var_rate_of_use_graph(const wstring&, const wstring& iArg, int iParam, wstring& oResult)
{
	std::map<wstring, RateOfUseGraph>::const_iterator it = rate_of_use_graph.find(iArg);
	if ( it != rate_of_use_graph.end() ) {
		const RateOfUseGraph& g = it->second;
		switch ( iParam ) {
		case GRAPH_SAKURA_NAME:		oResult = g.sakura_name; break;
		case GRAPH_KERO_NAME:		oResult = g.kero_name; break;
		case GRAPH_BOOT_COUNT:		oResult = g.boot_count; break;
		case GRAPH_BOOT_MINUTES:	oResult = g.boot_minutes; break;
		case GRAPH_BOOT_PERCENT:	oResult = g.boot_percent; break;
		case GRAPH_STATUS:			oResult = g.status; break;
		}
	}
	return	true;
}

//---------------------------------------------------------------------------
// ウインドウ・プロセス

//ウインドウ列挙
#ifndef POSIX
typedef struct EnumWindowsInfo
{
	const wchar_t* txt;
	wchar_t title[1024];
	bool isPartial;
	HWND hWnd;
} EnumWindowsInfo;

static BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	EnumWindowsInfo &inf = *reinterpret_cast<EnumWindowsInfo*>(lParam);

	inf.title[0] = L'\0';	// タイトルの無いウインドウでは GetWindowText が書き込まないことがある
	::GetWindowText(hwnd,inf.title,sizeof(inf.title)/sizeof(inf.title[0])-1);
	inf.title[sizeof(inf.title)/sizeof(inf.title[0])-1] = L'\0';

	if ( inf.title[0] ) {
		if ( inf.isPartial ) {
			if ( strstri_hz(inf.title,inf.txt) ) {
				inf.hWnd = hwnd;
				return FALSE;
			}
		}
		else {
			if ( _wcsicmp(inf.title,inf.txt) == 0 ) {
				inf.hWnd = hwnd;
				return FALSE;
			}
		}
	}

	return TRUE;
}
#endif

//ウインドウ探索　ウインドウ存在判定で使う
unsigned long Satori::FindTopLevelWindow(const wchar_t* txt,bool isPartial)
{
#ifdef POSIX
	return 0;
#else

	if ( ! txt || ! *txt ) { return 0; }

	EnumWindowsInfo inf;
	inf.txt = txt;
	inf.isPartial = isPartial;
	inf.hWnd = NULL;

	::EnumWindows(EnumWindowsProc,(LPARAM)&inf);

	return (unsigned long)inf.hWnd;
#endif
}

//プロセス探索　ウインドウ存在判定で使う
unsigned long Satori::FindProcessName(const wchar_t* txt,bool isPartial)
{
#ifdef POSIX
	return 0;
#else

	if ( ! txt || ! *txt ) { return 0; }

	HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	PROCESSENTRY32 pinfo = {0};
	pinfo.dwSize = sizeof(pinfo);

	DWORD pid = 0;

	if ( ::Process32First(hSnap,&pinfo) ) {
		do {
			const wchar_t *pName = wcsrchr(pinfo.szExeFile,L'\\');
			if ( pName ) {
				pName += 1;
			}
			else {
				pName = pinfo.szExeFile;
			}

			if ( isPartial ) {
				if ( strstri_hz(pName,txt) ) {
					pid = pinfo.th32ProcessID;
					break;
				}
			}
			else {
				if ( _wcsicmp(pName,txt) == 0 ) {
					pid = pinfo.th32ProcessID;
					break;
				}
			}
		} while ( ::Process32Next(hSnap,&pinfo) );
	}

	::CloseHandle(hSnap);

	return pid;

#endif
}

// iParamが1なら部分一致
bool	Satori::var_window_exists(const wstring&, const wstring& iArg, int iParam, wstring& oResult)
{
	oResult = ul2zen(FindTopLevelWindow(iArg.c_str(), iParam!=0));
	return	true;
}

bool	Satori::var_process_exists(const wstring&, const wstring& iArg, int iParam, wstring& oResult)
{
	oResult = ul2zen(FindProcessName(iArg.c_str(), iParam!=0));
	return	true;
}


//===========================================================================
//	システム変数
//===========================================================================

// return = SYSVAR_NONE(処理なし) / SYSVAR_SET(処理した) / SYSVAR_NOSET(処理したけど変数設定してはだめ)
int	Satori::system_variable_operation(wstring key, wstring value, wstring* result)
{
	int r = system_variable_operation_real(key,value,result);
	if ( r == SYSVAR_NOSET ) {
		variables.erase(key); //念の為
	}
	return r;
}

int	Satori::system_variable_operation_real(wstring key, wstring value, wstring* result)
{
	// 名前が完全に一致するもの
	static const SystemVariable names[] = {
		// 名前					処理（無ければ右のフラグか文字列に入れる）
		{ L"喋り間隔",			&Satori::sysvar_talk_interval,	0,	0 },
		{ L"喋り間隔誤差",		&Satori::sysvar_talk_interval_random,	0,	0 },
		{ L"見切れてても喋る",	0,	&Satori::is_call_ontalk_at_mikire,	0 },
		{ L"今回は喋らない",	0,	&Satori::return_empty,	0 },
		{ L"スクリプトの一番頭",	&Satori::sysvar_header_script,	0,	0 },
		{ L"呼び出し回数制限",	&Satori::sysvar_nest_limit,	0,	0 },
		{ L"括弧展開サイズ制限",	&Satori::sysvar_kakko_size_limit,	0,	0 },
		{ L"ジャンプ回数制限",	&Satori::sysvar_jump_limit,	0,	0 },
		{ L"呼び出し総数制限",	&Satori::sysvar_total_call_limit,	0,	0 },

		{ L"スコープ切り換え時",	0,	0,	&Satori::append_at_scope_change },
		{ L"さくらスクリプトによるスコープ切り換え時",	0,	0,	&Satori::append_at_scope_change_with_sakura_script },
		{ L"トーク開始時",		0,	0,	&Satori::append_at_talk_start },
		{ L"トーク終了時",		0,	0,	&Satori::append_at_talk_end },
		{ L"選択肢開始時",		0,	0,	&Satori::append_at_choice_start },
		{ L"選択肢終了時",		0,	0,	&Satori::append_at_choice_end },

		{ L"会話時サーフェス戻し",			&Satori::sysvar_surface_restore,	0,	0 },
		{ L"会話時サーフィス戻し",			&Satori::sysvar_surface_restore,	0,	0 },
		{ L"今回は会話時サーフェス戻し",	&Satori::sysvar_surface_restore_onetime,	0,	0 },
		{ L"今回は会話時サーフィス戻し",	&Satori::sysvar_surface_restore_onetime,	0,	0 },
		{ L"自動アンカー",		&Satori::sysvar_auto_anchor,	0,	0 },
		{ L"今回は自動アンカー",	0,	&Satori::auto_anchor_enable_onetime,	0 },
		{ L"自動改行挿入",		&Satori::sysvar_auto_newline,	0,	0 },
		{ L"今回は自動改行挿入",	0,	&Satori::auto_newline_enable_onetime,	0 },

		{ L"トーク中のなでられ反応",	0,	&Satori::insert_nade_talk_at_other_talk,	0 },
		{ L"なでられ時実行イベント",	&Satori::sysvar_stroked_event,	0,	0 },
		{ L"なでられ持続秒数",	&Satori::sysvar_nade_valid_time,	0,	0 },
		{ L"なでられ反応回数",	&Satori::sysvar_nade_sensitivity,	0,	0 },

		{ L"デバッグ",			0,	&Satori::fDebugMode,	0 },
		{ L"Log",				&Satori::sysvar_log,	0,	0 },
		{ L"RequestLog",		0,	&Satori::fRequestLog,	0 },
		{ L"OperationLog",		0,	&Satori::fOperationLog,	0 },
		{ L"ResponseLog",		0,	&Satori::fResponseLog,	0 },

		{ L"自動挿入ウェイトの倍率",	&Satori::sysvar_auto_insert_wait_rate,	0,	0 },
		{ L"自動挿入ウエイトの倍率",	&Satori::sysvar_auto_insert_wait_rate,	0,	0 },
		{ L"自動挿入ウェイトタイプ",	&Satori::sysvar_auto_insert_wait_type,	0,	0 },
		{ L"自動挿入ウエイトタイプ",	&Satori::sysvar_auto_insert_wait_type,	0,	0 },
		{ L"コミュニケートの検索方法",	&Satori::sysvar_communicate_search,	0,	0 },

		{ L"辞書フォルダ",		&Satori::sysvar_dic_folder,	0,	0 },
		{ L"セーブデータ暗号化",	0,	&Satori::fEncodeSavedata,	0 },
		{ L"タイマ変数はセーブしない",	0,	&Satori::fDontSaveTimerValue,	0 },
		{ L"セーブデータ読み込み",	&Satori::sysvar_savedata_status,	0,	0 },
		{ L"次のトーク",		&Satori::sysvar_next_talk,	0,	0 },
		{ L"トーク予約のキャンセル",	&Satori::sysvar_cancel_reserved_talk,	0,	0 },
		{ L"SAORI引数の計算",	&Satori::sysvar_saori_argument_calc,	0,	0 },
		{ L"辞書リロード",		&Satori::sysvar_reload,	0,	0 },
		{ L"れしば送信",		&Satori::sysvar_sender,	0,	0 },
		{ L"手動セーブ",		&Satori::sysvar_save,	0,	0 },
		{ L"自動セーブ間隔",	&Satori::sysvar_auto_save_interval,	0,	0 },
		{ L"教わること",		&Satori::sysvar_teach,	0,	0 },
		{ L"全タイマ解除",		&Satori::sysvar_clear_timers,	0,	0 },
		{ L"引数区切り追加",	&Satori::sysvar_add_delimiter,	0,	0 },
		{ L"引数区切り削除",	&Satori::sysvar_remove_delimiter,	0,	0 },
		{ L"NOTIFYの自動保存",	&Satori::sysvar_save_notify,	0,	0 },
		{ L"れしばログ一時保存件数",	&Satori::sysvar_delay_save_count,	0,	0 },
		{ L"外部から実行可能なイベントの接頭辞",	&Satori::sysvar_external_event_prefixes,	0,	0 },
	};

	// 名前の先頭・末尾で決まるもの。上から順に調べる。
	static const SystemVariablePattern patterns[] = {
		// 先頭				末尾				処理
		{ L"サーフェス加算値",	L"",			&Satori::sysvar_surface_add_value },
		{ L"デフォルトサーフェス",	L"",		&Satori::sysvar_default_surface },
		{ L"BalloonOffset",	L"",				&Satori::sysvar_balloon_offset },
		{ L"単語群「",		L"」の重複回避",	&Satori::sysvar_word_overlap },
		{ L"文「",			L"」の重複回避",	&Satori::sysvar_talk_overlap },
		{ L"次から",		L"回目のトーク",	&Satori::sysvar_reserve_talk },
		{ L"",				L"タイマ",			&Satori::sysvar_timer },
		{ L"Value",			L"",				&Satori::sysvar_response_value },
		{ L"返信ヘッダ「",	L"」",				&Satori::sysvar_response_header },
	};

	static std::map<wstring, const SystemVariable*>	name_map;
	if ( name_map.empty() ) {
		for ( int i=0 ; i<sizeof(names)/sizeof(names[0]) ; ++i ) {
			name_map[names[i].name] = &names[i];
		}
	}

	// ここにある名前は、下のどのパターンにも当てはまらないので先に調べてよい。
	std::map<wstring, const SystemVariable*>::const_iterator found = name_map.find(key);
	if ( found != name_map.end() ) {
		const SystemVariable& v = *(found->second);
		if ( v.handler ) {
			int r = (this->*(v.handler))(key, key, value, result);
			if ( r != SYSVAR_NONE ) {
				return	r;
			}
		}
		else if ( v.flag ) {
			this->*(v.flag) = (value==L"有効");
			return	SYSVAR_SET;
		}
		else {
			this->*(v.text) = zen2han(value);
			return	SYSVAR_SET;
		}
	}

	for ( int i=0 ; i<sizeof(patterns)/sizeof(patterns[0]) ; ++i ) {
		const SystemVariablePattern& pt = patterns[i];
		if ( !match_head_tail(key, pt.head, pt.tail) ) {
			continue;
		}
		int r = (this->*(pt.handler))(key, strip_head_tail(key, pt.head, pt.tail), value, result);
		if ( r != SYSVAR_NONE ) {
			return	r;
		}
	}

	return	SYSVAR_NONE;
}

//---------------------------------------------------------------------------
// トーク・スクリプト

// 喋りカウント初期化
static int	talk_interval_count_from(int talk_interval, int talk_interval_random)
{
	int	dist = static_cast<int>(talk_interval*(talk_interval_random/100.0));
	return ( dist==0 ) ? talk_interval : (talk_interval-dist)+(random(dist*2));
}

int	Satori::sysvar_talk_interval(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	talk_interval = zen2int(iValue);
	if ( talk_interval<3 ) talk_interval=0; // 3未満は喋らない
	talk_interval_count = talk_interval_count_from(talk_interval, talk_interval_random);
	return	SYSVAR_SET;
}

int	Satori::sysvar_talk_interval_random(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	talk_interval_random = zen2int(iValue);
	if ( talk_interval_random>100 ) talk_interval_random=100;
	if ( talk_interval_random<0 ) talk_interval_random=0;
	talk_interval_count = talk_interval_count_from(talk_interval, talk_interval_random);
	return	SYSVAR_SET;
}

int	Satori::sysvar_header_script(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	header_script = iValue;
	return	SYSVAR_SET;
}

int	Satori::sysvar_nest_limit(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	m_nest_limit = max(0, zen2int(iValue));
	return	SYSVAR_SET;
}

int	Satori::sysvar_kakko_size_limit(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	m_kakko_size_limit = max(0, zen2int(iValue));
	return	SYSVAR_SET;
}

int	Satori::sysvar_jump_limit(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	m_jump_limit = max(0, zen2int(iValue));
	return	SYSVAR_SET;
}

int	Satori::sysvar_total_call_limit(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	m_total_call_limit = max(0, zen2int(iValue));
	return	SYSVAR_SET;
}

static SurfaceRestoreMode	surface_restore_mode_from(const wstring& iValue)
{
	if ( iValue == L"有効" ) {
		return	SR_NORMAL;
	}
	else if ( iValue == L"強制" ) {
		return	SR_FORCE;
	}
	return	SR_NONE;
}

int	Satori::sysvar_surface_restore(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	surface_restore_at_talk = surface_restore_mode_from(iValue);
	return	SYSVAR_SET;
}

int	Satori::sysvar_surface_restore_onetime(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	surface_restore_at_talk_onetime = surface_restore_mode_from(iValue);
	return	SYSVAR_SET;
}

int	Satori::sysvar_auto_anchor(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	auto_anchor_enable = (iValue == L"有効");
	auto_anchor_enable_onetime = auto_anchor_enable;
	return	SYSVAR_SET;
}

int	Satori::sysvar_auto_newline(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	auto_newline_enable = (iValue == L"有効");
	auto_newline_enable_onetime = auto_newline_enable;
	return	SYSVAR_SET;
}

int	Satori::sysvar_auto_insert_wait_rate(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	rate_of_auto_insert_wait = min(1000, max(0, zen2int(iValue)));
	variables[L"自動挿入ウェイトの倍率"] = int2zen(rate_of_auto_insert_wait);
	return	SYSVAR_SET;
}

int	Satori::sysvar_auto_insert_wait_type(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue == L"一般" ) {
		type_of_auto_insert_wait = 2;
		variables[L"自動挿入ウェイトタイプ"] = L"一般";
	}
	else if ( iValue == L"無効" ) {
		type_of_auto_insert_wait = 0;
		variables[L"自動挿入ウェイトタイプ"] = L"無効";
	}
	else /* if ( iValue == "里々" ) */ {
		type_of_auto_insert_wait = 1;
		variables[L"自動挿入ウェイトタイプ"] = L"里々";
	}
	return	SYSVAR_SET;
}

//---------------------------------------------------------------------------
// なでられ

int	Satori::sysvar_stroked_event(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if (iValue == L"なでられ時の反応") {
		bool_of_action_when_ghost_is_stroked = true;
		variables[L"なでられ時実行イベント"] = L"なでられ時の反応";
	}
	else /* if ( iValue == "デフォルト" ) */ {
		bool_of_action_when_ghost_is_stroked = false;
		variables[L"なでられ時実行イベント"] = L"デフォルト";
	}
	return	SYSVAR_SET;
}

int	Satori::sysvar_nade_valid_time(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	nade_valid_time_initializer = zen2int(iValue);
	return	SYSVAR_SET;
}

int	Satori::sysvar_nade_sensitivity(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	nade_sensitivity = zen2int(iValue);
	return	SYSVAR_SET;
}

//---------------------------------------------------------------------------
// ログ・デバッグ

int	Satori::sysvar_log(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	GetSender().validate(iValue==L"有効");
	return	SYSVAR_SET;
}

int	Satori::sysvar_sender(const wstring& iKey, const wstring&, const wstring& iValue, wstring*)
{
	variables.erase(iKey);
	GetSender().reinit(iValue==L"有効");
	return	SYSVAR_SET;
}

int	Satori::sysvar_delay_save_count(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue.empty() ) {
		return	SYSVAR_NONE;
	}
	GetSender().set_delay_save_count(zen2int(iValue));
	return	SYSVAR_SET;
}

//---------------------------------------------------------------------------
// 辞書・セーブデータ

int	Satori::sysvar_communicate_search(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue == L"合計文字数" ) {
		type_of_communicate_search = COMSEARCH_LENGTH;
		variables[L"コミュニケートの検索方法"] = L"合計文字数";
	}
	else /* if ( iValue == "里々" ) */ {
		type_of_communicate_search = COMSEARCH_DEFAULT;
		variables[L"コミュニケートの検索方法"] = L"里々";
	}
	return	SYSVAR_SET;
}

int	Satori::sysvar_dic_folder(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	split(iValue, L",",dic_folder);
	reload_flag=true;
	return	SYSVAR_SET;
}

int	Satori::sysvar_reload(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue != L"実行" ) {
		return	SYSVAR_NONE;
	}
	reload_flag=true;
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_savedata_status(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	load_savedata_status = iValue;
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_save(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue != L"実行" ) {
		return	SYSVAR_NONE;
	}
	if ( is_dic_loaded ) {
		this->Save();
	}
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_auto_save_interval(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	mAutoSaveInterval = zen2int(iValue);
	mAutoSaveCurrentCount = mAutoSaveInterval;
	if ( mAutoSaveInterval > 0 )
		GetSender().sender() << L""  << itos(mAutoSaveInterval) << L"秒間隔で自動セーブを行います。" << std::endl;
	else
		GetSender().sender() << L"自動セーブは行いません。" << std::endl;
	return	SYSVAR_SET;
}

int	Satori::sysvar_save_notify(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue.empty() ) {
		return	SYSVAR_NONE;
	}
	is_save_notify = (iValue == L"有効");
	return	SYSVAR_SET;
}

int	Satori::sysvar_word_overlap(const wstring&, const wstring& iArg, const wstring& iValue, wstring*)
{
	words.setOC( iArg, iValue );
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_talk_overlap(const wstring&, const wstring& iArg, const wstring& iValue, wstring*)
{
	talks.setOC( iArg, iValue );
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_teach(const wstring&, const wstring&, const wstring& iValue, wstring* oResult)
{
	teach_genre=iValue;
	if ( oResult != NULL )
		*oResult += L"\\![open,teachbox]";
	return	SYSVAR_NOSET;
}

//---------------------------------------------------------------------------
// トーク予約・タイマ

int	Satori::sysvar_next_talk(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	int	count=1;
	while ( reserved_talk.find(count) != reserved_talk.end() )
		++count;
	reserved_talk[count] = iValue;
	GetSender().sender() << L"次回のランダムトークが「" << iValue << L"」に予約されました。" << std::endl;
	return	SYSVAR_NOSET;
}

// 次からn回目のトーク
int	Satori::sysvar_reserve_talk(const wstring&, const wstring& iArg, const wstring& iValue, wstring*)
{
	int	count = zen2int(iArg);
	if ( count<=0 ) {
		GetSender().sender() << L"トーク予約、設定値がヘンです。" << std::endl;
	}
	else {
		while ( reserved_talk.find(count) != reserved_talk.end() )
			++count;
		reserved_talk[count] = iValue;
		GetSender().sender() << count << L"回後のランダムトークが「" << iValue << L"」に予約されました。" << std::endl;
	}
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_cancel_reserved_talk(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue==L"＊" ) {
		reserved_talk.clear();
	}
	else {
		for (std::map<int, wstring>::iterator it=reserved_talk.begin(); it!=reserved_talk.end() ; ) {
			if ( iValue == it->second ) {
				reserved_talk.erase(it++);
			}
			else {
				++it;
			}
		}
	}
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_clear_timers(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue != L"実行" ) {
		return	SYSVAR_NONE;
	}
	for (strintmap::const_iterator i=timer_sec.begin();i!=timer_sec.end();++i) {
		variables.erase(i->first + L"タイマ");
	}
	timer_sec.clear();
	return	SYSVAR_NOSET;
}

// 名前タイマ
int	Satori::sysvar_timer(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring*)
{
	if ( iArg.empty() ) {
		return	SYSVAR_NONE;
	}
	const wstring&	timer_name = iArg;

	int sec = zen2int(iValue);
	if ( sec < 1 ) {
		if ( timer_sec.find(timer_name)!=timer_sec.end() ) {
			timer_sec.erase(timer_name);
			variables.erase(iKey);
			GetSender().sender() << L"タイマ「"  << timer_name << L"」の予約がキャンセルされました。" << std::endl;
		}
		else {
			GetSender().sender() << L"タイマ「"  << timer_name << L"」は元から予約されていません。" << std::endl;
		}
		return	SYSVAR_NOSET;
	}
	else {
		timer_sec[timer_name] = sec;
		GetSender().sender() << L"タイマ「"  << timer_name << L"」が" << sec << L"秒後に予約されました。" << std::endl;
		return	SYSVAR_SET;
	}
}

//---------------------------------------------------------------------------
// サーフェス・バルーン

int	Satori::sysvar_surface_add_value(const wstring&, const wstring& iArg, const wstring& iValue, wstring*)
{
	if ( !aredigits(iArg) ) {
		return	SYSVAR_NONE;
	}
	int n = zen2int(iArg);
	surface_add_value[n]= zen2int(iValue);

	variables[wstring()+L"デフォルトサーフェス"+itos(n)] = iValue;
	next_default_surface[n] = zen2int(iValue);
	if ( !is_speaked_anybody() )
		default_surface[n]=next_default_surface[n];
	return	SYSVAR_SET;
}

int	Satori::sysvar_default_surface(const wstring&, const wstring& iArg, const wstring& iValue, wstring*)
{
	if ( !aredigits(iArg) ) {
		return	SYSVAR_NONE;
	}
	int n = zen2int(iArg);
	next_default_surface[n]= zen2int(iValue);
	if ( !is_speaked_anybody() )
		default_surface[n]=next_default_surface[n];
	return	SYSVAR_SET;
}

int	Satori::sysvar_balloon_offset(const wstring&, const wstring& iArg, const wstring& iValue, wstring*)
{
	if ( !aredigits(iArg) ) {
		return	SYSVAR_NONE;
	}
	int n = stoi_internal(iArg);
	BalloonOffset[n] = iValue;
	validBalloonOffset[n] = true;
	return	SYSVAR_SET;
}

//---------------------------------------------------------------------------
// 関数呼び出し・応答・外部

int	Satori::sysvar_saori_argument_calc(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if (iValue==L"有効") {
		mSaoriArgumentCalcMode = SACM_ON;
	}
	else if (iValue==L"無効") {
		mSaoriArgumentCalcMode = SACM_OFF;
	}
	else {
		mSaoriArgumentCalcMode = SACM_AUTO;
	}
	return	SYSVAR_SET;
}

int	Satori::sysvar_add_delimiter(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue.empty() ) {
		return	SYSVAR_NONE;
	}
	mDelimiters.insert(iValue);
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_remove_delimiter(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	if ( iValue.empty() ) {
		return	SYSVAR_NONE;
	}
	mDelimiters.erase(iValue);
	return	SYSVAR_NOSET;
}

// Valuen
int	Satori::sysvar_response_value(const wstring&, const wstring& iArg, const wstring& iValue, wstring*)
{
	if ( !aredigits(iArg) ) {
		return	SYSVAR_NONE;
	}
	if(iValue!=L""){
		mResponseMap[wstring()+L"Reference"+iArg] = iValue;
	}else{
		mResponseMap.erase(wstring()+L"Reference"+iArg);
	}
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_response_header(const wstring&, const wstring& iArg, const wstring& iValue, wstring*)
{
	if(iValue!=L""){
		mResponseMap[iArg] = iValue;
	}else{
		mResponseMap.erase(iArg);
	}
	return	SYSVAR_NOSET;
}

int	Satori::sysvar_external_event_prefixes(const wstring&, const wstring&, const wstring& iValue, wstring*)
{
	split(iValue, L"、,", allow_external_event_prefixes);
	return	SYSVAR_SET;
}
