#include	"stltool.h"
#include	<sstream>
#include	<cassert>
#include	"charset.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

std::wostream& operator<<(std::wostream& o, const strvec& i) {
	for ( strvec::const_iterator p=i.begin() ; p!=i.end() ; ++p )
		o << *p << std::endl;
	return	o;
}

std::wostream& operator<<(std::wostream& o, const strmap& i) {
	for ( strmap::const_iterator p=i.begin() ; p!=i.end() ; ++p )
		o << p->first << L"=" << p->second << std::endl;
	return	o;
}

std::wostream& operator<<(std::wostream& o, const strintmap& i) {
	for ( strintmap::const_iterator p=i.begin() ; p!=i.end() ; ++p )
		o << p->first << L"=" << p->second << std::endl;
	return	o;
}

bool	aredigits(const wchar_t* p) {
	if ( *p==L'-' )
		++p;
	if ( *p==L'\0' )
		return false;
	while ( *p!=L'\0' ) {
		if ( *p<L'0' || *p>L'9' )
			return	false;
		++p;
	}
	return	true;
}

bool	arealphabets(const wchar_t* p) {
	while (*p) {
		wchar_t	c = *p++;
		if ( c>=L'a' && c<=L'z' )
			continue;
		if ( c>=L'A' && c<=L'Z' )
			continue;
		if ( c>=L'Ａ' && c<=L'Ｚ' )
			continue;
		if ( c>=L'ａ' && c<=L'ｚ' )
			continue;
		return	false;
	}
	return	true;
}


bool	replace_first(wstring& str, const wstring& before, const wstring& after) {
	wstring::size_type	pos = str.find(before);
	if ( pos == wstring::npos )
		return	false;
	str.replace(pos, before.size(), after);
	return	true;
}

int	replace(wstring& str, const wchar_t* before, const wchar_t* after) {
	if ( str.empty() || before[0] == 0 ) return 0;

	const wstring::size_type	beforeLength = wcslen(before);

	wstring::size_type	pos = str.find(before);
	if ( pos == wstring::npos ) return 0;

	wstring	result;
	result.reserve(str.size());
	wstring::size_type	start = 0;
	int	count=0;
	while ( pos != wstring::npos ) {
		result.append(str, start, pos-start);
		result += after;
		start = pos + beforeLength;
		pos = str.find(before, start);
		++count;
	}
	result.append(str, start, wstring::npos);
	str = result;
	return	count;
}

// 文字列消去
bool	erase_first(wstring& str, const wstring& before) {
	wstring::size_type	pos = str.find(before);
	if ( pos == wstring::npos )
		return	false;
	str.erase(pos, before.size());
	return	true;
}
int	erase_all(wstring& str, const wstring& before) {
	if ( str.empty() || before.empty() ) return 0;
	return	replace(str, before.c_str(), L"");
}

// 対象語句の数を数える
int	count(const wstring& str, const wstring& target) {
	if ( target.empty() ) return 0;
	int	count=0;
	wstring::size_type	pos = str.find(target);
	while ( pos != wstring::npos ) {
		++count;
		pos = str.find(target, pos + target.size());
	}
	return	count;
}


FILE*	w_fopen(const wstring& iFileName, const wchar_t* iMode) {
#ifdef POSIX
	return	fopen(WtoUTF8(iFileName).c_str(), WtoUTF8(iMode).c_str());
#else
	return	_wfopen(iFileName.c_str(), iMode);
#endif
}

// ファイルの存在を確認
bool	is_exist_file(const wstring& iFileName) {
	FILE*	fp = w_fopen(iFileName, L"rb");
	if ( fp == NULL )
		return	false;
	fclose(fp);
	return	true;
}

bool	bytes_from_file(std::string& o, const wstring& iFileName) {
	assert(!iFileName.empty());
	FILE*	fp = w_fopen(iFileName, L"rb");
	if ( fp == NULL )
		return	false;
	char	buf[4096];
	size_t	n;
	while ( (n = fread(buf, 1, sizeof(buf), fp)) > 0 ) {
		o.append(buf, n);
	}
	fclose(fp);
	return	true;
}

bool	bytes_to_file(const std::string& i, const wstring& iFileName) {
	assert(!iFileName.empty());
	FILE*	fp = w_fopen(iFileName, L"wb");
	if ( fp == NULL )
		return	false;
	bool	ok = ( fwrite(i.c_str(), 1, i.size(), fp) == i.size() );
	if ( fclose(fp) != 0 )
		ok = false;
	return	ok;
}

void	split_lines(const std::string& i, std::vector<std::string>& o) {
	std::string::size_type	start = 0;
	while ( start < i.size() ) {
		std::string::size_type	end = i.find('\n', start);
		std::string::size_type	next;
		if ( end == std::string::npos ) {
			end = i.size();
			next = end;
		}
		else {
			next = end + 1;
		}
		std::string::size_type	line_end = end;
		if ( line_end > start && i[line_end-1] == '\r' )
			--line_end;
		o.push_back(i.substr(start, line_end-start));
		start = next;
	}
}

CharactorSet	detect_lines_charset(std::vector<std::string>& io) {
	if ( io.empty() )
		return	CS_UTF8;
	if ( HasUTF8BOM(io[0]) ) {
		io[0].erase(0, 3);
		return	CS_UTF8;
	}
	for ( std::vector<std::string>::const_iterator i=io.begin() ; i!=io.end() ; ++i ) {
		if ( !IsValidUTF8(*i) )
			return	CS_SJIS;
	}
	return	CS_UTF8;
}

void	lines_to_strvec(std::vector<std::string>& i, strvec& o, CharactorSet cs) {
	if ( cs == CS_NULL ) {
		cs = detect_lines_charset(i);
	}
	else if ( cs == CS_UTF8 && !i.empty() && HasUTF8BOM(i[0]) ) {
		i[0].erase(0, 3);
	}
	o.reserve(o.size() + i.size());
	for ( std::vector<std::string>::const_iterator it=i.begin() ; it!=i.end() ; ++it ) {
		o.push_back(MBtoW(*it, cs));
	}
}

bool	strvec_from_file(
	strvec&	o,
	const wstring& iFileName,
	CharactorSet cs)
{
	std::string	bytes;
	if ( !bytes_from_file(bytes, iFileName) )
		return	false;
	std::vector<std::string>	lines;
	split_lines(bytes, lines);
	lines_to_strvec(lines, o, cs);
	return	true;
}

bool	strvec_to_file(
	const strvec& vec,
	const wstring& iFileName,
	CharactorSet cs)
{
	std::string	bytes;
	for ( strvec::const_iterator it=vec.begin() ; it!=vec.end() ; ++it ) {
		bytes += WtoMB(*it, cs);
		bytes += FILE_NEWLINE;
	}
	return	bytes_to_file(bytes, iFileName);
}


bool	strmap_from_file(strmap& o, const wstring& iFileName, const wstring& dlmt, const wstring& front_comment_mark, CharactorSet cs)
{
	strvec	lines;
	if ( !strvec_from_file(lines, iFileName, cs) )
		return	false;

	for ( strvec::const_iterator it=lines.begin() ; it!=lines.end() ; ++it )
	{
		const wstring&	line = *it;
		if ( line.compare(0, front_comment_mark.size(), front_comment_mark)==0 )
		{
			continue;
		}

		wstring::size_type pos = line.find(dlmt);
		if ( pos == wstring::npos )
		{
			o[ line ] = wstring();
		}
		else
		{
			o[ line.substr(0, pos) ] = line.substr( pos + dlmt.size() );
		}
	}
	return	true;
}

bool	strmap_to_file(const strmap& oMap, const wstring& iFileName, const wstring& dlmt, CharactorSet cs)
{
	strvec	lines;
	for ( strmap::const_iterator it=oMap.begin() ; it!=oMap.end() ; ++it )
		lines.push_back(it->first + dlmt + it->second);
	return	strvec_to_file(lines, iFileName, cs);
}


wstring	get_a_chr(const wchar_t*& p) {
	if ( *p==L'\0' )
		return	L"";
	const wchar_t*	start = p++;
	if ( IsHighSurrogate(*start) && IsLowSurrogate(*p) )
		++p;
	return	wstring(start, p);
}

size_t	count_chars(const wstring& str) {
	size_t	n = 0;
	for ( wstring::size_type i=0 ; i<str.size() ; ++i ) {
		if ( !IsLowSurrogate(str[i]) || i==0 || !IsHighSurrogate(str[i-1]) )
			++n;
	}
	return	n;
}

size_t	char_pos_to_index(const wstring& str, size_t char_pos) {
	wstring::size_type	i = 0;
	while ( char_pos > 0 && i < str.size() ) {
		if ( IsHighSurrogate(str[i]) && i+1 < str.size() && IsLowSurrogate(str[i+1]) )
			i += 2;
		else
			i += 1;
		--char_pos;
	}
	return	i;
}


std::string	encode(const std::string& s) {
	const char*	p = s.c_str();
	int	len = s.size();
	std::string	ret;

	for ( int n=0 ; n<len/2 ; ++n ) {
		ret += p[n];
		ret += p[len-n-1];
	}
	if ( len&1 ) ret += p[len/2];

	return	ret;
}

std::string	decode(const std::string& s) {
	const char*	p = s.c_str();
	int	len = s.size();
	std::string	ret;

	for ( int n=0 ; n<len ; n+=2 ) ret += p[n];
	for ( int m=len-((len&1)?2:1) ; m>=0 ; m-=2 ) ret += p[m];

	return	ret;
}

const wchar_t*	strstr_hz(const wchar_t* target, const wchar_t* find) {
	return	wcsstr(target, find);
}
const wchar_t*	strstri_hz(const wchar_t* target, const wchar_t* find) {
	size_t	len=wcslen(find);
	const wchar_t* p=target;
	while ( *p!=L'\0' ) {
		if ( _wcsnicmp(p, find, len)==0 )
			return	p;
		++p;
	}
	return	NULL;
}

//STLスタイルのstrstr_hz
std::wstring::size_type find_hz(const wchar_t* str, const wchar_t* target, std::wstring::size_type find_pos)
{
	const wchar_t *p = strstr_hz(str+find_pos, target);
	if ( p == NULL ) {
		return wstring::npos;
	}
	else {
		return p - str;
	}
}

bool	compare_head_s(const wchar_t* str, const wchar_t* head)
{
	if ( ! str || ! head || str[0] == 0 || head[0] == 0 ) { return false; }
	return wcsncmp(str, head, wcslen(head))==0;
}

bool	compare_head_nocase_s(const wchar_t* str, const wchar_t* head)
{
	if ( ! str || ! head || str[0] == 0 || head[0] == 0 ) { return false; }
	return _wcsnicmp(str, head, wcslen(head))==0;
}

bool	compare_tail_s(const wchar_t* str, const wchar_t* tail)
{
	size_t len = wcslen(tail);
	size_t str_len = wcslen(str);

	const int diff = str_len-len;
	if ( diff < 0 ) {
		return	false;
	}

	return wcscmp(str+diff, tail)==0;
}

bool	compare_tail_nocase_s(const wchar_t* str, const wchar_t* tail)
{
	size_t len = wcslen(tail);
	size_t str_len = wcslen(str);

	const int diff = str_len-len;
	if ( diff < 0 ) {
		return	false;
	}

	return _wcsicmp(str+diff, tail)==0;
}

const wchar_t*	find_final_char(const wchar_t* str, wchar_t c) {
	return	wcsrchr(str, c);
}

wstring	get_folder_name(const wstring& str) {
	wstring::size_type	pos = str.rfind(DIR_CHAR);
	if ( pos == wstring::npos )
		return	L"";
	return	str.substr(0, pos);
}

wstring	get_file_name(const wstring& str) {
	const wchar_t*	p = find_final_char(str.c_str(), DIR_CHAR);
	return	( p==NULL ) ? str : p+1;
}

wstring	get_extention(const wstring& str) {
	const wchar_t*	p = find_final_char(str.c_str(), L'.');
	return	( p==NULL ) ? L"" : p+1;
}

wstring	set_extention(const wstring& str, const wchar_t* new_ext) {
	const wchar_t*	p = find_final_char(str.c_str(), L'.');
	if ( p==NULL )
		if ( new_ext==NULL )
			return	str;
		else
			return	str+L"."+new_ext;
	else
		if ( new_ext==NULL )
			return	wstring(str.c_str(), p-str.c_str());
		else
			return	wstring(str.c_str(), p-str.c_str()+1)+new_ext;
}

wstring	set_filename(const wstring& str, const wchar_t* new_filename) {
	const wchar_t*	p = find_final_char(str.c_str(), DIR_CHAR);
	if ( p==NULL )
		if ( new_filename==NULL )
			return	str;
		else
			return	str+DIR_CHAR+new_filename;
	else
		if ( new_filename==NULL )
			return	wstring(str.c_str(), p-str.c_str());
		else
			return	wstring(str.c_str(), p-str.c_str()+1)+new_filename;
}



// .iniファイルを読み込み
bool	inimap::load(const wstring& iFileName, CharactorSet cs) {
	this->clear();

	strvec	lines;
	if ( !strvec_from_file(lines, iFileName, cs) )
		return	false;
	// 現在のセクションへのイテレータ
	inimap::iterator	theSection = this->end();

	// 各行に対し処理
	for ( strvec::const_iterator it=lines.begin() ; it!=lines.end() ; ++it ) {

		const wstring&	str = *it;	// このループで扱う行文字列

		if ( str.empty() || str[0]==L';' ) {
			// 空行またはコメント行
		}
		else if ( str.size()>=2 && str[0]==L'[' ) {
			// セクション名の設定行 [SectionName]
			wstring::size_type	end_pos = str.find(L']', 1);
			if ( end_pos == wstring::npos )
				return	false;	// 閉じカッコの無い大カッコを発見、異常とみなす
			wstring	section_name = str.substr(1, end_pos-1); // セクション名取得
			std::pair<inimap::iterator, bool> result = 
				this->insert( inimap::value_type(section_name, strmap()) ); // mapに挿入
			theSection = result.first;	// 現在のセクションを指すイテレータを取得
		}
		else {
			// 通常の行 key=value
			wstring::size_type	eq_pos = str.find(L'=', 0);
			if ( eq_pos == wstring::npos )
				continue;	// []も=も無く空行でもない妙な行

			// theSectionが未設定のままここに来たときは無名のセクションを設定
			if (theSection == this->end())
			{
				std::pair<inimap::iterator, bool> result = 
					this->insert( inimap::value_type(L"", strmap()) ); // mapに挿入
				theSection = result.first;	// 現在のセクションを指すイテレータを取得
			}

			// mapに挿入
			wstring	key = str.substr(0, eq_pos);
			wstring	value = str.substr(eq_pos+1, wstring::npos);
			theSection->second[key] = value;
		}
	}
	return	true;
}

// .iniファイルへ保存
bool	inimap::save(const wstring& iFileName, CharactorSet cs) const {
	strvec	lines;
	for (inimap::const_iterator i=this->begin() ; i!=this->end() ; ++i)
	{
		lines.push_back(L"[" + i->first + L"]");	// [SectionName]を出力
		for (strmap::const_iterator j=i->second.begin() ; j!=i->second.end() ; ++j)
			lines.push_back(j->first + L"=" + j->second);	// key=valueを出力
	}
	return	strvec_to_file(lines, iFileName, cs);
}


wstring	zen2han(const wchar_t *s)
{
	wstring str(s);

	for ( wstring::iterator i=str.begin() ; i!=str.end() ; ++i ) {
		wchar_t	c = *i;
		if ( c>=L'０' && c<=L'９' )
			*i = c - L'０' + L'0';
		else if ( c>=L'Ａ' && c<=L'Ｚ' )
			*i = c - L'Ａ' + L'A';
		else if ( c>=L'ａ' && c<=L'ｚ' )
			*i = c - L'ａ' + L'a';
		else if ( c==L'－' || c==0x2212 )	// FULLWIDTH HYPHEN-MINUS(CP932の0x817C) と MINUS SIGN
			*i = L'-';
		else if ( c==L'＋' )
			*i = L'+';
	}

	return	str;
}

wstring int2zen(int i) {
	static const wchar_t*	ary[] = {L"０",L"１",L"２",L"３",L"４",L"５",L"６",L"７",L"８",L"９"};
	
	wstring	zen;
	if ( i<0 ) {
		zen += L"－";
		i = -i; // INT_MINの時は符号が反転しない
	}
	wstring	han=itos(i);
	const wchar_t* p=han.c_str();
	if ( i==INT_MIN )
		++p;
	for (  ; *p != L'\0' ; ++p ) {
		assert(*p>=L'0' && *p<=L'9');
		zen += ary[*p-L'0'];
	}
	return	zen;
}

wstring ul2zen(unsigned long i) {
	static const wchar_t*	ary[] = { L"０", L"１", L"２", L"３", L"４", L"５", L"６", L"７", L"８", L"９" };

	wstring	zen;

	wstring	han = uitos(i);
	const wchar_t* p = han.c_str();

	for (; *p != L'\0'; ++p) {
		assert(*p >= L'0' && *p <= L'9');
		zen += ary[*p - L'0'];
	}
	return	zen;
}

int zen2int(const wchar_t *str)
{
	return stoi_internal(zen2han(str));
}

unsigned long zen2ul(const wchar_t *str)
{
	return stoui(zen2han(str));
}

