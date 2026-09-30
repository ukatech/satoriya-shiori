#include "SakuraDLLHost.h"

#include	<string>
#include	<deque>
using std::wstring;

// SAORI戻り値
struct SRV {
	int	mReturnCode;
	wstring	mResultString;
	SRV(int iReturnCode, wstring iResulutString=L"") : mReturnCode(iReturnCode), mResultString(iResulutString) {}
	SRV(wstring iResulutString) : mReturnCode(200), mResultString(iResulutString) {}
	SRV(const wchar_t* iResulutString) : mReturnCode(200), mResultString(iResulutString) {}
};


// SAORIベースクラス
class SaoriHost : public SakuraDLLHost
{
	virtual int	request(
		const wstring& i_protocol,
		const wstring& i_protocol_version,
		const wstring& i_command,
		const strpairvec& i_data,
		
		wstring& o_protocol,
		wstring& o_protocol_version,
		strpairvec& o_data);
protected:
	// 直前のリクエストが SecurityLevel: local（または指定なし）か。external と明示されたときだけ偽。
	bool	m_is_secure;
public:
	SaoriHost() : SakuraDLLHost(), m_is_secure(true) {}

	// SAORIとしての返答は常にUTF-8
	virtual CharactorSet response_charset(CharactorSet i_request_charset) { return CS_UTF8; }
	virtual ~SaoriHost() {}

	virtual SRV	request(std::deque<wstring>& iArguments, std::deque<wstring>& oValues)=0;
		// iArgumentsはいじられます。
};
