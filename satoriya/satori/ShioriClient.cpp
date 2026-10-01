#include "ShioriClient.h"
#include "../_/Sender.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


bool ShioriClient::load(
	const wstring& i_sender,
	const wstring& i_charset,
	const wstring& i_work_folder,
	const wstring& i_dll_fullpath)
{
	return SakuraDLLClient::load(
		i_sender,
		i_charset,
		L"SHIORI",
		L"3.0",
		i_work_folder,
		i_dll_fullpath);
}

int ShioriClient::request(
	const wstring& i_id, // OnBootとか
	const std::vector<wstring>& i_references, // Reference?
	bool i_is_secure, // SecurityLevel
	wstring& o_value, // 栞が返したさくらスクリプトや文字列リソース
	std::vector<wstring>& o_references // 複数戻り値。通常はOnCommunicateでしか使わない
	)
{
	strpairvec data;
	data.push_back( strpair(L"ID", i_id) );
	for (int n=0 ; n<i_references.size() ; ++n )
	{
		data.push_back( strpair( wstring(L"Reference") + itos(n), i_references[n]) );
	}
	data.push_back( strpair(L"SecurityLevel", (i_is_secure ? L"local" : L"external")) );

	// リクエスト実行
	strpairvec rdata;
	int r = SakuraDLLClient::request(L"GET", data, rdata);

	wstring value;
	for (int n=0 ; n<rdata.size() ; ++n )
	{
		static const int len = const_strlen(L"Reference");
		if ( wcsncmp(rdata[n].first.c_str(), L"Reference", len) == 0 )
		{
			const int ref_n = _wtoi(rdata[n].first.c_str() + len);
			if ( o_references.size() <= ref_n )
			{
				o_references.resize(ref_n+1);
			}
			o_references[ref_n] = rdata[n].second;
		}
		else if ( rdata[n].first == L"Value" )
		{
			o_value = rdata[n].second;
		}
		else if ( rdata[n].first == L"Sender" )
		{
			//GetSender().sender() << rdata[n].second << endl;
		}
	}

	return r;
}


