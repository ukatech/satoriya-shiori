#include "../_/stltool.h"
#include "SSTPClient.h"
#include "SakuraFMO.h"

// ƒCƒxƒ“ƒg‚ð‘—‚é“z‚Æ‚©
// http://futaba.sakura.st/sstp.html#notify11

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

extern bool readSakuraFMO(std::map<wstring, strmap>& oSakuraFMOMap);

bool direct_sstp(
	const wstring& i_script,
	const wstring& i_client_name,
	HWND i_client_window)
{
	SakuraFMO	fmo;
	if ( !fmo.update() )
	{
		return	false;
	}

	wstring request = wstring() +
		L"SEND SSTP/1.1" + CRLF +
		L"Charset: UTF-8" + CRLF +
		L"Sender: " + i_client_name + CRLF +
		L"HWnd: " + itos((int)i_client_window) + CRLF +
		L"Script: " + i_script + CRLF +
		L"Option: notranslate" + CRLF +
		CRLF;

	COPYDATASTRUCT cds;
	cds.dwData = 9801; // ‚Å‚¢‚¢‚Ì‚©‚È
	const std::string request_bytes = WtoUTF8(request);
	cds.cbData = request_bytes.size();
	cds.lpData = (LPVOID)request_bytes.c_str();

	for( std::map<wstring, strmap>::iterator i=fmo.begin() ; i!=fmo.end() ; ++i )
	{
		HWND host_window = (HWND)_wtoi(i->second[L"hwnd"].c_str());
		if ( host_window == NULL )
		{
			continue;
		}
		
		DWORD ret_dword = 0;
		::SendMessageTimeout(host_window, WM_COPYDATA, (WPARAM)i_client_window, (LPARAM)&cds,SMTO_NORMAL|SMTO_ABORTIFHUNG,5000,&ret_dword);
	}
	return true;
}
