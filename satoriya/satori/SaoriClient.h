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

	// 直前のrequestの応答のErrorDescriptionヘッダ（無ければ空）。
	const wstring& error_description() const { return m_error_description; }

protected:
	// 応答のヘッダの並びから、Result / Value[n] を取り出す。
	// 注意！Valueヘッダが存在しないときは、S?系システム変数を温存するためにo_valueを上書きしないこと！
	static void analyze_response(
		const strpairvec& i_response_data,
		wstring& o_result,
		std::vector<wstring>& o_value);

	wstring m_error_description;
};

// でもSakuraDLLClientから継承するのは不適切だ
// requestだけを取り出すべきか

#ifndef POSIX

// SAORI over DirectSSTP クライアント。
//  DLLを読まず、SAORI/1.0の要求を本体へDirect SSTP（WM_COPYDATA）で送り、本体に内蔵されたモジュールを呼ぶ。
//  仕様： ukadoc の spec_saori_ex.html
class SaoriDirectClient : public SaoriClient
{
public:
	// 要求（UTF-8のバイト列）を送り、応答（バイト列）を受け取る。送れなかった・応答が無かったときはfalse。
	typedef bool (*SendFunc)(void* i_context, const std::string& i_request, std::string& o_response);

	SaoriDirectClient(const wstring& i_module, SendFunc i_send, void* i_context)
		: m_module(i_module), m_send(i_send), m_context(i_context) {}
	virtual ~SaoriDirectClient() {}

	virtual bool load(
		const wstring& i_sender,
		const wstring& i_charset,
		const wstring& i_work_folder,
		const wstring& i_dll_fullpath);
	virtual void unload();
	virtual wstring get_version(const wstring& i_security_level);

	virtual int request(
		const std::vector<wstring>& i_argument,
		bool i_is_secure,
		wstring& o_result,
		std::vector<wstring>& o_value);

	const wstring& module() const { return m_module; }

private:
	wstring m_module;
	SendFunc m_send;
	void* m_context;
};

#endif // POSIX
