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
	const string& i_sender,
	const string& i_charset,
	const string& i_work_folder,
	const string& i_dll_fullpath)
{
	return SakuraDLLClient::load(
		i_sender,
		i_charset,
		"SHIORI",
		"3.0",
		i_work_folder,
		i_dll_fullpath);
}

int ShioriClient::request(
	const string& i_id, // OnBootとか
	const std::vector<string>& i_references, // Reference?
	bool i_is_secure, // SecurityLevel
	string& o_value, // 栞が返したさくらスクリプトや文字列リソース
	std::vector<string>& o_references // 複数戻り値。通常はOnCommunicateでしか使わない
	)
{
	strpairvec data;
	data.push_back( strpair("ID", i_id) );
	for (int n=0 ; n<i_references.size() ; ++n )
	{
		data.push_back( strpair( string("Reference") + itos(n), i_references[n]) );
	}
	data.push_back( strpair("SecurityLevel", (i_is_secure ? "local" : "external")) );

	// リクエスト実行
	strpairvec rdata;
	int r = SakuraDLLClient::request("GET", data, rdata);

	string value;
	for (int n=0 ; n<rdata.size() ; ++n )
	{
		static const int len = sizeof("Reference");
		if ( strncmp(rdata[n].first.c_str(), "Reference", len) == 0 )
		{
			const int ref_n = atoi(rdata[n].first.c_str() + len);
			if ( o_references.size() <= ref_n )
			{
				o_references.resize(ref_n+1);
			}
			o_references[ref_n] = rdata[n].second;
		}
		else if ( rdata[n].first == "Value" )
		{
			o_value = rdata[n].second;
		}
		else if ( rdata[n].first == "Sender" )
		{
			//GetSender().sender() << rdata[n].second << endl;
		}
	}

	return r;
}


