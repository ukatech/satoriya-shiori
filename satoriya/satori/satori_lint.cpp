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
// 上限のうち、error のために空けておく件数
static const size_t	LINT_ERROR_RESERVE = 200;
// そのうち notice の上限（件数が多くなりやすいので、error や warning を押し出さないように別に抑える）
static const size_t	LINT_MAX_NOTICES = 100;
// （）の入れ子をたどる深さの上限
static const int	LINT_MAX_DEPTH = 200;
// 名前の中の入れ子の（）の代わりに入れる文字（私用領域。展開後の中身は分からない）
static const wchar_t	LINT_NESTED = 0xE0FB;


// 問題の重さ。ErrorLevel の値（error / warning / notice）になる。
enum LintLevel
{
	LINT_NOTICE,
	LINT_WARNING,
	LINT_ERROR
};

// （）や＊の本文が実行すると、変数や単語を書き換える処理。
// if / unless / choice などは、選ばれなかった枝の（）も先に展開するので、枝にこれがあると両方の枝で実行される。
struct SatoriLintEffect
{
	bool	direct;				// 書き換える処理が必ず実行される
	wstring	what;				// 最初に見つけた処理（メッセージ用）
	std::set<wstring>	calls;	// 呼んでいる（名前）。呼び先の＊に処理があれば、それも実行される

	SatoriLintEffect() : direct(false) {}

	void	add_direct(const wstring& i_what)
	{
		if ( !direct ) {
			direct = true;
			what = i_what;
		}
	}
	void	merge(const SatoriLintEffect& other)
	{
		if ( other.direct ) {
			add_direct(other.what);
		}
		for ( std::set<wstring>::const_iterator i = other.calls.begin() ; i != other.calls.end() ; ++i ) {
			calls.insert(*i);
		}
	}
	// 中身を入れ替える（std::set の swap は定数時間）
	void	swap(SatoriLintEffect& other)
	{
		std::swap(direct, other.direct);
		what.swap(other.what);
		calls.swap(other.calls);
	}
};

struct SatoriLintContext
{
	strpairvec&	messages;	// （ErrorLevel の値、本文）
	int		count;			// 見つけた問題の数（messages に入りきらなかった分も数える）
	int		errors;			// そのうちエラーの数
	int		stored_errors;	// messages に入ったエラーの数
	int		stored_notices;	// messages に入った notice の数
	bool	reporting;		// 偽のときは名前などを集めるだけで、問題は報告しない
	wstring	file;			// 報告に付けるファイル名
	escaper	esc;			// φエスケープの復元用。実行中のものとは別
	int		depth;			// （）の入れ子の深さ
	bool	depth_reported;

	std::set<wstring>	talk_names;		// 辞書に書かれた＊の名前
	std::map<wstring, wstring>	talk_normalized;	// 空白・全角半角・大文字小文字を無くした＊の名前 → 元の名前
	std::set<wstring>	word_names;		// 辞書に書かれた＠の名前
	std::set<wstring>	saori_names;	// ＠SAORI に書かれた呼び出し名
	std::set<wstring>	assigned_vars;	// 辞書で代入されている変数名（字面で分かるもの）
	bool	dynamic_writer;	// 実行時に決まる名前の変数・単語群・SAORIを作るものがある

	SatoriLintEffect	effect;			// いま読んでいる（）や＊の中で実行される処理（入れ子の中も含む）
	SatoriLintEffect	last_effect;	// 直前に読み終えた（）のもの
	std::map<wstring, SatoriLintEffect>	talk_effects;	// ＊の名前 → 本文で実行される処理（1回目に集める）
	std::set<wstring>	effectful_talks;	// 呼ぶと処理が実行される＊の名前（talk_effects から求める）

	SatoriLintContext(strpairvec& i_messages)
		: messages(i_messages), count(0), errors(0), stored_errors(0), stored_notices(0), reporting(false), depth(0), depth_reported(false), dynamic_writer(false) {}

	void	add(LintLevel level, int line, const wstring& text)
	{
		if ( !reporting ) {
			return;
		}
		++count;
		if ( level == LINT_ERROR ) {
			++errors;
		}
		// error は最後まで入るように、それ以外は error の分の余地を残して打ち切る
		const size_t	limit = ( level == LINT_ERROR ) ? LINT_MAX_MESSAGES : LINT_MAX_MESSAGES - LINT_ERROR_RESERVE;
		if ( messages.size() >= limit ) {
			return;
		}
		if ( level == LINT_NOTICE && stored_notices >= static_cast<int>(LINT_MAX_NOTICES) ) {
			return;
		}

		const wchar_t*	label = L"warning";
		if ( level == LINT_ERROR ) {
			label = L"error";
			++stored_errors;
		}
		else if ( level == LINT_NOTICE ) {
			label = L"notice";
			++stored_notices;
		}
		wstring s = file;
		s += L"(" + itos(line) + L") : ";
		s += label;
		s += L": ";
		s += text;
		messages.push_back(strpair(label, s));
	}

	void	report(bool is_error, int line, const wstring& text)
	{
		add(is_error ? LINT_ERROR : LINT_WARNING, line, text);
	}

	void	notice(int line, const wstring& text)
	{
		add(LINT_NOTICE, line, text);
	}

	// talk_effects から、呼ぶと処理が実行される＊を求める（呼び先をたどる）。
	// 同じ名前の＠があるときは（名前）が単語群として読まれ、どの単語が選ばれるか分からないので数えない。
	void	resolve_effects()
	{
		effectful_talks.clear();
		std::map<wstring, SatoriLintEffect>::const_iterator t;
		for ( t = talk_effects.begin() ; t != talk_effects.end() ; ++t ) {
			if ( t->second.direct && word_names.find(t->first) == word_names.end() ) {
				effectful_talks.insert(t->first);
			}
		}
		bool	changed = true;
		while ( changed ) {
			changed = false;
			for ( t = talk_effects.begin() ; t != talk_effects.end() ; ++t ) {
				if ( effectful_talks.find(t->first) != effectful_talks.end() || word_names.find(t->first) != word_names.end() ) {
					continue;
				}
				for ( std::set<wstring>::const_iterator c = t->second.calls.begin() ; c != t->second.calls.end() ; ++c ) {
					if ( effectful_talks.find(*c) != effectful_talks.end() ) {
						effectful_talks.insert(t->first);
						changed = true;
						break;
					}
				}
			}
		}
	}

	// e が実行されると処理が起きるか。起きるなら what に何かを入れる。
	bool	has_effect(const SatoriLintEffect& e, wstring& what) const
	{
		if ( e.direct ) {
			what = e.what;
			return	true;
		}
		for ( std::set<wstring>::const_iterator c = e.calls.begin() ; c != e.calls.end() ; ++c ) {
			if ( effectful_talks.find(*c) != effectful_talks.end() ) {
				what = L"（" + *c + L"）の中の処理";
				return	true;
			}
		}
		return	false;
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

// φエスケープを元に戻す。escaper::unescape は記録した全部の置き換えを順に試すので、印が無い文字列では呼ばない。
static void	lint_unescape(escaper& esc, wstring& s)
{
	if ( s.find(static_cast<wchar_t>(0xE09E)) != wstring::npos ) {
		esc.unescape(s);
	}
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


// 半角・全角の空白か
static bool	is_space_char(wchar_t c)
{
	return	c == L' ' || c == L'　';
}

static bool	is_digit_char(wchar_t c)
{
	return	(c >= L'0' && c <= L'9') || (c >= L'０' && c <= L'９');
}

// 変数に代入される名前を調べる。＄の行と（set、名前、…）で共通。key はφエスケープを戻したもの。
// from_set は（set、名前、…）の場合。名前の衝突は、＄の行より数が多くなりやすい（生成した名前に set する辞書など）ので notice にする。
static void	lint_assigned_name(SatoriLintContext& ctx, const wstring& key, int line, bool from_set = false)
{
	if ( key.empty() ) {
		return;	// ＄そのまま表示
	}
	if ( aredigits(zen2han(key)) ) {
		ctx.report(true, line, quote(key) + L"　数字のみの変数名は扱えません");
		return;
	}
	ctx.assigned_vars.insert(key);
	// 空白だけの名前（＄　【タブ】 で空文字列の定数にする書き方）は意図したものとして扱う
	if ( (is_space_char(key[0]) || is_space_char(key[key.size()-1])) && !is_blank(key) ) {
		ctx.report(false, line, L"変数名" + quote(key) + L"の前か後ろに空白があります。空白も含めた名前で登録されるので、空白なしの名前では読めません（＝ やタブの手前の空白も名前に含まれます）");
	}
	if ( ctx.word_names.find(key) != ctx.word_names.end() || ctx.talk_names.find(key) != ctx.talk_names.end() ) {
		const wstring	text = L"変数" + quote(key) + L"と同じ名前の単語群か文があります。トラブルの元なので避けましょう";
		if ( from_set ) {
			ctx.notice(line, text);
		}
		else {
			ctx.report(false, line, text);
		}
	}
}

// タブ区切りの代入の値に、計算のつもりの演算子があるか。
// （）の外の演算子で、片側が数、反対側が（）のとき（（カウンタ）＋１ や １＋（カウンタ） の形）に限る。
static bool	has_calc_operator(const wchar_t* val)
{
	static const wchar_t	operators[] = { L'+', L'-', L'*', L'/', L'%', L'＋', L'－', L'×', L'÷', L'％', 0x2212, 0 };
	const size_t	n = wcslen(val);
	int	depth = 0;
	for ( size_t i=0 ; i<n ; ++i ) {
		const wchar_t	c = val[i];
		if ( c == L'（' ) {
			++depth;
			continue;
		}
		if ( c == L'）' ) {
			if ( depth > 0 ) {
				--depth;
			}
			continue;
		}
		if ( depth > 0 || wcschr(operators, c) == NULL ) {
			continue;
		}
		size_t	l = i;	// 演算子の手前の、空白を除いた終わり（この手前の文字を見る）
		while ( l > 0 && is_space_char(val[l-1]) ) {
			--l;
		}
		size_t	r = i + 1;
		while ( r < n && is_space_char(val[r]) ) {
			++r;
		}
		if ( l == 0 || r >= n ) {
			continue;
		}
		const wchar_t	prev = val[l-1];
		const wchar_t	next = val[r];
		if ( (prev == L'）' && is_digit_char(next)) || (is_digit_char(prev) && next == L'（') ) {
			return	true;
		}
	}
	return	false;
}


//---------------------------------------------------------------------------
//	（）と文章

// escaper がエスケープした文字の頭に付ける印（satori.cpp の escaper::sm_escape_code）
static wchar_t	escaper_mark()
{
	return	static_cast<wchar_t>(0xE09E);
}

// 直前の文字が、「１）」「a）」のような箇条書きの番号か。
static bool	is_list_marker_char(wchar_t c)
{
	return	(c >= L'0' && c <= L'9') || (c >= L'０' && c <= L'９')
		|| (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z')
		|| (c >= L'ａ' && c <= L'ｚ') || (c >= L'Ａ' && c <= L'Ｚ');
}

// 文章中の（…）を調べる。
void	Satori::lint_text(SatoriLintContext& ctx, const wchar_t* p, int line)
{
	wchar_t	previous = L'\0';
	int		escaped_open = 0;	// φ（ で表示するつもりの （ の数。対になる ） は報告しない
	while ( *p != L'\0' ) {
		const wchar_t	head = *p;
		a_chr c = next_a_chr(p);
		if ( c == L"（" ) {
			lint_kakko(ctx, p, line);
			previous = L'）';
			continue;
		}
		if ( head == escaper_mark() && ctx.reporting ) {
			// φでエスケープされた文字は「印+番号+空白」になっている。元が （ なら数える
			wstring	token(1, head);
			const wchar_t*	q = p;
			while ( *q >= L'0' && *q <= L'9' ) {
				token += *q++;
			}
			if ( *q == L' ' ) {
				token += *q;
			}
			lint_unescape(ctx.esc, token);
			if ( token == L"（" ) {
				++escaped_open;
			}
		}
		else if ( c == L"）" && ctx.reporting && !is_list_marker_char(previous) ) {
			if ( escaped_open > 0 ) {
				--escaped_open;
			}
			else {
				// 実行時は対応する （ が無い ） をそのまま表示する
				ctx.notice(line, L"対応する （ が無い ） があります。文字としてそのまま表示されます");
			}
		}
		previous = head;
	}
}

// 式の中の（…）を調べ、式の書き方を calc で確かめる。
// （…）は実行すると値になるので、数値の 1 に置き換えて確かめる（実行時も UnKakko(for_calc) してから calc する）。
void	Satori::lint_expression(SatoriLintContext& ctx, const wchar_t* p, int line)
{
	wstring	neutral;
	bool	has_kakko = false;
	while ( *p != L'\0' ) {
		a_chr c = next_a_chr(p);
		if ( c == L"（" ) {
			has_kakko = true;
			lint_kakko(ctx, p, line);
			neutral += L'1';	// 0 だと 割り算や剰余が 0 で割ることになって、誤って失敗する
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
			// 値を 1 に置き換えただけなので、実際の値では通ることもある
			ctx.report(false, line, L"計算式を確認してください（（）を数値に置き換えて検査しました）：" + quote(neutral));
		}
		else {
			ctx.report(true, line, L"計算式が正しくありません：" + quote(neutral));
		}
	}
}

// （ の直後 p から ） までを調べる。p は ） の次へ進める。
// この（）の中（入れ子を含む）で実行される処理を ctx.last_effect に残し、外側の ctx.effect に足す。
void	Satori::lint_kakko(SatoriLintContext& ctx, const wchar_t*& p, int line)
{
	SatoriLintEffect	outer;
	outer.swap(ctx.effect);
	lint_kakko_inner(ctx, p, line);
	ctx.last_effect = ctx.effect;
	outer.merge(ctx.effect);
	ctx.effect.swap(outer);
}

void	Satori::lint_kakko_inner(SatoriLintContext& ctx, const wchar_t*& p, int line)
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
		// 特殊形式の引数は、条件によって実行されないことがある。処理があるとは言えないので数えない
		ctx.effect = SatoriLintEffect();
		return;
	}

	wstring	name;
	std::vector<SatoriLintEffect>	nested;	// 名前の中の入れ子の（）ごとの処理（LINT_NESTED と同じ順）
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
			lint_kakko(ctx, p, line);
			nested.push_back(ctx.last_effect);
			name += LINT_NESTED;
		}
		else {
			name += c;
		}
	}

	FunctionCall	call;
	if ( FindFunctionCall(name, false, &ctx.saori_names, call) ) {
		lint_function_call(ctx, call, name, nested, line);
		return;
	}
	if ( !nested.empty() ) {
		return;	// 名前が実行時に決まる
	}
	lint_name(ctx, name, line);
}

// （名前）の名前が、実行時に見つかるものか。見つからなければ警告する。
// CallReal と同じ順に探す：SAORI・内蔵関数（呼び出し側で済み）→ 単語群 → 文 → 変数 → 内蔵変数
void	Satori::lint_name(SatoriLintContext& ctx, const wstring& name, int line)
{
	if ( name.empty() ) {
		// 名前の無い ＊ ＠ があれば、（） で呼べる（ランダムトークなど）
		if ( ctx.talk_names.find(name) == ctx.talk_names.end() && ctx.word_names.find(name) == ctx.word_names.end()
			&& !talks.is_exist(name) && !words.is_exist(name) ) {
			ctx.report(false, line, L"（） の中身が空で、名前の無い＊も＠もありません");
		}
		return;
	}

	wstring	plain = name;
	lint_unescape(ctx.esc, plain);
	if ( plain != name ) {
		return;	// φでエスケープされた文字を含む。辞書の名前とは別に扱われる
	}

	// ＊を呼ぶと、その本文の処理が実行される（呼び先に処理があるかは、全部読んでから調べる）
	ctx.effect.calls.insert(name);

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
	if ( compare_head(name, L"乱数") && name.size() >= 4 ) {
		// 乱数m～n。実行時は、区切りが 1 つでないと「乱数の指定が変です」と表示する（var_random）
		strvec	vec;
		if ( split(name.c_str() + 2, L"\xFF5E\x301C", vec) != 2 ) {
			ctx.report(false, line, quote(name) + L"は、乱数の指定が変です。「（乱数1～6）」のように、～を 1 つだけ書きます");
			return;
		}
	}
	if ( IsBuiltinName(name) || is_request_header_name(name) ) {
		return;
	}

	wstring	text = quote(name) + L"は、単語群・文・変数・内蔵関数・SAORIのどれにも見つかりません。";
	if ( ctx.dynamic_writer ) {
		// 実行時に名前が決まる変数などを作る記述があると、辞書だけでは無いと言い切れないので、重さを下げる
		text += L"（実行時に名前が決まる変数や単語群を作る記述があるので、あるのかもしれません）";
		text += L"（ を文字として表示するときは φ（ と書きます。";
		ctx.notice(line, text);
		return;
	}
	text += L"（ を文字として表示するときは φ（ と書きます。";
	ctx.report(false, line, text);
}

// 内蔵関数の呼び出し（特殊形式以外）を調べる。
void	Satori::lint_function_call(SatoriLintContext& ctx, const FunctionCall& call, const wstring& name, const std::vector<SatoriLintEffect>& nested, int line)
{
	if ( call.func == NULL ) {
		// SAORI。引数は SAORI ごとの決まりなので調べないが、if などの枝は調べる
		lint_eager_branches(ctx, call, nested, line);
		return;
	}
	const SatoriFunction& func = *call.func;
	if ( func.is_special ) {
		return;	// KakkoSection のほうで読んでいるので、ここには来ない
	}
	const bool	has_nested = !nested.empty();

	// 変数や単語を書き換える関数
	static const wchar_t* const writers[] = {
		L"set", L"変数の一括削除", L"変数の一括コピー", L"split_to", L"単語の追加", L"追加単語の削除", L"追加単語の全削除",
	};
	for ( int w=0 ; w<sizeof(writers)/sizeof(writers[0]) ; ++w ) {
		if ( call.name == writers[w] ) {
			ctx.effect.add_direct(L"（" + call.name + L"）");
			break;
		}
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
			lint_unescape(ctx.esc, key);
			lint_assigned_name(ctx, key, line, true);
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

// if / unless / iflist / switch / nswitch / choice（ssu）の枝を調べる。
// これらは特殊形式ではないので、引数の（）は選ぶ前に全部展開される。枝に処理があると、選ばれなかった枝でも実行される。
void	Satori::lint_eager_branches(SatoriLintContext& ctx, const FunctionCall& call, const std::vector<SatoriLintEffect>& nested, int line)
{
	if ( !ctx.reporting || nested.empty() ) {
		return;
	}

	// 枝になる引数は first から step おき。iflist / switch は（比べる値、結果）の組で、余りがあれば最後が else
	const wstring&	fn = call.name;
	size_t	first, step;
	bool	trailing_else = false;
	if ( fn == L"if" || fn == L"unless" || fn == L"nswitch" ) {
		first = 1;
		step = 1;
	}
	else if ( fn == L"choice" ) {
		first = 0;
		step = 1;
	}
	else if ( fn == L"iflist" || fn == L"switch" ) {
		first = 2;
		step = 2;
		trailing_else = true;
	}
	else {
		return;
	}

	strvec	args;
	SplitFunctionArguments(call, args);
	const size_t	argc = args.size();

	// 各引数の中の入れ子の（）が、nested の何番目から何個か
	size_t	total = 0;
	std::vector<size_t>	counts(argc);
	for ( size_t i=0 ; i<argc ; ++i ) {
		counts[i] = 0;
		for ( wstring::size_type c=0 ; c<args[i].size() ; ++c ) {
			if ( args[i][c] == LINT_NESTED ) {
				++counts[i];
			}
		}
		total += counts[i];
	}
	if ( total > nested.size() ) {
		return;
	}
	size_t	next_index = nested.size() - total;	// 名前の側にあった分（普通は 0）を飛ばす

	for ( size_t i=0 ; i<argc ; ++i ) {
		const size_t	start = next_index;
		next_index += counts[i];

		const bool	is_branch = ( i >= first && (i - first) % step == 0 ) || ( trailing_else && argc % 2 == 0 && i == argc - 1 && i >= first );
		if ( !is_branch || counts[i] == 0 ) {
			continue;
		}
		SatoriLintEffect	branch;
		for ( size_t k=start ; k<start+counts[i] ; ++k ) {
			branch.merge(nested[k]);
		}
		wstring	what;
		if ( ctx.has_effect(branch, what) ) {
			ctx.report(false, line, quote(fn) + L"は引数を先に展開するので、選ばれなかった枝も実行されます。枝に" + what
				+ L"があります。片方だけ実行したいときは、when か whenlist を使ってください");
			return;	// 1 つの（）につき 1 件
		}
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

// 比べるための名前：空白と全角半角と大文字小文字の違いを無くす。
static wstring	normalize_name(const wstring& name)
{
	wstring s = zen2han(name);
	erase_all(s, L" ");
	erase_all(s, L"\t");
	erase_all(s, L"　");
	for ( wstring::iterator i = s.begin() ; i != s.end() ; ++i ) {
		if ( *i >= L'A' && *i <= L'Z' ) {
			*i = static_cast<wchar_t>(*i - L'A' + L'a');
		}
	}
	return	s;
}

// 飛び先の＊の文が無いときの報告。
// 飛び先の無い＞や選択肢は、実行しても続きが実行されるだけで、任意の飛び先として使われることも多い（ひな形が用意するフックなど）。
// そのため、書き間違いらしいとき（空白・全角半角・大文字小文字だけが違う文がある）だけ warning にして、ほかは notice にする。
void	Satori::lint_missing_target(SatoriLintContext& ctx, const wchar_t* what, const wstring& id, int line)
{
	std::map<wstring, wstring>::const_iterator near_name = ctx.talk_normalized.find(normalize_name(id));
	if ( near_name != ctx.talk_normalized.end() ) {
		ctx.report(false, line, wstring(what) + quote(id) + L"という＊の文はありませんが、空白や全角半角・大文字小文字だけが違う" + quote(near_name->second) + L"があります");
		return;
	}
	ctx.notice(line, wstring(what) + quote(id) + L"という＊の文が見つかりません。任意の飛び先として使っているのでなければ、名前を確認してください");
}

// 選択肢の飛び先が、字面で決まるなら、その文があるか。URLやイベント名（On…）、script: なども使われる。
// ＿ラベル【タブ】ID と、さくらスクリプトの \q[ラベル,ID] の両方で使う。
void	Satori::lint_choice_target(SatoriLintContext& ctx, const wstring& target, int line)
{
	if ( target.empty() || target.find(L'（') != wstring::npos || !ctx.reporting ) {
		return;
	}
	wstring	id = target;
	lint_unescape(ctx.esc, id);
	const bool	is_special_id = compare_head(id, L"On") || compare_head(id, L"http") || id.find(L':') != wstring::npos;
	const bool	has_handler = ctx.talk_names.find(L"OnChoiceSelect") != ctx.talk_names.end() || talks.is_exist(L"OnChoiceSelect");
	if ( !is_special_id && !has_handler
		&& ctx.talk_names.find(id) == ctx.talk_names.end() && !talks.is_exist(id) ) {
		lint_missing_target(ctx, L"選択肢の飛び先", id, line);
	}
}

// 文字列中の \q[ラベル,ID,…] と \__q[ID,…] から ID を集める。
// 括弧の中の \] は閉じ括弧ではなく、"…" で括った中の , と ] は区切りにならない。
static void	find_choice_targets(const wchar_t* p, strvec& o_targets)
{
	while ( *p != L'\0' ) {
		if ( *p != L'\\' ) {
			++p;
			continue;
		}
		++p;
		if ( *p == L'\\' ) {
			++p;	// \\ はバックスラッシュそのもの
			continue;
		}

		size_t	target_index;
		if ( p[0] == L'q' && p[1] == L'[' ) {
			p += 2;
			target_index = 1;	// \q[ラベル,ID,…]
		}
		else if ( p[0] == L'_' && p[1] == L'_' && p[2] == L'q' && p[3] == L'[' ) {
			p += 4;
			target_index = 0;	// \__q[ID,…]
		}
		else {
			continue;
		}

		strvec	fields(1);
		bool	in_quote = false;
		bool	closed = false;
		while ( *p != L'\0' ) {
			const wchar_t	c = *p;
			if ( c == L'\\' && (p[1] == L']' || p[1] == L'\\') ) {
				fields.back() += p[1];
				p += 2;
			}
			else if ( c == L'"' ) {
				in_quote = !in_quote;
				++p;
			}
			else if ( !in_quote && c == L',' ) {
				fields.push_back(wstring());
				++p;
			}
			else if ( !in_quote && c == L']' ) {
				closed = true;
				++p;
				break;
			}
			else {
				fields.back() += c;
				++p;
			}
		}
		if ( closed && fields.size() > target_index ) {
			o_targets.push_back(fields[target_index]);
		}
	}
}

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

		lint_choice_target(ctx, target, line);
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
				lint_unescape(ctx.esc, target);
				if ( !target.empty() && target != L"OnTalk"
					&& ctx.talk_names.find(target) == ctx.talk_names.end() && !talks.is_exist(target) ) {
					lint_missing_target(ctx, L"ジャンプ先", target, line);
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
			wstring	text = L"＄による変数代入文には、タブによる区切りか、＝による計算式が必要です";
			if ( wcsstr(p, L"【タブ】") != NULL ) {
				text += L"（「【タブ】」という文字がそのまま書かれています。replace.txt に【タブ】をタブ文字に置き換える行を書くか、タブ文字にしてください）";
			}
			ctx.report(true, line, text);
			return;
		}

		const wstring	key_text(key_start, separator - key_start);
		lint_text(ctx, key_text.c_str(), line);
		if ( key_text.find(L'（') != wstring::npos ) {
			ctx.dynamic_writer = true;	// 変数名が実行時に決まる
		}
		else {
			wstring	key = key_text;
			lint_unescape(ctx.esc, key);
			lint_assigned_name(ctx, key, line);
		}

		const wchar_t*	value = separator + 1;
		if ( do_calc ) {
			lint_expression(ctx, value, line);
		}
		else {
			lint_text(ctx, value, line);
			// 自分自身に演算子をつないでいる形（＄回数【タブ】（回数）－１）。計算した数ではなく式の文字列が入るので、
			// 実行のたびに伸びていく。式のまま計算に使えば動くため、そうしている辞書も多く、notice にとどめる。
			// 自分自身を含まない形は、表示用の文字（[（x）/3] など）と区別できないので調べない。
			if ( has_calc_operator(value) ) {
				wstring	key = key_text;
				lint_unescape(ctx.esc, key);
				while ( !key.empty() && is_space_char(key[key.size()-1]) ) {
					key.erase(key.size()-1);
				}
				if ( !key.empty() && wstring(value).find(L"（" + key + L"）") != wstring::npos ) {
					ctx.notice(line, L"タブ区切りの代入は計算せず、式を文字列として入れます。自分自身を含むので、実行のたびに式が長くなっていきます（（" + key + L"）を表示すると式のまま出ます）。計算した数を入れるなら、＝ で書くか calc を使ってください");
				}
			}
		}
		// 本文に＄の行があると、その＊が（）で呼ばれたときも代入が実行される
		ctx.effect.add_direct(L"＄の行");
		return;
	}

	// 全角にするつもりだった行頭の記号
	if ( ctx.reporting && (*p == L'*' || *p == L'@' || *p == L'$') ) {
		// 区切り線（*****）や、そのまま表示したい記号の可能性もあるので notice にとどめる
		ctx.notice(line, wstring(L"行頭が半角の「") + *p + L"」です。この行は文の本文として扱われます（全角の「＊」「＠」「＄」にするつもりでは？）");
	}

	lint_text(ctx, p, line);
}

// ＊か＠ひとつ分を調べる。
void	Satori::lint_unit(SatoriLintContext& ctx, const satori_unit& unit)
{
	const bool	is_talk = ( unit.typemark == L"＊" );

	wstring	name = unit.name;
	lint_unescape(ctx.esc, name);

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

	// ここからが、（名前）で呼ばれたときに実行される本文
	ctx.effect = SatoriLintEffect();

	for ( size_t i=0 ; i<unit.body.size() ; ++i ) {
		const int	line = ( i < unit.body_lines.size() ) ? unit.body_lines[i] : unit.line;

		// さくらスクリプトの選択肢 \q[ラベル,ID] の飛び先（行ごとに、（）の中も含めて見る）
		if ( ctx.reporting && unit.body[i].find(L'\\') != wstring::npos ) {
			strvec	targets;
			find_choice_targets(unit.body[i].c_str(), targets);
			for ( size_t t=0 ; t<targets.size() ; ++t ) {
				lint_choice_target(ctx, targets[t], line);
			}
		}

		// ＠の本文は単語としてそのまま使われるので、＄の行は代入されない
		if ( !is_talk && unit.body[i].find(L"＄") == 0
			&& (unit.body[i].find(L'\t') != wstring::npos || unit.body[i].find(L'＝') != wstring::npos) ) {
			ctx.report(false, line, L"＠の本文では、＄の行は変数の代入になりません（単語としてそのまま使われます）。＊の本文に書くつもりでは？");
		}

		if ( is_talk ) {
			lint_body_line(ctx, unit.body[i].c_str(), line, i == 0);
		}
		else {
			lint_text(ctx, unit.body[i].c_str(), line);
		}
	}

	// 1回目に、＊ごとの処理を集める（2回目の報告で呼び先をたどるのに使う）
	if ( !ctx.reporting && is_talk ) {
		ctx.talk_effects[name].merge(ctx.effect);
	}
	ctx.effect = SatoriLintEffect();
}


//---------------------------------------------------------------------------
//	全体

void	Satori::Lint(strpairvec& oMessages)
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
			lint_unescape(ctx.esc, name);
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

	for ( std::set<wstring>::const_iterator t = ctx.talk_names.begin() ; t != ctx.talk_names.end() ; ++t ) {
		if ( !t->empty() ) {
			ctx.talk_normalized.insert(std::make_pair(normalize_name(*t), *t));
		}
	}

	// 1回目は変数への代入などを集めるだけ。2回目に、集めたものを使って報告する。
	for ( int pass=0 ; pass<2 ; ++pass ) {
		if ( pass == 1 ) {
			ctx.resolve_effects();	// 1回目で集めた＊ごとの処理から、呼ぶと処理が起きる＊を求める
		}
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

	// 入りきらなかった分は、数だけ知らせる
	const int	omitted = ctx.count - static_cast<int>(oMessages.size());
	if ( omitted > 0 ) {
		wstring	text = L"ほか" + itos(omitted) + L"件は省略しました";
		if ( ctx.errors > ctx.stored_errors ) {
			text += L"（うち error " + itos(ctx.errors - ctx.stored_errors) + L"件）";
		}
		oMessages.push_back(strpair(L"info", text));
	}
}
