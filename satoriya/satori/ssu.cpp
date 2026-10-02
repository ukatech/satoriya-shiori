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

static SRV	call_ssu(wstring iCommand, std::deque<wstring>& iArguments, std::deque<wstring>& oValues, bool iIsSecure);

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

	SRV result = call_ssu(theCommand, iArguments, oValues, i_is_secure);
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
	return	call_ssu(theCommand, iArguments, oValues, m_is_secure);
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
		d(regex_match);		d(regex_find);		d(regex_findall);	d(regex_count);
		d(regex_replace);	d(regex_replace_first);
		d(regex_erase);		d(regex_erase_first);
		d(regex_split);		d(regex_escape);
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

static SRV	call_ssu(wstring iCommand, std::deque<wstring>& iArguments, std::deque<wstring>& oValues, bool iIsSecure)
{
	const std::map<wstring, Command> &theMap = func_map();

	// 命令の存在を確認
	std::map<wstring, Command>::const_iterator i = theMap.find(iCommand);
	if ( i==theMap.end() )
		return SRV(400, wstring()+L"Error: '"+iCommand+L"'という名前の命令は定義されていません。");

	// ファイルシステムを操作する命令は、SecurityLevel: local でないと実行しない
	if ( !iIsSecure && (iCommand==L"mkdir" || iCommand==L"lsimg") )
		return SRV(400, wstring()+L"Error: '"+iCommand+L"'はlocalでないと実行できません。");

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
#include	"../deelx/deelx.h"

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

// 幅と精度の上限。巨大な値でメモリを使い果たさないように。
static const int	PRINTF_MAX_WIDTH = 4096;
static const int	PRINTF_MAX_PRECISION = 512;

bool	printf_format(const wchar_t*& p, std::deque<wstring>& iArguments, std::wstringstream& os)
{
	assert(*p==L'%');
	if ( iArguments.empty() )
		return	false;	// 置き換え対象が無い

	++p;
	// 引数は書式が正しいと分かってから取り除く（失敗したら % をそのまま出すので、引数は使わない）
	wstring	str = iArguments.front();

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
			if ( width < PRINTF_MAX_WIDTH ) {
				width = width*10 + (*p - L'0');
				if ( width > PRINTF_MAX_WIDTH ) { width = PRINTF_MAX_WIDTH; }
			}
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
			if ( precision < PRINTF_MAX_PRECISION ) {
				precision = precision*10 + (*p - L'0');
				if ( precision > PRINTF_MAX_PRECISION ) { precision = PRINTF_MAX_PRECISION; }
			}
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
				os.put((wchar_t)code);
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
	iArguments.pop_front();
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
			const wchar_t* p_before = p;
			if ( printf_format(p, iArguments, sf) ) {
				s << sf.str();
				continue;
			}
			p = p_before;	// 書式が不正なら % をそのまま出す（文字列の終端を越えて進まないように）
		}
		s.put(*p++);
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
		offset = -offset;
		// start += offset（負）をintの範囲を越えずに行う。負になったら0
		if ( start < offset )
			start = 0;
		else
			start -= offset;
	}
	assert(offset >= 0 );

	if ( start < 0 )
		start = 0;
	if ( start >= len )
		return	SRV(204);
	// start + offset は int を越えることがあるので、引き算で比べる
	if ( offset >= len - start )
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

// befores[n]をafters[n]に置き換える（afters[n]がNULLなら取り除く）。
// 先頭から一度だけ走査するので、置き換えた後の文字列が再び置き換えられることはない。
// 同じ位置で複数一致したら長いほうを、長さも同じなら前にあるほうを使う。
static wstring	replace_at_once(const wstring& str, const std::vector<const wstring*>& befores, const std::vector<const wstring*>& afters)
{
	const size_t	count = befores.size();
	size_t	n;
	std::vector<wstring::size_type>	next(count);	// 各置換前が次に現れる位置
	for ( n=0 ; n<count ; ++n ) {
		next[n] = befores[n]->empty() ? wstring::npos : str.find(*befores[n]);
	}

	wstring	result;
	wstring::size_type	start = 0;
	while ( true ) {
		size_t	hit = count;
		for ( n=0 ; n<count ; ++n ) {
			if ( next[n] == wstring::npos ) {
				continue;
			}
			if ( hit == count || next[n] < next[hit] || (next[n] == next[hit] && befores[n]->size() > befores[hit]->size()) ) {
				hit = n;
			}
		}
		if ( hit == count ) {
			break;
		}
		const wstring::size_type	pos = next[hit];
		result.append(str, start, pos-start);
		if ( afters[hit] != NULL ) {
			result += *afters[hit];
		}
		start = pos + befores[hit]->size();
		for ( n=0 ; n<count ; ++n ) {
			if ( next[n] != wstring::npos && next[n] < start ) {
				next[n] = str.find(*befores[n], start);
			}
		}
	}
	result.append(str, start, wstring::npos);
	return	result;
}

SRV _replace(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<3 || iArguments.size()%2==0 )
		return	SRV(400, L"引数の個数が正しくありません。");
	if ( iArguments.size()==3 ) {
		replace(iArguments[0], iArguments[1], iArguments[2]);
		return	SRV(200, iArguments[0]);
	}
	std::vector<const wstring*>	befores, afters;
	for ( int n=1 ; n+1<iArguments.size() ; n+=2 ) {
		befores.push_back(&iArguments[n]);
		afters.push_back(&iArguments[n+1]);
	}
	return	SRV(200, replace_at_once(iArguments[0], befores, afters));
}

SRV _replace_first(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()!=3 )
		return	SRV(400, L"引数の個数が正しくありません。");
	replace_first(iArguments[0], iArguments[1], iArguments[2]);
	return	iArguments[0];
}

SRV _erase(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	if ( iArguments.size()==2 ) {
		erase_all(iArguments[0], iArguments[1]);
		return	iArguments[0];
	}
	std::vector<const wstring*>	befores, afters;
	for ( int n=1 ; n<iArguments.size() ; ++n ) {
		befores.push_back(&iArguments[n]);
		afters.push_back(NULL);
	}
	return	replace_at_once(iArguments[0], befores, afters);
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

	// 先頭への連結を繰り返すと長さの2乗の時間がかかるので、文字ごとの位置を集めて後ろから並べる
	const wchar_t* const start = iArguments[0].c_str();
	std::vector<std::ptrdiff_t>	heads;
	const wchar_t* p = start;
	while (*p != L'\0') {
		heads.push_back(p - start);
		next_a_chr(p);
	}

	wstring	r;
	r.reserve(iArguments[0].size());
	for ( std::vector<std::ptrdiff_t>::size_type n = heads.size() ; n > 0 ; --n ) {
		const wchar_t* q = start + heads[n-1];
		r += next_a_chr(q);
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

// ここから正規表現（DEELX。../deelx は git サブモジュール）
// 文字列は wchar_t のまま扱い、位置は文字数（サロゲートペアは1文字）で返す。
// オプション（省略可）は文字の並び：i=大文字小文字を区別しない s=ドットが改行にも一致 m=^と$が行頭・行末に一致 x=パターン中の空白とコメントを無視
// (?i) (?s) (?m) といったパターン内指定も使える。置換後文字列では $1 $& ${名前} $$ などが使える。

typedef CRegexpT<wchar_t>	Regex;

// パターンをコンパイルする。失敗したらエラー文を返す（成功ならNULL）
// DEELX は書式エラーを報告しないので、閉じていない括弧などはそのまま解釈される。
static const wchar_t*	regex_compile(Regex& oRegex, const wstring& iPattern, const wstring& iOptions)
{
	int	flags = 0;
	for ( wstring::const_iterator i=iOptions.begin() ; i!=iOptions.end() ; ++i ) {
		switch ( *i ) {
		case L'i': case L'I':	flags |= IGNORECASE;	break;
		case L's': case L'S':	flags |= SINGLELINE;	break;
		case L'm': case L'M':	flags |= MULTILINE;	break;
		case L'x': case L'X':	flags |= EXTENDED;	break;
		case L' ': case L',':	break;
		default:
			return	L"正規表現のオプションが正しくありません（i, s, m, x が使えます）。";
		}
	}
	try {
		oRegex.Compile(iPattern.c_str(), static_cast<int>(iPattern.size()), flags);
	}
	catch ( ... ) {
		return	L"正規表現の書式が正しくありません。";
	}
	if ( oRegex.m_builder.m_pTopElx == NULL ) {
		return	L"正規表現の書式が正しくありません。";
	}
	return	NULL;
}

static const wstring&	regex_arg(const std::deque<wstring>& iArguments, size_t n);

// 引数 iPatternIndex がパターン、その次がオプション（省略可）
static const wchar_t*	regex_compile_args(Regex& oRegex, const std::deque<wstring>& iArguments, size_t iPatternIndex)
{
	return	regex_compile(oRegex, iArguments[iPatternIndex], regex_arg(iArguments, iPatternIndex+1));
}

// 全体と各グループの一致部分を oValues に入れる（一致しなかったグループは空文字列）
static void	regex_push_groups(const MatchResult& iResult, const wstring& iStr, std::deque<wstring>& oValues)
{
	for ( int n=0 ; n<=iResult.MaxGroupNumber() ; ++n ) {
		const int	s = iResult.GetGroupStart(n), e = iResult.GetGroupEnd(n);
		oValues.push_back(s>=0 && e>=s ? iStr.substr(s, e-s) : wstring());
	}
}

// 一致のたびに iFunc を呼ぶ。iFunc が false を返したら打ち切る。
// 空文字列に一致する場合も1文字ずつ進むので止まらなくなることはない。
template <class F>
static void	regex_each_match(const Regex& iRegex, const wstring& iStr, F& iFunc)
{
	CContext	context;
	iRegex.PrepareMatch(iStr.c_str(), static_cast<int>(iStr.size()), -1, &context);
	while ( true ) {
		MatchResult	r = iRegex.Match(&context);
		if ( ! r.IsMatched() || ! iFunc(r) ) {
			break;
		}
	}
}

struct RegexCounter {
	int	count;
	RegexCounter() : count(0) {}
	bool	operator()(const MatchResult&) { ++count; return true; }
};

struct RegexCollector {
	const wstring&	str;
	std::deque<wstring>&	values;
	RegexCollector(const wstring& s, std::deque<wstring>& v) : str(s), values(v) {}
	bool	operator()(const MatchResult& r) {
		values.push_back(str.substr(r.GetStart(), r.GetEnd()-r.GetStart()));
		return true;
	}
};

struct RegexSplitter {
	const wstring&	str;
	std::deque<wstring>&	values;
	const int	max_words;
	int	last;
	RegexSplitter(const wstring& s, std::deque<wstring>& v, int m) : str(s), values(v), max_words(m), last(0) {}
	bool	operator()(const MatchResult& r) {
		const int	s = r.GetStart(), e = r.GetEnd();
		if ( s==e && (s==0 || s==static_cast<int>(str.size())) ) {
			return true;	// 先頭・末尾の空一致では区切らない
		}
		if ( max_words > 0 && static_cast<int>(values.size()) >= max_words-1 ) {
			return false;
		}
		values.push_back(str.substr(last, s-last));
		last = e;
		return true;
	}
};

// 置換。iTimes<0 なら全部、そうでなければその回数まで。
static SRV	regex_replace_impl(const wstring& iStr, const wstring& iPattern, const wstring& iTo, const wstring& iOptions, int iTimes)
{
	Regex	re;
	const wchar_t*	err = regex_compile(re, iPattern, iOptions);
	if ( err ) {
		return	SRV(400, err);
	}
	int	result_length = 0;
	wchar_t*	r = re.Replace(iStr.c_str(), static_cast<int>(iStr.size()), iTo.c_str(), static_cast<int>(iTo.size()), result_length, -1, iTimes);
	if ( r == NULL ) {
		return	SRV(200, iStr);
	}
	wstring	result(r, result_length);
	Regex::ReleaseString(r);
	return	SRV(200, result);
}

// 省略された引数の代わりに使う空文字列
static const wstring&	regex_arg(const std::deque<wstring>& iArguments, size_t n)
{
	static const wstring	empty;
	return	n < iArguments.size() ? iArguments[n] : empty;
}

// regex_match(対象, パターン, [オプション])
// 一致すれば1、しなければ0。Value0に一致した部分、Value1以降に括弧のグループ。
SRV _regex_match(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	Regex	re;
	const wchar_t*	err = regex_compile_args(re, iArguments, 1);
	if ( err )
		return	SRV(400, err);
	const wstring&	str = iArguments[0];
	MatchResult	r = re.Match(str.c_str(), static_cast<int>(str.size()), -1);
	if ( ! r.IsMatched() )
		return	L"0";
	regex_push_groups(r, str, oValues);
	return	L"1";
}

// regex_find(対象, パターン, [オプション], [開始位置])
// 最初に一致した位置（0始まりの文字数）、なければ-1。Valueはregex_matchと同じ。
SRV _regex_find(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	Regex	re;
	const wchar_t*	err = regex_compile_args(re, iArguments, 1);
	if ( err )
		return	SRV(400, err);
	const wstring&	str = iArguments[0];
	int	start = 0;
	if ( iArguments.size()>3 ) {
		const int	len = static_cast<int>(count_chars(str));
		start = zen2int(iArguments[3]);
		if ( start < 0 )
			start = len + start;
		if ( start < 0 || start > len )
			return	L"-1";
		start = static_cast<int>(char_at(str.c_str(), start) - str.c_str());
	}
	MatchResult	r = re.Match(str.c_str(), static_cast<int>(str.size()), start);
	if ( ! r.IsMatched() )
		return	L"-1";
	regex_push_groups(r, str, oValues);
	return	itos(static_cast<long>(count_chars(str.substr(0, r.GetStart()))));
}

// regex_findall(対象, パターン, [オプション])
// 一致した部分すべてをValueに入れ、その個数を返す。
SRV _regex_findall(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	Regex	re;
	const wchar_t*	err = regex_compile_args(re, iArguments, 1);
	if ( err )
		return	SRV(400, err);
	std::deque<wstring>	found;
	RegexCollector	collector(iArguments[0], found);
	regex_each_match(re, iArguments[0], collector);
	oValues.insert(oValues.end(), found.begin(), found.end());
	return	itos(static_cast<long>(found.size()));
}

// regex_count(対象, パターン, [オプション])
SRV _regex_count(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	Regex	re;
	const wchar_t*	err = regex_compile_args(re, iArguments, 1);
	if ( err )
		return	SRV(400, err);
	RegexCounter	counter;
	regex_each_match(re, iArguments[0], counter);
	return	itos(counter.count);
}

// regex_replace(対象, パターン, 置換後, [オプション])
SRV _regex_replace(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<3 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	regex_replace_impl(iArguments[0], iArguments[1], iArguments[2], regex_arg(iArguments, 3), -1);
}

SRV _regex_replace_first(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<3 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	regex_replace_impl(iArguments[0], iArguments[1], iArguments[2], regex_arg(iArguments, 3), 1);
}

// regex_erase(対象, パターン, [オプション])
SRV _regex_erase(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	regex_replace_impl(iArguments[0], iArguments[1], wstring(), regex_arg(iArguments, 2), -1);
}

SRV _regex_erase_first(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	return	regex_replace_impl(iArguments[0], iArguments[1], wstring(), regex_arg(iArguments, 2), 1);
}

// regex_split(対象, パターン, [オプション], [最大分割数])
// パターンに一致した部分で区切り、各要素をValueに入れて個数を返す。
SRV _regex_split(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<2 )
		return	SRV(400, L"引数の個数が正しくありません。");
	Regex	re;
	const wchar_t*	err = regex_compile_args(re, iArguments, 1);
	if ( err )
		return	SRV(400, err);
	const wstring&	str = iArguments[0];
	std::deque<wstring>	pieces;
	RegexSplitter	splitter(str, pieces, iArguments.size()>3 ? zen2int(iArguments[3]) : 0);
	regex_each_match(re, str, splitter);
	pieces.push_back(str.substr(splitter.last));
	oValues.insert(oValues.end(), pieces.begin(), pieces.end());
	return	itos(static_cast<long>(pieces.size()));
}

// regex_escape(文字列)
// 正規表現の特殊文字を \ でエスケープして返す（文字列をそのままパターンに埋め込むため）。
SRV _regex_escape(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	if ( iArguments.size()<1 )
		return	L"";
	wstring	r;
	for ( wstring::const_iterator i=iArguments[0].begin() ; i!=iArguments[0].end() ; ++i ) {
		if ( wcschr(L"\\^$.|?*+()[]{}", *i) != NULL )
			r += L'\\';
		r += *i;
	}
	return	SRV(200, r);
}
