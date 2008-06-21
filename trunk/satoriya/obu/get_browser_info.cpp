
/*--------------------------
	include
--------------------------*/
#define _INC_OLE
#include <windows.h>
#undef  _INC_OLE
#include <commctrl.h>
#include <DDEML.H>

//#include "resource.h"

#include	<string>
using namespace std;

/*--------------------------
	define
--------------------------*/
#define BUFSIZE				256
#define MAXSIZE				65535

#define BROWSE_CNT			500

#define WM_GETBROUSERINFO	(WM_USER + 1)

#define	CODEPAGE	CP_WINANSI

/*--------------------------
	function
--------------------------*/

//DDE Callback
HDDEDATA CALLBACK DDECallback(UINT uType, UINT uFmt, HCONV hcconv, HSZ hszTpc1, HSZ hszTpc2,
        HDDEDATA hdata, DWORD dwData1, DWORD dwData2)
//
//HDDEDATA CALLBACK DDECallback(WORD wType, WORD wFmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA hData, DWORD lData1, DWORD lData2)
{
	return (HDDEDATA)0;
}

HCONV	DDE_Connect(DWORD iDDEID, const char* iDDE_ServerName) {
	HSZ hszService=DdeCreateStringHandle(iDDEID, iDDE_ServerName, CODEPAGE);
	HSZ hszTopic = DdeCreateStringHandle(iDDEID, "WWW_GetWindowInfo", CODEPAGE);
	HCONV hConv = DdeConnect(iDDEID, hszService, hszTopic, NULL);
	DdeFreeStringHandle(iDDEID, hszTopic);
	DdeFreeStringHandle(iDDEID, hszService);
	return	hConv;
}

//============================================================================

//最後にアクティブだったブラウザの情報の取得
BOOL DDE_GetActiveBrowseInfo(char *RetInfo, int RetInfoLen)
{
	HSZ hszParam;
	HCONV hConv;
	HDDEDATA hDDEData;
	DWORD m_dwDDEID = 0;

	//DDEの初期化
	if(DdeInitialize(&m_dwDDEID, 
		//(PFNCALLBACK)MakeProcInstance((FARPROC)DDECallback,	//AfxGetInstanceHandle()),
		DDECallback,
		CBF_SKIP_ALLNOTIFICATIONS | APPCMD_CLIENTONLY, 
		0L) != DMLERR_NO_ERROR){
		//MessageBox(NULL, "DDEの初期化に失敗しました。","ブラウザ情報の取得", MB_ICONWARNING | MB_OK);
		return FALSE;
	}


	static	bool	first = true;
	static const char*	DDE_ServerNames[] = 
		{"IEXPLORE", "Opera", "Firefox", "NETSCAPE", "Mozilla", "Netscape6", "Netscape Browser", 
		"MBROWSER", "SLEIPNIR", "MDIBROWSER", "TaBrowser", "Donut", "fub", "Cuam" };
	const int max = sizeof(DDE_ServerNames)/sizeof(DDE_ServerNames[0]);
	static	int	server_num=-1;
	static	int	search_num=0;

	if ( first ) {
		first = false;
		// 初回
		for (server_num=0 ; server_num<max ; ++server_num)
			if ( (hConv=DDE_Connect(m_dwDDEID, DDE_ServerNames[server_num])) != 0 )
				break;
	}
	else if ( server_num==-1 ) {
		if ( (hConv=DDE_Connect(m_dwDDEID, DDE_ServerNames[search_num])) != 0 )
			server_num = search_num;
		else
			if ( ++search_num >= max )
				search_num = 0;
	}
	else {
		hConv=DDE_Connect(m_dwDDEID, DDE_ServerNames[search_num]);
	}

	if ( hConv==0 ) {
		server_num = -1;
		DdeUninitialize(m_dwDDEID);
		return FALSE;
	} 
	
	// 0xFFFFFFFF は最後にアクティブだったブラウザを指す。
	// ブラウザのIDを指定するとそのブラウザの情報をもってくる。
	hszParam = DdeCreateStringHandle(m_dwDDEID, "0xFFFFFFFF", CODEPAGE);
	//Client Transaction
	hDDEData = DdeClientTransaction(NULL, 0, hConv, hszParam, CF_TEXT, XTYP_REQUEST, 10000L, NULL);
	DdeFreeStringHandle(m_dwDDEID, hszParam);
	if(hDDEData == 0){
		DdeDisconnect(hConv);
		DdeUninitialize(m_dwDDEID);
		return FALSE;
	}
	
	//Get Data
	DdeGetData(hDDEData, (LPBYTE)RetInfo, RetInfoLen, 0);
	/*string	str(RetInfo, RetInfoLen);
	::OutputDebugString(str.c_str());
	::OutputDebugString("\n");*/

	DdeFreeDataHandle(hDDEData);
	DdeDisconnect(hConv);
	DdeUninitialize(m_dwDDEID);
	return TRUE;
}

//============================================================================

#include	<mbctype.h>
//DDEでブラウザから取得した情報を URL , Title , Frame に展開する
//	"http://hoge.com/","hoge","Frame"
void GetDDEStr(char *buf, string &URL, string &Title)
{
	char *p = (char *)buf;

	URL.erase();

	//URL
	if(*p == '\"'){
		p++;
	}
	for( ;*p != '\"' && *p != '\0';p++){
		if(IsDBCSLeadByte((BYTE)*p) == TRUE){
			URL += *(p++);
		}else if(*p == '\\'){
			p++;
		}
		URL += *p;
	}

	//Title
	if ( *p == 0 ) { return; }
	for(;(*p == '\"' || *p == ',') && *p != '\0';p++);
	if ( *p == 0 ) { return; }

	for( ;*p != '\"' && *p != '\0';p++){
		if(IsDBCSLeadByte((BYTE)*p) == TRUE){
			Title += *(p++);
		}else if(*p == '\\'){
			p++;
		}
		Title += *p;
	}
}


//============================================================================

bool	get_browser_info(string& URL, string& Title) {
	string Frame;
	char InfoStr[MAXSIZE];

	if(DDE_GetActiveBrowseInfo(InfoStr, MAXSIZE-2) == FALSE) {
		//MessageBox(NULL, "ブラウザが起動されていないか、ブラウザ情報の取得に失敗しました。",
		//	"ブラウザ情報の取得", MB_ICONWARNING | MB_OK);
		return	false;
	}
	//取得した文字列を展開する。
	GetDDEStr(InfoStr, URL, Title);

	return	true;
}


