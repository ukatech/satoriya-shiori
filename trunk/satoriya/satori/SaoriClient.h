#include "SakuraDLLClient.h"

// SAORI/1.0 -universal クライアント
//  「各栞から呼び出せる共通プラグイン規格対応DLL」を呼ぶ。
class SaoriClient : public SakuraDLLClient
{
public:
	SaoriClient() {}
	virtual ~SaoriClient() {}

	bool load(
		const string& i_sender,
		const string& i_charset,
		const string& i_work_folder,
		const string& i_dll_fullpath);

	int request(
		const vector<string>& i_argument,
		bool i_is_secure,
		string& o_result,
		vector<string>& o_value);
};

// でもSakuraDLLClientから継承するのは不適切だ
// requestだけを取り出すべきか


