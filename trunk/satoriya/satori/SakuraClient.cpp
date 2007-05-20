#include "SakuraClient.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


// リクエストを送り、レスポンスを受け取る。戻り値はリターンコード。
int SakuraClient::request(
	const string& i_protocol,
	const string& i_protocol_version,
	const string& i_command,
	const strpairvec& i_data,
	
	string& o_protocol,
	string& o_protocol_version,
	strpairvec& o_data)
{
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// リクエスト文字列を作成

	string	request = i_command + " " + i_protocol + "/" + i_protocol_version + CRLF;
	for ( strpairvec::const_iterator it = i_data.begin() ; it != i_data.end() ; ++it )
	{
		request += it->first + ": " + it->second + CRLF;
	}
	request += CRLF;


	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// リクエスト実行

	string response = this->request(request);


	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// お返事を解析

	// 一行目を切り出し
	o_protocol = cut_token(response, "/");
	o_protocol_version = cut_token(response, " ");
	int return_code = atoi( cut_token(response, " ").c_str() );
	string return_string = cut_token(response, CRLF);

	// 以降のデータ行を切り出し
	while ( response.size() > 0 )
	{
		string value = cut_token(response, CRLF);
		string key = cut_token(value, ": ");
		o_data.push_back( strpair(key, value) );
	}
	
	return return_code;
}
