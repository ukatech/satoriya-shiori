//---------------------------------------------------------------------------
//	静的検査（lint）
//
//	辞書を実行せずに、書き方の誤りを調べる。
//	文法を別に書き直さず、実行と同じ処理を使う：
//	・前処理と＊／＠への分割は satori_load_dict.cpp の pre_process / lines_to_units
//	・（）の読み方は KakkoSection（satori_Kakko.cpp / satori_tool.cpp）と同じ順
//	・名前の探し方は CallReal と同じ順（SAORI・内蔵関数 → 単語群 → 文 → 変数 → 内蔵変数）
//	・内蔵関数の表は satori_builtin.cpp の function_table、内蔵変数の表は同じファイルの builtin_*
//	・式は calc
//	実行系の読み方を変えたら、ここも見直すこと。
//
//	何も実行しない。SAORIの呼び出し、変数の変更、乱数、呼び出し回数の消費をしてはいけない。
//---------------------------------------------------------------------------
#include	"satori.h"
#include	"satori_function_table.h"
#include	"satori_dict.h"
#include	"../_/Utilities.h"
#include	"../_/charset.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


// 報告するメッセージの上限（超えた分は数だけ数える）
static const size_t	LINT_MAX_MESSAGES = 1000;
// （）の入れ子をたどる深さの上限
static const int	LINT_MAX_DEPTH = 200;
// 名前の中の入れ子の（）の代わりに入れる文字（私用領域。展開後の中身は分からない）
static const wchar_t	LINT_NESTED = 0xE0FB;


struct SatoriLintContext
{
	strvec&	messages;
	int		count;			// 見つけた問題の数（messages に入りきらなかった分も数える）
	int		errors;			// そのうちエラーの数
	bool	reporting;		// 偽のときは名前などを集めるだけで、問題は報告しない
	wstring	file;			// 報告に付けるファイル名
	escaper	esc;			// φエスケープの復元用。実行中のものとは別
	int		depth;			// （）の入れ子の深さ
	bool	depth_reported;

	std::set<wstring>	talk_names;		// 辞書に書かれた＊の名前
	std::set<wstring>	word_names;		// 辞書に書かれた＠の名前
	std::set<wstring>	saori_names;	// ＠SAORI に書かれた呼び出し名
	std::set<wstring>	assigned_vars;	// 辞書で代入されている変数名（字面で分かるもの）
	bool	dynamic_writer;	// 実行時に決まる名前の変数・単語群・SAORIを作るものがある

	SatoriLintContext(strvec& i_messages)
		: messages(i_messages), count(0), errors(0), reporting(false), depth(0), depth_reported(false), dynamic_writer(false) {}

	void	report(bool is_error, int line, const wstring& text)
	{
		if ( !reporting ) {
			return;
		}
		++count;
		if ( is_error ) {
			++errors;
		}
		if ( messages.size() >= LINT_MAX_MESSAGES ) {
			return;
		}
		wstring s = file;
		s += L"(" + itos(line) + L") : ";
		s += is_error ? L"error: " : L"warning: ";
		s += text;
		messages.push_back(s);
	}
};

// 入れ子の深さを数える（途中で抜けても戻るようにデストラクタで減らす）
class LintDepthCounter
{
	int& m_depth;
public:
	LintDepthCounter(int& i_depth) : m_depth(i_depth) { ++m_depth; }
	~LintDepthCounter() { --m_depth; }
};

// 辞書ひとつ分を前処理して分けた結果
struct LintFile
{
	wstring	label;
	bool	loaded;
	bool	closed;		// pre_process が最後まで通ったか
	pre_process_report	report;
	std::vector<satori_unit>	units;

	LintFile() : loaded(false), closed(true) {}
};


// メッセージに入れる名前。長すぎるものは縮める
static wstring	quote(const wstring& name)
{
	const wstring::size_type max_length = 40;
	if ( name.size() > max_length ) {
		return	L"「" + name.substr(0, max_length) + L"…」";
	}
	return	L"「" + name + L"」";
}

// 前後の空白（半角・全角・タブ）しかないか
static bool	is_blank(const wstring& s)
{
	wstring t = s;
	erase_all(t, L"　");
	erase_all(t, L" ");
	erase_all(t, L"\t");
	return	t.empty();
}

// base を取り除き、区切りを / にした、報告用のファイル名
static wstring	lint_label(const wstring& path, const wstring& base)
{
	wstring label = path;
	if ( !base.empty() && compare_head(label, base) ) {
		label.erase(0, base.size());
	}
	for ( wstring::iterator i = label.begin() ; i != label.end() ; ++i ) {
		if ( *i == L'\\' ) {
			*i = L'/';
		}
	}
	return	label;
}

// SHIORI/3.0 の標準的なリクエストヘッダの名前か。（Reference0）などは実行時に値が入るので、未定義とは言えない。
static bool	is_request_header_name(const wstring& name)
{
	static const wchar_t* const headers[] = {
		L"ID", L"Charset", L"Sender", L"SenderType", L"SecurityLevel", L"Status", L"BaseID", L"Event",
	};
	for ( int i=0 ; i<sizeof(headers)/sizeof(headers[0]) ; ++i ) {
		if ( name == headers[i] ) {
			return	true;
		}
	}
	return	compare_head(name, L"Reference") && name.size() > 9 && aredigits(name.c_str()+9);
}


//---------------------------------------------------------------------------
//	（）と文章

// 文章中の（…）を調べる。
void	Satori::lint_text(SatoriLintContext& ctx, const wchar_t* p, int line)
{
	while ( *p != L'\0' ) {
		a_chr c = next_a_chr(p);
		if ( c == L"（" ) {
			lint_kakko(ctx, p, line);
		}
	}
}

// 式の中の（…）を調べ、式の書き方を calc で確かめる。
// （…）は実行すると値になるので、数値の 0 に置き換えて確かめる（実行時も UnKakko(for_calc) してから calc する）。
void	Satori::lint_expression(SatoriLintContext& ctx, const wchar_t* p, int line)
{
	wstring	neutral;
	bool	has_kakko = false;
	while ( *p != L'\0' ) {
		a_chr c = next_a_chr(p);
		if ( c == L"（" ) {
			has_kakko = true;
			lint_kakko(ctx, p, line);
			neutral += L'0';
		}
		else {
			neutral += c;
		}
	}

	if ( is_blank(neutral) ) {
		return;
	}

	wstring	probe = neutral;
	if ( !calc(probe) ) {
		if ( has_kakko ) {
			// 値を 0 に置き換えただけなので、実際の値では通ることもある
			ctx.report(false, line, L"計算式を確認してください（（）を数値に置き換えて検査しました）：" + quote(neutral));
		}
		else {
			ctx.report(true, line, L"計算式が正しくありません：" + quote(neutral));
		}
	}
}

// （ の直後 p から ） までを調べる。p は ） の次へ進める。
void	Satori::lint_kakko(SatoriLintContext& ctx, const wchar_t*& p, int line)
{
	if ( ctx.depth >= LINT_MAX_DEPTH ) {
		if ( !ctx.depth_reported ) {
			ctx.depth_reported = true;
			ctx.report(false, line, L"（）の入れ子が深すぎるので、これより内側は調べません");
		}
		return;
	}
	LintDepthCounter depth_counter(ctx.depth);

	// when などの特殊形式は、引数を展開せずに読む（KakkoSection と同じ）
	wstring	delimiter;
	const wchar_t*	args = NULL;
	const SatoriFunction*	special = FindSpecialFunction(p, delimiter, args);
	if ( special != NULL ) {
		strvec	arguments;
		const wchar_t*	pp = args;
		if ( !split_special_arguments(pp, delimiter, arguments) ) {
			// 実行時もここで p を進めず、続きを普通の文字として読む
			ctx.report(true, line, L"（ に対応する ） がありません");
			return;
		}
		p = pp;
		lint_special_function(ctx, *special, arguments, line);
		return;
	}

	wstring	name;
	bool	has_nested = false;
	while ( true ) {
		if ( *p == L'\0' ) {
			ctx.report(true, line, L"（ に対応する ） がありません");
			return;
		}
		a_chr c = next_a_chr(p);
		if ( c == L"）" ) {
			break;
		}
		else if ( c == L"（" ) {
			// 内側の展開結果は実行するまで分からない
			has_nested = true;
			lint_kakko(ctx, p, line);
			name += LINT_NESTED;
		}
		else {
			name += c;
		}
	}

	FunctionCall	call;
	if ( FindFunctionCall(name, false, &ctx.saori_names, call) ) {
		lint_function_call(ctx, call, name, has_nested, line);
		return;
	}
	if ( has_nested ) {
		return;	// 名前が実行時に決まる
	}
	lint_name(ctx, name, line);
}

// （名前）の名前が、実行時に見つかるものか。見つからなければ警告する。
// CallReal と同じ順に探す：SAORI・内蔵関数（呼び出し側で済み）→ 単語群 → 文 → 変数 → 内蔵変数
void	Satori::lint_name(SatoriLintContext& ctx, const wstring& name, int line)
{
	if ( name.empty() ) {
		ctx.report(false, line, L"（） の中身が空です");
		return;
	}

	wstring	plain = name;
	ctx.esc.unescape(plain);
	if ( plain != name ) {
		return;	// φでエスケープされた文字を含む。辞書の名前とは別に扱われる
	}

	if ( ctx.word_names.find(name) != ctx.word_names.end() || words.is_exist(name) ) {
		return;
	}
	if ( ctx.talk_names.find(name) != ctx.talk_names.end() || talks.is_exist(name) ) {
		return;
	}

	int	ref;
	wchar_t	first_char;
	if ( IsArrayValue(name, ref, first_char) ) {
		return;	// Ｒ０ Ｈ１ Ａ２ Ｓ３ Ｃ４ など
	}
	if ( ctx.assigned_vars.find(name) != ctx.assigned_vars.end() || variables.find(name) != variables.end() ) {
		return;
	}
	if ( IsBuiltinName(name) || is_request_header_name(name) ) {
		return;
	}

	wstring	text = quote(name) + L"は、単語群・文・変数・内蔵関数・SAORIのどれにも見つかりません。";
	if ( ctx.dynamic_writer ) {
		text += L"（実行時に名前が決まる変数や単語群を作る記述があるので、あるのかもしれません）";
	}
	text += L"（ を文字として表示するときは φ（ と書きます。";
	ctx.report(false, line, text);
}

// 内蔵関数の呼び出し（特殊形式以外）を調べる。
void	Satori::lint_function_call(SatoriLintContext& ctx, const FunctionCall& call, const wstring& name, bool has_nested, int line)
{
	if ( call.func == NULL ) {
		return;	// SAORI。引数は SAORI ごとの決まりなので調べない
	}
	const SatoriFunction& func = *call.func;
	if ( func.is_special ) {
		return;	// KakkoSection のほうで読んでいるので、ここには来ない
	}

	strvec	args;
	SplitFunctionArguments(call, args);
	const int	argc = static_cast<int>(args.size());

	// 入れ子の（）があると、展開後の区切りの数が変わるので、個数は調べない
	if ( !has_nested ) {
		if ( (func.min_args >= 0 && argc < func.min_args) || (func.max_args >= 0 && argc > func.max_args) ) {
			ctx.report(true, line, quote(call.name) + L"の引数の個数が正しくありません（" + itos(argc) + L"個）");
		}
	}

	if ( args.empty() ) {
		return;
	}
	const wstring&	arg0 = args[0];
	const bool	arg0_literal = ( arg0.find(LINT_NESTED) == wstring::npos );

	switch ( func.arg_kind ) {
	case ARG_SET:
		if ( arg0_literal ) {
			wstring key = arg0;
			ctx.esc.unescape(key);
			if ( aredigits(zen2han(key)) ) {
				ctx.report(true, line, quote(key) + L"　数字のみの変数名は扱えません");
			}
			else if ( !key.empty() ) {
				ctx.assigned_vars.insert(key);
			}
		}
		else {
			ctx.dynamic_writer = true;
		}
		break;
	case ARG_CALL_NAME:
	case ARG_LOOP_NAME:
		if ( arg0_literal ) {
			lint_name(ctx, arg0, line);
			if ( func.arg_kind == ARG_LOOP_NAME ) {
				ctx.assigned_vars.insert(arg0 + L"カウンタ");
			}
		}
		break;
	case ARG_WRITES:
		ctx.dynamic_writer = true;
		break;
	default:
		break;
	}
}

// 特殊形式（when / whenlist / times / while / for）を調べる。
void	Satori::lint_special_function(SatoriLintContext& ctx, const SatoriFunction& func, const strvec& args, int line)
{
	const int	argc = static_cast<int>(args.size());
	int	i;

	if ( (func.min_args >= 0 && argc < func.min_args) || (func.max_args >= 0 && argc > func.max_args) ) {
		ctx.report(true, line, L"引数の個数が正しくありません（" + itos(argc) + L"個）");
		// どれが式かは分からないので、全部を文章として読む
		for ( i=0 ; i<argc ; ++i ) {
			lint_text(ctx, args[i].c_str(), line);
		}
		return;
	}

	switch ( func.arg_kind ) {
	case ARG_WHEN:
		// 先頭が式、残りは本文
		lint_expression(ctx, args[0].c_str(), line);
		for ( i=1 ; i<argc ; ++i ) {
			lint_text(ctx, args[i].c_str(), line);
		}
		break;
	case ARG_LOOP:
		// 最後が本文、それ以外は式
		for ( i=0 ; i<argc-1 ; ++i ) {
			lint_expression(ctx, args[i].c_str(), line);
		}
		lint_text(ctx, args[argc-1].c_str(), line);
		break;
	default:
		// ARG_WHENLIST：比べる値と、（式の続き、結果）の組。式の続きは前に比べる値が付くので、式としては調べない
		for ( i=0 ; i<argc ; ++i ) {
			lint_text(ctx, args[i].c_str(), line);
		}
		break;
	}
}


//---------------------------------------------------------------------------
//	＊の本文の行

// 本文の1行を調べる。SentenceToSakuraScriptInternal が行の頭で分ける順（＿、＞≫≧、＄、本文）と同じ。
void	Satori::lint_body_line(SatoriLintContext& ctx, const wchar_t* p, int line, bool is_first_line)
{
	if ( is_first_line && *p == L'→' ) {
		p += 1;	// 相手のゴーストの指定
	}

	// 選択肢	＿ラベル	ジャンプ先
	if ( *p == L'＿' ) {
		const size_t	len = wcslen(p);
		if ( len <= 1 ) {
			ctx.report(false, line, L"選択肢記法の後に文字列が存在しないので、無視されます");
			return;
		}
		if ( len > 1023 ) {
			ctx.report(false, line, L"選択肢記法が長すぎるので、無視されます");
			return;
		}
		const wstring	choice_line(p+1);
		const wstring::size_type	tab_pos = choice_line.find(L'\t');
		wstring	target;	// 選ばれたとき実行される文の名前（OnChoiceSelect は ID をそのまま文の名前として呼ぶ）
		if ( tab_pos == wstring::npos ) {
			lint_text(ctx, choice_line.c_str(), line);
			target = choice_line;
		}
		else {
			lint_text(ctx, choice_line.substr(0, tab_pos).c_str(), line);
			const wstring::size_type	start = choice_line.find_first_not_of(L'\t', tab_pos);
			if ( start != wstring::npos ) {
				const wstring::size_type	end = choice_line.find(L'\t', start);
				target = choice_line.substr(start, (end == wstring::npos) ? wstring::npos : end-start);
			}
			lint_text(ctx, choice_line.c_str() + tab_pos + 1, line);
		}

		// 飛び先が字面で決まるなら、その文があるか。URLやイベント名（On…）、script: なども使われる
		if ( !target.empty() && target.find(L'（') == wstring::npos && ctx.reporting ) {
			wstring	id = target;
			ctx.esc.unescape(id);
			const bool	is_special_id = compare_head(id, L"On") || compare_head(id, L"http") || id.find(L':') != wstring::npos;
			const bool	has_handler = ctx.talk_names.find(L"OnChoiceSelect") != ctx.talk_names.end() || talks.is_exist(L"OnChoiceSelect");
			if ( !is_special_id && !has_handler
				&& ctx.talk_names.find(id) == ctx.talk_names.end() && !talks.is_exist(id) ) {
				ctx.report(false, line, L"選択肢の飛び先" + quote(id) + L"という＊の文が見つかりません");
			}
		}
		return;
	}

	// ジャンプ	＞飛び先	条件
	if ( *p == L'＞' || *p == L'≫' || *p == L'≧' ) {
		strvec	words;
		split(p+1, L"\t", words, 2);

		if ( words.size() >= 2 ) {
			if ( is_blank(words[1]) ) {
				ctx.report(false, line, L"ジャンプの条件式が空なので、無視されます");
			}
			else {
				lint_expression(ctx, words[1].c_str(), line);
			}
		}
		if ( words.size() >= 1 ) {
			lint_text(ctx, words[0].c_str(), line);

			// ＞だけは飛び先が文の名前。≫≧は検索なので調べない。空は「ランダムトーク」、OnTalk は予約トークがあるので存在しなくてよい
			if ( *p == L'＞' && words[0].find(L'（') == wstring::npos && ctx.reporting ) {
				wstring	target = words[0];
				ctx.esc.unescape(target);
				if ( !target.empty() && target != L"OnTalk"
					&& ctx.talk_names.find(target) == ctx.talk_names.end() && !talks.is_exist(target) ) {
					ctx.report(true, line, L"ジャンプ先" + quote(target) + L"という＊の文がありません");
				}
			}
		}
		return;
	}

	// 変数の設定	＄名前	値　か　＄名前＝式
	if ( *p == L'＄' ) {
		const wchar_t*	key_start = p + 1;
		const wchar_t*	separator = NULL;
		bool	do_calc = false;
		if ( !split_variable_assignment(key_start, separator, do_calc) ) {
			ctx.report(true, line, L"＄による変数代入文には、タブによる区切りか、＝による計算式が必要です");
			return;
		}

		const wstring	key_text(key_start, separator - key_start);
		lint_text(ctx, key_text.c_str(), line);
		if ( key_text.find(L'（') != wstring::npos ) {
			ctx.dynamic_writer = true;	// 変数名が実行時に決まる
		}
		else {
			wstring	key = key_text;
			ctx.esc.unescape(key);
			if ( key.empty() ) {
				// ＄そのまま表示
			}
			else if ( aredigits(zen2han(key)) ) {
				ctx.report(true, line, quote(key) + L"　数字のみの変数名は扱えません");
			}
			else {
				ctx.assigned_vars.insert(key);
				if ( ctx.word_names.find(key) != ctx.word_names.end() || ctx.talk_names.find(key) != ctx.talk_names.end() ) {
					ctx.report(false, line, L"変数" + quote(key) + L"と同じ名前の単語群か文があります。トラブルの元なので避けましょう");
				}
			}
		}

		const wchar_t*	value = separator + 1;
		if ( do_calc ) {
			lint_expression(ctx, value, line);
		}
		else {
			lint_text(ctx, value, line);
		}
		return;
	}

	// 全角にするつもりだった行頭の記号
	if ( ctx.reporting && (*p == L'*' || *p == L'@' || *p == L'$') ) {
		ctx.report(false, line, wstring(L"行頭が半角の「") + *p + L"」です。この行は文の本文として扱われます（全角の「＊」「＠」「＄」にするつもりでは？）");
	}

	lint_text(ctx, p, line);
}

// ＊か＠ひとつ分を調べる。
void	Satori::lint_unit(SatoriLintContext& ctx, const satori_unit& unit)
{
	const bool	is_talk = ( unit.typemark == L"＊" );

	wstring	name = unit.name;
	ctx.esc.unescape(name);

	// 名前や条件の中の（）
	lint_text(ctx, unit.name.c_str(), unit.line);
	if ( !unit.condition.empty() ) {
		if ( is_blank(unit.condition) ) {
			// 空の条件は常に真として扱われる
		}
		else {
			lint_expression(ctx, unit.condition.c_str(), unit.line);
		}
	}

	if ( !is_talk && !name.empty() && ctx.talk_names.find(name) != ctx.talk_names.end() ) {
		ctx.report(false, unit.line, L"同じ名前" + quote(name) + L"の単語群と文があります。トラブルの元なので避けましょう");
	}

	for ( size_t i=0 ; i<unit.body.size() ; ++i ) {
		const int	line = ( i < unit.body_lines.size() ) ? unit.body_lines[i] : unit.line;
		if ( is_talk ) {
			lint_body_line(ctx, unit.body[i].c_str(), line, i == 0);
		}
		else {
			lint_text(ctx, unit.body[i].c_str(), line);
		}
	}
}


//---------------------------------------------------------------------------
//	全体

int	Satori::Lint(strvec& oMessages, int& oErrors)
{
	SatoriLintContext	ctx(oMessages);

	static std::vector<wstring> typemarks;
	if ( typemarks.empty() )
	{
		typemarks.push_back(L"＊");
		typemarks.push_back(L"＠");
	}

	// 読み込んだときと同じ手順で、辞書を読み直して分ける
	std::vector<LintFile>	files(m_dic_files.size());
	for ( size_t f=0 ; f<m_dic_files.size() ; ++f ) {
		LintFile&	file = files[f];
		file.label = lint_label(m_dic_files[f].first, mBaseFolder);

		strvec	raw;
		if ( !select_dict_and_load_to_vector(m_dic_files[f].first, raw, false, m_dic_files[f].second ? CS_UTF8 : CS_NULL) ) {
			continue;
		}
		file.loaded = true;

		strvec	preprocessed;
		file.closed = pre_process(raw, preprocessed, ctx.esc, replace_before_dic, &file.report);
		lines_to_units(preprocessed, typemarks, L"\t", file.units, &file.report.line_numbers);
	}

	// 名前を集める：＊と＠、＠SAORI の呼び出し名
	for ( size_t f=0 ; f<files.size() ; ++f ) {
		for ( size_t u=0 ; u<files[f].units.size() ; ++u ) {
			const satori_unit&	unit = files[f].units[u];
			wstring	name = unit.name;
			ctx.esc.unescape(name);
			if ( unit.typemark == L"＊" ) {
				ctx.talk_names.insert(name);
			}
			else {
				ctx.word_names.insert(name);
				if ( name == L"SAORI" ) {
					for ( size_t i=0 ; i<unit.body.size() ; ++i ) {
						strvec	fields;
						split(unit.body[i], L",", fields);
						if ( !fields.empty() && !fields[0].empty() ) {
							ctx.saori_names.insert(fields[0]);
						}
					}
				}
			}
		}
	}

	// 1回目は変数への代入などを集めるだけ。2回目に、集めたものを使って報告する。
	for ( int pass=0 ; pass<2 ; ++pass ) {
		ctx.reporting = ( pass == 1 );
		for ( size_t f=0 ; f<files.size() ; ++f ) {
			const LintFile&	file = files[f];
			ctx.file = file.label;
			ctx.depth = 0;
			ctx.depth_reported = false;

			if ( !file.loaded ) {
				ctx.report(true, 0, L"辞書を読み込めませんでした");
				continue;
			}

			if ( !file.closed ) {
				ctx.report(true, file.report.unclosed_line, L"（ が閉じられないままファイルが終わっています。この行以降が読み込まれません");
			}
			for ( size_t s=0 ; s<file.report.suspect_lines.size() ; ++s ) {
				ctx.report(false, file.report.suspect_lines[s], L"（ が閉じられていない可能性があります（続きの行が＊か＠で始まっています）");
			}
			if ( file.report.dangling_escape_line > 0 ) {
				ctx.report(false, file.report.dangling_escape_line, L"ファイルの最後がφで終わっています");
			}

			for ( size_t u=0 ; u<file.units.size() ; ++u ) {
				lint_unit(ctx, file.units[u]);
			}
		}
	}

	oErrors = ctx.errors;
	return	ctx.count;
}
