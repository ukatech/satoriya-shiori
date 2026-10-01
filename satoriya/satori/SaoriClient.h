#pragma once

#include "SakuraDLLClient.h"

// SAORI/1.0 -universal クライアント
//  「各栞から呼び出せる共通プラグイン規格対応DLL」を呼ぶ。
class SaoriClient : public SakuraDLLClient
{
public:
	SaoriClient() {}
	virtual ~SaoriClient() {}

	virtual bool load(
		const wstring& i_sender,
		const wstring& i_charset,
		const wstring& i_work_folder,
		const wstring& i_dll_fullpath);

	virtual int request(
		const std::vector<wstring>& i_argument,
		bool i_is_secure,
		wstring& o_result,
		std::vector<wstring>& o_value);
};

// でもSakuraDLLClientから継承するのは不適切だ
// requestだけを取り出すべきか


