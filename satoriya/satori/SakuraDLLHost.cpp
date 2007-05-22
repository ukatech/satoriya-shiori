#ifndef POSIX
#  include <windows.h>
#else
#  include <stdlib.h>
#  include <string.h>
#endif
#include "SakuraDLLHost.h"
#include <iostream>
#include "../_/Sender.h"

//////////DEBUG/////////////////////////
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

#ifdef POSIX
extern "C" int load(char* i_data, long i_data_len) {
	Sender::initialize();
    string the_base_folder(i_data, i_data_len);
    free(i_data);
    sender << the_base_folder << endl;
    return SakuraDLLHost::I()->load(the_base_folder);
}
#else
extern "C" __declspec(dllexport) BOOL __cdecl load(HGLOBAL i_data, long i_data_len)
{
	string the_base_folder((char*)::GlobalLock(i_data), i_data_len);
	::GlobalFree(i_data);
	sender << the_base_folder << endl;
	return SakuraDLLHost::I()->load(the_base_folder);
}
#endif

#ifdef POSIX
extern "C" int unload(void)
#else
extern "C" __declspec(dllexport) BOOL __cdecl unload(void)
#endif
{
	return SakuraDLLHost::I()->unload();
}

#ifdef POSIX
extern "C" char* request(char* i_data, long* io_data_len) {
    // O[o�Ƃ�
    string the_req_str(i_data, *io_data_len);
    free(i_data);

    // NGXg

    string the_resp_str = SakuraDLLHost::I()->request(the_req_str);

    // O[oŕԂ
    *io_data_len = the_resp_str.size();
    char* the_return_data = static_cast<char*>(malloc(*io_data_len));
    memcpy(the_return_data, the_resp_str.c_str(), *io_data_len);
    return the_return_data;
}
#else
extern "C" __declspec(dllexport) HGLOBAL __cdecl request(HGLOBAL i_data, long* io_data_len)
{
	// O[o�Ƃ�
	string the_req_str((char*)::GlobalLock(i_data), *io_data_len);
	::GlobalFree(i_data);

	// NGXg
s
	string	the_resp_str = SakuraDLLHost::I()->request(the_req_str);

	// O[oŕԂ
	*io_data_len = the_resp_str.size();
	HGLOBAL the_return_data = ::GlobalAlloc(GMEM_FIXED, *io_data_len);
	::CopyMemory(the_return_data, the_resp_str.c_str(), *io_data_len);
	return	the_return_data;
}
#endif




string SakuraDLLHost::request(const string& i_request_string)
{
	//sender << "--- Request ---" << endl << i_request_string << endl;

	// HTTP�`̗v���

	// restɂ͖�́uc�BĂ�
	string rest = i_request_string;
	
	// �ڂ��
	string command = cut_token(rest, CRLF);
	// ��' ' �A��~�gRƂĔF�
	string protocol, protocol_version;
	for ( int n = command.size()-1 ; n >= 0 ; --n )
	{
		if ( command[n] == ' ' )
		{
			protocol_version = command.substr(n+1); // B��ācc
			protocol = cut_token(protocol_version, "/"); // /�O��B /�SႢ󂯂�
			command = command.substr(0, n);
			break;
		}
	}
	
	// ȍ~̃f[^s��
	strpairvec data;
	while ( rest.size() > 0 )
	{
		string value = cut_token(rest, CRLF);
		string key = cut_token(value, ": ");
		data.push_back( strpair(key, value) );
	}
	
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	// NGXg�s�
	
	// ߂�i[pIuWFNg
	string r_protocol, r_protocol_version;
	strpairvec r_data;
	
	int r_return_code = request(
		protocol, protocol_version, command, data, 
		r_protocol, r_protocol_version, r_data);

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

	// ԓ�TTP��Ƃč\zAԂB
	
	string response;
	response += r_protocol + "/" + r_protocol_version + " ";
	
	switch ( r_return_code ) 
	{
	case 200: response += "200 OK"; break;
	case 204: response += "204 No Content"; break;
	case 400: response += "400 Bad Request"; break;
	default: response += "500 Internal Server Error"; break;
	}
	response += CRLF;

	// Charset�t�
	bool charset_exists = false;
	for (strpairvec::iterator ite = r_data.begin(); ite != r_data.end(); ite++) {
	    if (ite->first == "Charset") {
		charset_exists = true;
		break;
	    }
	}
	if (!charset_exists) {
	    r_data.push_back(strpair("Charset", "Shift_JIS"));
	}
	
	for ( strpairvec::const_iterator i = r_data.begin() ; i != r_data.end() ; ++i )
	{
		response += i->first + ": " + i->second + CRLF;
	}
	response += CRLF;

	//sender << "--- Response ---" << endl << response << endl;
	return response;
}



