#ifndef	SODATE_PASSWORD_H
#define	SODATE_PASSWORD_H

// sodate.dat に保存するパスワードの簡易暗号化（sodate / sodate_setup 共通）
// 保存形式は旧版と同じ。平文のバイト列はUTF-8で、旧版で保存したもの（SJIS）も読める。

#include	<string>
#include	<vector>
#include	"../_/stltool.h"
#include	"../_/charset.h"

static const byte	PASSWORD_XOR_MAGIC = 186;

inline wstring	decode_password(const wstring& i_stored) {
	std::string	str = decode(WtoSJIS(i_stored));
	int	len = str.size()/2;
	std::vector<byte>	buf(len+1, 0);
	string_to_binary(str, &buf[0]);
	xor_filter(&buf[0], len, PASSWORD_XOR_MAGIC);
	std::string	plain = decode( reinterpret_cast<const char*>(&buf[0]) );
	return	MBtoW(plain, CS_NULL);
}

inline wstring	encode_password(const wstring& i_plain) {
	std::string	str = encode(WtoUTF8(i_plain));
	int	len = str.size();
	std::vector<byte>	buf(len+1, 0);
	if ( len > 0 )
		memcpy(&buf[0], str.c_str(), len);
	xor_filter(&buf[0], len, PASSWORD_XOR_MAGIC);
	return	ascii_to_w( encode( binary_to_string(&buf[0], len) ).c_str() );
}

#endif	// SODATE_PASSWORD_H
