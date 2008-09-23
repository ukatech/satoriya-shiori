#include <algorithm>

#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」

/*--------------------------
	include
--------------------------*/
#include "get_browser_info.h"

/*--------------------------
	define
--------------------------*/
#define MAXSIZE				65535
#define	CODEPAGE			CP_WINANSI

/*--------------------------
	function
--------------------------*/

//DDE Callback
HDDEDATA CALLBACK DDECallback(UINT uType, UINT uFmt, HCONV hcconv, HSZ hszTpc1, HSZ hszTpc2,
        HDDEDATA hdata, DWORD dwData1, DWORD dwData2)
{
	return (HDDEDATA)0;
}

//============================================================================

CBrowserInfo::CBrowserInfo()
{
	m_first = true;
	m_search_num = 0;
	m_server_num = -1;

	m_dwDDEID = 0;
	::DdeInitialize(&m_dwDDEID, DDECallback, CBF_SKIP_ALLNOTIFICATIONS | APPCMD_CLIENTONLY, 0L);

	m_hszTopic = ::DdeCreateStringHandle(m_dwDDEID,"WWW_GetWindowInfo", CODEPAGE);
	m_hszListWindows = ::DdeCreateStringHandle(m_dwDDEID,"WWW_ListWindows", CODEPAGE);
}

CBrowserInfo::~CBrowserInfo()
{
	::DdeFreeStringHandle(m_dwDDEID, m_hszTopic);
	::DdeFreeStringHandle(m_dwDDEID, m_hszListWindows);

	::DdeUninitialize(m_dwDDEID);
	m_dwDDEID = 0;
}

//============================================================================

#include	<mbctype.h>
//DDEでブラウザから取得した情報を URL , Title , Frame に展開する
//	"http://hoge.com/","hoge","Frame"
static void __fastcall GetDDEStr(char *buf, std::string &URL, std::string &Title)
{
	char *p = (char *)buf;

	URL.erase();
	Title.erase();

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

static const char*	DDE_ServerNames[] = 
	{"IEXPLORE", "Opera", "Firefox", "NETSCAPE", "Mozilla", "Netscape6", "Netscape Browser", "chrome",
	"MBROWSER", "SLEIPNIR", "MDIBROWSER", "TaBrowser", "Donut", "fub", "fub.net", "Cuam" };
static const int max_svr = sizeof(DDE_ServerNames)/sizeof(DDE_ServerNames[0]);

bool CBrowserInfo::Get(std::string& URL, std::string& Title)
{
	if ( m_dwDDEID == 0 ) { return false; }

	std::vector< str_pair > URLV;

	if ( m_first ) {
		m_first = false;
		// 初回
		for (m_server_num=0 ; m_server_num<max_svr ; ++m_server_num) {
			if ( GetMultiImpl(DDE_ServerNames[m_server_num],URLV,true) ) {
				break;
			}
		}
	}
	else if ( m_server_num < 0 ) {
		// チェックするリストがでかくなってきたので3つずつぐらいチェック
		for ( int dummycount = 0 ; dummycount < 3 ; ++dummycount ) {
			if ( GetMultiImpl(DDE_ServerNames[m_search_num],URLV,true) ) {
				m_server_num = m_search_num;
				++m_search_num;
				break;
			}
			else {
				if ( ++m_search_num >= max_svr ) {
					m_search_num = 0;
				}
			}
		}
	}
	else {
		GetMultiImpl(DDE_ServerNames[m_server_num],URLV,true);
	}

	if ( URLV.size() ) {
		URL = URLV[0].first;
		Title = URLV[0].second;
		return true;
	}
	m_server_num = -1;
	return false;
}

//============================================================================

struct str_pair_less {
  bool operator()(const str_pair& x, const str_pair& y) const {
    return x.first < y.first;
  }
};

struct str_pair_equal {
  bool operator()(const str_pair& x, const str_pair& y) const {
    return x.first == y.first;
  }
};

bool CBrowserInfo::GetMulti(std::vector< str_pair > & URLV )
{
	if ( m_dwDDEID == 0 ) { return false; }

	URLV.clear();

	for ( int i = 0 ; i < max_svr ; ++i ) {
		GetMultiImpl(DDE_ServerNames[i],URLV,false);
	}

	std::sort(URLV.begin(), URLV.end(), str_pair_less());
	URLV.erase(std::unique(URLV.begin(), URLV.end(), str_pair_equal()), URLV.end());

	return URLV.size() != 0;
}

//============================================================================

bool CBrowserInfo::GetMultiImpl(const char *iDDE_ServerName,
	std::vector< str_pair > & URLV,
	bool isActiveOnly)
{
	if ( m_dwDDEID == 0 ) { return false; }

	HSZ hszService = ::DdeCreateStringHandle(m_dwDDEID, iDDE_ServerName, CODEPAGE);

	DWORD listWindows[100] = {0};
	if ( ! isActiveOnly ) {
		HCONV hConv = ::DdeConnect(m_dwDDEID, hszService, m_hszListWindows, NULL);
		if ( hConv ) {
			HDDEDATA hDDEData = ::DdeClientTransaction(NULL, 0, hConv, m_hszListWindows, CF_TEXT, XTYP_REQUEST, 10000L, NULL);
			if ( hDDEData ) {
				::DdeGetData(hDDEData, (LPBYTE)listWindows, sizeof(listWindows)-1, 0);
				listWindows[99] = 0; //最悪の場合の終端設定
				::DdeFreeDataHandle(hDDEData);
			}
			::DdeDisconnect(hConv);
		}
	}
	if ( ! listWindows[0] ) {
		listWindows[0] = 0xffffffffU;
		listWindows[1] = 0;
	}

	//接続
	HCONV hConv = ::DdeConnect(m_dwDDEID, hszService, m_hszTopic, NULL);
	::DdeFreeStringHandle(m_dwDDEID, hszService);

	if ( ! hConv ) {
		return false;
	}

	std::string URL;
	std::string Title;

	HSZ hszParam;
	UINT count = 0;
	char num[32];

	while ( listWindows[count] ) {
		sprintf(num,"%#x",listWindows[count]);
		hszParam = ::DdeCreateStringHandle(m_dwDDEID, num, CODEPAGE);

		//Client Transaction
		HDDEDATA hDDEData = ::DdeClientTransaction(NULL, 0, hConv, hszParam, CF_TEXT, XTYP_REQUEST, 10000L, NULL);

		if ( hDDEData ) {
			//Get Data
			DWORD len = ::DdeGetData(hDDEData, NULL, 0, 0) + 1;
			if ( len >= m_infoStrBuf.size() ) {
				m_infoStrBuf.resize(len);
			}
			
			len = ::DdeGetData(hDDEData, (LPBYTE)&m_infoStrBuf[0], len-1, 0);
			m_infoStrBuf[len] = 0;

			::DdeFreeDataHandle(hDDEData);

			GetDDEStr(&m_infoStrBuf[0], URL, Title);
			URLV.push_back(str_pair(URL,Title));
		}

		::DdeFreeStringHandle(m_dwDDEID, hszParam);
		++count;
	}

	::DdeDisconnect(hConv);

	return true;
}

