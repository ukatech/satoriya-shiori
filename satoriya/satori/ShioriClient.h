#include "SakuraDLLClient.h"

// SHIORI/3.0クライアント
class ShioriClient : public SakuraDLLClient
{
public:
	ShioriClient() {}
	virtual ~ShioriClient() {}

	bool load(
		const wstring& i_sender, // クライアント名。USER_AGENTみたいなもの。
		const wstring& i_charset, // 文字コードセット。通常はShift_JIS
		const wstring& i_work_folder, // 栞のloadに渡すフォルダ名（辞書のあるフォルダ名）
		const wstring& i_dll_fullpath); // 栞のフルパス

	int request(
		const wstring& i_id, // OnBootとか
		const std::vector<wstring>& i_references, // Reference?
		bool i_is_secure, // SecurityLevel
		wstring& o_value, // 栞が返したさくらスクリプトや文字列リソース
		std::vector<wstring>& o_references // 複数戻り値。通常はOnCommunicateでしか使わない
		);
};