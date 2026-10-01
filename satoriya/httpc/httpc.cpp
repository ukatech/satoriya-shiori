#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」

//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
/*
httpc
要Charset, Thread
*/
#include	<windows.h>
#include	<wininet.h>
#include	<assert.h>
#include	"../_/stltool.h"
#include	"../_/charset.h"
#include	"../_/Thread.h"
#include	"../_/dsstp.h"
#include	"../satori/SaoriHost.h"

typedef std::set<wstring>	optset;

class httpc : public SaoriHost {
public:
	virtual bool	load(const wstring& i_base_folder);
	virtual SRV		request(std::deque<wstring>& iArguments, std::deque<wstring>& oValues);
};
SakuraDLLHost* SakuraDLLHost::m_dll = new httpc;

static	wstring	base_folder=L"";

//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// 文字コードの判別・変換
// 取得したデータはバイト列なので、ここで内部表現（wstring）にする。
// JIS / EUC-JP は一旦 Shift_JIS に変換してから SJIStoW する。

enum HttpCharset { HC_NULL, HC_JIS, HC_SJIS, HC_EUC, HC_UTF8, HC_UTF16BE, HC_UTF16LE };

inline bool	in_area(unsigned char c, int min, int max) { return ( c>=min && c<=max ); }

// Shift_JIS の「〓」。JIS X 0212 など Shift_JIS に無い文字の代わりに使う。
static const char	SJIS_GETA[] = "\x81\xAC";

// JIS X 0208 の2バイト（各0x21～0x7E）を Shift_JIS にして o に足す
static void	append_jis_as_sjis(unsigned int c1, unsigned int c2, std::string& o) {
	if ( c1 & 1 )
		c2 += 0x1F;
	else
		c2 += 0x7D;
	if ( c2 >= 0x7F )
		++c2;
	c1 = ((c1 - 0x21) >> 1) + 0x81;
	if ( c1 > 0x9F )
		c1 += 0x40;
	o += static_cast<char>(c1);
	o += static_cast<char>(c2);
}

// ISO-2022-JP → Shift_JIS
static std::string	jis_to_sjis(const std::string& in) {
	enum { MODE_ASCII, MODE_KANJI, MODE_KANJI_0212, MODE_KANA } mode = MODE_ASCII;
	std::string	o;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(in.c_str());
	const unsigned char* end = p + in.size();
	while ( p < end ) {
		if ( p[0]==0x1b && end-p>=3 ) {
			if ( p[1]=='$' && (p[2]=='@' || p[2]=='B') ) { mode=MODE_KANJI; p+=3; continue; }
			if ( p[1]=='&' && p[2]=='@' ) { p+=3; continue; }	// JIS X 0208-1990 の前置き
			if ( p[1]=='$' && p[2]=='(' && end-p>=4 && p[3]=='D' ) { mode=MODE_KANJI_0212; p+=4; continue; }
			if ( p[1]=='(' && (p[2]=='B' || p[2]=='J' || p[2]=='H') ) { mode=MODE_ASCII; p+=3; continue; }
			if ( p[1]=='(' && p[2]=='I' ) { mode=MODE_KANA; p+=3; continue; }
		}

		if ( (mode==MODE_KANJI || mode==MODE_KANJI_0212) && end-p>=2 && in_area(p[0],0x21,0x7e) && in_area(p[1],0x21,0x7e) ) {
			if ( mode==MODE_KANJI )
				append_jis_as_sjis(p[0], p[1], o);
			else
				o += SJIS_GETA;
			p+=2;
		}
		else if ( mode==MODE_KANA && in_area(p[0],0x21,0x5f) ) {
			o += static_cast<char>(p[0]+0x80);	// 半角カナ
			++p;
		}
		else {
			o += static_cast<char>(*p++);
		}
	}
	return	o;
}

// EUC-JP → Shift_JIS
static std::string	euc_to_sjis(const std::string& in) {
	std::string	o;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(in.c_str());
	const unsigned char* end = p + in.size();
	while ( p < end ) {
		if ( p[0]==0x8e && end-p>=2 && in_area(p[1],0xa1,0xdf) ) {
			o += static_cast<char>(p[1]);	// 半角カナ
			p+=2;
		}
		else if ( p[0]==0x8f && end-p>=3 && in_area(p[1],0xa1,0xfe) && in_area(p[2],0xa1,0xfe) ) {
			o += SJIS_GETA;	// JIS X 0212
			p+=3;
		}
		else if ( in_area(p[0],0xa1,0xfe) && end-p>=2 && in_area(p[1],0xa1,0xfe) ) {
			append_jis_as_sjis(p[0]&0x7f, p[1]&0x7f, o);
			p+=2;
		}
		else {
			o += static_cast<char>(*p++);
		}
	}
	return	o;
}

// 全体がEUC-JPとして正しいか
static bool	is_valid_euc(const std::string& in) {
	const unsigned char* p = reinterpret_cast<const unsigned char*>(in.c_str());
	const unsigned char* end = p + in.size();
	while ( p < end ) {
		if ( p[0] < 0x80 )
			++p;
		else if ( p[0]==0x8e && end-p>=2 && in_area(p[1],0xa1,0xdf) )
			p+=2;
		else if ( p[0]==0x8f && end-p>=3 && in_area(p[1],0xa1,0xfe) && in_area(p[2],0xa1,0xfe) )
			p+=3;
		else if ( in_area(p[0],0xa1,0xfe) && end-p>=2 && in_area(p[1],0xa1,0xfe) )
			p+=2;
		else
			return	false;
	}
	return	true;
}

// 大文字小文字を問わず比較
static std::string::size_type	find_nocase(const std::string& str, const char* substr) {
	const std::string::size_type	len = strlen(substr);
	for ( std::string::size_type i=0 ; i+len<=str.size() ; ++i ) {
		if ( _strnicmp(str.c_str()+i, substr, len)==0 )
			return	i;
	}
	return	std::string::npos;
}

static bool	head_nocase(const char* p, const char* head) {
	return	_strnicmp(p, head, strlen(head))==0;
}

// 文字コード自動判別
//   UTFのBOM、charset= / encoding= の指定、JISのエスケープシーケンス、UTF-8として正しいか、
//   EUC/SJISの判別、の順で判定する。判定できなければSJISとする。
static HttpCharset	detect_charset(const std::string& str) {

	// UTFのBOMによる判別
	if ( str.size()>=2 && (unsigned char)str[0]==0xFE && (unsigned char)str[1]==0xFF )
		return	HC_UTF16BE;
	if ( str.size()>=2 && (unsigned char)str[0]==0xFF && (unsigned char)str[1]==0xFE )
		return	HC_UTF16LE;
	if ( HasUTF8BOM(str) )
		return	HC_UTF8;

	// charset指定っぽい文字列があればそれを使う。ただし一番先頭にくるものを採用
	std::string::size_type	pos = find_nocase(str, "charset=");
	if ( pos == std::string::npos )
		pos = find_nocase(str, "encoding=");
	if ( pos != std::string::npos ) {
		const char* enc = str.c_str() + pos;
		while ( isalpha((unsigned char)*enc) ) ++enc;
		while ( *enc=='=' || *enc=='\"' || *enc==' ' || *enc=='\'') ++enc;
		if ( head_nocase(enc, "iso-2022-jp") )
			return	HC_JIS;
		else if ( head_nocase(enc, "shift_jis") || head_nocase(enc, "x-sjis") || head_nocase(enc, "windows-31j") )
			return	HC_SJIS;
		else if ( head_nocase(enc, "euc-jp") || head_nocase(enc, "x-euc-jp") )
			return	HC_EUC;
		else if ( head_nocase(enc, "utf") ) // -8 じゃなくてもとりあえず。
			return	HC_UTF8;
	}

	// jisのエスケープシーケンスがあればjisに確定
	if ( str.find("\x1b$B") != std::string::npos ||
		str.find("\x1b$@") != std::string::npos ||
		str.find("\x1b$(D") != std::string::npos ||
		str.find("\x1b(I") != std::string::npos )
		return	HC_JIS;

	// UTF-8として正しければUTF-8
	if ( IsValidUTF8(str) )
		return	HC_UTF8;

	// eucかsjisかの確実な判別
	const unsigned char* p = reinterpret_cast<const unsigned char*>(str.c_str());
	const unsigned char* end = p + str.size();
	while ( p < end ) {
		if ( p[0] < 0x80 )
			++p;
		else if ( in_area(p[0],0x81,0x9f) )
			return	HC_SJIS;
		else if ( in_area(p[0],0xa1,0xdf) && end-p>=2 && ( p[1]<0x80 || in_area(p[1],0x80,0xa0) ) )
			return	HC_SJIS;
		else if ( in_area(p[0],0xf0,0xfe) )
			return	HC_EUC;
		else
			++p;
	}

	// 確定できなければ、EUCとして正しいならEUC、そうでなければSJIS
	return	is_valid_euc(str) ? HC_EUC : HC_SJIS;
}

static wstring	utf16_to_w(const std::string& str, bool is_big_endian) {
	wstring	o;
	for ( std::string::size_type i=0 ; i+1<str.size() ; i+=2 ) {
		const unsigned char b0 = str[i], b1 = str[i+1];
		o += static_cast<wchar_t>( is_big_endian ? ((b0<<8)|b1) : ((b1<<8)|b0) );
	}
	return	o;
}

static wstring	decode_body(const std::string& str, const optset& opt) {
	HttpCharset	cs;
	if ( opt.find(L"jis") != opt.end() )
		cs = HC_JIS;
	else if ( opt.find(L"euc") != opt.end() )
		cs = HC_EUC;
	else if ( opt.find(L"utf8") != opt.end() )
		cs = HC_UTF8;
	else if ( opt.find(L"sjis") != opt.end() )
		cs = HC_SJIS;
	else
		cs = detect_charset(str);

	wstring	o;
	switch ( cs ) {
	case HC_JIS: o = SJIStoW(jis_to_sjis(str)); break;
	case HC_EUC: o = SJIStoW(euc_to_sjis(str)); break;
	case HC_UTF8: o = UTF8toW(str); break;
	case HC_UTF16BE: o = utf16_to_w(str, true); break;
	case HC_UTF16LE: o = utf16_to_w(str, false); break;
	default: o = SJIStoW(str); break;
	}

	// BOMを取り除く
	if ( !o.empty() && o[0]==0xFEFF )
		o.erase(0, 1);
	return	o;
}

//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// タグの処理

static wstring	erase_tag(const wstring& i) {
	wstring	o;
	bool	valid=true;
	for ( const wchar_t* p=i.c_str() ; *p != L'\0' ; ++p ) {
		if ( valid ) {
			if ( *p==L'<' )
				valid=false;
			else
				o += *p;
		}
		else if ( *p==L'>' ) {
			valid=true;
		}
	}
	return	o;
}

struct Tag {
	wstring	name;
	strmap	prop;
};

static wstring	tag_to_ss(Tag& tag) {
	const wchar_t* nm = tag.name.c_str();
	strmap&	pp = tag.prop;

	if ( _wcsicmp(L"/tr", nm)==0) return L"\\n";
	if ( _wcsicmp(L"/li", nm)==0) return L"\\n";
	if ( _wcsicmp(L"br", nm)==0) return L"\\n";
	if ( _wcsicmp(L"p", nm)==0) return L"\\n";
	if ( _wcsicmp(L"hr", nm)==0) return L"\\n-----------------------------------------------\\n";
	if ( _wcsicmp(L"a", nm)==0 && pp.find(L"href")!=pp.end() ) return wstring() + L"\\_a[" + pp[L"href"] + L"]";
	if ( _wcsicmp(L"/a", nm)==0) return L"\\_a";
	if ( _wcsicmp(L"img", nm)==0 && pp.find(L"alt")!=pp.end() && pp[L"alt"].size()>0 ) return wstring()+L"〔"+pp[L"alt"]+L"〕";
	return	L"";
}

inline bool	is_delimiter(wchar_t c) { return ( c==L' ' || c==L'\t' || c==L'\r' || c==L'\n' ); }

static void	skip_delimiter(const wchar_t*& p) {
	while ( *p!=L'\0' && is_delimiter(*p) )
		++p;
}

static wstring	read_word(const wchar_t*& p) {
	wstring	o;
	skip_delimiter(p);
	if (*p==L'"') {
		++p;
		while ( *p!=L'\0' && *p!=L'"' )
			o += *p++;
		if (*p==L'"' )
			++p;
	} else {
		while ( *p!=L'\0' && !(is_delimiter(*p) || *p==L'=' || *p==L'>') )
			o += *p++;
	}
	skip_delimiter(p);
	return	o;
}

static Tag	analyze_tag(const wstring& str) {
	const wchar_t* p = str.c_str();
	Tag	tag;
	tag.name=read_word(p);
	while (*p!=L'\0') {
		wstring	key=read_word(p);
		if ( *p==L'=' ) {
			wstring	value=read_word(++p);
			tag.prop[key]=value;
		} else
			tag.prop[key]=L"";
	}
	return	tag;
}

// <...>を読み込み
static wstring	read_tag_area(const wchar_t*& p) {
	if ( *p!=L'<' )
		return	L"";
	wstring	o;
	while (*++p!=L'>' && *p!=L'\0')
		o += *p;
	if ( *p==L'>' )
		++p;
	return	o;
}

static wstring	translate_tag(const wstring& i) {
	wstring	o;
	const wchar_t* p=i.c_str();
	while ( *p != L'\0' ) {
		if (*p==L'<') {
			wstring	tag_area = read_tag_area(p);
			Tag	tag = analyze_tag(tag_area);
			o += tag_to_ss(tag);
		} 
		else
			o += *p++;
	}
	replace(o, L"&nbsp;", L"");
	replace(o, L"&lt;", L"<");
	replace(o, L"&gt;", L">");
	replace(o, L"&amp;", L"\\&");
	replace(o, L"&copy;", L"(c)");
	while ( replace(o, L"\\n\\n", L"\\n")>0 )
		NULL;

	// 行数を制限する。桁数は半角換算で数える。
	const int column_max=48, row_max=120;
	int	column=0, row=1;
	p=o.c_str();
	while ( *p!=L'\0' && row<row_max) {
		if ( p[0]==L'\\' && p[1]==L'n' ) {
			column=0;
			row++;
			p+=2;
		}
		else {
			column += static_cast<int>(count_width(get_a_chr(p)));
			if ( column >= column_max ) {
				column=0;
				row++;
			}
		}
	}
	if ( *p!=L'\0' )
		o = o.substr(0, p-o.c_str());

	return	o;
}

//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// HTTP

static const wchar_t	AGENT_NAME[] = L"httpc_for_saori";

static bool	checkHTTP() {
	HINTERNET hInternet = ::InternetOpen(
		AGENT_NAME,
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		0);
	if ( hInternet==NULL )
		return	false;
	::InternetCloseHandle(hInternet);
	return	true;
}

// URLの内容をバイト列で取得
static bool	readHTTP(const wstring& iURL, std::string& oData) {

	/* WININET初期化 */
	HINTERNET hInternet = ::InternetOpen(
		AGENT_NAME,
		INTERNET_OPEN_TYPE_PRECONFIG,
		NULL,
		NULL,
		0);
	if ( hInternet==NULL )
		return	false;

	/* URLのオープン */
	HINTERNET hFile = ::InternetOpenUrl(
		hInternet,
		iURL.c_str(),
		NULL,
		0,
		INTERNET_FLAG_RELOAD,
		0);
	if ( hFile==NULL ) {
		::InternetCloseHandle(hInternet);
		return	false;
	}

	/* オープンしたURLからデータを(8192バイトずつ)読み込む */
	for(;;) {
		DWORD ReadSize=0;
		char Buf[8192];
		BOOL bResult = ::InternetReadFile(
			hFile,
			Buf,
			sizeof(Buf),
			&ReadSize);

		/* 全て読み込んだか、失敗したらループを抜ける */
		if ( !bResult || ReadSize == 0 )
			break;

		oData.append(Buf, ReadSize);
	}

	/* 後処理 */
	::InternetCloseHandle(hFile);
	::InternetCloseHandle(hInternet);
	return	true;
}

static bool	getHTTP(const wstring& iURL, const optset& opt, wstring& oResult) {

	std::string	data;
	if ( !readHTTP(iURL, data) )
		return	false;

	// 文字コードの判別・変換
	const wstring	str = decode_body(data, opt);

	// 改行コード（とNUL）を消去
	oResult.erase();
	for ( wstring::const_iterator it=str.begin() ; it!=str.end() ; ++it ) {
		if ( *it!=L'\r' && *it!=L'\n' && *it!=L'\0' )
			oResult += *it;
	}

	// タグの処理
	if ( opt.find(L"translate_tag") != opt.end() )
		oResult=translate_tag(oResult);
	else if ( opt.find(L"erase_tag") != opt.end() )
		oResult=erase_tag(oResult);

	return	true;
}


static bool	saveHTTP(const wstring& iURL, const wstring& iFileName) {

	std::string	data;
	if ( !readHTTP(iURL, data) )
		return	false;

	return	bytes_to_file(data, iFileName);
}


/*
	httpc.dll

　SAORI規格のhttpクライアント。
　Web上の各種データをHTTPで取得する。

　引数の指定により、４通りの動作をする。


・引数なし

  インターネットに接続できるかどうかの確認。
  接続できるなら1、できないなら0を返す。


・引数１つ

  第１引数にURLを指定、取得したものをそのまま返す。


・引数２つ

  第１引数にURLを指定、
　取得したものを第２引数のファイル名で保存。
　成功したら1、失敗したら0を返す。


・引数３つ　

  第１引数にURLを指定、
  取得したものから、第２・３引数で囲まれる範囲を返す。

  例えば、
  第１引数が　http://www.yahoo.co.jp/
  第２引数が　href="
  第３引数が　"

  だったとすると、ヤフーのトップページから、一番最初にあるリンク先URLが取得される。
  また、複数返値から２つ目以降のリンクも取得できる。


*/
static wstring	execute(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {

	// 先頭に並んだオプションを取り出す
	static const wchar_t*	exist_options[] = 
		{L"euc", L"jis", L"sjis", L"utf8", L"erase_tag", L"translate_tag"};
	static const int	max = sizeof(exist_options)/sizeof(exist_options[0]);
	optset	opt;
	while ( !iArguments.empty() ) {
		int i=0;
		for ( ; i<max ; ++i ) {
			if ( iArguments.front()==exist_options[i] )
				break;
		}
		if ( i==max )
			break;
		opt.insert( iArguments.front() );
		iArguments.pop_front();
	}

	wstring	data;

	if ( iArguments.size()==0 )
		return	checkHTTP() ? L"1" : L"0";
	else if ( iArguments.size()==1 )
		return	getHTTP(iArguments[0], opt, data) ? data : L"";
	else if ( iArguments.size()==2 )
		return	saveHTTP(iArguments[0], base_folder+iArguments[1]) ? L"1" : L"0";
	else if ( iArguments.size()==3 ) {

		if ( !getHTTP(iArguments[0], opt, data) )
			return	L"";

		int	find_count=0;
		const wstring&	start = iArguments[1];
		const wstring&	end = iArguments[2];
		wstring	ret_str=L"";
		while(true) {
			wstring::size_type	n = data.find(start);
			if ( n==wstring::npos )
				break;

			data.erase(0, n+start.size());
			n = data.find(end);
			if ( n==wstring::npos )
				break;

			wstring	value( data.c_str(), n );	// 内容取得
			if ( find_count++ == 0 )
				ret_str = value;
			else
				oValues.push_back(value);

			data.erase(0, n+end.size());
		}
		return	ret_str;
	}

	return	wstring();
}

class HttpcThread : public Thread {
	wstring	mID;
	std::deque<wstring>	mArguments;
	virtual	DWORD	ThreadMain() {
		std::deque<wstring>	refs;
		wstring	result = execute(mArguments, refs);	// ref3～
		refs.push_front(result);	// ref2
		refs.push_front(L"-");	// ref1
		refs.push_front(mID);	// ref0
		sendDirectSSTP_for_NOTIFY(L"httpc", L"OnHttpcNotify", refs);
		delete this;
		return	0;
	}
public:
	HttpcThread(const wstring& iID, const std::deque<wstring>& iArguments) : 
	  Thread(), mID(iID), mArguments(iArguments) {}
};

bool	httpc::load(const wstring& i_base_folder) {
	base_folder = i_base_folder;
	return true;
}

SRV	httpc::request(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {

	if ( iArguments.size()>0 && iArguments[0]==L"bg" ) {
		iArguments.pop_front();
		if ( iArguments.size()<1 )
			return	SRV(L"bgするならIDを指定していただけませんと。");
		wstring	id = iArguments[0];
		iArguments.pop_front();
		(new HttpcThread(id, iArguments))->create();
		return	SRV(L"");
	}

	return	SRV(execute(iArguments, oValues));
}
