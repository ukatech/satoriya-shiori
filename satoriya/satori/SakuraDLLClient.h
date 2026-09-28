#pragma once

#include "SakuraClient.h"

#ifndef POSIX
#  include <windows.h> // HMODULE,BOOL,HGLOBALとか
#endif

// dllホルダー。dllをロードしてリクエストを送る
class SakuraDLLClient : public SakuraClient
{
public:
	SakuraDLLClient();
	virtual ~SakuraDLLClient();

	// はじめるよ
	virtual bool load(
		const wstring& i_sender,
		const wstring& i_charset,
		const wstring& i_protocol,
		const wstring& i_protocol_version,
		const wstring& i_work_folder,	// 作業ディレクトリ。DLL::loadの引数。最後に \ または / が必要
		const wstring& i_dll_fullpath);	// DLLのフルパス

	// おわるよ
	virtual void unload();

	// バージョン取得。GET Versionして"SAORI/1.0" みたいのを返す。
	virtual wstring get_version(const wstring& i_security_level);

	// 素のリクエスト文字列を送り、素のレスポンス文字列を受け取る。
	virtual wstring request(const wstring& i_request_string);

	// リクエストを送り、レスポンスを受け取る。戻り値はリターンコード。
	virtual int request(
		const wstring& i_command,
		const strpairvec& i_data,
		strpairvec& o_data);

private:
	wstring m_sender;
	wstring m_charset;
	wstring m_protocol;
	wstring m_protocol_version;

#ifdef POSIX
	void*   mModule;
	bool    (*mLoad)(char* h, long len);
	char*   (*mRequest)(char* h, long* len);
	bool    (*mUnload)();
#else
	HMODULE	mModule;
	BOOL	(*mLoad)(HGLOBAL h, long len);
	BOOL	(*mLoadU)(HGLOBAL h, long len);	// UTF-8版load
	HGLOBAL	(*mRequest)(HGLOBAL h, long* len);
	BOOL	(*mUnload)();
#endif
};
