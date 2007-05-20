#include	<string>
using namespace std;

#include	"simple_stack.h"
#include	"stltool.h"



// 数値演算を行って結果をStringに。返値falseなら式が変。
// ２項演算子 +,-,*,/,%,^,<,>,<=,>=,==,!=,&&,||
// 単項演算子 +,-,!
// 被演算子 整数値, カッコ。
// 全て半角であること。空白等は認めない
extern bool calc(const char* iExpression, int* oResult);
// 半角全角スペースとタブ記号の消去、数字・記号の半角化まで全部やったげる
extern	bool calc(string& ioString);


struct calc_element {
	string	str;
	int		priority;
	calc_element(string _str, int _priority) : str(_str), priority(_priority) {}
	calc_element() : str(), priority(0) {}
};

static bool	make_deque(const char*& p, deque<calc_element>& oDeque) {

	while (true) {

		// 被演算子または単項演算子を取得

		if ( *p == '(' ) {
			oDeque.push_back( calc_element("(", 110) );
			if ( !make_deque(++p, oDeque) )	// カッコ内を再帰処理
				return	false;	// エラーはトップまで伝える
			if ( *p++ !=')' )
				return	false;
			oDeque.push_back( calc_element(")", 10) );
		}
		else {
			if ( !isdigit(*p) ) {
				string	str;
				if ( *p=='!' ) str="!";
				else if ( *p=='+' ) str="+";
				else if ( *p=='-' ) str="-";
				else return false;	// 単項演算子じゃない、順番が変
				++p;
				oDeque.push_back( calc_element(str, 90) );
				continue;
			}

			int	len=0;
			while (isdigit(p[len])) ++len;

			oDeque.push_back( calc_element(string(p,len), 100) );
			p+=len;
		}

		// 被演算子の後にのみ、正常脱出
		if ( *p=='\0' || *p==')' )
			return	true;

		// ２項演算子を取得

		const char*	oprs[] = { // 長いもの順に比較するの。
			"&&","||","==","!=","<=",">=","<",">","+","-","*","/","%"};

		int	len=0;
		for (int i=0 ; i<sizeof(oprs)/sizeof(oprs[0]) ; ++i) {
			len = strlen(oprs[i]);
			if ( strncmp(p, oprs[i], len) == 0 )
				break;
		}
		if ( i==sizeof(oprs)/sizeof(oprs[0]) )
			return	false;	// どの演算子でもない

		// 演算子に応じて優先度を設定
		string	str(p,len);
		p+=len;
		int	priority;

		if ( str=="^" ) { priority=80; }
		else if ( str=="*" || str=="/" || str=="%" ) { priority=70; }
		else if ( str=="+" || str=="-" ) { priority=60; }
		else if ( str=="<" || str==">" || str=="<=" || str==">=" ) { priority=80; }
		else if ( str=="==" || str=="!=" ) { priority=45; }
		else if ( str=="&&" ) { priority=40; }
		else if ( str=="||" ) { priority=35; }
		else return	false;

		oDeque.push_back( calc_element(str, priority) );
	}
}

// ２項演算
#define	a_op_b(op)	\
	else if ( el.str == #op ) {	\
		assert(stack.size()>=2); \
		int	result = stack.from_top(1) op stack.from_top(0); \
		stack.pop(2); stack.push(result); }

static int	calc_polish(simple_stack<calc_element>& polish) {
	simple_stack<int>	stack;
	for ( int n=0 ; n<polish.size()-1 ; n++ ) {
		calc_element&	el=polish[n];
		if ( el.priority==100 ) { // 被演算子
			stack.push(stoi(el.str));
		}
		else if ( el.priority==90 ) {	// 単項演算子
			assert(stack.size()>=1);
			if ( el.str=="!" ) stack.push( !stack.pop() );
			else if ( el.str=="+" ) NULL;
			else if ( el.str=="-" ) stack.push( -stack.pop() );
			else assert(0);
		}
		a_op_b(^)
		a_op_b(*)
		a_op_b(/)
		a_op_b(%)
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

bool calc(const char* iExpression, int* oResult) {
	deque<calc_element>	org;
	if ( !make_deque(iExpression, org) )
		return	false;
	if ( *iExpression!='\0' )
		return	false;	// なんかゴミが残ってた？

	simple_stack<calc_element>	stack,polish;
	stack.push(calc_element("Guard", 0));	// 番兵

	deque<calc_element>::const_iterator i;
	for ( i=org.begin() ; i!=org.end() ; ++i ) {
		while ( i->priority <= stack.top().priority && stack.top().str != "(" )
			polish.push(stack.pop());
		if ( i->str != ")" ) stack.push(*i); else stack.pop();
	}

	// stackから残りを取り出す
	while ( !stack.empty() )
		polish.push(stack.pop());

	// 計算
	*oResult = calc_polish(polish);
	return	true;
}


bool calc(string& ioString) {
	erase(ioString, "　");
	erase(ioString, " ");
	erase(ioString, "\t");
	replace(ioString, "＋", "+");
	replace(ioString, "－", "-");
	replace(ioString, "＊", "*");
	replace(ioString, "×", "*");
	replace(ioString, "／", "/");
	replace(ioString, "÷", "/");
	replace(ioString, "％", "%");
	replace(ioString, "＾", "^");
	replace(ioString, "＜", "<");
	replace(ioString, "＞", ">");
	replace(ioString, "＝", "=");
	replace(ioString, "！", "!");
	replace(ioString, "＆", "&");
	replace(ioString, "｜", "|");
	replace(ioString, "（", "(");
	replace(ioString, "）", ")");
	replace(ioString, "０", "0");
	replace(ioString, "１", "1");
	replace(ioString, "２", "2");
	replace(ioString, "３", "3");
	replace(ioString, "４", "4");
	replace(ioString, "５", "5");
	replace(ioString, "６", "6");
	replace(ioString, "７", "7");
	replace(ioString, "８", "8");
	replace(ioString, "９", "9");
	int	result;
	if ( !calc(ioString.c_str(), &result) )
		return	false;
	ioString = itos(result);
	return	true;
}
