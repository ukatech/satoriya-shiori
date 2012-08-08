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
#ifndef for
#define for if(0);else for
#endif	// for
#endif	// _MSC_VER

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
using namespace std;


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

typedef vector<string> strvec;
typedef	map<string, string>	strmap;
typedef	set<string>	stringset;
typedef	list<string>	strlist;
typedef	map<string, int>	strintmap;
typedef	unsigned char	byte;
typedef	pair<string, string>	strpair;
typedef vector<strpair> strpairvec;
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

class inimap : public map<string, strmap>	{
public:
	bool	load(const string& iFileName);
	bool	save(const string& iFileName) const;
};
class inivec : public vector< pair<string, strpairvec> > {
public:
	bool	load(const string& iFileName);
};

// デリミタまで読み込み
bool	getline(istream& i, string& o, int delimtier='\n');
bool	getline(istream& i, int& o, int delimtier='\n');

// intとの相互変換
inline long stoi(const string& s) { return strtol(s.c_str(),NULL,10); }
inline long stoi(const char* s) { return strtol(s,NULL,10); }
inline unsigned long stoui(const string& s) { return strtoul(s.c_str(),NULL,10); }
inline unsigned long stoui(const char* s) { return strtoul(s,NULL,10); }

inline string	itos(long i, const char* iFormat="%d") { char buf[32]; sprintf(buf,iFormat,i); return buf; }
inline string	uitos(unsigned long i, const char* iFormat="%u") { char buf[32]; sprintf(buf,iFormat,i); return buf; }

// それてなに。
bool	aredigits(const char* p);
inline bool	aredigits(const string& s) { return aredigits(s.c_str()); }

bool	arealphabets(const char* p);
inline bool	arealphabets(const string& s) { return arealphabets(s.c_str()); }

// target中の最初にfind文字列が出現する位置を返す。半角全角両対応
const char*	strstr_hz(const char* target, const char* find);

// 文字列置換
bool	replace_first(string& str, const string& before, const string& after);

int	replace(string& str, const char* before, const char* after);
inline int	replace(string& str, const string& before, const string& after) {
	return replace(str,before.c_str(),after.c_str());
}
inline int	replace(string& str, const char* before, const string& after) {
	return replace(str,before,after.c_str());
}
inline int	replace(string& str, const string& before, const char* after) {
	return replace(str,before.c_str(),after);
}

template<class T>
int	multi_replace(string& str, T* array1, T* array2, int array_size) {
	int	count=0;
	for (int i=0 ; i<array_size ; ++i)
		count += replace(str, array1[i], array2[i]);
	return	count;
}

// 文字列消去
bool	erase_first(string& str, const string& before);
int	erase(string& str, const string& before);
// 対象語句の数を数える
int	count(const string& str, const string& target);
// 対象語句の存在確認
std::string::size_type find_hz(const char* str, const char* target, std::string::size_type find_pos = 0);
inline std::string::size_type find_hz(const string& str, const string& target,std::string::size_type find_pos = 0) {
	return	find_hz(str.c_str(), target.c_str(), find_pos);
}
inline std::string::size_type find_hz(const char* str, const string& target,std::string::size_type find_pos = 0) {
	return	find_hz(str, target.c_str(), find_pos);
}
inline std::string::size_type find_hz(const string& str, const char* target,std::string::size_type find_pos = 0)   {
	return	find_hz(str.c_str(), target, find_pos);
}

// dequeの後ろから n 個目を参照する
template<class T>
T&	from_back(deque<T>& iDeque, int n) {
	assert(n>=0 && n<iDeque.size());
	return	iDeque[iDeque.size()-1-n];
}

// ファイル←→string
bool	string_from_file(string& o, const string& iFileName);
bool	string_to_file(const string& i, const string& iFileName);
// ファイル←→strvec
bool	strvec_from_file(strvec& o, const string& iFileName);
bool	strvec_to_file(const strvec& i, const string& iFileName);
// ファイル←→strmap
bool	strmap_from_file(strmap& o, const string& iFileName, const string& dlmt=",", const string& front_comment_mark="#");
bool	strmap_to_file(const strmap& i, const string& iFileName, const string& dlmt);
// 全角半角問わず一文字取得
string	get_a_chr(const char*& p);

// 文字単位に分割
template<class T>
int	split(const char* p, T& o) {
	while ( *p != '\0' )
		o.push_back(get_a_chr(p));
	return	o.size();
}

template<class T>
inline int split(const string& i, T& o) {
	return split(i.c_str(),o);
}

// 単語単位に分割 max_wordsは最大切り出し単語数。0なら制限しない。
template<class T>
int	split(const char* p, const char* dp, T& o, int max_words=0, bool split_one=false)
{
	set<string>	dlmt_set;
	while ( *dp != '\0' )
		dlmt_set.insert(get_a_chr(dp));

	if ( dlmt_set.empty() )
		return	split(p, o);

	if ( max_words==1 ) {
		o.push_back(p);
		return	1;
	}

	string	word;
	while ( *p != '\0' ) {
		string	c = get_a_chr(p);
		if ( dlmt_set.find(c) != dlmt_set.end() ) {
			if ( word.size() > 0 || split_one ) {
				o.push_back(word);

				if ( max_words>0 && static_cast<int>(o.size()+1) >= max_words ) {	// 単語数制限
					word = p;
					break;
				}
				else {
					word="";
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

	return	o.size();
}

template<class T>
inline int split(const string& i, const string& dlmt, T& o, int max_words=0, bool split_one=false) { return split(i.c_str(),dlmt.c_str(),o,max_words,split_one); }

template<class T>
inline int split(const char* p, const string& dlmt, T& o, int max_words=0, bool split_one=false) { return split(p,dlmt.c_str(),o,max_words,split_one); }

template<class T>
inline int split(const string& i, const char* dp, T& o, int max_words=0, bool split_one=false) { return split(i.c_str(),dp,o,max_words,split_one); }



inline int	splitToSet(const string& iString, set<string>& oSet, int iDelimiter) {
	oSet.clear();
	const char* start=iString.c_str();
	const char* p=start;

	for ( ; *p!='\0' ; ++p )
	{
		if ( *p==iDelimiter )
		{
			if ( start<p )
			{
				oSet.insert( string(start, p-start) );
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
int	keys(const map<K,V>& iMap, C& oContainer) {
	oContainer.clear();
	for ( typename map<K,V>::const_iterator i=iMap.begin() ; i!=iMap.end() ; ++i)
		oContainer.push_back(i->first);
	return	oContainer.size();
}
// mapからkeyの一覧を取得
template<typename C, typename K, typename V>
C	keys(const map<K,V>& iMap) {
	C	theContainer;
	keys<C,K,V>(iMap, theContainer);
	return	theContainer;
}

// mapからvalueの一覧を取得
template<typename C, typename K, typename V>
int	values(const map<K,V>& iMap, C& oContainer) {
	oContainer.clear();
	for ( typename map<K,V>::const_iterator i=iMap.begin() ; i!=iMap.end() ; ++i)
		oContainer.push_back(i->second);
	return	oContainer.size();
}
// mapからvalueの一覧を取得
template<typename C, typename K, typename V>
C	values(const map<K,V>& iMap) {
	C	theContainer;
	keys<C,K,V>(iMap, theContainer);
	return	theContainer;
}

// コンテナ要素を単一のstringに結合。dlmtを間に挟む。返値はstringの大きさ
template<class T>
int	combine(string& out, const T& in, const string& dlmt="", bool add_dlmt_on_final=false) {
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
string	combine(const T& in, const string& dlmt="", bool add_dlmt_on_final=false) {
	string	s;
	combine(s, in, dlmt, add_dlmt_on_final);
	return	s;
}



// ファイルの存在を確認
bool	is_exist_file(const string& iFileName);

// strの先頭がheadであればtrue
bool	compare_head_s(const char* str, const char* head);

inline bool	compare_head(const string& str, const string& head) {
	return compare_head_s(str.c_str(),head.c_str());
}
inline bool	compare_head(const string& str, const char* head) {
	return compare_head_s(str.c_str(),head);
}
inline bool	compare_head(const char* str, const char* head) {
	return compare_head_s(str,head);
}

bool	compare_head_nocase_s(const char* str, const char* head);

inline bool	compare_head_nocase(const string& str, const string& head) {
	return compare_head_nocase_s(str.c_str(),head.c_str());
}
inline bool	compare_head_nocase(const string& str, const char* head) {
	return compare_head_nocase_s(str.c_str(),head);
}
inline bool	compare_head_nocase(const char* str, const char* head) {
	return compare_head_nocase_s(str,head);
}

// strの末尾がtailであればtrue
bool	compare_tail_s(const char* str, const char* tail);

inline bool compare_tail(const string& str, const string& tail) {
	return compare_tail_s(str.c_str(),tail.c_str());
}	
inline bool compare_tail(const string& str, const char* tail) {
	return compare_tail_s(str.c_str(),tail);
}	
inline bool compare_tail(const char* str, const char* tail) {
	return compare_tail_s(str,tail);
}	

bool	compare_tail_nocase_s(const char* str, const char* tail);
	
inline bool compare_tail_nocase(const string& str, const string& tail) {
	return compare_tail_nocase_s(str.c_str(),tail.c_str());
}	
inline bool compare_tail_nocase(const string& str, const char* tail) {
	return compare_tail_nocase_s(str.c_str(),tail);
}	
inline bool compare_tail_nocase(const char* str, const char* tail) {
	return compare_tail_nocase_s(str,tail);
}	

// target中の最初にfind文字列が出現する位置を返す。半角全角両対応
const char*	strstr_hz(const char* target, const char* find);

// 特定の一文字が最後に出現する位置を返す
const char*	find_final_char(const char* str, char c);
inline char* find_final_char(char* str, char c) { return const_cast<char*>(find_final_char(static_cast<const char*>(str), c)); }
/*#include	<mbctype.h>	// for _ismbblead,_ismbbtrail
template<class T>
T*	find_final_char(T* str, const T& c) {
	T* last=NULL;
	for (T* p=str ; *p!='\0' ; p+=_ismbblead(*p)?2:1)
		if ( *p==c )
			last=p;
	return	last;
}*/

// 簡易暗号化
string	encode(const string& s);
string	decode(const string& s);

// バイナリデータと16進数表現を相互変換
string	binary_to_string(const byte* iArray, int iLength);
void	string_to_binary(const string& iString, byte* oArray);
								// oArray には iString.size()/2 バイトのメモリを確保してあること。


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

// xor フィルタ
void	xor_filter(byte* iArray, int iLength, byte _xor);

// フルパスの分解
string	get_file_name(const string& str);
string	get_folder_name(const string& str);
string	get_extention(const string& str);
// 拡張子を変更したものを返す
string	set_extention(const string& str, const char* new_ext);
inline string	set_extention(const string& str, const string& new_ext) { return set_extention(str, new_ext.c_str()); }
// 拡張子を返す
inline string	get_extension(const string& str) {
	const char* p = find_final_char(str.c_str(), '.');
	return p ? p+1 : "";
}
// ファイル名部分を変更したものを返す
string	set_filename(const string& str, const char* new_filename);
inline string	set_filename(const string& str, const string& new_filename) { return set_filename(str, new_filename.c_str()); }

// 出力
ostream& operator<<(ostream& o, const strvec& i);
ostream& operator<<(ostream& o, const strmap& i);
ostream& operator<<(ostream& o, const strintmap& i);
inline ostream& operator<<(ostream& o, const strpairvec& i) {
	for ( strpairvec::const_iterator p=i.begin() ; p!=i.end() ; ++p )
		o << p->first << ": " << p->second << endl;
	return	o;
}


// 入力
istream& operator>>(istream& i, strvec& o);
istream& operator>>(istream& i, strmap& o);
istream& operator>>(istream& i, strintmap& o);

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

// printf互換で文字列を生成し、string型で返す。
extern string stringf(const char* iFormat, ...);

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

string	int2zen(int i);

int     zen2int(const char *str);
inline int zen2int(const string &s)
{
	return zen2int(s.c_str());
}

string  zen2han(const char *str);
inline string zen2han(const string &s)
{
	return zen2han(s.c_str()); 
}

// ディレクトリ区切り
#ifdef POSIX
  #define DIR_CHAR '/'
  inline string unify_dir_char(const string& i_str) { string r=i_str; replace(r, "\\", "/"); return r; }
#else
  #define DIR_CHAR '\\'
  inline string unify_dir_char(const string& i_str) { string r=i_str; replace(r, "/", "\\"); return r; }
#endif


// DOS/WindowsやHTTPなどにおける行デリミタ
static const string CRLF = "\x0d\x0a";


#endif	//	STLTOOL_H
