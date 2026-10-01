#ifndef SAKURACLIENT_H
#define SAKURACLIENT_H

#include "SakuraCS.h"

// SHIORI, SAORI, MAKOTO, SSTPなどのクライアント。
// リクエストを送る側の純粋仮想ベースクラス。
class SakuraClient : public SakuraCS
{
public:
	SakuraClient() {}
	virtual ~SakuraClient() {}
	
	// 素のリクエスト文字列を送り、素のレスポンス文字列を受けとる。
	// 実装は通信手段による。
	virtual wstring request(const wstring& i_request_string)=0;

	// リクエストを送り、レスポンスを受け取る。戻り値はリターンコード。
	virtual int request(
		const wstring& i_protocol,
		const wstring& i_protocol_version,
		const wstring& i_command,
		const strpairvec& i_data,
		
		wstring& o_protocol,
		wstring& o_protocol_version,
		strpairvec& o_data);
};


#endif // SAKURACLIENT_H
