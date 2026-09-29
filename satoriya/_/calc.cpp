#include	"stltool.h"
#include	"simple_stack.h"
#ifdef POSIX
#  include      "Utilities.h"
#else
#  include	<mbctype.h>
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

// 数値・文字列演算を行って結果をStringに。返値falseなら式が変。
// ２項演算子 +,-,*,/,%,^,<,>,<=,>=,==,!=,&&,||,=~,!~
// 単項演算子 +,-,!
// 被演算子 整数値、カッコ、文字列

// 数値は全て演算可能、文字列を扱う演算は以下のみ。ただし、数値は文字列へキャストされる
// 文字列 != 文字列
// 文字列 == 文字列
// 文字列 + 文字列
// 文字列 - 文字列　消去
// 文字列 < 文字列　長さ比較
// 文字列 > 文字列　長さ比較
// 文字列 <= 文字列　長さ比較
// 文字列 >= 文字列　長さ比較
// 文字列 * 数値　
// 文字列 =~ 文字列
// 文字列 !~ 文字列

// 全て半角であること。空白等は認めない
extern bool calc(const wchar_t* iExpression, wstring& oResult,bool isStrict);
// 半角全角スペースとタブ記号の消去、数字・記号の半角化まで全部やったげる
extern	bool calc(wstring& ioString,bool isStrict = false);


// 数値は64bit整数（VC6には long long が無いので __int64）
#if defined(_MSC_VER) && _MSC_VER <= 1200
typedef __int64				calc_int;
typedef unsigned __int64	calc_uint;
#else
typedef long long			calc_int;
typedef unsigned long long	calc_uint;
#endif

static const calc_uint	CALC_INT_MAX = ~static_cast<calc_uint>(0) >> 1;

// 10進数の文字列を数値に。範囲外は wcstol と同じく最大値・最小値にする
static calc_int	stoi64(const wstring& s) {
	const wchar_t*	p = s.c_str();
	const bool	minus = ( *p==L'-' );
	if ( minus || *p==L'+' )
		++p;
	const calc_uint	limit = minus ? CALC_INT_MAX+1 : CALC_INT_MAX;
	calc_uint	n = 0;
	for ( ; *p>=L'0' && *p<=L'9' ; ++p ) {
		const calc_uint	d = *p - L'0';
		if ( n > (limit-d)/10 ) {
			n = limit;
			break;
		}
		n = n*10 + d;
	}
	return	static_cast<calc_int>( minus ? 0-n : n );
}

static wstring	i64tos(calc_int i) {
	wchar_t	buf[24];
	wchar_t*	p = buf + sizeof(buf)/sizeof(buf[0]) - 1;
	*p = L'\0';
	// 最小値は符号を反転できないので、符号なしで桁を求める
	calc_uint	n = static_cast<calc_uint>(i);
	if ( i<0 )
		n = 0-n;
	do {
		*--p = static_cast<wchar_t>(L'0' + static_cast<int>(n%10));
		n /= 10;
	} while ( n!=0 );
	if ( i<0 )
		*--p = L'-';
	return	p;
}

// 符号反転（最小値はそのまま）
static calc_int	neg64(calc_int i) {
	return	static_cast<calc_int>( 0-static_cast<calc_uint>(i) );
}


struct calc_element {
	wstring	str;
	int		priority;
	calc_element(wstring _str, int _priority) : str(_str), priority(_priority) {}
	calc_element() : str(), priority(0) {}
};



// 使用可能演算子か？　そうなら長さ（1or2）を、さもなくば 0 を返す。
inline int check_operator(const wchar_t* p) {

	static const wchar_t*	oprs[] = { // 長いもの順に比較するの。
		L"&&",L"||",L"==",L"!=",L"<=",L">=",L"=~",L"!~",L"<",L">",L"+",L"-",L"*",L"/",L"%",L"^"};
	static const int	num_oprs = sizeof(oprs)/sizeof(oprs[0]);

	for (int i=0 ; i<num_oprs ; ++i) {
		int len=wcslen(oprs[i]);
		if ( wcsncmp(p, oprs[i], len) == 0 )
			return	len;
	}
	return	0;	// どの演算子でもない
}



inline bool my_isdigit(int c) {
	return	( c>=L'0' && c<=L'9' );
}

// 数値か？　そうなら長さを、さもなくば 0 を返す。
inline int check_number(const wchar_t* start_pos) {
	const wchar_t* p=start_pos;
	while ( my_isdigit(*p) )
		++p;
	int	len = p-start_pos;

	if ( len==0 )
		return	0;	// 最初から違う
	if ( *p==L'\0' || *p==L')' )
		return	len;	// 数値で項が終わってるならtrue
	if ( check_operator(p)>0 )
		return	len;	// 次が演算子でもＯＫ
	return	0;	// そうでないなら、これは文字列だろう。
}


static bool	make_array(const wchar_t*& p, std::vector<calc_element>& oData) {


	while (true) {

		// 被演算子または単項演算子を取得
		int	len;

		if ( *p == L'(' ) {
			oData.push_back( calc_element(L"(", 110) );
			if ( !make_array(++p, oData) )	// カッコ内を再帰処理
				return	false;	// エラーはトップまで伝える
			if ( *p++ !=L')' )
				return	false;
			oData.push_back( calc_element(L")", 10) );
		}
		else if ( *p==L'-' && (len=check_number(p+1))!=0 ) {	// 負の数値
			// 単項演算子と数値に分けると -9223372036854775808 の 9223372036854775808 が読めないので、符号ごと数値として扱う
			oData.push_back( calc_element(wstring(p,len+1), 100) );
			p+=len+1;
		}
		else if (*p==L'!' || *p==L'+' || *p==L'-') {	// 単項演算子
			oData.push_back( calc_element(wstring(p++, 1), 90) );
			continue;
		}
		else if ( (len=check_number(p))!=0 ) {	// 被演算子（数値）
			oData.push_back( calc_element(wstring(p,len), 100) );
			p+=len;
		}
		else {	// 被演算子（文字列）
			//return	false;	// ここを有効にすると文字列演算をしない。扱わせようとするとエラー

			// 演算子or終了まで全てを文字列と見なす
			const wchar_t*	start=p;
			while (*p!=L'\0' && *p!=L')') {
				if (*p==L'!' || *p==L'+' || *p==L'-' )
					break;
				if ( check_operator(p) )
					break;
				++p;
			}
			if (p==start)
				return	false;	// 被演算子が必要なのに、ない。

			oData.push_back( calc_element(wstring(start,p-start), 100) );
		}

		// 項の終了。被演算子の後であるこの場所でのみ正常脱出
		if ( *p==L'\0' || *p==L')' )
			return	true;

		// ２項演算子を取得
		if ( (len=check_operator(p))==0 )
			return	false;	// どの演算子でもない
		wstring	str(p,len);
		p+=len;

		// 演算子に応じて優先度を設定
		int	priority;
		if ( str==L"^" ) { priority=80; }
		else if ( str==L"=~" || str==L"!~" ) { priority=75; } // パターンマッチ
		else if ( str==L"*" || str==L"/" || str==L"%" ) { priority=70; }
		else if ( str==L"+" || str==L"-" ) { priority=60; }
		else if ( str==L"<" || str==L">" || str==L"<=" || str==L">=" ) { priority=50; }
		else if ( str==L"==" || str==L"!=" ) { priority=45; }
		else if ( str==L"&&" ) { priority=40; }
		else if ( str==L"||" ) { priority=35; }
		else return	false;

		oData.push_back( calc_element(str, priority) );
	}
}

#ifdef NDEBUG
#define assert_special(a) if ( !(a) ) { return false; }
#else
#define assert_special(a) assert(a)
#endif

// ２項演算（数値のみ用）
#define	a_op_b(op)	\
	else if ( el.str == ascii_to_w(#op) ) {	\
		assert_special(stack.size()>=2); \
		if ( !aredigits(stack.from_top(0)) || !aredigits(stack.from_top(1)) ){ return false; }\
		calc_int	result = stoi64(stack.from_top(1)) op stoi64(stack.from_top(0)); \
		stack.pop(2); stack.push(i64tos(result)); }

// ２項演算（stringとして扱う != と == 用）
#define	even_a_op_b(op)	\
	else if ( el.str == ascii_to_w(#op) ) {	\
		assert_special(stack.size()>=2); \
		calc_int	result = stack.from_top(1) op stack.from_top(0); \
		stack.pop(2); stack.push(i64tos(result)); }

// ２項演算（stringとして扱う != と == 用）
#define	length_a_op_b(op)	\
	else if ( el.str == ascii_to_w(#op) ) {	\
		assert_special(stack.size()>=2); \
		if ( aredigits(stack.from_top(0)) && aredigits(stack.from_top(1)) ){\
			calc_int	result = stoi64(stack.from_top(1)) op stoi64(stack.from_top(0)); \
			stack.pop(2); stack.push(i64tos(result)); \
		} else {\
			calc_int	result = count_chars(stack.from_top(1)) op count_chars(stack.from_top(0)); \
			stack.pop(2); stack.push(i64tos(result)); \
		} \
	}


static bool	calc_polish(simple_stack<calc_element>& polish, wstring& oResult,bool isStrict) {
	simple_stack<wstring>	stack;
	for ( int n=0 ; n<polish.size()-1 ; n++ ) {
		calc_element&	el=polish[n];
		if ( el.priority==100 ) { // 被演算子
			stack.push(el.str);
		}
		else if ( el.priority==90 ) {	// 単項演算子
			assert_special(stack.size()>=1);
			if ( !aredigits(stack.top()) )
				return	false;
			if ( el.str==L"!" ) stack.push( i64tos(!stoi64(stack.pop())) );
			else if ( el.str==L"+" ) /*NOOP*/;
			else if ( el.str==L"-" ) stack.push( i64tos(neg64(stoi64(stack.pop()))) );
			else assert_special(0);
		}
		else if ( el.str == L"^" ) {
			assert_special(stack.size()>=2);
			wstring	rhs=stack.pop(), lhs=stack.pop();
			if ( !aredigits(lhs) || !aredigits(rhs) ) { return false; }
			const calc_int	base = stoi64(lhs);
			const calc_int	exponent = stoi64(rhs);
			calc_int	result = 1;
			if ( exponent < 0 ) {
				// negative exponent: integer result
				if ( base == 0 ) { return false; }
				if ( base == 1 ) { result = 1; }
				else if ( base == -1 ) { result = (exponent % 2 == 0) ? 1 : -1; }
				else { result = 0; }
			}
			else {
				calc_uint	r = 1, b = (calc_uint)base;
				for ( calc_int e = exponent ; e > 0 ; e >>= 1 ) {
					if ( e & 1 ) { r *= b; }
					b *= b;
				}
				result = (calc_int)r;
			}
			stack.push(i64tos(result));
		}
		else if ( el.str == L"*" ) {
			assert_special(stack.size()>=2);
			wstring	rhs=stack.pop(), lhs=stack.pop();
			if ( aredigits(lhs) && aredigits(rhs) ) {
				stack.push(i64tos( stoi64(lhs)*stoi64(rhs) )); 
			} else if ( aredigits(rhs) && ! isStrict ) {
				calc_int	num = stoi64(rhs);
				stack.push(L"");
				for (calc_int i=0;i<num;++i)
					stack.top() += lhs;
			} else {
				return	false;
			}
		}
		else if (el.str == L"/") {
			assert_special(stack.size() >= 2);
			wstring	rhs = stack.pop(), lhs = stack.pop();
			if (aredigits(lhs) && aredigits(rhs) && stoi64(rhs) != 0) {
				// 最小値 / -1 はCPU例外になるので、-1 で割るのは符号反転で済ませる
				const calc_int	r = stoi64(rhs);
				stack.push(i64tos(r == -1 ? neg64(stoi64(lhs)) : stoi64(lhs) / r));
			}
			else {
				return false;
			}
		}
		else if (el.str == L"%") {
			assert_special(stack.size() >= 2);
			wstring	rhs = stack.pop(), lhs = stack.pop();
			if (aredigits(lhs) && aredigits(rhs) && stoi64(rhs) != 0) {
				// 最小値 % -1 もCPU例外になる。-1 で割った余りは常に 0
				const calc_int	r = stoi64(rhs);
				stack.push(i64tos(r == -1 ? 0 : stoi64(lhs) % r));
			}
			else {
				return false;
			}
		}
		else if ( el.str == L"+" ) {
			assert_special(stack.size()>=2);
			wstring	rhs=stack.pop(), lhs=stack.pop();
			if ( aredigits(lhs) && aredigits(rhs) ) {
				stack.push(i64tos( stoi64(lhs)+stoi64(rhs) )); 
			} else if ( ! isStrict ) {
				stack.push(lhs+rhs); 
			} else {
				return false;
			}
		}
		else if ( el.str == L"-" ) {
			assert_special(stack.size()>=2);
			wstring	rhs=stack.pop(), lhs=stack.pop();
			if ( aredigits(lhs) && aredigits(rhs) ) {
				stack.push(i64tos( stoi64(lhs)-stoi64(rhs) )); 
			} else if ( ! isStrict ) {
				erase_all(lhs, rhs);
				stack.push(lhs);
			} else {
				return false;
			}
		}
		else if ( el.str == L"=~" || el.str == L"!~" ) {
			// パターンマッチ
			assert_special(stack.size()>=2);
			wstring	rhs=stack.pop(), lhs=stack.pop();

			const bool	found = ( lhs.find(rhs) != wstring::npos );
			stack.push( ((el.str == L"=~") == found) ? L"1" : L"0" );
		}
		length_a_op_b(<)
		length_a_op_b(>)
		length_a_op_b(<=)
		length_a_op_b(>=)
		even_a_op_b(==)
		even_a_op_b(!=)
		a_op_b(&&)
		a_op_b(||)
		else 
			assert_special(0);

	}
	assert_special(stack.size()==1);
	oResult = stack.pop();
	return	true;
}


bool calc(const wchar_t* iExpression, wstring& oResult,bool isStrict) {
	std::vector<calc_element>	org;
	if ( !make_array(iExpression, org) )
		return	false;
	if ( *iExpression!=L'\0' )
		return	false;	// なんかゴミが残ってた？

	simple_stack<calc_element>	stack,polish;
	stack.push(calc_element(L"Guard", 0));	// 番兵

	std::vector<calc_element>::const_iterator i;
	for ( i=org.begin() ; i!=org.end() ; ++i ) {
		// ^ is right-associative
		while ( (i->str == L"^" ? i->priority < stack.top().priority : i->priority <= stack.top().priority) && stack.top().str != L"(" )
			polish.push(stack.pop());
		if ( i->str != L")" ) stack.push(*i); else stack.pop();
	}

	// stackから残りを取り出す
	while ( !stack.empty() )
		polish.push(stack.pop());

	// 計算
	return	calc_polish(polish, oResult,isStrict);
}


bool calc(wstring& ioString,bool isStrict)
{
	wstring iString = ioString;

	erase_all(iString, L"　");
	erase_all(iString, L" ");
	erase_all(iString, L"\t");

	// ﾆｮﾛは単体で演算子にはしたくないー
	replace(iString, L"＝\xFF5E", L"=~");	// FULLWIDTH TILDE（CP932の0x8160）
	replace(iString, L"！\xFF5E", L"!~");
	replace(iString, L"＝\x301C", L"=~");	// WAVE DASH
	replace(iString, L"！\x301C", L"!~");

	replace(iString, L"＋", L"+");
	replace(iString, L"－", L"-");
	replace(iString, L"\x2212", L"-");	// MINUS SIGN（UTF-8辞書で使われやすい）
	replace(iString, L"＊", L"*");
	replace(iString, L"×", L"*");
	replace(iString, L"／", L"/");
	replace(iString, L"÷", L"/");
	replace(iString, L"％", L"%");
	replace(iString, L"＾", L"^");
	replace(iString, L"＜", L"<");
	replace(iString, L"＞", L">");
	replace(iString, L"＝", L"=");
	replace(iString, L"！", L"!");
	replace(iString, L"＆", L"&");
	replace(iString, L"｜", L"|");
	replace(iString, L"（", L"(");
	replace(iString, L"）", L")");
	replace(iString, L"０", L"0");
	replace(iString, L"１", L"1");
	replace(iString, L"２", L"2");
	replace(iString, L"３", L"3");
	replace(iString, L"４", L"4");
	replace(iString, L"５", L"5");
	replace(iString, L"６", L"6");
	replace(iString, L"７", L"7");
	replace(iString, L"８", L"8");
	replace(iString, L"９", L"9");

	wstring	theResult;
	if ( !calc(iString.c_str(), theResult, isStrict) ) {
		return	false;
	}

	//全角・半角とかをむやみに変換しないように気をつける
	if ( theResult != iString ) {
		ioString = theResult;
	}
	return	true;
}
