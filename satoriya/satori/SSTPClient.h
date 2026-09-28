
#include <windows.h>
#include <string>
using std::wstring;


bool direct_sstp(
	const wstring& i_script = L"\\0\\s[0]Ç…Ç±Ç…Ç±ÅB\\1\\s[10]Ç…Ç±Ç…Ç±ÅB\\e",
	const wstring& i_client_name = L"ëóêMé“Ç≥ÇÒ",
	HWND i_client_window = NULL);

#include "SakuraClient.h"
/*

class SSTPClient : public SakuraClient
{
public:
	SSTPClient() {}
	virtual ~SSTPClient() {}
	
	virtual string request(const string& i_request_string);

	// ÇÒÅ`

};
*/
