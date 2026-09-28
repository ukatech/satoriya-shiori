#include	"charset.h"

#ifdef POSIX
#include	<iconv.h>
#include	<errno.h>
#else
#include	<windows.h>
#endif

#include	<cstring>

//----------------------------------------------------------------------
// UTF-8 (自前実装)

static void	append_codepoint(std::wstring& o, unsigned long cp)
{
	if ( sizeof(wchar_t) == 2 && cp >= 0x10000 ) {
		cp -= 0x10000;
		o += (wchar_t)(0xD800 + (cp >> 10));
		o += (wchar_t)(0xDC00 + (cp & 0x3FF));
	}
	else {
		o += (wchar_t)cp;
	}
}

// p から1文字デコードする。不正なら 0 を返す。成功したら使ったバイト数を返す。
static int	decode_utf8_char(const unsigned char* p, size_t len, unsigned long& cp)
{
	unsigned char c = p[0];
	int n;
	unsigned long min;
	if ( c < 0x80 ) { cp = c; return 1; }
	else if ( c >= 0xC2 && c <= 0xDF ) { n = 2; cp = c & 0x1F; min = 0x80; }
	else if ( c >= 0xE0 && c <= 0xEF ) { n = 3; cp = c & 0x0F; min = 0x800; }
	else if ( c >= 0xF0 && c <= 0xF4 ) { n = 4; cp = c & 0x07; min = 0x10000; }
	else { return 0; }

	if ( len < (size_t)n ) { return 0; }
	for ( int i = 1 ; i < n ; ++i ) {
		if ( (p[i] & 0xC0) != 0x80 ) { return 0; }
		cp = (cp << 6) | (p[i] & 0x3F);
	}
	if ( cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF) ) { return 0; }
	return n;
}

std::wstring	UTF8toW(const std::string& str)
{
	std::wstring o;
	o.reserve(str.size());
	const unsigned char* p = (const unsigned char*)str.c_str();
	size_t len = str.size();
	while ( len > 0 ) {
		unsigned long cp;
		int n = decode_utf8_char(p, len, cp);
		if ( n == 0 ) {
			// 不正なバイトは U+FFFD にして1バイト進める
			o += (wchar_t)0xFFFD;
			n = 1;
		}
		else {
			append_codepoint(o, cp);
		}
		p += n;
		len -= n;
	}
	return o;
}

std::string		WtoUTF8(const std::wstring& str)
{
	std::string o;
	o.reserve(str.size() * 3);
	const wchar_t* p = str.c_str();
	const wchar_t* e = p + str.size();
	while ( p < e ) {
		unsigned long cp = (unsigned long)*p++;
		if ( sizeof(wchar_t) == 2 ) {
			cp &= 0xFFFF;
			if ( cp >= 0xD800 && cp <= 0xDBFF && p < e && (*p & 0xFFFF) >= 0xDC00 && (*p & 0xFFFF) <= 0xDFFF ) {
				cp = 0x10000 + ((cp - 0xD800) << 10) + ((*p++ & 0xFFFF) - 0xDC00);
			}
			else if ( cp >= 0xD800 && cp <= 0xDFFF ) {
				cp = 0xFFFD;
			}
		}
		if ( cp > 0x10FFFF ) { cp = 0xFFFD; }

		if ( cp < 0x80 ) {
			o += (char)cp;
		}
		else if ( cp < 0x800 ) {
			o += (char)(0xC0 | (cp >> 6));
			o += (char)(0x80 | (cp & 0x3F));
		}
		else if ( cp < 0x10000 ) {
			o += (char)(0xE0 | (cp >> 12));
			o += (char)(0x80 | ((cp >> 6) & 0x3F));
			o += (char)(0x80 | (cp & 0x3F));
		}
		else {
			o += (char)(0xF0 | (cp >> 18));
			o += (char)(0x80 | ((cp >> 12) & 0x3F));
			o += (char)(0x80 | ((cp >> 6) & 0x3F));
			o += (char)(0x80 | (cp & 0x3F));
		}
	}
	return o;
}

bool	IsValidUTF8(const char* p, size_t len)
{
	const unsigned char* up = (const unsigned char*)p;
	while ( len > 0 ) {
		unsigned long cp;
		int n = decode_utf8_char(up, len, cp);
		if ( n == 0 ) { return false; }
		up += n;
		len -= n;
	}
	return true;
}

bool	HasUTF8BOM(const std::string& str)
{
	return str.size() >= 3 &&
		(unsigned char)str[0] == 0xEF && (unsigned char)str[1] == 0xBB && (unsigned char)str[2] == 0xBF;
}

CharactorSet	DetectCharset(const std::string& str)
{
	if ( HasUTF8BOM(str) || IsValidUTF8(str) ) {
		return CS_UTF8;
	}
	return CS_SJIS;
}

//----------------------------------------------------------------------
// Shift_JIS / ACP

#ifdef POSIX

static std::string	iconv_convert(const std::string& str, const char* to, const char* from)
{
	iconv_t cd = iconv_open(to, from);
	if ( cd == (iconv_t)-1 ) {
		return std::string();
	}

	std::string o;
	char buf[1024];
	char* in = const_cast<char*>(str.c_str());
	size_t inleft = str.size();
	while ( inleft > 0 ) {
		char* out = buf;
		size_t outleft = sizeof(buf);
		size_t r = iconv(cd, &in, &inleft, &out, &outleft);
		o.append(buf, out - buf);
		if ( r == (size_t)-1 ) {
			if ( errno == E2BIG ) { continue; }
			// 変換できない文字は ? にして1バイト（1文字）飛ばす
			o += '?';
			++in;
			--inleft;
		}
	}
	iconv(cd, NULL, NULL, NULL, NULL);
	iconv_close(cd);
	return o;
}

static const char*	sjis_iconv_name()
{
	static const char* name = NULL;
	if ( name == NULL ) {
		static const char* candidates[] = { "CP932", "SHIFT_JIS", "SJIS" };
		name = candidates[0];
		for ( size_t i = 0 ; i < sizeof(candidates)/sizeof(candidates[0]) ; ++i ) {
			iconv_t cd = iconv_open("UTF-8", candidates[i]);
			if ( cd != (iconv_t)-1 ) {
				iconv_close(cd);
				name = candidates[i];
				break;
			}
		}
	}
	return name;
}

std::wstring	SJIStoW(const std::string& str)
{
	return UTF8toW(iconv_convert(str, "UTF-8", sjis_iconv_name()));
}

std::string		WtoSJIS(const std::wstring& str)
{
	// 変換できない文字があった場合、iconv_convert は1バイトずつ飛ばしてしまうので、
	// 1文字ずつ変換して1文字を1つの ? にする。
	std::string o;
	const wchar_t* p = str.c_str();
	const wchar_t* e = p + str.size();
	while ( p < e ) {
		const wchar_t* s = p;
		++p;
		if ( IsHighSurrogate(*s) && p < e && IsLowSurrogate(*p) ) { ++p; }
		if ( *s < 0x80 ) {
			o += (char)*s;
			continue;
		}
		std::string r = iconv_convert(WtoUTF8(std::wstring(s, p)), sjis_iconv_name(), "UTF-8");
		if ( r.empty() || r[0] == '?' ) {
			o += '?';
		}
		else {
			o += r;
		}
	}
	return o;
}

std::wstring	ACPtoW(const std::string& str) { return UTF8toW(str); }
std::string		WtoACP(const std::wstring& str) { return WtoUTF8(str); }

#else

static std::wstring	mb2w(const std::string& str, UINT cp)
{
	if ( str.empty() ) { return std::wstring(); }
	int len = ::MultiByteToWideChar(cp, 0, str.c_str(), (int)str.size(), NULL, 0);
	if ( len <= 0 ) { return std::wstring(); }
	std::wstring o(len, L'\0');
	::MultiByteToWideChar(cp, 0, str.c_str(), (int)str.size(), &o[0], len);
	return o;
}

static std::string	w2mb(const std::wstring& str, UINT cp)
{
	if ( str.empty() ) { return std::string(); }
	// 似た文字への置き換え（best fit）はさせず、変換できない文字は ? にする
	int len = ::WideCharToMultiByte(cp, WC_NO_BEST_FIT_CHARS, str.c_str(), (int)str.size(), NULL, 0, NULL, NULL);
	if ( len <= 0 ) { return std::string(); }
	std::string o(len, '\0');
	::WideCharToMultiByte(cp, WC_NO_BEST_FIT_CHARS, str.c_str(), (int)str.size(), &o[0], len, NULL, NULL);
	return o;
}

std::wstring	SJIStoW(const std::string& str) { return mb2w(str, 932); }
std::string		WtoSJIS(const std::wstring& str) { return w2mb(str, 932); }
std::wstring	ACPtoW(const std::string& str) { return mb2w(str, CP_ACP); }
std::string		WtoACP(const std::wstring& str) { return w2mb(str, CP_ACP); }

#endif

//----------------------------------------------------------------------

std::wstring	MBtoW(const std::string& str, CharactorSet cs)
{
	if ( cs == CS_NULL ) {
		cs = DetectCharset(str);
	}
	switch ( cs ) {
	case CS_UTF8:
		if ( HasUTF8BOM(str) ) {
			return UTF8toW(str.substr(3));
		}
		return UTF8toW(str);
	case CS_ACP:
		return ACPtoW(str);
	default:
		return SJIStoW(str);
	}
}

std::string		WtoMB(const std::wstring& str, CharactorSet cs)
{
	switch ( cs ) {
	case CS_SJIS:
		return WtoSJIS(str);
	case CS_ACP:
		return WtoACP(str);
	default:
		return WtoUTF8(str);
	}
}

static bool	charset_name_equal(const std::wstring& name, const char* candidate)
{
	// 大文字小文字、'-' と '_' の有無を無視して比較する
	std::wstring::const_iterator i = name.begin();
	const char* p = candidate;
	while ( true ) {
		while ( i != name.end() && (*i == L'-' || *i == L'_' || *i == L' ') ) { ++i; }
		while ( *p == '-' || *p == '_' ) { ++p; }
		if ( i == name.end() || *p == '\0' ) {
			return i == name.end() && *p == '\0';
		}
		wchar_t a = *i;
		char b = *p;
		if ( a >= L'A' && a <= L'Z' ) { a = a - L'A' + L'a'; }
		if ( b >= 'A' && b <= 'Z' ) { b = b - 'A' + 'a'; }
		if ( a != (wchar_t)b ) { return false; }
		++i;
		++p;
	}
}

CharactorSet	CharsetFromName(const std::wstring& name)
{
	static const char* sjis_names[] = { "shift_jis", "sjis", "x-sjis", "cp932", "ms932", "windows-31j" };
	static const char* utf8_names[] = { "utf-8", "utf8" };
	size_t n;
	for ( n = 0 ; n < sizeof(sjis_names)/sizeof(sjis_names[0]) ; ++n ) {
		if ( charset_name_equal(name, sjis_names[n]) ) { return CS_SJIS; }
	}
	for ( n = 0 ; n < sizeof(utf8_names)/sizeof(utf8_names[0]) ; ++n ) {
		if ( charset_name_equal(name, utf8_names[n]) ) { return CS_UTF8; }
	}
	return CS_NULL;
}

const wchar_t*	CharsetName(CharactorSet cs)
{
	switch ( cs ) {
	case CS_SJIS: return L"Shift_JIS";
	case CS_UTF8: return L"UTF-8";
	default: return L"UTF-8";
	}
}
