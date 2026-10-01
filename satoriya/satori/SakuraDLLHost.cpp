#ifndef POSIX
#  include <windows.h>
#else
#  include <stdlib.h>
#  include <string.h>
#  include "satori.h"
#endif
#include "SakuraDLLHost.h"
#include <iostream>
#include "../_/Sender.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

#ifndef POSIX
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	return TRUE;
}
#endif

// 受け取ったメモリをstd::stringにする。lenには終端のNULが含まれている場合もある。
static std::string	to_bytes(const char* p, long len)
{
	if ( p == NULL || len <= 0 ) { return std::string(); }
	std::string r(p, len);
	std::string::size_type nul = r.find('\0');
	if ( nul != std::string::npos ) {
		r.erase(nul);
	}
	return r;
}

#ifdef POSIX

// POSIXではパスはUTF-8で渡される
extern "C" int satori_load(char* i_data, long i_data_len) {
//	GetSender().initialize();
    wstring the_base_folder = UTF8toW(to_bytes(i_data, i_data_len));
    free(i_data);
    GetSender().sender() << the_base_folder << std::endl;
    int id = SakuraDLLHost::Create<Satori>();
    SakuraDLLHost::Select(id);
    SakuraDLLHost::I()->load(the_base_folder);
    return id;
}

extern "C" int load(char* i_data, long i_data_len) {
//	GetSender().initialize();
    wstring the_base_folder = UTF8toW(to_bytes(i_data, i_data_len));
    free(i_data);
    GetSender().sender() << the_base_folder << std::endl;
    return SakuraDLLHost::I()->load(the_base_folder);
}
#else
// loadu が先に呼ばれていたら load は無視する（DLL共通仕様）
static bool s_loadu_called = false;

// 従来のload。パスはOSの既定コードページ。
extern "C" __declspec(dllexport) BOOL __cdecl load(HGLOBAL i_data, long i_data_len)
{
	wstring the_base_folder = ACPtoW(to_bytes((const char*)::GlobalLock(i_data), i_data_len));
	::GlobalFree(i_data);
	if ( s_loadu_called ) {
		return TRUE;
	}
	GetSender().sender() << the_base_folder << std::endl;
	try {
		return SakuraDLLHost::I()->load(the_base_folder);
	}
	catch ( ... ) {
		// 例外を呼び出し元に漏らさない
		return FALSE;
	}
}

// UTF-8版load。
extern "C" __declspec(dllexport) BOOL __cdecl loadu(HGLOBAL i_data, long i_data_len)
{
	wstring the_base_folder = UTF8toW(to_bytes((const char*)::GlobalLock(i_data), i_data_len));
	::GlobalFree(i_data);
	s_loadu_called = true;
	GetSender().sender() << the_base_folder << std::endl;
	try {
		return SakuraDLLHost::I()->load(the_base_folder);
	}
	catch ( ... ) {
		// 例外を呼び出し元に漏らさない
		return FALSE;
	}
}

// チェックツール（tama/tamac）のログ受信ウィンドウを指定する（YAYA互換）
extern "C" __declspec(dllexport) BOOL __cdecl logsend(long hwnd)
{
	GetSender().set_receiver_window((HWND)hwnd);
	return TRUE;
}
#endif

// ログをコールバックで受け取る（YAYA互換）。設定中は logsend のウィンドウには送らない
#ifdef POSIX
extern "C" void Set_loghandler(void (*loghandler)(const wchar_t *str, int mode, int id))
#else
extern "C" __declspec(dllexport) void __cdecl Set_loghandler(void (*loghandler)(const wchar_t *str, int mode, int id))
#endif
{
	GetSender().set_loghandler(loghandler);
}

#ifdef POSIX
extern "C" int satori_unload(int id)
{
    SakuraDLLHost::Select(id);
	int ret = SakuraDLLHost::I()->unload();
    SakuraDLLHost::Destroy(id);
    return ret;
}

extern "C" int unload(void)
#else
extern "C" __declspec(dllexport) BOOL __cdecl unload(void)
#endif
{
#ifndef POSIX
	s_loadu_called = false;
#endif
	try {
		return SakuraDLLHost::I()->unload();
	}
	catch ( ... ) {
		// 例外を呼び出し元に漏らさない
		return 0;
	}
}

#ifdef POSIX
static char* return_bytes(const std::string& the_resp_str, long* io_data_len)
{
    *io_data_len = the_resp_str.size();
    char* the_return_data = static_cast<char*>(malloc(*io_data_len + 1));
    memcpy(the_return_data, the_resp_str.c_str(), *io_data_len + 1);
    return the_return_data;
}

extern "C" char* satori_request(int id, char* i_data, long* io_data_len) {
    // グローバルメモリを受けとる
    std::string the_req_str(i_data, *io_data_len);
    free(i_data);

    SakuraDLLHost::Select(id);
    // リクエスト実行
    std::string the_resp_str = SakuraDLLHost::I()->request_bytes(the_req_str);

    // グローバルメモリで返す
    return return_bytes(the_resp_str, io_data_len);
}

extern "C" char* request(char* i_data, long* io_data_len) {
    // グローバルメモリを受けとる
    std::string the_req_str(i_data, *io_data_len);
    free(i_data);

    // リクエスト実行
    std::string the_resp_str = SakuraDLLHost::I()->request_bytes(the_req_str);

    // グローバルメモリで返す
    return return_bytes(the_resp_str, io_data_len);
}
#else
static HGLOBAL return_bytes(const std::string& the_resp_str, long* io_data_len)
{
	*io_data_len = the_resp_str.size();
	HGLOBAL the_return_data = ::GlobalAlloc(GMEM_FIXED, *io_data_len + 1);
	::CopyMemory(the_return_data, the_resp_str.c_str(), *io_data_len + 1);
	return	the_return_data;
}

extern "C" __declspec(dllexport) HGLOBAL __cdecl request(HGLOBAL i_data, long* io_data_len)
{
	// グローバルメモリを受けとる
	std::string the_req_str((const char*)::GlobalLock(i_data), *io_data_len);
	::GlobalFree(i_data);

	// リクエスト実行
	std::string	the_resp_str = SakuraDLLHost::I()->request_bytes(the_req_str);

	// グローバルメモリで返す
	return return_bytes(the_resp_str, io_data_len);
}
#endif


#ifndef POSIX
extern "C" __declspec(dllexport) HGLOBAL __cdecl getversionlist(HGLOBAL i_data, long* io_data_len)
{
	// グローバルメモリを受けとる
	wstring the_req_str = ACPtoW(to_bytes((const char*)::GlobalLock(i_data), *io_data_len));
	::GlobalFree(i_data);

	// リクエスト実行
	wstring	the_resp_str = SakuraDLLHost::I()->getversionlist(the_req_str);

	// グローバルメモリで返す
	return return_bytes(WtoACP(the_resp_str), io_data_len);
}
#endif


std::string SakuraDLLHost::request_bytes(const std::string& i_request_bytes)
{
	CharactorSet cs = CharsetFromName(UTF8toW(find_charset_header(i_request_bytes)));
	if ( cs == CS_NULL ) {
		cs = DetectCharset(i_request_bytes);
	}
	wstring the_response = request(MBtoW(i_request_bytes, cs), cs);

	// 応答のCharsetヘッダで変換する
	CharactorSet response_cs = CharsetFromName(UTF8toW(find_charset_header(WtoUTF8(the_response))));
	if ( response_cs == CS_NULL ) {
		response_cs = CS_UTF8;
	}
	return WtoMB(the_response, response_cs);
}

wstring SakuraDLLHost::request(const wstring& i_request_string, CharactorSet i_request_charset)
{
	//GetSender().sender() << "--- Request ---" << endl << i_request_string << endl;

	// HTTPもどき形式の要求文字列を解析する

	// restには未解釈の「残り」が入っている
	wstring rest = i_request_string;
	
	// 一行目を切り出し
	wstring command = cut_token(rest, CRLF);
	// 後ろから ' ' を探し、見つかればそれ以降をプロトコル部分として認識する
	wstring protocol, protocol_version;
	for ( int n = command.size()-1 ; n >= 0 ; --n )
	{
		if ( command[n] == L' ' )
		{
			protocol_version = command.substr(n+1); // いったん全部を預けて……
			protocol = cut_token(protocol_version, L"/"); // /より前を取り出す。 /が無ければ全部貰い受ける
			command = command.substr(0, n);
			break;
		}
	}
	
	// 以降のデータ行を切り出し
	strpairvec data;
	while ( rest.size() > 0 )
	{
		wstring value = cut_token(rest, CRLF);
		wstring key = cut_token(value, L": ");
		data.push_back( strpair(key, value) );
	}
	
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	// リクエストを実行する
	
	// 戻り値格納用オブジェクト
	wstring r_protocol, r_protocol_version;
	strpairvec r_data;
	
	// 例外（メモリ不足や範囲外など）を呼び出し元に漏らすと、ベースウェアごと落ちてしまう。
	// ここで受け止めて、500 を返す。
	int r_return_code;
	try
	{
		r_return_code = request(
			protocol, protocol_version, command, data, 
			r_protocol, r_protocol_version, r_data);
	}
	catch ( const std::exception& e )
	{
		GetSender().sender() << L"内部エラー（例外）でリクエストを中断しました: " << ascii_to_w(e.what()) << std::endl;
		r_return_code = -1;
	}
	catch ( ... )
	{
		GetSender().sender() << L"内部エラー（例外）でリクエストを中断しました。" << std::endl;
		r_return_code = -1;
	}
	if ( r_return_code == -1 )
	{
		on_request_exception();
		r_return_code = 500;
		r_data.clear();
		r_protocol = protocol;
		r_protocol_version = protocol_version;
	}

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// 返答をHTTPもどき文字列形式として構築し、返す。
	
	wstring response;
	response += r_protocol + L"/" + r_protocol_version + L" ";
	
	switch ( r_return_code ) 
	{
	case 200: response += L"200 OK"; break;
	case 204: response += L"204 No Content"; break;
	case 400: response += L"400 Bad Request"; break;
	default: response += L"500 Internal Server Error"; break;
	}
	response += CRLF;

	// Charsetが無ければ付ける。先頭に置く。
	bool charset_exists = false;
	for (strpairvec::const_iterator ite = r_data.begin(); ite != r_data.end(); ite++) {
	    if (ite->first == L"Charset") {
			charset_exists = true;
			break;
	    }
	}
	if (!charset_exists) {
	    r_data.insert(r_data.begin(), strpair(L"Charset", CharsetName(response_charset(i_request_charset))));
	}
	
	for ( strpairvec::const_iterator i = r_data.begin() ; i != r_data.end() ; ++i )
	{
		response += i->first + L": " + i->second + CRLF;
	}
	response += CRLF;

	//GetSender().sender() << "--- Response ---" << endl << response << endl;
	return response;
}




