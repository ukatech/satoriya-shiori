#include "SakuraDLLClient.h"
#include "../_/Sender.h"
#include <assert.h>

#ifdef POSIX
#  include <dlfcn.h>
#  include <string.h>
#  include <stdlib.h>
#  define FALSE 0
#endif

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


SakuraDLLClient::SakuraDLLClient()
{
	m_version_reply_has_charset = true;
	mModule = NULL;
	mLoad = NULL;
#ifndef POSIX
	mLoadU = NULL;
#endif
	mRequest = NULL;
	mUnload = NULL;
}

SakuraDLLClient::~SakuraDLLClient()
{
	unload();
}

wstring	SakuraDLLClient::request(const wstring& iRequestString)
{
	if ( mRequest==NULL )
	{
		GetSender().errsender() << L"SakuraDLLClient::request: ロードしていないライブラリにrequestしようとしました。" << satori::endl;
		return	L"";
	}

	CharactorSet cs = CharsetFromName(m_charset);
	if ( cs == CS_NULL ) {
		cs = CS_UTF8;
	}
	const std::string theRequest = WtoMB(iRequestString, cs);

	long len = theRequest.length();

#ifdef POSIX
	char* h = static_cast<char*>(malloc(len + 1));
#else
	HGLOBAL h = ::GlobalAlloc(GMEM_FIXED, len + 1);
#endif

	if ( h == NULL )
	{
		GetSender().errsender() << L"SakuraDLLClient::request: リクエスト用のメモリを確保できませんでした。" << satori::endl;
		return	L"";
	}

	memcpy(h, theRequest.c_str(), len + 1); //ZeroTermまで
	h = mRequest(h, &len);

	std::string theResponse;
	if ( h ) {
	#ifdef POSIX
		theResponse.assign(static_cast<char*>(h), len);
		free(h);
	#else
		void *pLock = ::GlobalLock(h);
		if ( pLock ) {
			theResponse.assign(static_cast<char*>(pLock), len);
			::GlobalUnlock(h);
			::GlobalFree(h);
		}
		else { //バグ対策 - GlobalAllocで確保してないポインタ向け
			theResponse.assign(reinterpret_cast<char*>(h), len);
		}
	#endif
	}
	else {
		return wstring(L"");
	}

	// 返答はCharsetヘッダに従う。無ければ、Shift_JISで送った相手ならShift_JIS、それ以外は判定する。
	// （Shift_JISの半角カナはUTF-8として正しい並びになることがあるので、判定に任せない）
	CharactorSet response_cs = CharsetFromName(UTF8toW(find_charset_header(theResponse)));
	if ( response_cs == CS_NULL && cs == CS_SJIS ) {
		response_cs = CS_SJIS;
	}
	return MBtoW(theResponse, response_cs);
}

// バージョン取得。GET Versionして"SAORI/1.0" みたいのを返す。
wstring SakuraDLLClient::get_version(const wstring& i_security_level)
{
	strpairvec data;
	data.push_back( strpair(L"Sender", m_sender) );
	data.push_back( strpair(L"Charset", m_charset) );
	data.push_back( strpair(L"SecurityLevel", i_security_level) );

	wstring r_protocol, r_protocol_version;
	strpairvec r_data;
	this->SakuraClient::request(
		m_protocol,
		m_protocol_version,
		L"GET Version",
		data,
		r_protocol,
		r_protocol_version,
		r_data);

	m_version_reply_has_charset = false;
	for ( strpairvec::const_iterator i = r_data.begin() ; i != r_data.end() ; ++i )
	{
		wstring key = i->first;
		if ( ! key.empty() && key[key.size()-1] == L':' ) { key.erase(key.size()-1); }
		if ( _wcsicmp(key.c_str(), L"Charset") == 0 && ! i->second.empty() ) {
			m_version_reply_has_charset = true;
		}
	}

	return r_protocol + L"/" + r_protocol_version;
}

// リクエストを送り、レスポンスを受け取る。戻り値はリターンコード。
int SakuraDLLClient::request(
	const wstring& i_command,
	const strpairvec& i_data,
	strpairvec& o_data)
{
	if ( ! mRequest ) { return 204; }

	wstring r_protocol, r_protocol_version;
	int return_code = this->SakuraClient::request(
		m_protocol,
		m_protocol_version,
		i_command,
		i_data,
		r_protocol,
		r_protocol_version,
		o_data);
	return return_code;
}

void	SakuraDLLClient::unload()
{
#ifdef POSIX
    if (mModule != NULL) {
	    dlclose(mModule);
	}
#else
	if ( mUnload != NULL )
		mUnload();
	if ( mModule != NULL )
		::FreeLibrary(mModule);
#endif
	mModule = NULL;
	mLoad = NULL;
#ifndef POSIX
	mLoadU = NULL;
#endif
	mRequest = NULL;
	mUnload = NULL;
}


bool	SakuraDLLClient::load(
	const wstring& i_sender,
	const wstring& i_charset,
	const wstring& i_protocol,
	const wstring& i_protocol_version,
	const wstring& i_work_folder,
	const wstring& i_dll_fullpath)
{
	unload();

	m_sender = i_sender;
	m_charset = i_charset;
	m_protocol = i_protocol;
	m_protocol_version = i_protocol_version;

	wstring work_folder = unify_dir_char(i_work_folder);
	wstring dll_fullpath = unify_dir_char(i_dll_fullpath);

	GetSender().sender() << L"SakuraDLLClient::load '" << dll_fullpath << L"' ...";

#ifdef POSIX
	mModule = dlopen(WtoUTF8(dll_fullpath).c_str(), RTLD_LAZY);
	if (mModule == NULL) {
	    GetSender().sender() << L"failed." << std::endl;
	    GetSender().errsender() << UTF8toW(dlerror()) << satori::endl;
	    return false;
	}
#else
	mModule = ::LoadLibraryEx(dll_fullpath.c_str(),NULL,LOAD_WITH_ALTERED_SEARCH_PATH);
	if ( mModule==NULL ) {
		GetSender().sender() << L"failed." << std::endl;
		GetSender().errsender() << dll_fullpath + L": LoadLibraryで失敗。" << satori::endl;
		return	false;
	}
#endif

#ifdef POSIX
	mLoad = (bool (*)(char*, long))dlsym(mModule, "load");
	mRequest = (char* (*)(char*, long*))dlsym(mModule, "request");
	mUnload = (bool (*)())dlsym(mModule, "unload");
#else
	mLoad = (BOOL (*)(HGLOBAL, long))::GetProcAddress(mModule, "load");
	if ( ! mLoad ) {
		mLoad = (BOOL (*)(HGLOBAL, long))::GetProcAddress(mModule, "_load");
	}
	mLoadU = (BOOL (*)(HGLOBAL, long))::GetProcAddress(mModule, "loadu");
	if ( ! mLoadU ) {
		mLoadU = (BOOL (*)(HGLOBAL, long))::GetProcAddress(mModule, "_loadu");
	}
	mRequest = (HGLOBAL (*)(HGLOBAL, long*))::GetProcAddress(mModule, "request");
	if ( ! mRequest ) {
		mRequest = (HGLOBAL (*)(HGLOBAL, long*))::GetProcAddress(mModule, "_request");
	}
	mUnload = (BOOL (*)())::GetProcAddress(mModule, "unload");
	if ( ! mUnload ) {
		mUnload = (BOOL (*)())::GetProcAddress(mModule, "_unload");
	}
#endif

	if ( mRequest==NULL )
	{
		GetSender().sender() << L"failed." << std::endl;
		unload();
		GetSender().errsender() << dll_fullpath + L": requestがエクスポートされていません。" << satori::endl;
		return	false;
	}

#ifdef POSIX
	bool (*theLoad)(char*, long) = mLoad;
	const std::string theFolder = WtoUTF8(work_folder);
#else
	// loaduがあればUTF-8でパスを渡す。無ければOSの既定コードページで渡す。
	BOOL (*theLoad)(HGLOBAL, long) = mLoadU ? mLoadU : mLoad;
	const std::string theFolder = mLoadU ? WtoUTF8(work_folder) : WtoACP(work_folder);
#endif

	if ( theLoad!=NULL )
	{
		long len = theFolder.length();
#ifdef POSIX
		char* h = static_cast<char*>(malloc(len + 1));
#else
		HGLOBAL h = ::GlobalAlloc(GMEM_FIXED, len + 1);
#endif
		if ( h == NULL )
		{
			GetSender().sender() << L"failed." << std::endl;
			unload();
			GetSender().errsender() << dll_fullpath + L": load()に渡すメモリを確保できませんでした。" << satori::endl;
			return	false;
		}
		memcpy(h, theFolder.c_str(), len + 1); //ZeroTermまで

		if ( theLoad(h, len) == FALSE )
		{
			GetSender().sender() << L"failed." << std::endl;
			unload();
			GetSender().errsender() << dll_fullpath + L": load()がFALSEを返しました。" << satori::endl;
			return	false;
		}
	}

	GetSender().sender() << L"succeed." <<std::endl;
	return	true;
}
