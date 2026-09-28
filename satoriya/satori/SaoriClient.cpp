#include "SaoriClient.h"
#include "../_/stltool.h"

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
	//---------------------
	// リクエスト作成

	strpairvec data;

	data.push_back( strpair(L"Charset", L"UTF-8" ) );
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
	// 注意！Valueヘッダが存在しないときは、S?系システム変数を温存するためにデータを上書きしないこと！
	// 他ゴーストの互換性問題に注意（o_value）

	wstring result;
	int maxValueSize = -1;

	for ( strpairvec::const_iterator i = r_data.begin() ; i != r_data.end() ; ++i )
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

	return return_code;
}
