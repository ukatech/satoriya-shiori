#include "SakuraCS.h"

// 「dllとして呼ばれる側」を作る際に使用するクラス。
// これを継承したクラスを定義し、m_dllに唯一の実体を代入する。
class SakuraDLLHost : public SakuraCS
{
protected:
	static SakuraDLLHost* m_dll;
public:
	static SakuraDLLHost* I() { return m_dll; }

	SakuraDLLHost() : SakuraCS() {}
	virtual ~SakuraDLLHost() {}

	virtual bool	load(const string& i_base_folder) { return true; }
	virtual bool	unload() { return true; }

	// 素のリクエスト文字列を受け取り、素のレスポンス文字列を返す。
	// 内部で↓を呼ぶ。
	virtual string request(const string& i_request_string);
	
	// リクエストを実行。
	// 継承してオーバーライドしてください。
	// 戻り値はリターンコード。取り得る値は200,204,400,500。
	virtual int	request(
		const string& i_protocol,
		const string& i_protocol_version,
		const string& i_command,
		const strpairvec& i_data,
		
		string& o_protocol,
		string& o_protocol_version,
		strpairvec& o_data)=0;
};
