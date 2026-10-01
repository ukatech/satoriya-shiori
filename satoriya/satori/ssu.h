
#pragma once

#ifndef SSU_SAORI_CALL_INTERFACE

#include	"SaoriClient.h"

void get_ssu_funclist(std::vector<wstring> &funclist);

class ssu : public SaoriClient {
public:
	ssu() {
		//SATORIロード時に呼ばれてるのでrandomizeは要らない
	}
	virtual ~ssu() {
		//同じくsetlocale等も要らない
	}

	virtual int request(
		const std::vector<wstring>& i_argument,
		bool i_is_secure,
		wstring& o_result,
		std::vector<wstring>& o_value);

	virtual bool load(
		const wstring& i_sender,
		const wstring& i_charset,
		const wstring& i_work_folder,
		const wstring& i_dll_fullpath);
	virtual void unload();
	virtual wstring request(const wstring& i_request_string);
	virtual wstring get_version(const wstring& i_security_level);
};

#endif