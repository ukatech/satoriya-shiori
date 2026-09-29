//------------------------------------------------
//
//	里々同梱ユーティリティライブラリ　ssu.dll
//
#include    "random.h"
#include    "ssu.h"
#include	<map>
#include	<algorithm>
#include	<time.h>


//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

//================================================================================================

#include	"SaoriHost.h"

static SRV	call_ssu(wstring iCommand, std::deque<wstring>& iArguments, std::deque<wstring>& oValues);

#ifndef SSU_SAORI_CALL_INTERFACE

//SaoriClientに直結するインターフェース

//要らないインターフェースは潰しておく
bool ssu::load(
	const wstring& i_sender,
	const wstring& i_charset,
	const wstring& i_work_folder,
	const wstring& i_dll_fullpath)
{
	return true;
}
void ssu::unload()
{
	//NOOP
}
wstring ssu::request(const wstring& i_request_string)
{
	return L"";
}
wstring ssu::get_version(const wstring& i_security_level)
{
	return L"SAORI/1.0";
}

int ssu::request(
		const std::vector<wstring>& i_argument,
		bool i_is_secure,
		wstring& o_result,
	std::vector<wstring>& o_value)
{
	if ( i_argument.size()<1 ) {
		o_result = L"命令が指定されていません";
		return 400;
	}

	// 最初の引数は命令名として扱う
	std::vector<wstring>::const_iterator i_arg = i_argument.begin();
	wstring theCommand = *i_arg;
	++i_arg;

	std::deque<wstring> iArguments;
	for ( ; i_arg != i_argument.end() ; ++i_arg ) {
		iArguments.push_back(*i_arg);
	}

	std::deque<wstring> oValues;

	SRV result = call_ssu(theCommand, iArguments, oValues);
	o_result = result.mResultString;

	// 注意！Valueヘッダ相当が存在しないときは、S?系システム変数を温存するためにデータを上書きしないこと！
	if ( ! oValues.empty() ) {
		o_value.clear();
		for (std::deque<wstring>::const_iterator o_val = oValues.begin() ; o_val != oValues.end() ; ++o_val ) {
			o_value.push_back(*o_val);
		}
	}

	return result.mReturnCode;
}

#else

//通常のSSU(SAORI版)
//ビルド時 SSU_SAORI_CALL_INTERFACE を定義すると有効になる。SSU単独ビルド時に定義すること。

class ssu : public SaoriHost {
public:
	ssu() {
		randomize();
	}
	virtual bool	load(const wstring& i_base_folder) {
		setlocale(LC_ALL, "Japanese");
		return true;
	}
	virtual SRV		request(std::deque<wstring>& iArguments, std::deque<wstring>& oValues);
};
SakuraDLLHost* SakuraDLLHost::m_dll = new ssu;

SRV	ssu::request(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<1 )
		return	SRV(400, L"命令が指定されていません");

	// 最初の引数は命令名として扱う
	wstring	theCommand = iArguments.front();
	iArguments.pop_front();
	return	call_ssu(theCommand, iArguments, oValues);
}

#endif

//================================================================================================

typedef SRV (*Command)(std::deque<wstring>&, std::deque<wstring>&);

static const std::map<wstring, Command> &func_map(void)
{
	// 名前と命令を関連付けたmap
	static std::map<wstring, Command>	theMap;
	if ( theMap.empty() )
	{ 
		// 初回準備
		#define	d(iName)	\
			SRV	_##iName(std::deque<wstring>&, std::deque<wstring>&); \
			theMap[ ascii_to_w(#iName) ] = _##iName
		// 命令一覧の宣言と関連付け。
		d(calc);			d(calc_float);		d(if);				d(unless);
		d(nswitch);			d(switch);			d(iflist);			d(substr);
		d(split);			d(split_string);	d(join);
		d(replace);			d(replace_first);	d(erase);
		d(erase_first);		d(count);
		d(compare);				d(compare_head);			d(compare_tail);
		d(compare_case);		d(compare_head_case);		d(compare_tail_case);
		d(length);			d(is_empty);		d(is_digit);
		d(is_alpha);		d(zen2han);			d(han2zen);			d(hira2kata);
		d(kata2hira);		d(sprintf);			d(reverse);			d(at);
		d(choice);
		d(lsimg);			d(mkdir);
		#undef	d
	}
	return theMap;
}

void get_ssu_funclist(std::vector<wstring> &funclist)
{
	const std::map<wstring, Command> &theMap = func_map();

	funclist.clear();

	for (std::map<wstring, Command>::const_iterator i = theMap.begin() ; i != theMap.end() ; ++i ) {
		funclist.push_back(i->first);
	}
}

static SRV	call_ssu(wstring iCommand, std::deque<wstring>& iArguments, std::deque<wstring>& oValues)
{
	const std::map<wstring, Command> &theMap = func_map();

	// 命令の存在を確認
	std::map<wstring, Command>::const_iterator i = theMap.find(iCommand);
	if ( i==theMap.end() )
		return SRV(400, wstring()+L"Error: '"+iCommand+L"'という名前の命令は定義されていません。");

	// 実際に呼ぶ
	return	i->second(iArguments, oValues);
}

// ここから実装

#ifdef POSIX
#  include      "../_/Utilities.h"
#  include <sys/stat.h>
#  include <sys/types.h>
#else
#  include	<windows.h>
#endif
#include	"../_/stltool.h"

// 変換テーブル。前後のテーブルは同じ位置の文字が対応する。
static const wchar_t	kata[] = L"アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヰヱヲンァィゥェォャュョヮッガギグゲゴザジズゼゾダヂヅデドバビブベボパピプペポ";
static const wchar_t	hira[] = L"あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもやゆよらりるれろわゐゑをんぁぃぅぇぉゃゅょゎっがぎぐげござじずぜぞだぢづでどばびぶべぼぱぴぷぺぽ";

//半角全角変換テーブル
static const wchar_t	zen_alpha[] = L"ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ";
static const wchar_t	han_alpha[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

static const wchar_t	zen_digit[] = L"０１２３４５６７８９";
static const wchar_t	han_digit[] = L"0123456789";

static const wchar_t   zen_symbol[] = L"　！”＃＄％＆’（）＝～｜‘｛＋＊｝＜＞？＿ー＾￥＠「；：」、。・÷×－，．［］";
static const wchar_t   han_symbol[] = L" !\"#$%&'()=~|`{+*}<>?_-^\\@[;:],.･/*-,.[]";

static const wchar_t   zen_kana_1[] = L"アイウエオカキクケコサシスセソタチツテトナニヌネノハヒフヘホマミムメモヤユヨラリルレロワヲンァィゥェォャュョッ、。ー";
static const wchar_t   han_kana_1[] = L"ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜｦﾝｧｨｩｪｫｬｭｮｯ､｡ｰ";

// 全角1文字が半角2文字（濁点・半濁点付き）に対応する
static const wchar_t   zen_kana_2[] = L"ガギグゲゴザジズゼゾダヂヅデドバビブベボパピプペポ";
static const wchar_t   han_kana_2[] = L"ｶﾞｷﾞｸﾞｹﾞｺﾞｻﾞｼﾞｽﾞｾﾞｿﾞﾀﾞﾁﾞﾂﾞﾃﾞﾄﾞﾊﾞﾋﾞﾌﾞﾍﾞﾎﾞﾊﾟﾋﾟﾌﾟﾍﾟﾎﾟ";

// fromの各文字（from_len文字ずつ）をtoの対応する文字（to_len文字ずつ）に置き換える。countは組の数。
static void	replace_by_table(wstring& str, const wchar_t* from, size_t from_len, const wchar_t* to, size_t to_len, size_t count)
{
	for ( size_t n=0 ; n<count ; ++n ) {
		replace(str, wstring(from + n*from_len, from_len), wstring(to + n*to_len, to_len));
	}
}

extern	bool calc(wstring& ioString,bool isStrict = false);
extern	bool calc_float(wstring& ioString);

static wstring zen2han_internal(wstring &str,unsigned int flag = 0xffffU);

#include	<sstream>

// n文字移動（サロゲートペアは1文字）、超過時はNULL
static const wchar_t*	char_at(const wchar_t* p, int n) {
	for (int i=0 ; i<n ; ++i) {
		if ( *p == L'\0' )
			return	NULL;
		get_a_chr(p);
	}
	return	p;
}

bool	printf_format(const wchar_t*& p, std::deque<wstring>& iArguments, std::wstringstream& os)
{
	assert(*p==L'%');
	if ( iArguments.empty() )
		return	false;	// 置き換え対象が無い

	++p;
	wstring	str = iArguments.front();
	iArguments.pop_front();

	// フラグ指定読み込み
	bool isSharp=false;

	while (true) {
		if ( *p == L'-' ) { os << std::left; ++p; }
		else if ( *p == L'+' ) { os << std::showpos; ++p; }
		else if ( *p == L'0' ) { os.fill(L'0'); os<< std::internal; ++p; }
		else if ( *p == L' ' ) { os.fill(L' '); os<< std::internal; ++p; }
		else if ( *p == L'#' ) { isSharp = true; ++p; }
		else break;
	}

	// 幅指定読み込み
	int	width=0;
	bool	isReadWidth = false;
	if ( *p==L'*' ) {
		isReadWidth = true;
		++p;
	} else {
		while ( *p>=L'0' && *p<=L'9' ) {
			width = width*10 + (*p - L'0');
			++p;
			os.width(width);
		}
	}

	// 精度指定読み込み
	int	precision = 0;
	bool	hasPrecision = false;
	if ( *p == L'.' ) {
		++p;
		while ( *p>=L'0' && *p<=L'9' ) {
			precision = precision*10 + (*p - L'0');
			++p;
		}
		os.precision(precision);
		hasPrecision = true;
	}

	// フォーマット設定 - #フラグ
	if ( isSharp ) {
		switch (*p) {
			case L'o':
			case L'x':
			case L'X':
				os << std::showbase;
				break;
			case L'e':
			case L'E':
			case L'f':
			case L'g':
			case L'G':
				os << std::showpoint;
				break;
		}
	}
	

	// 変換文字に応じて挿入
	switch (*p) {
	case L's':
	case L'S':
		{
			// 幅は半角換算（全角文字は2）で数える。streamはwchar_t単位で埋めるので差分だけ幅を減らす。
			if ( hasPrecision ) {
				const wchar_t* end_p = char_at(str.c_str(), precision);
				if ( end_p != NULL ) { str.erase(end_p - str.c_str()); }
			}
			if ( width > 0 ) {
				const int	extra = (int)count_width(str) - (int)str.size();
				os.width( width > extra ? width - extra : 0 );
			}
			os << str;
			break;
		}
	case L'c':
	case L'C':
		{
			const unsigned long code = (unsigned long)zen2int(str);
			if ( sizeof(wchar_t) == 2 && code >= 0x10000 && code <= 0x10FFFF ) {
				// 補助面の文字はサロゲートペアにする
				wchar_t pair[3] = { (wchar_t)(0xD800 + ((code - 0x10000) >> 10)), (wchar_t)(0xDC00 + ((code - 0x10000) & 0x3FF)), 0 };
				os << pair;
			}
			else {
				os << (wchar_t)code;
			}
			break;
		}
	case L'i':
	case L'd':
		{
			os << zen2int(str);
			break;
		}
	case L'o':
		{
			os << std::oct << wcstoul(zen2han_internal(str).c_str(),NULL,10);
			break;
		}
	case L'u':
		{
			os << wcstoul(zen2han_internal(str).c_str(),NULL,10);
			break;
		}
	case L'x':
		{
			os << std::hex << std::nouppercase << wcstoul(zen2han_internal(str).c_str(),NULL,10);
			break;
		}
	case L'X':
		{
			os << std::hex << std::uppercase << wcstoul(zen2han_internal(str).c_str(),NULL,10);
			break;
		}
	case L'e':
		{
			os << std::scientific << std::nouppercase << wcstod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case L'E':
		{
			os << std::scientific << std::uppercase << wcstod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case L'g':
		{
			os << std::scientific << std::fixed << std::nouppercase << wcstod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case L'G':
		{
			os << std::scientific << std::fixed << std::uppercase << wcstod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case L'f':
		{
			os << std::fixed << wcstod(zen2han_internal(str).c_str(),NULL);
			break;
		}
	case L'n': break;
	case L'p': break;
	default: return false;
	}
	++p;
	return	true;
}

wstring	sprintf(std::deque<wstring>& iArguments) {
	std::wstringstream s;
	wstring	str = iArguments.front();
	iArguments.pop_front();
	const wchar_t* p = str.c_str();
	while ( *p!=L'\0' ) {
		if ( *p==L'%' ) {
			std::wstringstream sf;
			if ( printf_format(p, iArguments, sf) ) {
				s << sf.str();
				continue;
			}
		}
		s << *p++;
	}
	return	s.str();
}


SRV _calc(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=1 )
		return	SRV(400, L"引数の個数が正しくありません。");
	wstring	exp = iArguments[0];
	if ( !calc(exp) )
		return	SRV(400, wstring()+L"'"+iArguments[0]+L"' 式が計算不能です。"); // 「能」の2バイト目は「\」
	return	exp;
}

SRV _calc_float(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=1 )
		return	SRV(400, L"引数の個数が正しくありません。");
	wstring	exp = iArguments[0];
	if ( !calc_float(exp) )
	    return	SRV(400, wstring()+L"'"+iArguments[0]+L"' 式が計算不能です。");
	return	exp;
}

SRV _if(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 || iArguments.size()>3 )
		return	SRV(400, L"引数の個数が正しくありません。");
	wstring	exp = iArguments[0];
	if ( !calc(exp) )
		return	SRV(400, wstring()+L"'"+iArguments[0]+L"' 式が計算不能です。");
	if ( zen2int(exp) != 0 )
		return	iArguments[1];	// 真
	else
		if ( iArguments.size()==3 )
			return	iArguments[2];	// 偽
		else
			return	SRV(204);	// 偽でelseなし
}

SRV _unless(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 || iArguments.size()>3 )
		return	SRV(400, L"引数の個数が正しくありません。");
	wstring	exp = iArguments[0];
	if ( !calc(exp) )
		return	SRV(400, wstring()+L"'"+iArguments[0]+L"' 式が計算不能です。");
	if ( zen2int(exp) == 0 )
		return	iArguments[1];	// 偽
	else
		if ( iArguments.size()==3 )
			return	iArguments[2];	// 真
		else
			return	SRV(204);	// 真でelseなし
}

SRV _nswitch(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数が足りません。");

	int	n = zen2int(iArguments[0]);
	//iArguments.pop_front();
	//if ( iArguments.size()>n )
	if ( n>0 && iArguments.size()>n )
		return	SRV(200, iArguments[n]);
	else
		return	SRV(204);
}

SRV _switch(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数が足りません。");

	const wstring	lhs = iArguments[0];
	const int max = iArguments.size();
	for (int i=1 ; i<max ; i+=2) {
		if ( i==max-1 ) // 引数が奇数個の場合、最後の１つはelse式
			return	SRV(200, iArguments[i]);
		wstring	exp = wstring(L"(") + lhs + L")==(" + iArguments[i] + L")";
		if ( !calc(exp) )
			return	SRV(400, wstring()+L"switchの"+itos((i-1)/2+1)+L"個目、式 '"+exp+L"' は計算不能でした。");
		if ( zen2int(exp) != 0 )
			return	SRV(200, iArguments[i+1]);
	}
	return	SRV(204);
}

SRV _iflist(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数が足りません。");

	const wstring	lhs = iArguments[0];
	const int max = iArguments.size();
	for (int i=1 ; i<max ; i+=2) {
		if ( i==max-1 ) // 引数が奇数個の場合、最後の１つはelse扱い。ここまできたら無条件でそれを返す。
			return	SRV(200, iArguments[i]);
		wstring	exp = lhs + iArguments[i];
		if ( !calc(exp) )
			return	SRV(400, wstring()+L"iflistの"+itos((i-1)/2+1)+L"個目、式 '"+exp+L"' は計算不能でした。");
		if ( zen2int(exp) != 0 )
			return	SRV(200, iArguments[i+1]);
	}
	return	SRV(204);
}


SRV _substr(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {

	if ( iArguments.size()<1 )
		return	SRV(400, L"引数が足りません。");

	// 対象文字列
	const wchar_t* p = iArguments[0].c_str();
	if ( iArguments.size()==1 )
		return	SRV(200, p); // 引数１個なら全体を返す

	const int	len = count_chars(p);

	// 始点
	int	start = zen2int(iArguments[1]);
	if ( start < 0 )
		start = len + start;

	// 始点からのオフセット値
	int offset = (iArguments.size()<=2) ? len : zen2int(iArguments[2]);
	if ( offset==0 || offset==INT_MIN ) // INT_MINの時は符号反転が効かないので0扱い。
		return	SRV(204);
	if ( offset<0 ) {
		start += offset;
		offset = -offset;
	}
	assert(offset >= 0 );

	if ( start < 0 )
		start = 0;
	if ( start >= len )
		return	SRV(204);
	if ( start + offset >= len )
		offset = len - start;

	const wchar_t* const start_p = char_at(p, start);
	const wchar_t* const end_p = char_at(start_p, offset);
	return	SRV(200, wstring(start_p, end_p));
}

SRV _split(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size() < 1 )
		return	SRV(400, L"引数の個数が正しくありません。");

	strvec	vec;
	if ( iArguments.size()==1 ) {
		split(iArguments[0],vec);
	}
	else {
		int max_words = 0;
		if ( iArguments.size() > 2 ) {
			max_words = zen2int(iArguments[2]);
		}

		bool split_one = false;
		if ( iArguments.size() > 3 ) {
			split_one = zen2int(iArguments[3]) != 0;
		}

		split(iArguments[0].c_str(),iArguments[1].c_str(),vec,max_words,split_one);
	}

	for ( strvec::iterator i=vec.begin() ; i!=vec.end() ; ++i )
		oValues.push_back(*i);
	return	SRV(200, itos(vec.size()));
}

SRV _split_string(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size() < 1 )
		return	SRV(400, L"引数の個数が正しくありません。");

	strvec	vec;
	if ( iArguments.size()==1 ) {
		split(iArguments[0],vec);
	}
	else {
		int max_words = 0;
		if ( iArguments.size() > 2 ) {
			max_words = zen2int(iArguments[2]);
		}

		bool split_one = false;
		if ( iArguments.size() > 3 ) {
			split_one = zen2int(iArguments[3]) != 0;
		}

		split_string(iArguments[0].c_str(),iArguments[1].c_str(),vec,max_words,split_one);
	}

	for ( strvec::iterator i=vec.begin() ; i!=vec.end() ; ++i )
		oValues.push_back(*i);
	return	SRV(200, itos(vec.size()));
}

SRV _join(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<1 )
		return	SRV(400, L"引数の個数が正しくありません。");
	if ( iArguments.size()<2 )
		return	L"";

	wstring	r = iArguments[1];
	for (int n=2 ; n<iArguments.size() ; ++n)
		r += iArguments[0] + iArguments[n];
	return	r;
}

SRV _replace(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=3 )
		return	SRV(400, L"引数の個数が正しくありません。");
	replace(iArguments[0], iArguments[1], iArguments[2]);
	return	SRV(200, iArguments[0]);
}

SRV _replace_first(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=3 )
		return	SRV(400, L"引数の個数が正しくありません。");
	replace_first(iArguments[0], iArguments[1], iArguments[2]);
	return	iArguments[0];
}

SRV _erase(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	erase_all(iArguments[0], iArguments[1]);
	return	iArguments[0];
}

SRV _erase_first(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	erase_first(iArguments[0], iArguments[1]);
	return	iArguments[0];
}

SRV _count(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size() >= 2 ) {
		int cnt = 0;

		int argsize = iArguments.size();

		for ( int arg = 0 ; arg < (argsize-1) ; ++arg ) {
			cnt += count(iArguments[arg], iArguments[argsize-1]);
		}

		return	itos( cnt );
	}
	else if ( iArguments.size()==1 ) { //arg0が空っぽで切り詰められた
		return  L"0";
	}
	else {
		return	SRV(400, L"引数の個数が正しくありません。");
	}
}

SRV _compare_case(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	(wcscmp(zen2han_internal(iArguments[0]).c_str(), zen2han_internal(iArguments[1]).c_str())==0) ? L"1" : L"0";
}

SRV _compare(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	(_wcsicmp(zen2han_internal(iArguments[0]).c_str(), zen2han_internal(iArguments[1]).c_str())==0) ? L"1" : L"0";
}

SRV _compare_head_case(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	compare_head(zen2han_internal(iArguments[0]), zen2han_internal(iArguments[1])) ? L"1" : L"0";
}

SRV _compare_head(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	compare_head_nocase(zen2han_internal(iArguments[0]), zen2han_internal(iArguments[1])) ? L"1" : L"0";
}

SRV _compare_tail_case(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	compare_tail(zen2han_internal(iArguments[0]), zen2han_internal(iArguments[1])) ? L"1" : L"0";
}

SRV _compare_tail(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	compare_tail_nocase(zen2han_internal(iArguments[0]), zen2han_internal(iArguments[1])) ? L"1" : L"0";
}

SRV _length(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<1 )
		return	L"0";
	return	itos( count_chars(iArguments[0]) );
}

SRV _is_empty(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<1 )
		return	L"1";
	if ( iArguments[0].empty() )
		return	L"1";
	else
		return	L"0";
}

SRV _is_digit(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<1 || iArguments[0].empty() ) {
		return	L"0";
	}

	int dot_count = 0;
	if ( iArguments.size()>=2 ) {
		if ( wcsstr(iArguments[1].c_str(),L"整数") || iArguments[1]==L"integer" || iArguments[1]==L"int" ) {
			dot_count = 1;
		}
	}

	const wchar_t* p = iArguments[0].c_str();

	if ( *p == L'-' || *p == L'+' || *p == L'－' || *p == L'＋' ) {
		++p;
	}

	if ( *p == 0 ) { return L"0"; }

	for ( ; *p ; ++p ) {
		if ( (*p>=L'0' && *p<=L'9') || (*p>=L'０' && *p<=L'９') ) {
			continue;
		}

		if ( *p==L'.' || *p==L'．' ) {
			if ( dot_count == 0 ) {
				dot_count += 1;
				continue;
			}
		}

		return	L"0";
	}
	return	L"1";
}

SRV _is_alpha(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<1 || iArguments[0].empty() )
		return	L"0";
	return	arealphabets(iArguments[0]) ? L"1" : L"0";
}

SRV _zen2han(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( ! iArguments.size() )
		return	SRV(400, L"引数の個数が正しくありません。");

	unsigned int flag = 0xffff;
	if ( iArguments.size() >= 2 ) {
		flag = 0;
		if ( iArguments[1].find(L"アルファベット") != wstring::npos ) {
			flag |= 0x1;
		}
		if ( iArguments[1].find(L"数字") != wstring::npos ) {
			flag |= 0x2;
		}
		if ( iArguments[1].find(L"記号") != wstring::npos ) {
			flag |= 0x4;
		}
		if ( iArguments[1].find(L"カナ") != wstring::npos ) {
			flag |= 0x8;
		}
	}
	return zen2han_internal(iArguments[0],flag);
}

wstring zen2han_internal(wstring &str,unsigned int flag)
{
	if ( flag & 0x1 ) { //アルファベット
		replace_by_table(str, zen_alpha, 1, han_alpha, 1, const_strlen(han_alpha));
	}
	if ( flag & 0x2 ) { //数字
		replace_by_table(str, zen_digit, 1, han_digit, 1, const_strlen(han_digit));
	}
	if ( flag & 0x4 ) { //記号
		replace_by_table(str, zen_symbol, 1, han_symbol, 1, const_strlen(han_symbol));
	}
	if ( flag & 0x8 ) { //カナ
		replace_by_table(str, zen_kana_2, 1, han_kana_2, 2, const_strlen(zen_kana_2));
		replace_by_table(str, zen_kana_1, 1, han_kana_1, 1, const_strlen(han_kana_1));
	}
	return	str;
}

SRV _han2zen(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( ! iArguments.size() )
		return	SRV(400, L"引数の個数が正しくありません。");

	unsigned int flag = 0xffff;
	if ( iArguments.size() >= 2 ) {
		flag = 0;
		if ( iArguments[1].find(L"アルファベット") != wstring::npos ) {
			flag |= 0x1;
		}
		if ( iArguments[1].find(L"数字") != wstring::npos ) {
			flag |= 0x2;
		}
		if ( iArguments[1].find(L"記号") != wstring::npos ) {
			flag |= 0x4;
		}
		if ( iArguments[1].find(L"カナ") != wstring::npos ) {
			flag |= 0x8;
		}
	}

	wstring&	str=iArguments[0];

	if ( flag & 0x1 ) { //アルファベット
		replace_by_table(str, han_alpha, 1, zen_alpha, 1, const_strlen(han_alpha));
	}
	if ( flag & 0x2 ) { //数字
		replace_by_table(str, han_digit, 1, zen_digit, 1, const_strlen(han_digit));
	}
	if ( flag & 0x4 ) { //記号
		replace_by_table(str, han_symbol, 1, zen_symbol, 1, const_strlen(han_symbol));
	}
	if ( flag & 0x8) { //カナ
		replace_by_table(str, han_kana_2, 2, zen_kana_2, 1, const_strlen(zen_kana_2));
		replace_by_table(str, han_kana_1, 1, zen_kana_1, 1, const_strlen(han_kana_1));
	}

	return	str;
}

SRV _hira2kata(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=1 )
		return	SRV(400, L"引数の個数が正しくありません。");

	wstring&	str=iArguments[0];
	for (wstring::size_type i=0 ; i<str.size() ; ++i) {
		const wchar_t* found = wcschr(hira, str[i]);
		if ( found && *found ) {
			str[i] = kata[found - hira];
		}
	}
	return	iArguments[0];
}

SRV _kata2hira(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=1 )
		return	SRV(400, L"引数の個数が正しくありません。");

	wstring&	str=iArguments[0];
	for (wstring::size_type i=0 ; i<str.size() ; ++i) {
		const wchar_t* found = wcschr(kata, str[i]);
		if ( found && *found ) {
			str[i] = hira[found - kata];
		}
	}
	return	iArguments[0];
}

SRV _sprintf(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.empty() )
		return	SRV(400, L"引数が足りません。");
	return	sprintf(iArguments);
}

SRV _reverse(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.empty() )
		return	SRV(400, L"引数が足りません。");

	wstring	r;
	const wchar_t* p = iArguments[0].c_str();
	while (*p != L'\0') {
		r = get_a_chr(p) + r;
	}

	return	r;
}

SRV _at(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {

	if ( iArguments.size()==2 ) {
		const wchar_t* p = char_at(iArguments.at(0).c_str(), zen2int(iArguments.at(1)));
		return	(p==NULL || *p==L'\0') ? wstring() : get_a_chr(p);
	}
	//else if ( iArguments.size()==3 ) {
	//}
	else
		return	SRV(400, L"引数が正しくありません。");
}



/*
if ( compare_head(theCommand, "tm") ) {
	string	TimeCommands(const string& iCommand, const deque<string>& iArguments);
	return	TimeCommands(theCommand, iArguments);
}
*/
SRV _choice(std::deque<wstring>& iArguments, std::deque<wstring>& oValues)
{
	if ( iArguments.size()==0 )
	{
		return	L"";
	}
	return iArguments[ random(iArguments.size()) ];
}

SRV _lsimg(std::deque<wstring>& iArguments, std::deque<wstring>& oValues)
{
	if (iArguments.size() == 0)
		return L"0";
#ifdef WIN32
	WIN32_FIND_DATA wfd;
	wstring d(iArguments[0]);
	if (!compare_tail(d, L"\\")) d += L'\\';
	d += L'*';
	HANDLE h = FindFirstFile(d.c_str(), &wfd);
	if (h == INVALID_HANDLE_VALUE)
		return L"0";
	do {
		wstring lo(wfd.cFileName);
		std::transform(lo.begin(), lo.end(), lo.begin(), towlower);
		if (compare_tail(lo, L".png") ||
			compare_tail(lo, L".jpg") ||
			compare_tail(lo, L".jpe") ||
			compare_tail(lo, L".jpeg") ||
			compare_tail(lo, L".bmp"))
		{
			oValues.push_back(wfd.cFileName);
		}
	} while (FindNextFile(h, &wfd));
	FindClose(h);
	return itos(oValues.size());
#else
	// TODO だれかなんとかして
	return L"0";
#endif
}

#ifdef WIN32
static inline int mkdir(const wchar_t* path, int mode)
{
	return !CreateDirectory(path, NULL);
}
#endif
SRV _mkdir(std::deque<wstring>& iArguments, std::deque<wstring>& oValues)
{
	if (iArguments.size() == 0)
		return L"";
#ifdef WIN32
	if (mkdir(iArguments[0].c_str(), 0777) == 0)
#else
	if (mkdir(WtoUTF8(iArguments[0]).c_str(), 0777) == 0)
#endif
		return L"1";
	else
		return L"0";
}
