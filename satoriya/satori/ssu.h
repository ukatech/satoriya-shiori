
#pragma once

#ifndef SSU_SAORI_CALL_INTERFACE

#include	"SaoriClient.h"

void get_ssu_funclist(std::vector<wstring> &funclist);

// regex_* のパターンの中で、パターンに定義の無いサブルーチン名（(?&名前) や \g<名前>）が出てきたときに、
// その名前の中身を求める関数。取得できたら true を返して o_result に入れる。NULL ならすべて未定義として扱う。
typedef bool (*SsuNameResolver)(const wstring& i_name, wstring& o_result, void* i_context);
void set_ssu_name_resolver(SsuNameResolver i_resolver, void* i_context);

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