
#pragma once

#ifndef SSU_SAORI_CALL_INTERFACE

#include	"SaoriClient.h"

void get_ssu_funclist(std::vector<string> &funclist);

class ssu : public SaoriClient {
public:
	ssu() {
		//SATORIロード時に呼ばれてるのでrandomizeは要らない
	}
	virtual ~ssu() {
		//同じくsetlocale等も要らない
	}

	virtual int request(
		const std::vector<string>& i_argument,
		bool i_is_secure,
		string& o_result,
		std::vector<string>& o_value);

	virtual bool load(
		const string& i_sender,
		const string& i_charset,
		const string& i_work_folder,
		const string& i_dll_fullpath);
	virtual void unload();
	virtual string request(const string& i_request_string);
	virtual string get_version(const string& i_security_level);
};

#endif