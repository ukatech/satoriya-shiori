#include	"stltool.h"
#include	<sstream>
#include	<cassert>
#ifdef POSIX
#  include      "Utilities.h"
#else
#  include      "Utilities.h"
//#  include	<mbctype.h>	// for _ismbblead,_ismbbtrail
#endif

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

ostream& operator<<(ostream& o, const strvec& i) {
	for ( strvec::const_iterator p=i.begin() ; p!=i.end() ; ++p )
		o << *p << endl;
	return	o;
}

ostream& operator<<(ostream& o, const strmap& i) {
	for ( strmap::const_iterator p=i.begin() ; p!=i.end() ; ++p )
		o << p->first << "=" << p->second << endl;
	return	o;
}

ostream& operator<<(ostream& o, const strintmap& i) {
	for ( strintmap::const_iterator p=i.begin() ; p!=i.end() ; ++p )
		o << p->first << "=" << p->second << endl;
	return	o;
}
istream& operator>>(istream& i, strvec& o) {
	string	str;
	while ( getline(i, str) )
		o.push_back(str);
	return	i;
}
istream& operator>>(istream& i, strmap& o) {
	string	key, value;
	while ( getline(i, key, '=') && getline(i, value) )
		o[key] = value;
	return	i;
}
istream& operator>>(istream& i, strintmap& o) {
	string	key, value;
	while ( getline(i, key, '=') && getline(i, value) )
		o[key] = stoi(value);
	return	i;
}

// ストリームから、デリミタまでを読み込んでstringに格納
bool	getline(istream& i, string& o, int delimtier) {
	if ( i.peek() == EOF )
		return	false;	// ファイルの終端ですがな。

	int	c;	// 一文字保持
	stringstream	line;	// 行を格納するストリーム
	while ( (c=i.get()) != delimtier && c!=EOF)
		line.put(c);
	o=line.str();
	return	true;
}

bool	getline(istream& i, int& o, int delimtier) {
	if ( i.peek() == EOF )
		return	false;	// ファイルの終端ですがな。

	o=0;
	int	c;	// 一文字保持
	while ( (c=i.get()) != delimtier && c!=EOF) {
		if ( isdigit(c) ) {
			o *= 10;
			o += c-'0';
		}
	}
	return	true;
}

bool	aredigits(const char* p) {
	if ( *p=='-' )
		++p;
	if ( *p=='\0' )
		return false;
	while ( *p!='\0' )
		if ( !isdigit(*p++) )
			return	false;
	return	true;
}

bool	arealphabets(const char* p) {
	while (*p) {
		int	c = *p++;
		if ( c>='a' && c<='z' )
			continue;
		if ( c>='A' && c<='Z' )
			continue;
		if ( c==(-126) ) {	// sjisにおけるＡ-Ｚ,ａ-ｚを含む１バイト目
			c = *p++;
			if ( c>=("Ａ")[1] && c<=("Ｚ")[1] )
				continue;
			if ( c>=("ａ")[1] && c<=("ｚ")[1] )
				continue;
		}
		return	false;
	}
	return	true;
}


bool	replace_first(string& str, const string& before, const string& after) {
	int	pos = str.find(before);
	if ( pos == string::npos )
		return	false;
	str.replace(pos, before.size(), after);
	return	true;
}

/*int	replace(string& str, const string& before, const string& after) {
	int	n=0;
	while (replace_first(str, before, after))
		++n;
	return	n;
}
*/

int	replace(string& str, const char* before, const char* after) {
	if ( str=="" || before[0] == 0 ) return 0;

	// 文字列長の計算用
	const int		beforeLength = strlen(before);
	const int		afterLength = strlen(after);
	const int		textLength = str.size();
	const int		diffLength = afterLength - beforeLength;

	// 置き換え対象がいくつあるかをカウントしておく
	const char*	found=str.c_str();
	int	count=0;
	while ( (found=strstr_hz(found, before)) != NULL ) {
		found += beforeLength;
		++count;
	}
	if ( count==0 ) return 0;

	// 中間バッファを確保
	char*	buf = new char[textLength + diffLength*count + 1];

	// 置き換えループ
	const char*	pread = str.c_str();
	char*	pwrite = buf;
	found = str.c_str();
	while ( (found=strstr_hz(pread, before)) != NULL ) {
		// 対象文字列直前までをコピー
		strncpy(pwrite, pread, found-pread);
		pwrite += found-pread;
		pread = found;

		// 置き換え文字列をコピー
		strcpy(pwrite, after);
		pwrite += afterLength;
		pread += beforeLength;
	}
	// 残りをコピー
	strcpy(pwrite, pread);

	// 過去の領域を解放、バッファの実体を内容とする。
	str = buf;
	delete buf;
	return	count;
}

// 文字列消去
bool	erase_first(string& str, const string& before) {
	int	pos = str.find(before);
	if ( pos == string::npos )
		return	false;
	str.erase(pos, before.size());
	return	true;
}
int	erase(string& str, const string& before) {
	if ( str=="" || before=="" ) return 0;

	// 文字列長の計算用
	const int		beforeLength = before.size();
	const int		textLength = str.size();

	// 置き換え対象がいくつあるかをカウントしておく
	const char*	found=str.c_str();
	int	count=0;
	while ( (found=strstr_hz(found, before.c_str())) != NULL ) {
		found += beforeLength;
		++count;
	}
	if ( count==0 ) return 0;

	// 中間バッファを確保
	char*	buf = new char[textLength - beforeLength*count + 1];

	// 消去ループ
	const char*	pread = str.c_str();
	char*	pwrite = buf;
	found = str.c_str();
	while ( (found=strstr_hz(pread, before.c_str())) != NULL ) {
		strncpy(pwrite, pread, found-pread);
		pwrite += found-pread;
		pread = found+beforeLength;
	}
	// 残りをコピー
	strcpy(pwrite, pread);

	// 過去の領域を解放、バッファの実体を内容とする。
	str = buf;
	delete buf;
	return	count;
}

// 対象語句の数を数える
int	count(const string& str, const string& target) {
	const char*	found=str.c_str();
	int	count=0;
	while ( (found=strstr_hz(found, target.c_str())) != NULL ) {
		found += target.size();
		++count;
	}
	return	count;
}


// ファイルの存在を確認
bool	is_exist_file(const string& iFileName) {
	ifstream	in(iFileName.c_str());
	if ( !in.is_open() )
		return	false;
	in.close();
	return	true;
}

bool	strvec_from_file(
	strvec&	o,
	const string& iFileName)
{
	assert(!iFileName.empty());
	ifstream	in(iFileName.c_str());
	if ( !in.is_open() )
		return	false;
	while ( in.peek() != EOF ) {
		// １行読み込み
		stringstream	line;
		int	c;
		while ( (c=in.get()) != '\n' && c!=EOF) {
		    if (c != '\r') {
				line.put(c);
		    }
		}
		o.push_back( line.str() );
	}
	in.close();
	return	true;
}

bool	strvec_to_file(
	const strvec& vec,
	const string& iFileName)
{
	assert(!iFileName.empty());
	ofstream	out(iFileName.c_str());
	if ( !out.is_open() )
		return	false;
	strvec::const_iterator	it;
	for (it=vec.begin() ; it!=vec.end() ; ++it)
		out << *it << endl;
	out.close();
	return	true;
}


bool	strmap_from_file(strmap& o, const string& iFileName, const string& dlmt, const string& front_comment_mark)
{
	ifstream	in(iFileName.c_str());
	if ( !in.is_open() )
		return	false;
	while ( in.peek() != EOF )
	{
		// １行読み込み
		string line;
		int	c;
		while ( (c=in.get()) != '\n' && c!=EOF)
		{
		    if (c != '\r') {
				line += c;
		    }
		}

		if ( line.compare(0, front_comment_mark.size(), front_comment_mark)==0 )
		{
			continue;
		}

		string::size_type pos = line.find(dlmt);
		if ( pos == string::npos )
		{
			o[ line ] = string();
		}
		else
		{
			o[ line.substr(0, pos) ] = line.substr( pos + dlmt.size() );
		}
	}
	in.close();
	return	true;
}

bool	strmap_to_file(const strmap& oMap, const string& iFileName, const string& dlmt)
{
	ofstream	out(iFileName.c_str());
	if ( !out.is_open() )
		return	false;
	strmap::const_iterator	it;
	for (it=oMap.begin() ; it!=oMap.end() ; ++it)
		out << it->first << dlmt << it->second << endl;
	out.close();
	return	true;
}

bool	string_from_file(string& o, const string& iFileName) {
	ifstream	in(iFileName.c_str());
	if ( !in.is_open() )
		return	false;
	/*in.seekg(0, ios::end);
	streampos	size = in.tellg();
	in.seekg(0, ios::beg);
	in.read(out, size);*/
	while (in.peek() != EOF)
		o+=in.get();
	in.close();
	return	true;
}

bool	string_to_file(const string& i, const string& iFileName) {
	ofstream	out(iFileName.c_str());
	if ( !out.is_open() )
		return	false;
	out.write(i.c_str(), i.size());
	out.close();
	return	true;
}


string	get_a_chr(const char*& p) {
	if ( *p=='\0' )
		return	"";
	char	buf[3];
	if ( p[0] == static_cast<char>(0xffU) ) { //内部で特殊な表現としている
		buf[0]=*p++;
		buf[1]='\0';
	}
	else if ( _ismbblead(p[0]) ) {
		buf[0]=*p++;
		buf[1]=*p++;
		buf[2]='\0';
	}
	else {
		buf[0]=*p++;
		buf[1]='\0';
	}
	return	buf;
}



string	encode(const string& s) {
	const char*	p = s.c_str();
	int	len = s.size();
	string	ret;

	for ( int n=0 ; n<len/2 ; ++n ) {
		ret += p[n];
		ret += p[len-n-1];
	}
	if ( len&1 ) ret += p[len/2];

	return	ret;
}

string	decode(const string& s) {
	const char*	p = s.c_str();
	int	len = s.size();
	string	ret;

	for ( int n=0 ; n<len ; n+=2 ) ret += p[n];
	for ( int n=len-((len&1)?2:1) ; n>=0 ; n-=2 ) ret += p[n];

	return	ret;
}
/*

string	encode(const string& in) {
	int	len = in.size();
	char*	buf = new char[len+1];
	buf[len]='\0';
	
	const char*	i = in.c_str();
	char*	o = buf;

	for ( int n=0 ; n<len/2 ; ++n ) {
		*o++ += i[n];
		*o++ += i[len-n-1];
	}
	if ( len&1 )
		*o++ += i[len/2];

	string	ret = buf;
	delete [] buf;
	return	ret;
}

string	decode(const string& in) {
	int	len = in.size();
	char*	buf = new char[len+1];
	buf[len]='\0';
	
	const char*	i = in.c_str();
	char*	o = buf;

	for ( int n=0 ; n<len ; n+=2 )
		*o++ += i[n];
	for ( n=len-((len&1)?2:1) ; n>=0 ; n-=2 )
		*o++ += i[n];

	string	ret = buf;
	delete [] buf;
	return	ret;
}
*/

//SJIS判定つきstrstr
const char*	strstr_hz(const char* target, const char* find) {
	int	len=strlen(find);
	const char* p=target;
	while ( *p!='\0' ) {
		if ( strncmp(p, find, len)==0 )
			return	p;
		if ( _ismbblead(*p) )
			p+=2; 
		else
			++p;
	}
	return	NULL;
}

//STLスタイルのstrstr_hz
std::string::size_type find_hz(const char* str, const char* target, std::string::size_type find_pos)
{
	const char *p = strstr_hz(str+find_pos, target);
	if ( p == NULL ) {
		return string::npos;
	}
	else {
		p -= find_pos;
		return p - str;
	}
}

bool	compare_head_s(const char* str, const char* head)
{
	if ( ! str || ! head || str[0] == 0 || head[0] == 0 ) { return false; }
	return strncmp(str, head, strlen(head))==0;
}

bool	compare_head_nocase_s(const char* str, const char* head)
{
	if ( ! str || ! head || str[0] == 0 || head[0] == 0 ) { return false; }
#ifdef _MSC_VER 
	return _strnicmp(str, head, strlen(head))==0;
#else
	return strnicmp(str, head, strlen(head))==0;
#endif
}

bool	compare_tail_s(const char* str, const char* tail)
{
	size_t len = strlen(tail);
	size_t str_len = strlen(str);

	const int diff = str_len-len;
	if ( diff < 0 ) {
		return	false;
	}

	return strcmp(str+diff, tail)==0;
}

bool	compare_tail_nocase_s(const char* str, const char* tail)
{
	size_t len = strlen(tail);
	size_t str_len = strlen(str);

	const int diff = str_len-len;
	if ( diff < 0 ) {
		return	false;
	}

	return stricmp(str+diff, tail)==0;
}

inline int charactor_to_binary(char c) {
	if ( c>='0' && c<='9' )
		return	c-'0';
	else if ( c>='a' && c<='f' )
		return	c-'a'+10;
	else if ( c>='A' && c<='F' )
		return	c-'A'+10;
	else {
		assert(0);
		return	0;
	}
}

void	string_to_binary(const string& iString, byte* oArray) {
	int	len = iString.size()/2;
	for ( int i=0 ; i<len ; ++i)
		oArray[i] = charactor_to_binary(iString[i*2])*16 + charactor_to_binary(iString[i*2+1]);
}


inline char binary_to_charactor(int b) {
	if ( b<10 )
		return	'0'+b;
	else if ( b<16 )
		return	'a'+(b-10);
	else {
		assert(0);
		return	0;
	}
}

string	binary_to_string(const byte* iArray, int iLength) {
	char*	buf = new char[iLength*2];
	for ( int i=0 ; i<iLength ; ++i) {
		buf[i*2] = binary_to_charactor(iArray[i]/16);
		buf[i*2+1] = binary_to_charactor(iArray[i]%16);
	}
	string	str(buf, iLength*2);
	delete [] buf;
	return	str;
}

void	xor_filter(byte* ioArray, int iLength, byte iXorValue) {
	for ( int i=0 ; i<iLength ; ++i)
		ioArray[i] ^= iXorValue;
}

const char*	find_final_char(const char* str, char c) {
	const char*	last=NULL, *p=str;
	for (;*p; p+=_ismbblead(*p)?2:1)
		if ( *p==c )
			last=p;
	return	last;
}

string	get_folder_name(const string& str) {
	char*	buf = new char[str.size()+1];
	strcpy(buf, str.c_str());
	char*	p = find_final_char(buf, DIR_CHAR);
	string	ret;
	if ( p==NULL )
		ret = "";
	else {
		*p='\0';
		ret = buf;
	}
	delete [] buf;
	return	ret;
}

string	get_file_name(const string& str) {
	const char*	p = find_final_char(str.c_str(), DIR_CHAR);
	return	( p==NULL ) ? str : p+1;
}

string	get_extention(const string& str) {
	const char*	p = find_final_char(str.c_str(), '.');
	return	( p==NULL ) ? "" : p+1;
}

string	set_extention(const string& str, const char* new_ext) {
	const char*	p = find_final_char(str.c_str(), '.');
	if ( p==NULL )
		if ( new_ext==NULL )
			return	str;
		else
			return	str+"."+new_ext;
	else
		if ( new_ext==NULL )
			return	string(str.c_str(), p-str.c_str());
		else
			return	string(str.c_str(), p-str.c_str()+1)+new_ext;
}

string	set_filename(const string& str, const char* new_filename) {
	const char*	p = find_final_char(str.c_str(), DIR_CHAR);
	if ( p==NULL )
		if ( new_filename==NULL )
			return	str;
		else
			return	str+DIR_CHAR+new_filename;
	else
		if ( new_filename==NULL )
			return	string(str.c_str(), p-str.c_str());
		else
			return	string(str.c_str(), p-str.c_str()+1)+new_filename;
}



// .iniファイルを読み込み
bool	inimap::load(const string& iFileName) {
	this->clear();

	// ファイルを開く
	ifstream	in(iFileName.c_str());
	if ( !in.is_open() )
		return	false;
	// 現在のセクションへのイテレータ
	inimap::iterator	theSection = this->end();

	// 各行に対し処理
	while ( in.peek() != EOF ) {

		string	str;	// このループで扱う行文字列

		{// １行読み込んでstrに格納
			stringstream	line;
			int	c;
			while ( (c=in.get()) != '\n' && c!=EOF) {
			    if (c != '\r') {
					line.put(c);
			    }
			}
			str = line.str();
		}

		if ( str.empty() || str[0]==';' ) {
			// 空行またはコメント行
		}
		else if ( str.size()>=2 && str[0]=='[' ) {
			// セクション名の設定行 [SectionName]
			string::size_type	end_pos = str.find(']', 1);
			if ( end_pos == string::npos )
				return	false;	// 閉じカッコの無い大カッコを発見、異常とみなす
			string	section_name = str.substr(1, end_pos-1); // セクション名取得
			pair<inimap::iterator, bool> result = 
				this->insert( inimap::value_type(section_name, strmap()) ); // mapに挿入
			theSection = result.first;	// 現在のセクションを指すイテレータを取得
		}
		else {
			// 通常の行 key=value
			string::size_type	eq_pos = str.find('=', 0);
			if ( eq_pos == string::npos )
				continue;	// []も=も無く空行でもない妙な行

			// theSectionが未設定のままここに来たときは無名のセクションを設定
			if (theSection == this->end())
			{
				pair<inimap::iterator, bool> result = 
					this->insert( inimap::value_type("", strmap()) ); // mapに挿入
				theSection = result.first;	// 現在のセクションを指すイテレータを取得
			}

			// mapに挿入
			string	key = str.substr(0, eq_pos);
			string	value = str.substr(eq_pos+1, string::npos);
			theSection->second[key] = value;
		}
	}
	return	true;
}

// .iniファイルへ保存
bool	inimap::save(const string& iFileName) const {

	// ファイルを開く
	ofstream	out(iFileName.c_str());
	if ( !out.is_open() )
		return	false;

	for (inimap::const_iterator i=this->begin() ; i!=this->end() ; ++i)
	{
		out << "[" << i->first << "]" << endl;	// [SectionName]を出力
		for (strmap::const_iterator j=i->second.begin() ; j!=i->second.end() ; ++j)
			out << j->first << "=" << j->second << endl;	// key=valueを出力
	}

	return	true;
}


// .iniファイルを読み込み
bool	inivec::load(const string& iFileName) {
	this->clear();

	// ファイルを開く
	ifstream	in(iFileName.c_str());
	if ( !in.is_open() )
		return	false;
	// 現在のセクションへのイテレータ
	iterator	theSection = this->end();

	// 各行に対し処理
	while ( in.peek() != EOF ) {

		string	str;	// このループで扱う行文字列

		{// １行読み込んでstrに格納
			int	c;
			while ( (c=in.get()) != '\n' && c!=EOF) {
			    if (c != '\r') {
					str += c;
			    }
			}
		}

		if ( str.empty() ) {
			// 空行
		}
		else if ( str.size()>=2 && str[0]=='[' ) {
			// セクション名の設定行 [SectionName]
			string::size_type	end_pos = str.find(']', 1);
			if ( end_pos == string::npos )
				return	false;	// 閉じカッコの無い大カッコを発見、異常とみなす
			string	section_name = str.substr(1, end_pos-1); // セクション名取得
			
			this->push_back( value_type(section_name, strpairvec()) ); // vectorに追加
			--(theSection = this->end());	// 現在のセクションを指すイテレータを取得
		}
		else {
			// 通常の行 key=value
			string::size_type	eq_pos = str.find('=', 0);
			if ( eq_pos == string::npos )
				continue;	// []も=も無く空行でもない妙な行

			// theSectionが未設定のままここに来たときは無名のセクションを設定
			if (theSection == this->end())
			{
				this->push_back( value_type("", strpairvec()) ); // vectorに追加
				--(theSection = this->end());	// 現在のセクションを指すイテレータを取得
			}
	
			string	key = str.substr(0, eq_pos);
			string	value = str.substr(eq_pos+1, string::npos);
			theSection->second.push_back( strpair(key, value) );
		}
	}
	return	true;
}

// printf互換で文字列を生成し、string型で返す。
#include	<cstdarg>
string	stringf(const char* iFormat, ...) {
	static const int BUF_SIZE = 4096;
	char	buf[BUF_SIZE];
	va_list	argptr;
	va_start(argptr, iFormat);
#ifdef POSIX
#  define _vsnprintf vsnprintf
#endif
	_vsnprintf(buf, BUF_SIZE-1, iFormat, argptr);
	va_end(argptr);
	return	buf;
}

string	zen2han(const char *s)
{
	string str(s);

	static const char	before[] = "０１２３４５６７８９ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ－＋";
	static const char	after[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-+";
	char	buf1[3]="\0\0", buf2[2]="\0";
	for (int n=0 ; n<sizeof(after) ; ++n) {
		buf1[0]=before[n*2];
		buf1[1]=before[n*2+1];
		buf2[0]=after[n];
		replace(str, buf1, buf2);
	}

	return	str;
}

string int2zen(int i) {
	static const char*	ary[] = {"０","１","２","３","４","５","６","７","８","９"};
	
	string	zen;
	if ( i<0 ) {
		zen += "－";
		i = -i; // INT_MINの時は符号が反転しない
	}
	string	han=itos(i);
	const char* p=han.c_str();
	if ( i==INT_MIN )
		++p;
	for (  ; *p != '\0' ; ++p ) {
		assert(*p>='0' && *p<='9');
		zen += ary[*p-'0'];
	}
	return	zen;
}

int zen2int(const char *str)
{
	return stoi(zen2han(str));
}


