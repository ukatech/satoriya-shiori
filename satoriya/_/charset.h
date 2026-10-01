/*
	文字コード変換。
	内部表現は std::wstring (Windowsでは UTF-16、POSIXでは UTF-32)。
	外部とのやりとり（ファイル、SHIORI/SAORI/SSTPのリクエスト等）は std::string のバイト列で、
	ここの関数で内部表現との相互変換を行う。

	UTF-8 との変換は自前実装なので環境に依存しない。
	Shift_JIS との変換は Windows では CP932 の API、POSIX では iconv を使う。
*/

#ifndef CHARSET_H_INCLUDED
#define CHARSET_H_INCLUDED

#include	<string>

// 文字コード
enum CharactorSet
{
	CS_NULL,	// 不明・自動判定
	CS_SJIS,	// Shift_JIS (CP932)
	CS_UTF8,	// UTF-8
	CS_ACP,		// 環境の既定コードページ（Windows: CP_ACP、POSIX: UTF-8）
};

// 相互変換
std::wstring	SJIStoW(const std::string& str);
std::string		WtoSJIS(const std::wstring& str);
std::wstring	UTF8toW(const std::string& str);
std::string		WtoUTF8(const std::wstring& str);
std::wstring	ACPtoW(const std::string& str);
std::string		WtoACP(const std::wstring& str);

// 指定の文字コードで変換。CS_NULL の場合は DetectCharset で判定する。
std::wstring	MBtoW(const std::string& str, CharactorSet cs);
std::string		WtoMB(const std::wstring& str, CharactorSet cs);

// Charset ヘッダ等の名前との変換。知らない名前は CS_NULL を返す。
CharactorSet	CharsetFromName(const std::wstring& name);
const wchar_t*	CharsetName(CharactorSet cs);

// 厳密に正しい UTF-8 のバイト列か
bool	IsValidUTF8(const char* p, size_t len);
inline bool	IsValidUTF8(const std::string& str) { return IsValidUTF8(str.c_str(), str.size()); }

// 先頭が UTF-8 の BOM か
bool	HasUTF8BOM(const std::string& str);

// BOM があるか、全体が正しい UTF-8 なら CS_UTF8、そうでなければ CS_SJIS を返す。
CharactorSet	DetectCharset(const std::string& str);

// UTF-16 のサロゲートペアの判定（wchar_t が 32bit の環境では常に false）
inline bool	IsHighSurrogate(wchar_t c) { return sizeof(wchar_t) == 2 && c >= 0xD800 && c <= 0xDBFF; }
inline bool	IsLowSurrogate(wchar_t c) { return sizeof(wchar_t) == 2 && c >= 0xDC00 && c <= 0xDFFF; }

#endif //CHARSET_H_INCLUDED
