#ifdef _MSC_VER 

	// VC用の各種設定

	// テンプレート名が長い時の警告を抑制
	#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
	#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」

	// forスコープをANSI準拠させる
	#ifndef for
	#define for if(0);else for
	#endif	// for

#endif	// _MSC_VER

#include	<iostream>
#include	<exception>
#include	<stdexcept>
#include	<string>
#include	<vector>
#include	<map>
#include	<set>
#include	<fstream>
#include	<sstream>
#include	<cstdarg>
#include	<cstdio> // for sprintf
using namespace std;

typedef unsigned char byte;
typedef vector<string>	strvec;
typedef pair<string, string>	strpair;

//----------------------------------------------------
// ここでだけ使う例外オブジェクト
class util_error : public runtime_error 
{
public:
	util_error(const string& iWhat) : runtime_error(iWhat) {}
};

//----------------------------------------------------
// グローバル関数

// 文字列は数字で構成されているか？（マイナスも可）
bool	aredigits(const string& s);

// 特定の一文字が最後に出現する位置を返す
const char*	find_final_char(const char* str, char c);
inline char* find_final_char(char* str, char c) { return const_cast<char*>(find_final_char(static_cast<const char*>(str), c)); }

// 拡張子を変更したものを返す
string	set_extension(const string& str, const char* new_ext);
inline string	set_extension(const string& str, const string& new_ext) { return set_extension(str, new_ext.c_str()); }

// 文字列置換
bool	replace_first(string& str, const string& before, const string& after);
int	replace(string& str, const string& before, const string& after);

// 文字列の分割
inline vector<string> split(const string& i_dlmt, string i_target)
{
	vector<string> v;
	string::size_type found_pos;
	while ( (found_pos = i_target.find(i_dlmt)) != string::npos )
	{
		v.push_back(i_target.substr(0, found_pos) );
		i_target = i_target.substr( found_pos + i_dlmt.size() );
	}
	v.push_back(i_target);
	return v;
}

// printf互換で文字列を生成し、string型で返す。
	// unixでは_vsnprintfが使えないため、
	// この手の関数ではバッファ長(ここでは固定で8192byte)を越える可能性があります。
	// 無制限のユーザ入力など、長さが不定の文字列は渡さないでください。
string	stringf(const char* iFormat, ...);

// 空白文字か？
inline bool is_space(int c) { return c==' '||c=='\t'; }

// 単語構成文字か？
inline bool is_word_element(int c) { return isdigit(c)||isalpha(c)||c=='-'||c=='_'; }

// 整数をstringに変換
inline string itos(int n) { char buf[256]; sprintf(buf,"%d",n); return buf; }

// stringを整数に変換
inline int stoi(const string& s) { return atoi(s.c_str()); }

// 指定文字列は、指定の判定関数でtrueを返す文字のみで構成されているか？
template<typename Function>
bool are_elements(const string& s, Function f) 
{
	for ( const char* p=s.c_str() ; *p!='\0' ; ++p )
	{
		if ( !f(*p) )
		{
			return	false;
		}
	}
	return	true;
}

//-------------------------------------------------------------------

// multimapにインタフェースを追加したもの。
template<typename Key, typename Value>
class my_multimap : public multimap<Key, Value>
{
public:
	// キーを指定して値を取得。要素が無い場合は例外を投げる。
	Value get(const Key& iKey) const 
	{
		const_iterator i = lower_bound(iKey);
		if ( i==end() )
			throw util_error("strmap::get(\"" + iKey + "\"): key was not found.");
		return i->second;
	}
	
	// キーを指定して値を参照。キーが無ければキー値ペアを挿入
	Value& operator[](const Key& iKey) 
	{
		multimap<Key, Value>::iterator i = lower_bound(iKey); // ::以左が無いとparse errorと言われる。
		if ( i==end() )
		{
			i = insert( pair<Key, Value>(iKey, Value()) );
		}
		return i->second;
	}
	
	// iKeyの存在を確認
	bool exists(const Key& iKey) const 
	{
		return	find(iKey) != end();
	}
};

// キーも値も文字列であるmy_multimap。ファイルから内容を構築できる。
class strmap : public my_multimap<string, string>
{
public:
	// 指定ファイルの各行をデリミタで分割し、map化する。
	void load(const string& iFileName, int iDelimiter='=', int iComment='#');
};


//-------------------------------------------------------------------
// syslogへ出力するostream

#ifdef _MSC_VER 
	// VCコンパイル時用のダミー
	#define LOG_INFO 0
#else
	#include	<syslog.h>
#endif	// _MSC_VER

class syslog_streambuf : public streambuf 
{
	string	m_line;	// 今回の行。endlがくるまで保持。
	string	m_last_line; // 直前に出力した行。
	int	m_level; // ログの報告レベル。
public:
	syslog_streambuf() : streambuf(), m_level(LOG_INFO) {}
	virtual int overflow(int c=EOF);

	void	level(int i_level) { m_level=i_level; }
	string	last_line() { return m_last_line; }
};

class syslog_stream : public ostream 
{
	syslog_streambuf	m_buf;
public:
	syslog_stream() : ostream(&m_buf) {}
	void	level(int i_level) { m_buf.level(i_level); }
	string	last_line() { return m_buf.last_line(); }
};




//----------------------------------------------------
// グローバルオブジェクト

// デバッグ出力用ストリーム
extern	ostream&	cdbg;
// ログ出力用ストリーム。slog.level()で報告レベルを変更できる。
extern	syslog_stream	slog;

//-------------------------------------------------------------------
// 符号化/複合化

// BASE64エンコード／デコード
vector<byte> decodeBASE64(string i_encoded_str);
string encodeBASE64(const vector<byte>& i_data);

// RC4エンコード（デコードとエンコードは同一の処理）
void encodeRC4(const byte* i_buf, byte* o_buf, int i_buflen, const byte* i_key, int i_keylen);
vector<byte> encodeRC4(const vector<byte>& i_buf, const vector<byte>& i_key);
inline string encodeRC4(string i_buf, const vector<byte>& i_key)
{
	vector<byte> the_buf = decodeBASE64(i_buf);
	vector<byte> the_r_buf = encodeRC4(the_buf, i_key);
	return	string( (char*) &the_r_buf.front() , the_r_buf.size() );
}


//-------------------------------------------------------------------
// ディレクトリリスティング

// opendir, readdir, closedirのラッパ
#ifdef _MSC_VER
	
#else
	#include <sys/types.h>
	#include <dirent.h>
#endif // _MSC_VER

class directory
{
	DIR* m_dir;
public:
	directory() : m_dir(NULL) {}
	~directory() { close(); }

	bool	open(const string& i_dirname) {
		close();
		m_dir = opendir(i_dirname.c_str());
		return m_dir != NULL;
	}
	
	void	close() {
		if (m_dir != NULL) {
			closedir(m_dir);
			m_dir = NULL;
		}
	}
	
	struct dirent * read() {
		if (m_dir == NULL)
			return NULL;
		return readdir(m_dir);
	}
};

#include <sys/stat.h> // for stat

// 指定ディレクトリのファイル名とファイル情報の一覧を取得する
inline bool get_files(const string& i_directory_name, map<string, struct stat>& o_files) 
{
	directory	dir;
	if ( !dir.open(i_directory_name) )
		return	false;

	struct dirent *p;
	while ((p = dir.read()) != NULL ) 
	{
		struct stat st;
		if ( stat((i_directory_name + DIR_CHAR + p->d_name).c_str(), &st) )
			continue;
		o_files[p->d_name] = st;
	}
	
	return	true;
}

//-------------------------------------------------------------------
// メール送信

void send_a_mail(
	const string& i_host,
	unsigned short i_port, // =25
	const string& i_from,
	const string& i_to,
	const string& i_subject,
	const vector<string>& i_message);



#endif	// PROV_BATCH_UTIL_H
