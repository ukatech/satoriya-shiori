/*
	今のところ「euc,jisをsjisに変換する」ことしかできない。
	ただし拡張は容易。

	判別・変換にはこちらを参照した。
	http://www.mars.dti.ne.jp/~torao/program/appendix/japanese.html

	判別ルーチンの挙動
	
		テキスト中に　charset=??? または encoding=??? があれば採用（複数あった場合は最初のものを使う）
		???の文字列としては iso-2022-jp, shift_jis, x-sjis, x-euc-jp, euc-jp, UTF-16BE, UTF-16LE, UTF-8 を認識。

		判定の順序は、UTFのBOM, charset, encoding, jisエスケープ, euc/sjis判定である。
		Unicodeは内容による判別は行わない。

		charsetが無い場合でも、jisは確実な判定ができる。
		eucとsjisは、互いに使われていないコードが存在すれば確実な判定ができるが、
		完全に重なった部分の文字しか使われていなかった場合、判別不能として CS_NULL を返す。


	sjis2jisはここから。
	ソースは殆どそのまんま。
	http://plaza13.mbn.or.jp/~konton/TIPPRG/tipprg03.html
*/


// 文字コード
enum CharactorSet
{
	CS_NULL,
	CS_JIS,
	CS_SJIS,
	CS_EUC,
	CS_UTF8,
	CS_UTF16BE,
	CS_UTF16LE,
};

// 文字コード判別（確実ではない。判別に失敗するとCS_NULLを返す）
CharactorSet	getCharactorSet(const char* p);
inline CharactorSet	getCharactorSet(const string& str) { return getCharactorSet(str.c_str()); } 

// 文字コード変換
string sjis2euc(const string& str);
string sjis2jis(const string& str);

string euc2sjis(const string& str);
string euc2jis(const string& str);

string jis2sjis(const string& str);
string jis2euc(const string& str);

string	UTF8toSJIS(const string& str);
string	UTF16BEtoSJIS(const wchar_t* p);
string	UTF16LEtoSJIS(const wchar_t* p);

// 不明のコードを自動判別し、とにかくsjisを返す。無理なこともある。
inline string convertSomethingToSJIS(const char* p) {
	switch ( getCharactorSet(p) ) {
	case CS_JIS: return jis2sjis(p); 
	case CS_EUC: return euc2sjis(p);
	case CS_UTF8: return UTF8toSJIS(p);
	case CS_UTF16BE: return UTF16BEtoSJIS((const wchar_t*)p);
	case CS_UTF16LE: return UTF16LEtoSJIS((const wchar_t*)p);
	default: return p; // 変換しない
	}
}

