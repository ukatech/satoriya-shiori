#include	"utils.h"

// グローバルオブジェクトの実体
ostream&	cdbg = cerr;
syslog_stream	slog;

string	stringf(const char* iFormat, ...) 
{
	char	buf[8192];
	va_list	argptr;
	va_start(argptr, iFormat);
	vsprintf(buf, iFormat, argptr);
	va_end(argptr);
	return	buf;
}

void strmap::load(const string& iFileName, int iDelimiter, int iComment) 
{
	cdbg << "strmap::load(\"" << iFileName << "\", " << iDelimiter << ", " << iComment << ")" << endl;
	const string BASE = iFileName + ": ";

	// ファイルを開く
	ifstream	in(iFileName.c_str());
	if ( !in.is_open() )
		throw util_error(BASE + "file can't open");

	// 各行に対し処理
	int	line_number = 0; // 作業中の物理行
	while ( in.peek() != EOF ) {
		string	line;
		
		// 今回扱う一論理行を取得
		while (true) 
		{
			// \r/\n/EOFまでを取得
			int	c;
			while ( (c=in.get())!='\r' && c!='\n' && c!=EOF)
			{
				line += c;
			}
			line_number += 1;

			// 行デリミタが\rだったとき、次の文字が\nなら、それを読み飛ばしておく
			if ( c=='\r' && in.peek()=='\n' )
			{
				c = in.get();
			}

			// 行末が\\であり、かつ行デリミタがEOFでなければ\nを付加して次行を接続する。
			if ( !line.empty() && line[line.size()-1] == DIR_CHAR && c!=EOF )
			{
				line[line.size()-1] = '\n';
				continue;
			}
			break;	
		}
		const string BASE = iFileName + ": line " + itos(line_number) + ": ";
		//cdbg << line_number << ": " << line << endl;
		
		
		// デリミタ文字、コメント文字、それぞれの位置を取得
		string::size_type	dlm_pos = line.find(iDelimiter);
		string::size_type	cmt_pos = line.find(iComment);
		
		// コメント文字があり、かつデリミタ文字が無いorデリミタ文字より左にある場合
		if ( cmt_pos != string::npos && (dlm_pos == string::npos || cmt_pos < dlm_pos ) )
		{
			line = line.substr(0, cmt_pos); // コメント文字以降を削除
		}
		
		// この時点で「空行」「キー=値」「それ以外（エラー）」のいずれかになっている。
		
		// 空行なら次の行へ
		if ( are_elements(line, is_space) )
		{
			continue;
		}
		
		// デリミタ文字が無いならエラー
		if ( dlm_pos == string::npos )
			throw util_error(BASE + "delimiter not found.");
		
		// デリミタ文字の左をキー、右を値とする
		string key = line.substr(0, dlm_pos);
		string value = line.substr(dlm_pos+1);
		
		// キーの前後の空白を削る
		{
			int n;
			for (n=0 ; n<key.size() && is_space(key[n]) ; ++n)
				NULL;
			key = key.substr(n);
			for (n=key.size()-1 ; n>=0 && is_space(key[n]) ; --n)
				NULL;
			key = key.substr(0, n+1);
		}
		
		// キー文字列の正当性を確認
		{
			if ( key.empty() )
				throw util_error( BASE + "key is empty. " );
			if ( !are_elements(key, is_word_element) )
				throw util_error( BASE + "invalid key." );
		}

		// 項目追加
		//(*this)[key] = value;
		insert( pair<string, string>(key, value) );
		
		//cdbg << "strmap::load " << key << "=\"" << value << "\"" << endl;
	}
	
}



int syslog_streambuf::overflow(int c) 
{
	// マルチバイト文字は考慮していません

	if ( c=='\n' ) // endl
	{
		cdbg << "syslog: " << m_line << endl;
		syslog(m_level, "%s", m_line.c_str());
		m_level = LOG_INFO;

		m_last_line = m_line;
		m_line = "";
	}
	else
	{
		m_line += c;
	}
	return	c;
}

const char*	find_final_char(const char* str, char c) {
	const char*	last=NULL, *p=str;
//	for (;*p; p+=_ismbblead(*p)?2:1)
	for (;*p; p+=1)
		if ( *p==c )
			last=p;
	return	last;
}


string	set_extension(const string& str, const char* new_ext) {
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

bool	replace_first(string& str, const string& before, const string& after) {
	int	pos = str.find(before);
	if ( pos == string::npos )
		return	false;
	str.replace(pos, before.size(), after);
	return	true;
}

int	replace(string& str, const string& before, const string& after) {
	if ( str=="" || before=="" ) return 0;

	// 文字列長の計算用
	const int		beforeLength = before.size();
	const int		afterLength = after.size();
	const int		textLength = str.size();
	const int		diffLength = afterLength - beforeLength;

	// 置き換え対象がいくつあるかをカウントしておく
	const char*	found=str.c_str();
	int	count=0;
	while ( (found=strstr(found, before.c_str())) != NULL ) {
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
	while ( (found=strstr(pread, before.c_str())) != NULL ) {
		// 対象文字列直前までをコピー
		strncpy(pwrite, pread, found-pread);
		pwrite += found-pread;
		pread = found;

		// 置き換え文字列をコピー
		strcpy(pwrite, after.c_str());
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


// BASE64エンコード
string encodeBASE64(const vector<byte>& i_data)
{
	static const char table[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	string	r;
	int len = i_data.size();
	
	// 24bitを4文字に変換
	const unsigned char *p = (byte*)(&i_data.front());
	while (len > 0)
	{
		// 1文字目 1-6bit  xxxxxx--:--------:--------
		r += table[*p>>2];
		
		// 2文字目 7-12bit ------xx:xxxx----:--------
		if ( len-1 > 0 )
			r += table[((*p & 0x3)<<4) | ((int)(*++p & 0xF0)>>4)];
		else
			r += table[((*p & 0x3)<<4) ];
		
		len--;
		
		// 3文字目 13-18bit --------:----xxxx:xx------
		if ( len > 0 )
			if ( len-1 > 0 )
				r += table[((*p & 0xF)<<2) | ((int)(*++p & 0xC0)>>6)];
			else
				r += table[((*p & 0xF)<<2)];
		else
			r += '=';
		
		// 4文字目 19-24bit --------:--------:--xxxxxx
		r += (--len>0? table[*p & 0x3F]: '=');
		
		if(--len>0) p++;
	}
	return	r;
}



// BASE64デコード
vector<byte> decodeBASE64(string i_encoded_str)
{
	static const byte reverse_64[] = {
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x00 - 0x0F
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   // 0x10 - 0x1F
	      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,   // 0x20 - 0x2F
	     52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,   // 0x30 - 0x3F
	      0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,   // 0x40 - 0x4F
	     15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,   // 0x50 - 0x5F
	      0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,   // 0x60 - 0x6F
	     41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0    // 0x70 - 0x7F
	};

	const int len = i_encoded_str.length();
	const byte* p = (const byte*)(i_encoded_str.c_str());
	vector<byte> r;
	while (*p!='=')
	{
		if (*p=='\0' || *(p+1)=='=') break;
        r.push_back(reverse_64[*p]<<2 | reverse_64[*++p]>>4);
		if (*p=='\0' || *(p+1)=='=') break;
        r.push_back(reverse_64[*p]<<4 | reverse_64[*++p]>>2);
		if (*p=='\0' || *(p+1)=='=') break;
        r.push_back(reverse_64[*p]<<6 | reverse_64[*++p]);
		if (*p=='\0' || *(p+1)=='=') break;
		++p;
    }
    return r;
}



// RC4エンコード兼デコード（同一の処理）
#include "openssl/e_os2.h"
#include <openssl/rc4.h>
void encodeRC4(
	const byte* i_buf,
	byte* o_buf,
	int i_buflen,
	const byte* i_key,
	int i_keylen)
{
	RC4_KEY key;
	RC4_set_key(&key, i_keylen, i_key);
	memset(o_buf, 0x00, i_buflen);
	RC4(&key, i_buflen, i_buf, o_buf);
}

vector<byte> encodeRC4(const vector<byte>& i_buf, const vector<byte>& i_key)
{
	vector<byte> r(i_buf.size(), 0);
	encodeRC4(&i_buf.front(), &r.front(), i_buf.size(),  &i_key.front(), i_key.size());
	return r;
}


bool	aredigits(const string& s) {
	const char* p = s.c_str();
	if ( *p=='-' )
		++p;
	if ( *p=='\0' )
		return false;
	while ( *p!='\0' )
		if ( !isdigit(*p++) )
			return	false;
	return	true;
}


//-------------------------------------------------------------------
// 文字コードの変換

typedef unsigned char byte;
typedef unsigned short word;

// JISコードのエスケープ文字列
static const char* SHIFT2B = "\x1b\x24\x42"; // ２バイトへ
static const char* SHIFT1B = "\x1b\x28\x42"; // １バイトへ

static string euc2jis(const string& in)
{
	string out;
	bool shift = false;
	for(int i=0; i < in.size(); ++i)
	{
		byte c0 = in[i];
		byte c1 = in[i+1];

		//if ( (c0 >= 0x00 && c0 <=0x7F) || c1 == '\0' ) // c0 >= 0x00 is always true
		if ( c0 <=0x7F || c1 == '\0' )
		{
			// ASCII
			if ( shift )
			{
				out += SHIFT1B;
				shift = false;
			}
			out += c0;
		}
		else
		{
			// 2bytes
			if ( !shift )
			{
				out += SHIFT2B;
				shift = true;
			}
			++i;

			// EUC半角カナをEUC全角カナに変換する（JISでは半角カナを扱えないため）
			if ( c0 == 0x8E && c1 >= 0xA0 && c1 <= 0xDF )
			{
				static const word euckana_han2zen[16*4] = {	0xA1A1,0xA1A3,0xA1D6,0xA1D7,0xA1A2,0xA1A6,0xA5F2,0xA5A1,0xA5A3,0xA5A5,0xA5A7,0xA5A9,0xA5E3,0xA5E5,0xA5E7,0xA5C3,0xA1BC,0xA5A2,0xA5A4,0xA5A6,0xA5A8,0xA5AA,0xA5AB,0xA5AD,0xA5AF,0xA5B1,0xA5B3,0xA5B5,0xA5B7,0xA5B9,0xA5BB,0xA5BD,0xA5BF,0xA5C1,0xA5C4,0xA5C6,0xA5C8,0xA5CA,0xA5CB,0xA5CC,0xA5CD,0xA5CE,0xA5CF,0xA5D2,0xA5D5,0xA5D8,0xA5DB,0xA5DE,0xA5DF,0xA5E0,0xA5E1,0xA5E2,0xA5E4,0xA5E6,0xA5E8,0xA5E9,0xA5EA,0xA5EB,0xA5EC,0xA5ED,0xA5EF,0xA5F3,0xA1AB,0xA1AC };
				word w = euckana_han2zen[c1 - 0xA0];
				c0 = w >> 8;
				c1 = w & 0x00ff;
			}

			out += c0 & 0x7F;
			out += c1 & 0x7F;
		}
	}

	// シフト状態のままなら戻しておく
	if ( shift )
	{
		out += SHIFT1B;
	}
	return out;
}





//-------------------------------------------------------------------
// メール送信

#include	"TCPSocket.h"

inline void receive_and_check(
	TCPSocket& s,
	const char* request_code,
	const string& error_happen_position)
{
	string r = s.receive_line();
	if ( r.substr(0, 3) != request_code )
	{
		throw util_error(string() + "send_a_mail: " + error_happen_position + ": " + r);
	}
}


void send_a_mail(
	const string& i_host,
	unsigned short i_port,
	const string& i_from,
	const string& i_to,
	const string& i_subject,
	const vector<string>& i_message)
{

	try 
	{
		TCPSocket s;
	
		s.connect(i_host, i_port);
		receive_and_check(s, "220", "connect");
	
		s.send_line( string() + "HELO " + i_host );
		receive_and_check(s, "250", "HELO");
	
		s.send_line( string() + "MAIL FROM:<" + i_from + ">" );
		receive_and_check(s, "250", "MAIL FROM");
	
		s.send_line( string() + "RCPT TO:<" + i_to + ">" );
		receive_and_check(s, "250", "RCPT TO");
	
		s.send_line("DATA");
		receive_and_check(s, "354", "DATA");
	
		s.send_line( string() + "From: " + i_from + "" );
		s.send_line( string() + "To: " + i_to + "" );
		s.send_line( string() + "Subject: " + euc2jis(i_subject) + "" );
		for ( vector<string>::const_iterator i = i_message.begin() ; i != i_message.end() ; ++i )
		{
			s.send_line( euc2jis(*i) );
		}
		
		s.send_line(".");
		receive_and_check(s, "250", "data");
	
		s.send_line("QUIT");
		receive_and_check(s, "221", "QUIT");
	}
	catch (socket_error& ex)
	{
		throw util_error(string() + "send_a_mail: socket_error: " + ex.what());
	}

}

