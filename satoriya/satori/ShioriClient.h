#include "SakuraDLLClient.h"

// SHIORI/3.0クライアント
class ShioriClient : public SakuraDLLClient
{
public:
	ShioriClient() {}
	virtual ~ShioriClient() {}

	bool load(
		const string& i_sender, // クライアント名。USER_AGENTみたいなもの。
		const string& i_charset, // 文字コードセット。通常はShift_JIS
		const string& i_work_folder, // 栞のloadに渡すフォルダ名（辞書のあるフォルダ名）
		const string& i_dll_fullpath); // 栞のフルパス

	int request(
		const string& i_id, // OnBootとか
		const vector<string>& i_references, // Reference?
		bool i_is_secure, // SecurityLevel
		string& o_value, // 栞が返したさくらスクリプトや文字列リソース
		vector<string>& o_references // 複数戻り値。通常はOnCommunicateでしか使わない
		);
};