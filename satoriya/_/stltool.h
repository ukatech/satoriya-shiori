/*
	stltool.h
*/
#ifndef	STLTOOL_H
#define	STLTOOL_H

#ifdef _MSC_VER 
// テンプレート名が長い時の警告を抑制
#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」
// forスコープをANSI準拠させる
#if (defined(_MSC_VER) && (_MSC_VER <= 1200))
#ifndef for
#define for if(0);else for
#endif	// for
#endif
#endif	// _MSC_VER

#define TRUE 1
#include	<climits>
#include	<cstdlib>
#include	<cstring>

#include	<cwchar>
#include	<cwctype>

// ワイド文字版CRTのうちVC独自のものをPOSIXで補う
#ifdef POSIX
#define _wcsicmp wcscasecmp
#define _wcsnicmp wcsncasecmp
inline int _wtoi(const wchar_t* s) { return (int)wcstol(s,NULL,10); }
inline long _wtol(const wchar_t* s) { return wcstol(s,NULL,10); }
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1200
namespace std {
	#ifdef _WIN64
	typedef __int64             ptrdiff_t;
	#else  /* _WIN64 */
	typedef int            ptrdiff_t;
	#endif  /* _WIN64 */
}
#endif

#include	<iostream>
#include	<string>
#include	<map>
#include	<set>
#include	<vector>
#include	<list>
#include	<fstream>
#include	<deque>
#include	<cassert>
#include    <stdio.h>
#include	"charset.h"
using std::wstring;

#define const_strlen(s) ((sizeof(s) / sizeof(s[0]))-1)

/*class strvec : public vector<string>
{
public:
	strvec() : vector<string>() {}
	strvec(const_iterator first, const_iterator last) : vector<string>(first, last) {}

	string join(const string& i_delimiter="")
	{
		string r;
		for (iterator i=begin() ; i!=end() ;)
		{
			if ( i!=begin() )
				r += i_delimiter;
			r += *i;
		}
		return r;
	}
}:*/

typedef std::vector<wstring> strvec;
typedef	std::map<wstring, wstring>	strmap;
typedef	std::set<wstring>	stringset;
typedef	std::list<wstring>	strlist;
typedef	std::map<wstring, int>	strintmap;
typedef	unsigned char	byte;
typedef	std::pair<wstring, wstring>	strpair;
typedef std::vector<strpair> strpairvec;
/*
{
public:
	strpairvec() : vector<strpair>() {}
	void push_back(const strpair& element)
	{
		this->vector<strpair>::push_back( element );
	}

	void push_back(const string& first, const string& second)
	{
		this->vector<strpair>::push_back( strpair(first, second) );
	}
};
*/

class inimap : public std::map<wstring, strmap>	{
public:
	bool	load(const wstring& iFileName, CharactorSet cs=CS_NULL);
	bool	save(const wstring& iFileName, CharactorSet cs=CS_UTF8) const;
};

// ASCIIのみのバイト列をwstringにする（sprintf等の結果用）
inline wstring	ascii_to_w(const char* p) { wstring r; while (*p) { r += static_cast<wchar_t>(static_cast<unsigned char>(*p++)); } return r; }
// wstringの書式指定（ASCIIのみ）をバイト列にする
inline std::string	w_to_ascii(const wchar_t* p) { std::string r; while (*p) { r += static_cast<char>(*p++); } return r; }

// intとの相互変換
inline long stoi_internal(const wstring& s) { return wcstol(s.c_str(),NULL,10); }
inline long stoi_internal(const wchar_t* s) { return wcstol(s,NULL,10); }
inline unsigned long stoui(const wstring& s) { return wcstoul(s.c_str(),NULL,10); }
inline unsigned long stoui(const wchar_t* s) { return wcstoul(s,NULL,10); }

// 書式は数値1つを取るprintf書式（ASCIIのみ）
inline wstring	itos(long i, const wchar_t* iFormat=L"%d") { char buf[64]; sprintf(buf,w_to_ascii(iFormat).c_str(),i); return ascii_to_w(buf); }
inline wstring	uitos(unsigned long i, const wchar_t* iFormat=L"%u") { char buf[64]; sprintf(buf,w_to_ascii(iFormat).c_str(),i); return ascii_to_w(buf); }

inline bool stobool(const wchar_t *s) { return ( _wcsicmp(s,L"true") == 0 || _wtoi(s) != 0 ); }
inline bool stobool(const wstring &s) { return stobool(s.c_str()); }

// それてなに。
bool	aredigits(const wchar_t* p);
inline bool	aredigits(const wstring& s) { return aredigits(s.c_str()); }

bool	arealphabets(const wchar_t* p);
inline bool	arealphabets(const wstring& s) { return arealphabets(s.c_str()); }

// target中の最初にfind文字列が出現する位置を返す。（hz は旧SJIS版で全角半角対応だった名残）
const wchar_t*	strstr_hz(const wchar_t* target, const wchar_t* find);
const wchar_t*	strstri_hz(const wchar_t* target, const wchar_t* find);

// 文字列置換
bool	replace_first(wstring& str, const wstring& before, const wstring& after);

int	replace(wstring& str, const wchar_t* before, const wchar_t* after);
inline int	replace(wstring& str, const wstring& before, const wstring& after) {
	return replace(str,before.c_str(),after.c_str());
}
inline int	replace(wstring& str, const wchar_t* before, const wstring& after) {
	return replace(str,before,after.c_str());
}
inline int	replace(wstring& str, const wstring& before, const wchar_t* after) {
	return replace(str,before.c_str(),after);
}

template<class T>
int	multi_replace(wstring& str, T* array1, T* array2, int array_size) {
	int	count=0;
	for (int i=0 ; i<array_size ; ++i)
		count += replace(str, array1[i], array2[i]);
	return	count;
}

// 文字列消去
bool	erase_first(wstring& str, const wstring& before);
int	erase_all(wstring& str, const wstring& before);
// 対象語句の数を数える
int	count(const wstring& str, const wstring& target);
// 対象語句の存在確認
std::wstring::size_type find_hz(const wchar_t* str, const wchar_t* target, std::wstring::size_type find_pos = 0);
inline std::wstring::size_type find_hz(const wstring& str, const wstring& target,std::wstring::size_type find_pos = 0) {
	return	find_hz(str.c_str(), target.c_str(), find_pos);
}
inline std::wstring::size_type find_hz(const wchar_t* str, const wstring& target,std::wstring::size_type find_pos = 0) {
	return	find_hz(str, target.c_str(), find_pos);
}
inline std::wstring::size_type find_hz(const wstring& str, const wchar_t* target,std::wstring::size_type find_pos = 0)   {
	return	find_hz(str.c_str(), target, find_pos);
}

// dequeの後ろから n 個目を参照する
template<class T>
T&	from_back(std::deque<T>& iDeque, int n) {
	assert(n>=0 && n<iDeque.size());
	return	iDeque[iDeque.size()-1-n];
}

// ファイルを開く。パスはワイド文字列（POSIXではUTF-8に変換して開く）
FILE*	w_fopen(const wstring& iFileName, const wchar_t* iMode);

// ファイル←→バイト列
bool	bytes_from_file(std::string& o, const wstring& iFileName);
bool	bytes_to_file(const std::string& i, const wstring& iFileName);
// バイト列を行に分割する。改行は CRLF/LF 両対応で、改行文字は含まない。
void	split_lines(const std::string& i, std::vector<std::string>& o);
// 行群の文字コードを判定する（先頭行のBOM、全行が正しいUTF-8か）。BOMは取り除く。
CharactorSet	detect_lines_charset(std::vector<std::string>& io);
// 行群をwstringに変換する。cs==CS_NULLなら判定する。
void	lines_to_strvec(std::vector<std::string>& i, strvec& o, CharactorSet cs);

// ファイル←→strvec
// 読み込み時、cs==CS_NULLなら文字コードを判定する。
bool	strvec_from_file(strvec& o, const wstring& iFileName, CharactorSet cs=CS_NULL);
bool	strvec_to_file(const strvec& i, const wstring& iFileName, CharactorSet cs=CS_UTF8);
// ファイル←→strmap
bool	strmap_from_file(strmap& o, const wstring& iFileName, const wstring& dlmt=L",", const wstring& front_comment_mark=L"#", CharactorSet cs=CS_NULL);
bool	strmap_to_file(const strmap& i, const wstring& iFileName, const wstring& dlmt, CharactorSet cs=CS_UTF8);

// テキストファイルの改行
#ifdef POSIX
static const char	FILE_NEWLINE[] = "\n";
#else
static const char	FILE_NEWLINE[] = "\r\n";
#endif

// 一文字取得（サロゲートペアは1文字として扱う）
wstring	get_a_chr(const wchar_t*& p);

// 文字列の末尾に追加する。
// VC6のbasic_stringは容量を32文字ずつしか増やさず、少しずつ追加すると長さの2乗の時間がかかるので、足りなければ倍々に確保する。
inline wstring&	append_grow(wstring& s, const wchar_t* p, size_t n) {
	const size_t need = s.size() + n;
	if ( need > s.capacity() ) {
		s.reserve(need * 2);
	}
	return s.append(p, n);
}
inline wstring&	append_grow(wstring& s, const wstring& t) {
	return append_grow(s, t.c_str(), t.size());
}

// 一文字（サロゲートペアは2単位）を、ヒープを使わずに持つ。
// VC6のbasic_stringは1文字でもヒープを確保するので、1文字ずつ回すループでは get_a_chr の代わりに next_a_chr を使う。
class a_chr {
	wchar_t	m_buf[3];
	size_t	m_len;
	friend a_chr	next_a_chr(const wchar_t*& p);
public:
	a_chr() : m_len(0) { m_buf[0] = L'\0'; }
	const wchar_t*	c_str() const { return m_buf; }
	size_t	size() const { return m_len; }
	wstring	str() const { return wstring(m_buf, m_len); }
	bool	operator==(const wchar_t* s) const { return wcscmp(m_buf, s) == 0; }
	bool	operator!=(const wchar_t* s) const { return !(*this == s); }
	bool	operator==(const wstring& s) const { return s.size() == m_len && wcscmp(m_buf, s.c_str()) == 0; }
	bool	operator!=(const wstring& s) const { return !(*this == s); }
};

// 一文字取得（サロゲートペアは1文字として扱う）。get_a_chr と同じだが a_chr で返す。
inline a_chr	next_a_chr(const wchar_t*& p) {
	a_chr	c;
	if ( *p == L'\0' )
		return	c;
	c.m_buf[c.m_len++] = *p++;
	if ( IsHighSurrogate(c.m_buf[0]) && IsLowSurrogate(*p) )
		c.m_buf[c.m_len++] = *p++;
	c.m_buf[c.m_len] = L'\0';
	return	c;
}

inline wstring&	operator+=(wstring& s, const a_chr& c) { return append_grow(s, c.c_str(), c.size()); }
inline wstring	operator+(const a_chr& c, const wstring& s) { wstring r(c.c_str(), c.size()); r += s; return r; }
inline wstring	operator+(const a_chr& c, wchar_t w) { wstring r(c.c_str(), c.size()); r += w; return r; }

// 文字数（サロゲートペアは1文字として数える）
size_t	count_chars(const wstring& str);
// 半角換算の幅（半角文字は1、全角文字は2。SJIS時代のバイト数に相当）
size_t	count_width(const wstring& str);
size_t	count_width(const wchar_t* p, size_t len);
inline size_t	count_width(const a_chr& c) { return count_width(c.c_str(), c.size()); }
// 文字単位の位置を wchar_t 単位の位置に変換する。文字数を超えたら str.size() を返す。
size_t	char_pos_to_index(const wstring& str, size_t char_pos);

// 文字単位に分割
template<class T>
int	split(const wchar_t* p, T& o) {
	while ( *p != L'\0' )
		o.push_back(get_a_chr(p));
	return	o.size();
}

template<class T>
inline int split(const wstring& i, T& o) {
	return split(i.c_str(),o);
}

// 分割(区切り文字列は1文字ずつ候補扱い) max_wordsは最大切り出し単語数。0なら制限しない。
template<class T>
int	split(const wchar_t* p, const wchar_t* dp, T& o, int max_words=0, bool split_one=false)
{
	std::set<wstring>	dlmt_set;
	while ( *dp != L'\0' )
		dlmt_set.insert(get_a_chr(dp));

	if ( dlmt_set.empty() )
		return	split(p, o);

	if ( max_words==1 ) {
		o.push_back(p);
		return	1;
	}

	wstring	word;
	while ( *p != L'\0' ) {
		wstring	c = get_a_chr(p);
		if ( dlmt_set.find(c) != dlmt_set.end() ) {
			if ( word.size() > 0 || split_one ) {
				o.push_back(word);

				if ( max_words>0 && static_cast<int>(o.size()+1) >= max_words ) {	// 単語数制限
					word = p;
					break;
				}
				else {
					word=L"";
				}
			}
		}
		else {
			word += c;
		}
	}
	if ( word.size() > 0 ) { //split_oneがフラグONでも最後のゴミは無視
		o.push_back(word);
	}

	if ( o.size() == 0 ) { //ほんとになにもない場合の番兵
		o.push_back(L"");
		return 1;
	}

	return	o.size();
}

template<class T>
inline int split(const wstring& i, const wstring& dlmt, T& o, int max_words=0, bool split_one=false) { return split(i.c_str(),dlmt.c_str(),o,max_words,split_one); }

template<class T>
inline int split(const wchar_t* p, const wstring& dlmt, T& o, int max_words=0, bool split_one=false) { return split(p,dlmt.c_str(),o,max_words,split_one); }

template<class T>
inline int split(const wstring& i, const wchar_t* dp, T& o, int max_words=0, bool split_one=false) { return split(i.c_str(),dp,o,max_words,split_one); }


// 分割(単純分割) max_wordsは最大切り出し単語数。0なら制限しない。
template<class T>
int	split_string(const wchar_t* p, const wchar_t* dp, T& o, int max_words=0, bool split_one=false)
{
	if ( *dp == 0 ) {
		return	split(p, o);
	}

	if ( max_words==1 ) {
		o.push_back(p);
		return	1;
	}

	wstring	word;
	const wchar_t *dpr = dp;
	wstring  dpc = get_a_chr(dpr); //とりあえず1文字目をとっておく
	size_t  dpl = wcslen(dpr);

	while ( *p != L'\0' ) {
		wstring	c = get_a_chr(p);

		if ( c == dpc && wcsncmp(p,dpr,dpl) == 0 ) { //strncmpで2文字目以降を単純比較マッチ
			if ( word.size() > 0 || split_one ) {
				o.push_back(word);

				if ( max_words>0 && static_cast<int>(o.size()+1) >= max_words ) {	// 単語数制限
					word = p + dpl;
					break;
				}
				else {
					word=L"";
				}
			}
			p += dpl;
		}
		else {
			word += c;
		}
	}
	if ( word.size() > 0 ) { //split_oneがフラグONでも最後のゴミは無視
		o.push_back(word);
	}

	if ( o.size() == 0 ) { //ほんとになにもない場合の番兵
		o.push_back(L"");
		return 1;
	}

	return	o.size();
}

template<class T>
inline int split_string(const wstring& i, const wstring& dlmt, T& o, int max_words=0, bool split_one=false) { return split_string(i.c_str(),dlmt.c_str(),o,max_words,split_one); }

template<class T>
inline int split_string(const wchar_t* p, const wstring& dlmt, T& o, int max_words=0, bool split_one=false) { return split_string(p,dlmt.c_str(),o,max_words,split_one); }

template<class T>
inline int split_string(const wstring& i, const wchar_t* dp, T& o, int max_words=0, bool split_one=false) { return split_string(i.c_str(),dp,o,max_words,split_one); }


inline int	splitToSet(const wstring& iString, std::set<wstring>& oSet, int iDelimiter) {
	oSet.clear();
	const wchar_t* start=iString.c_str();
	const wchar_t* p=start;

	for ( ; *p!=L'\0' ; ++p )
	{
		if ( *p==iDelimiter )
		{
			if ( start<p )
			{
				oSet.insert( wstring(start, p-start) );
			}
			start = p+1;
		}
	}
	if ( start<p )
	{
		oSet.insert(start);
	}
	return	oSet.size();
}



// mapからkeyの一覧を取得
template<typename C, typename K, typename V>
int	keys(const std::map<K,V>& iMap, C& oContainer) {
	oContainer.clear();
	for ( typename std::map<K,V>::const_iterator i=iMap.begin() ; i!=iMap.end() ; ++i)
		oContainer.push_back(i->first);
	return	oContainer.size();
}
// mapからkeyの一覧を取得
template<typename C, typename K, typename V>
C	keys(const std::map<K,V>& iMap) {
	C	theContainer;
	keys<C,K,V>(iMap, theContainer);
	return	theContainer;
}

// mapからvalueの一覧を取得
template<typename C, typename K, typename V>
int	values(const std::map<K,V>& iMap, C& oContainer) {
	oContainer.clear();
	for ( typename std::map<K,V>::const_iterator i=iMap.begin() ; i!=iMap.end() ; ++i)
		oContainer.push_back(i->second);
	return	oContainer.size();
}
// mapからvalueの一覧を取得
template<typename C, typename K, typename V>
C	values(const std::map<K,V>& iMap) {
	C	theContainer;
	values<C,K,V>(iMap, theContainer);
	return	theContainer;
}

// コンテナ要素を単一のstringに結合。dlmtを間に挟む。返値はstringの大きさ
template<class T>
int	combine(wstring& out, const T& in, const wstring& dlmt=L"", bool add_dlmt_on_final=false) {
	typename T::const_iterator i=in.begin();
	if ( add_dlmt_on_final ) {
		for (; i!=in.end() ;++i) {
			out += *i;
			out += dlmt;
		}
	}
	else {
		int	size=in.size();
		for (int n=0 ; n<size ; ++n,++i) {
			out += *i;
			if (n<size-1)
				out += dlmt;
		}
	}
	return	out.size();
}
template<class T>
wstring	combine(const T& in, const wstring& dlmt=L"", bool add_dlmt_on_final=false) {
	wstring	s;
	combine(s, in, dlmt, add_dlmt_on_final);
	return	s;
}



// ファイルの存在を確認
bool	is_exist_file(const wstring& iFileName);

// strの先頭のheadと末尾のtailを取り除いた部分を返す。先頭・末尾が一致しているかは確認しない。
inline wstring	strip_head_tail(const wstring& str, const wchar_t* head, const wchar_t* tail) {
	const wstring::size_type h = wcslen(head), t = wcslen(tail);
	if ( str.size() < h+t ) { return wstring(); }
	return	str.substr(h, str.size()-h-t);
}

// strの先頭がheadであればtrue
bool	compare_head_s(const wchar_t* str, const wchar_t* head);

inline bool	compare_head(const wstring& str, const wstring& head) {
	return compare_head_s(str.c_str(),head.c_str());
}
inline bool	compare_head(const wstring& str, const wchar_t* head) {
	return compare_head_s(str.c_str(),head);
}
inline bool	compare_head(const wchar_t* str, const wchar_t* head) {
	return compare_head_s(str,head);
}

bool	compare_head_nocase_s(const wchar_t* str, const wchar_t* head);

inline bool	compare_head_nocase(const wstring& str, const wstring& head) {
	return compare_head_nocase_s(str.c_str(),head.c_str());
}
inline bool	compare_head_nocase(const wstring& str, const wchar_t* head) {
	return compare_head_nocase_s(str.c_str(),head);
}
inline bool	compare_head_nocase(const wchar_t* str, const wchar_t* head) {
	return compare_head_nocase_s(str,head);
}

// strの末尾がtailであればtrue
bool	compare_tail_s(const wchar_t* str, const wchar_t* tail);

inline bool compare_tail(const wstring& str, const wstring& tail) {
	return compare_tail_s(str.c_str(),tail.c_str());
}	
inline bool compare_tail(const wstring& str, const wchar_t* tail) {
	return compare_tail_s(str.c_str(),tail);
}	
inline bool compare_tail(const wchar_t* str, const wchar_t* tail) {
	return compare_tail_s(str,tail);
}	

bool	compare_tail_nocase_s(const wchar_t* str, const wchar_t* tail);
	
inline bool compare_tail_nocase(const wstring& str, const wstring& tail) {
	return compare_tail_nocase_s(str.c_str(),tail.c_str());
}	
inline bool compare_tail_nocase(const wstring& str, const wchar_t* tail) {
	return compare_tail_nocase_s(str.c_str(),tail);
}	
inline bool compare_tail_nocase(const wchar_t* str, const wchar_t* tail) {
	return compare_tail_nocase_s(str,tail);
}	

// 特定の一文字が最後に出現する位置を返す
const wchar_t*	find_final_char(const wchar_t* str, wchar_t c);
inline wchar_t* find_final_char(wchar_t* str, wchar_t c) { return const_cast<wchar_t*>(find_final_char(static_cast<const wchar_t*>(str), c)); }
/*#include	<mbctype.h>	// for _ismbblead,_ismbbtrail
template<class T>
T*	find_final_char(T* str, const T& c) {
	T* last=NULL;
	for (T* p=str ; *p!='\0' ; p+=_ismbblead(*p)?2:1)
		if ( *p==c )
			last=p;
	return	last;
}*/

// 簡易暗号化（バイト列の並べ替え）
std::string	encode(const std::string& s);
std::string	decode(const std::string& s);

// バイト列と16進文字列の相互変換
std::string	binary_to_string(const byte* iArray, int iLength);
void	string_to_binary(const std::string& iString, byte* oArray);
// xor フィルタ
void	xor_filter(byte* ioArray, int iLength, byte iXorValue);


// コンテナ内を検索、存在有無をboolで返す
/*template<typename C, typename E>
bool exists(const C& iC, const E& iE) {
	for ( typename C::const_iterator i=iC.begin() ; i!=iC.end() ; ++i )
		if ( *i == iE )
			return	true;
	return	false;
}

template<typename K, typename V, typename E>
inline bool exists< map<K, V> >(const map<K,V>& iC, const E& iE) {
	return	(iC.find(iE) != iC.end());
}

template<typename T, typename E>
inline bool exists< set<T> >(const set<T>& iC, const E& iE) {
	return	(iC.find(iE) != iC.end());
}
*/

// フルパスの分解
wstring	get_file_name(const wstring& str);
wstring	get_folder_name(const wstring& str);
wstring	get_extention(const wstring& str);
// 拡張子を変更したものを返す
wstring	set_extention(const wstring& str, const wchar_t* new_ext);
inline wstring	set_extention(const wstring& str, const wstring& new_ext) { return set_extention(str, new_ext.c_str()); }
// 拡張子を返す
inline wstring	get_extension(const wstring& str) {
	const wchar_t* p = find_final_char(str.c_str(), L'.');
	return p ? p+1 : L"";
}
// ファイル名部分を変更したものを返す
wstring	set_filename(const wstring& str, const wchar_t* new_filename);
inline wstring	set_filename(const wstring& str, const wstring& new_filename) { return set_filename(str, new_filename.c_str()); }

// 出力
std::wostream& operator<<(std::wostream& o, const strvec& i);
std::wostream& operator<<(std::wostream& o, const strmap& i);
std::wostream& operator<<(std::wostream& o, const strintmap& i);
inline std::wostream& operator<<(std::wostream& o, const strpairvec& i) {
	for ( strpairvec::const_iterator p=i.begin() ; p!=i.end() ; ++p )
		o << p->first << L": " << p->second << std::endl;
	return	o;
}


/*
// なんか、あんまり使っていないもの。。

// ファイルアクセサ
template<class T>
bool	put(const char* iFileName, T& i) {
	ofstream	o(iFileName);
	if ( !o.is_open() )
		return	false;
	o<<i;
	return	true;
}
template<class T>
bool	get(const char* iFileName, T& o) {
	ifstream	i(iFileName);
	if ( !i.is_open() )
		return	false;
	i>>o;
	return	true;
}


template<class T>
bool	read_text_file(
	T& oT,	// require "push_back" method. vector, list, deque, ...
	const string& iFileName) 
{
	ifstream	in(iFileName.c_str());
	if ( !in.is_open() )
		return	false;
	while ( in.peek() != EOF ) {
		// １行読み込み
		strstream	line;
		int	c;
		while ( (c=in.get()) != '\n' && c!=EOF)
			line.put(c);
		line.put('\0');

		// 行ストリームを固定、各行に対し処理
		char* p=line.str();
		oT.push_back(p);
		// 行ストリームの固定を解除
		line.rdbuf()->freeze(0);
	}
	in.close();
	return	true;
}

template<class T>
bool	write_text_file(
	const T& iT,	// コンテナはbegin()とend()とForwardAccessIteratorが要る。荷物には operator<< が要る。つか原則stringだな。
	const string& iFileName)
{
	ofstream	out(iFileName.c_str());
	if ( !out.is_open() )
		return	false;
	T::const_iterator	it;
	for (it=iT.begin() ; it!=iT.end() ; ++it)
		out << *it << endl;
	out.close();
	return	true;
}
*/

// 任意の桁を四捨五入する。figure省略時は小数第一位を四捨五入。
template<typename T>
T	round(T num, const int figure=0) {
	if ( figure>=0 ) {
		for (int i=0 ; i<figure ; ++i) { num *= 10; }
		num = int(num + static_cast<T>(0.5));
		for (int i=0 ; i<figure ; ++i) { num /= 10; }
	}
	else {
		for (int i=0 ; i>figure ; --i) { num /= 10; }
		num = int(num + static_cast<T>(0.5));
		for (int i=0 ; i>figure ; --i) { num *= 10; }
	}
	return	num;
}

wstring	int2zen(int i);
wstring ul2zen(unsigned long i);

int     zen2int(const wchar_t *str);
inline int zen2int(const wstring &s)
{
	return zen2int(s.c_str());
}

unsigned long zen2ul(const wchar_t *str);
inline unsigned long zen2ul(const wstring &s)
{
	return zen2ul(s.c_str());
}

wstring  zen2han(const wchar_t *str);
inline wstring zen2han(const wstring &s)
{
	return zen2han(s.c_str()); 
}

// ディレクトリ区切り
#ifdef POSIX
  #define DIR_CHAR L'/'
  inline wstring unify_dir_char(const wstring& i_str) { wstring r=i_str; replace(r, L"\\", L"/"); return r; }
#else
  #define DIR_CHAR L'\\'
  inline wstring unify_dir_char(const wstring& i_str) { wstring r=i_str; replace(r, L"/", L"\\"); return r; }
#endif


// DOS/WindowsやHTTPなどにおける行デリミタ
static const wstring CRLF = L"\x0d\x0a";


#endif	//	STLTOOL_H
