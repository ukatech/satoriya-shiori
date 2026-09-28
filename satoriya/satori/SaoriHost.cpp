#include "SaoriHost.h"
#include <stdlib.h>

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

int	SaoriHost::request(
	const wstring& i_protocol,
	const wstring& i_protocol_version,
	const wstring& i_command,
	const strpairvec& i_data,
	
	wstring& o_protocol,
	wstring& o_protocol_version,
	strpairvec& o_data)
{
	// 返すプロトコルは最初に決めておく
	o_protocol = L"SAORI";
	o_protocol_version = L"1.0";

	// SAORIであること。バージョンは最低たる1.0を満たしていれば良いので問わない
	if ( i_protocol != L"SAORI" ) { return 400; }

	// GET Versionならそのまま200を返す
	if ( i_command == L"GET Version" ) { return 200; }
	// それ以外のcommandは不正
	if ( i_command != L"EXECUTE" ) { return 400; }

	// 引数を Argument? に格納
	std::deque<wstring> arguments;
	for ( strpairvec::const_iterator it=i_data.begin() ; it!=i_data.end() ; ++it)
	{
		if ( it->first.compare(0, 8, L"Argument") == 0 )
		{
			const int n = _wtoi(it->first.c_str() + 8);
			if ( n>=0 && n<65536 )
			{
				if ( arguments.size() <= n )
				{
					arguments.resize(n+1);
				}
				arguments[n] = it->second;
			}
		}
	}

	// SAORI実行
	std::deque<wstring> values;
	SRV	srv = request(arguments, values);

	// 戻り値を解析＆格納
	o_data.push_back( strpair(L"Result", srv.mResultString) );
	int n=0;
	for (std::deque<wstring>::const_iterator it=values.begin() ; it!=values.end() ; ++it)
	{
		o_data.push_back( strpair(wstring(L"Value") + itos(n++), *it) );
	}

	return srv.mReturnCode;
}
