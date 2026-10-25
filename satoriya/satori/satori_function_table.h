//---------------------------------------------------------------------------
//	内蔵関数の表の型
//
//	satori_builtin.cpp（実行）と satori_lint.cpp（静的検査）が同じ表を見るために分けてある。
//	satori.h を先に include すること。
//---------------------------------------------------------------------------
#ifndef SATORI_FUNCTION_TABLE_H
#define SATORI_FUNCTION_TABLE_H

// 内蔵関数。iArgvは引数、for_calc / for_non_talk は特殊形式の引数の展開に使う。
typedef wstring (Satori::*FunctionHandler)(const strvec& iArgv, bool for_calc, bool for_non_talk);

// 引数の意味。lint が式と本文を見分けるのに使う。処理（func_*）の引数の扱いを変えたら、ここも合わせること。
enum SatoriFunctionArgKind {
	ARG_PLAIN,		// 式は無い（引数は文字列として使う）
	ARG_SET,		// 先頭が代入する変数名（set）
	ARG_CALL_NAME,	// 先頭が（）として呼ばれる名前（call / vncall）
	ARG_LOOP_NAME,	// 先頭が（）として呼ばれる名前で、「名前カウンタ」という変数を作る（loop）
	ARG_WRITES,		// 実行時に決まる名前の変数や単語群、SAORIを作る（split_to など）。lint は未定義の名前を断定できなくなる
	ARG_WHEN,		// 先頭が式、残りは本文
	ARG_LOOP,		// 最後が本文、それ以外は式
	ARG_WHENLIST,	// 先頭は比べる値、続く（式の続き、結果）の組、最後に余りがあれば else
};

struct SatoriFunction {
	FunctionHandler	handler;
	bool	is_special;		// 特殊形式：引数を展開せずに渡し、必要なものだけ展開する
	bool	need_secure;	// SecurityLevel: local のときだけ実行できる
	int		min_args;		// 引数の最少個数（-1なら lint は個数を調べない）
	int		max_args;		// 引数の最大個数（-1なら上限なし）
	int		arg_kind;		// SatoriFunctionArgKind
};

struct SatoriFunctionTable {
	std::map<wstring, SatoriFunction>	functions;
	// 特殊形式の名前（KakkoSectionで（の直後と比べる）
	std::vector<const std::pair<const wstring, SatoriFunction>*>	specials;
};

#endif // SATORI_FUNCTION_TABLE_H
