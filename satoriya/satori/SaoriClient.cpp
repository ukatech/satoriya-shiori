#include "SaoriClient.h"
#include "../_/stltool.h"
#include "../_/charset.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

bool SaoriClient::load(
	const wstring& i_sender,
	const wstring& i_charset,
	const wstring& i_work_folder,
	const wstring& i_dll_fullpath)
{
	return this->SakuraDLLClient::load(i_sender, i_charset, L"SAORI", L"1.0", i_work_folder, i_dll_fullpath);
}

int SaoriClient::request(
	const std::vector<wstring>& i_argument,
	bool i_is_secure,
	wstring& o_result,
	std::vector<wstring>& o_value)
{
	m_error_description.erase();

	//---------------------
	// リクエスト作成

	strpairvec data;

	data.push_back( strpair(L"Charset", charset() ) );
	data.push_back( strpair(L"Sender", L"SATORI" ) );
	data.push_back( strpair(L"SecurityLevel", (i_is_secure ? L"Local" : L"External") ) );

	int idx=0;
	for ( std::vector<wstring>::const_iterator i=i_argument.begin() ; i!=i_argument.end() ; ++i,++idx )
	{
		data.push_back( strpair(wstring(L"Argument")+itos(idx), *i) );
	}

	//---------------------
	// リクエスト実行

	strpairvec r_data;
	int return_code = this->SakuraDLLClient::request(L"EXECUTE", data, r_data);

	//---------------------
	// 返答を解析

	analyze_response(r_data, o_result, o_value);

	return return_code;
}

// 注意！Valueヘッダが存在しないときは、S?系システム変数を温存するためにデータを上書きしないこと！
// 他ゴーストの互換性問題に注意（o_value）
void SaoriClient::analyze_response(
	const strpairvec& i_response_data,
	wstring& o_result,
	std::vector<wstring>& o_value)
{
	int maxValueSize = -1;

	for ( strpairvec::const_iterator i = i_response_data.begin() ; i != i_response_data.end() ; ++i )
	{
		const wstring& key = i->first;
		const wstring& value = i->second;

		if ( compare_head(key, L"Value") && iswdigit(key[const_strlen(L"Value")]) )
		{
			const int	pos = _wtoi(key.c_str() + const_strlen(L"Value"));
			if ( pos<0 || pos>65536 )
			{
				continue;
			}

			if ( o_value.size() <= pos )
			{
				o_value.resize(pos+1);
			}
			o_value[pos] = value;
			if ( maxValueSize < pos ) {
				maxValueSize = pos;
			}
		}
		else if ( key==L"Result" )
		{
			o_result = value;
		}
	}

	//Valueヘッダがなかった場合のみ切り詰める
	if ( maxValueSize >= 0 ) {
		o_value.resize(maxValueSize+1);
	}
}

#ifndef POSIX

//---------------------------------------------------------------------------
// SAORI over DirectSSTP

bool SaoriDirectClient::load(
	const wstring& i_sender,
	const wstring& i_charset,
	const wstring& i_work_folder,
	const wstring& i_dll_fullpath)
{
	return true;
}

void SaoriDirectClient::unload()
{
	//NOOP
}

wstring SaoriDirectClient::get_version(const wstring& i_security_level)
{
	return L"SAORI/1.0";
}

// ヘッダの値に書けない文字（改行と\0）を含むか。含んだまま送ると、後ろにヘッダを足せてしまう
static bool has_header_forbidden_char(const wstring& s)
{
	static const wchar_t forbidden[] = { L'\r', L'\n', L'\0' };
	return s.find_first_of(forbidden, 0, sizeof(forbidden)/sizeof(forbidden[0])) != wstring::npos;
}

int SaoriDirectClient::request(
	const std::vector<wstring>& i_argument,
	bool i_is_secure,
	wstring& o_result,
	std::vector<wstring>& o_value)
{
	m_error_description.erase();

	//---------------------
	// リクエスト作成
	// 文字コードは常にUTF-8（応答も同じ文字コードで返る）。

	if ( has_header_forbidden_char(m_module) ) {
		m_error_description = L"invalid module name";
		return 400;
	}
	wstring request = L"EXECUTE SAORI/1.0\r\nModule: " + m_module + L"\r\nCharset: UTF-8\r\nSender: SATORI\r\nSecurityLevel: ";
	request += i_is_secure ? L"Local" : L"External";
	request += L"\r\n";

	int idx=0;
	for ( std::vector<wstring>::const_iterator i=i_argument.begin() ; i!=i_argument.end() ; ++i,++idx )
	{
		if ( has_header_forbidden_char(*i) ) {
			m_error_description = L"Argument" + itos(idx) + L" contains a line break";
			return 400;
		}
		request += L"Argument" + itos(idx) + L": " + *i + L"\r\n";
	}
	request += L"\r\n";

	//---------------------
	// リクエスト実行

	std::string response;
	if ( m_send == NULL || ! m_send(m_context, WtoUTF8(request), response) ) {
		m_error_description = L"no response from baseware";
		return 500;
	}

	//---------------------
	// 返答を解析

	// 末尾に\0が付いてくることがある
	const std::string::size_type nul = response.find('\0');
	if ( nul != std::string::npos ) {
		response.erase(nul);
	}
	CharactorSet cs = CharsetFromName(UTF8toW(find_charset_header(response)));
	if ( cs == CS_NULL ) {
		cs = CS_UTF8;
	}
	const wstring text = MBtoW(response, cs);

	// 1行目：SAORI/1.0 200 OK
	// 対応していない本体はSAORI/1.0の要求をSSTPの要求として読み、SSTP/1.x のエラーを返す
	if ( ! compare_head(text, L"SAORI/") ) {
		m_error_description = L"baseware does not support SAORI over DirectSSTP";
		return 500;
	}
	wstring::size_type line_end = text.find(L"\r\n");
	if ( line_end == wstring::npos ) {
		line_end = text.size();
	}
	const wstring::size_type space = text.find(L' ');
	if ( space == wstring::npos || space >= line_end ) {
		m_error_description = L"invalid response";
		return 500;
	}
	const int return_code = _wtoi(text.c_str() + space + 1);

	// 以降のヘッダ。空行（または終わり）まで
	strpairvec r_data;
	wstring::size_type start = (line_end < text.size()) ? line_end + 2 : text.size();
	while ( start < text.size() ) {
		wstring::size_type end = text.find(L"\r\n", start);
		if ( end == wstring::npos ) {
			end = text.size();
		}
		if ( end == start ) {
			break;
		}
		const wstring line = text.substr(start, end - start);
		const wstring::size_type colon = line.find(L':');
		if ( colon != wstring::npos ) {
			wstring::size_type value_start = colon + 1;
			while ( value_start < line.size() && line[value_start] == L' ' ) {
				++value_start;
			}
			r_data.push_back( strpair(line.substr(0, colon), line.substr(value_start)) );
		}
		start = end + 2;
	}

	for ( strpairvec::const_iterator i = r_data.begin() ; i != r_data.end() ; ++i ) {
		if ( _wcsicmp(i->first.c_str(), L"ErrorDescription") == 0 ) {
			m_error_description = i->second;
		}
	}
	analyze_response(r_data, o_result, o_value);

	return return_code;
}

#endif // POSIX
