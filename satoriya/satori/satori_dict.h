//---------------------------------------------------------------------------
//	辞書の前処理と＊／＠の単位への分割
//
//	satori_load_dict.cpp（読み込み）と satori_lint.cpp（静的検査）が同じ処理を使うために分けてある。
//	satori.h を先に include すること。
//---------------------------------------------------------------------------
#ifndef SATORI_DICT_H
#define SATORI_DICT_H

// ＊（トーク）か＠（単語群）ひとつ分
struct satori_unit
{
	wstring typemark;
	wstring name;
	wstring condition;
	strvec body;

	// 以下は lint 用の行番号（1始まり。行番号を渡さなければ 0）
	int line;				// ＊／＠の行
	std::vector<int> body_lines;	// body の各行の先頭の行

	satori_unit() : line(0) {}
};

// pre_process が見つけた、辞書の書き方の問題
struct pre_process_report
{
	std::vector<int> line_numbers;	// 前処理後の各行が、元ファイルの何行目から始まるか（1始まり）
	int unclosed_line;				// 閉じていない（ を開いた行（無ければ 0）
	int dangling_escape_line;		// ファイル末尾が φ で終わっているときの、その行（無ければ 0）
	std::vector<int> suspect_lines;	// （ の続きの行が ＊／＠ で始まっている。）の書き忘れが疑われる、（ を開いた行

	pre_process_report() : unclosed_line(0), dangling_escape_line(0) {}
};

// プリプロセス的な処理。
// 改行キャンセル適用、コメント削除、before_replaceの適用。falseは（ が閉じないままファイルが終わった。
bool pre_process(
	const strvec& in,
	strvec& out,
	escaper& io_escaper,
	strmap& io_replace_dic,
	pre_process_report* o_report = NULL);

// 行の並びを、行頭が i_typemarks のいずれかである行を頭とする単位に分ける。
// i_line_numbers があれば、各単位に行番号を入れる（pre_process_report::line_numbers を渡す）。
void lines_to_units(
	const std::vector<wstring>& i_lines,
	const std::vector<wstring>& i_typemarks,
	const wstring& i_name_cond_delimiter,
	std::vector<satori_unit>& o_units,
	const std::vector<int>* i_line_numbers = NULL);

#endif // SATORI_DICT_H
