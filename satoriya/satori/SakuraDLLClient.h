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
		const string& i_sender,
		const string& i_charset,
		const string& i_protocol,
		const string& i_protocol_version,
		const string& i_work_folder,	// 作業ディレクトリ。DLL::loadの引数。最後に \ または / が必要
		const string& i_dll_fullpath);	// DLLのフルパス

	// おわるよ
	virtual void unload();

	// バージョン取得。GET Versionして"SAORI/1.0" みたいのを返す。
	virtual string get_version(const string& i_security_level);

	// 素のリクエスト文字列を送り、素のレスポンス文字列を受け取る。
	virtual string request(const string& i_request_string);

	// リクエストを送り、レスポンスを受け取る。戻り値はリターンコード。
	virtual int request(
		const string& i_command,
		const strpairvec& i_data,
		strpairvec& o_data);

private:
	string m_sender;
	string m_charset;
	string m_protocol;
	string m_protocol_version;

#ifdef POSIX
	void*   mModule;
	bool    (*mLoad)(char* h, long len);
	char*   (*mRequest)(char* h, long* len);
	bool    (*mUnload)();
#else
	HMODULE	mModule;
	BOOL	(*mLoad)(HGLOBAL h, long len);
	HGLOBAL	(*mRequest)(HGLOBAL h, long* len);
	BOOL	(*mUnload)();
#endif
};
