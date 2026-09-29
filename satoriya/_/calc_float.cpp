#include	"stltool.h"
#include	"simple_stack.h"

#include <ctype.h>
#include <math.h>

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


typedef	double	VALUE_TYPE;

// 数値演算を行って結果をStringに。返値falseなら式が変。
// ２項演算子 +,-,*,/,%,^,<,>,<=,>=,==,!=,&&,||
// 単項演算子 +,-,!
// 被演算子 整数値, カッコ。
// 全て半角であること。空白等は認めない
extern bool calc_float(const wchar_t* iExpression, int* oResult);
// 半角全角スペースとタブ記号の消去、数字・記号の半角化まで全部やったげる
extern	bool calc_float(wstring& ioString);


struct calc_element {
	wstring	str;
	int		priority;
	calc_element(wstring _str, int _priority) : str(_str), priority(_priority) {}
	calc_element() : str(), priority(0) {}
};

static bool	make_array(const wchar_t*& p, std::vector<calc_element>& oData) {

	while (true) {

		// 被演算子または単項演算子を取得

		if ( *p == L'(' ) {
			oData.push_back( calc_element(L"(", 110) );
			if ( !make_array(++p, oData) )	// カッコ内を再帰処理
				return	false;	// エラーはトップまで伝える
			if ( *p++ !=L')' )
				return	false;
			oData.push_back( calc_element(L")", 10) );
		}
		else {
			if ( !iswdigit(*p) && (*p)!=L'.') {
				wstring	str;
				if ( *p==L'!' ) str=L"!";
				else if ( *p==L'+' ) str=L"+";
				else if ( *p==L'-' ) str=L"-";
				else return false;	// 単項演算子じゃない、順番が変
				++p;
				oData.push_back( calc_element(str, 90) );
				continue;
			}

			int	len=0;
			while (iswdigit(p[len]) || p[len]==L'.') ++len;

			wstring	str(p,len);
			if ( count(str,L".")>=2 )
				return	false;	// 小数点が２個以上ある
			oData.push_back( calc_element(str, 100) );
			p+=len;
		}

		// 被演算子の後にのみ、正常脱出
		if ( *p==L'\0' || *p==L')' )
			return	true;

		// ２項演算子を取得

		const wchar_t*	oprs[] = { // 長いもの順に比較するの。
			L"&&",L"||",L"==",L"!=",L"<=",L">=",L"<",L">",L"+",L"-",L"*",L"/",L"^"/*,"."*/};

		int	len=0, i=0;
		for (i=0 ; i<sizeof(oprs)/sizeof(oprs[0]) ; ++i) {
			len = wcslen(oprs[i]);
			if ( wcsncmp(p, oprs[i], len) == 0 )
				break;
		}
		if ( i==sizeof(oprs)/sizeof(oprs[0]) )
			return	false;	// どの演算子でもない

		// 演算子に応じて優先度を設定
		wstring	str(p,len);
		p+=len;
		int	priority;

		/*if ( str=="." ) { priority=85; }	// 小数点
		else */if ( str==L"^" ) { priority=80; }
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

// ２項演算
#define	a_op_b(op)	\
	else if ( el.str == ascii_to_w(#op) ) {	\
		assert(stack.size()>=2); \
		VALUE_TYPE	result = stack.from_top(1) op stack.from_top(0); \
		stack.pop(2); stack.push(result); }
//「（calc_float,5/3）」
static VALUE_TYPE	calc_polish(simple_stack<calc_element>& polish) {
	simple_stack<VALUE_TYPE>	stack;
	for ( int n=0 ; n<polish.size()-1 ; n++ ) {
		calc_element&	el=polish[n];
		if ( el.priority==100 ) { // 被演算子
			stack.push( wcstod(el.str.c_str(), NULL) );
		}
		else if ( el.priority==90 ) {	// 単項演算子
			assert(stack.size()>=1);
			if ( el.str==L"!" ) stack.push( !stack.pop() );
			else if (el.str == L"+") /*NOOP*/;
			else if ( el.str==L"-" ) stack.push( -stack.pop() );
			else assert(0);
		}
		/*else if ( el.priority==85 ) {	// 小数点
			assert(stack.size()>=2);
			assert(el.str==".");
			char	buf[256];
			sprintf(buf, "%d.%d", int(stack.from_top(1)), int(stack.from_top(0)));
			stack.pop(2);
			stack.push( atof(buf) );
		}*/
		else if ( el.str == L"^" ) {
			assert(stack.size()>=2);
			VALUE_TYPE	result = pow( stack.from_top(1), stack.from_top(0) );
			stack.pop(2); stack.push(result); }
		a_op_b(*)
		a_op_b(/)
		a_op_b(+)
		a_op_b(-)
		a_op_b(<)
		a_op_b(>)
		a_op_b(<=)
		a_op_b(>=)
		a_op_b(==)
		a_op_b(!=)
		a_op_b(&&)
		a_op_b(||)
		else 
			assert(0);

	}
	assert(stack.size()==1);
	return	stack.pop();
}

bool calc_float(const wchar_t* iExpression, VALUE_TYPE* oResult) {
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
	*oResult = calc_polish(polish);
	return	true;
}


bool calc_float(wstring& ioString) {
	erase_all(ioString, L"　");
	erase_all(ioString, L" ");
	erase_all(ioString, L"\t");
	replace(ioString, L"＋", L"+");
	replace(ioString, L"－", L"-");
	replace(ioString, L"\x2212", L"-");	// MINUS SIGN（UTF-8辞書で使われやすい）
	replace(ioString, L"＊", L"*");
	replace(ioString, L"×", L"*");
	replace(ioString, L"／", L"/");
	replace(ioString, L"÷", L"/");
	replace(ioString, L"＜", L"<");
	replace(ioString, L"＞", L">");
	replace(ioString, L"＝", L"=");
	replace(ioString, L"！", L"!");
	replace(ioString, L"＆", L"&");
	replace(ioString, L"｜", L"|");
	replace(ioString, L"（", L"(");
	replace(ioString, L"）", L")");
	replace(ioString, L"０", L"0");
	replace(ioString, L"１", L"1");
	replace(ioString, L"２", L"2");
	replace(ioString, L"３", L"3");
	replace(ioString, L"４", L"4");
	replace(ioString, L"５", L"5");
	replace(ioString, L"６", L"6");
	replace(ioString, L"７", L"7");
	replace(ioString, L"８", L"8");
	replace(ioString, L"９", L"9");
	replace(ioString, L"．", L".");
	VALUE_TYPE	result;
	if ( !calc_float(ioString.c_str(), &result) )
		return	false;

	char	buf[128];
	sprintf(buf, "%f", result);
	ioString = ascii_to_w(buf);

	while ( compare_tail(ioString, L"0") )
		ioString.assign(ioString.c_str(), ioString.size()-1);
	if ( compare_tail(ioString, L".") )
		ioString.assign(ioString.c_str(), ioString.size()-1);

	return	true;
}
