#include "SakuraClient.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


// リクエストを送り、レスポンスを受け取る。戻り値はリターンコード。
int SakuraClient::request(
	const wstring& i_protocol,
	const wstring& i_protocol_version,
	const wstring& i_command,
	const strpairvec& i_data,
	
	wstring& o_protocol,
	wstring& o_protocol_version,
	strpairvec& o_data)
{
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// リクエスト文字列を作成

	wstring	request = i_command + L" " + i_protocol + L"/" + i_protocol_version + CRLF;
	for ( strpairvec::const_iterator it = i_data.begin() ; it != i_data.end() ; ++it )
	{
		request += it->first + L": " + it->second + CRLF;
	}
	request += CRLF;


	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// リクエスト実行

	wstring response = this->request(request);


	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// お返事を解析

	// 一行目を切り出し
	o_protocol = cut_token(response, L"/");
	o_protocol_version = cut_token(response, L" ");
	int return_code = _wtoi( cut_token(response, L" ").c_str() );
	wstring return_string = cut_token(response, CRLF);

	// 以降のデータ行を切り出し
	while ( response.size() > 0 )
	{
		wstring value = cut_token(response, CRLF);
		wstring key = cut_token(value, L": ");
		o_data.push_back( strpair(key, value) );
	}
	
	return return_code;
}
