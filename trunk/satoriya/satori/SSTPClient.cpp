#include "../_/stltool.h"
#include "SSTPClient.h"
#include "SakuraFMO.h"

// ƒCƒxƒ“ƒg‚ð‘—‚é“z‚Æ‚©
// http://futaba.sakura.st/sstp.html#notify11


extern bool readSakuraFMO(map<string, strmap>& oSakuraFMOMap);

bool direct_sstp(
	const string& i_script,
	const string& i_client_name,
	HWND i_client_window)
{
	SakuraFMO	fmo;
	if ( !fmo.update() )
	{
		return	false;
	}

	string request = string() +
		"SEND SSTP/1.1" + CRLF +
		"Sender: " + i_client_name + CRLF +
		"HWnd: " + itos((int)i_client_window) + CRLF +
		"Charset: Shift_JIS" + CRLF +
		"Script: " + i_script + CRLF +
		"Option: notranslate" + CRLF +
		CRLF;

	COPYDATASTRUCT cds;
	cds.dwData = 9801; // ‚Å‚¢‚¢‚Ì‚©‚È
	cds.cbData = request.size();
	cds.lpData = (LPVOID)request.c_str();

	for( map<string, strmap>::iterator i=fmo.begin() ; i!=fmo.end() ; ++i )
	{
		HWND host_window = (HWND)atoi(i->second["hwnd"].c_str());
		if ( host_window == NULL )
		{
			continue;
		}
		
		DWORD ret_dword = 0;
		::SendMessageTimeout(host_window, WM_COPYDATA, (WPARAM)i_client_window, (LPARAM)&cds,SMTO_BLOCK|SMTO_ABORTIFHUNG,5000,&ret_dword);
	}
	return true;
}
