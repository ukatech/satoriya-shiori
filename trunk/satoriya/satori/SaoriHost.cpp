#include "SaoriHost.h"
#include <stdlib.h>

int	SaoriHost::request(
	const string& i_protocol,
	const string& i_protocol_version,
	const string& i_command,
	const strpairvec& i_data,
	
	string& o_protocol,
	string& o_protocol_version,
	strpairvec& o_data)
{
	// 返すプロトコルは最初に決めておく
	o_protocol = "SAORI";
	o_protocol_version = "1.0";

	// SAORIであること。バージョンは最低たる1.0を満たしていれば良いので問わない
	if ( i_protocol != "SAORI" ) { return 400; }

	// GET Versionならそのまま200を返す
	if ( i_command == "GET Version" ) { return 200; }
	// それ以外のcommandは不正
	if ( i_command != "EXECUTE" ) { return 400; }

	// 引数を Argument? に格納
	deque<string> arguments;
	for ( strpairvec::const_iterator it=i_data.begin() ; it!=i_data.end() ; ++it)
	{
		if ( it->first.compare(0, 8, "Argument") == 0 )
		{
			const int n = atoi(it->first.c_str() + 8);
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
	deque<string> values;
	SRV	srv = request(arguments, values);

	// 戻り値を解析＆格納
	o_data.push_back( strpair("Charset", "Shift_JIS") );
	o_data.push_back( strpair("Result", srv.mResultString) );
	int n=0;
	for ( deque<string>::const_iterator it=values.begin() ; it!=values.end() ; ++it)
	{
		o_data.push_back( strpair(string("Value") + itos(n++), *it) );
	}

	return srv.mReturnCode;
}
