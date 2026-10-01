// saori.cpp - obu.dll
#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」
#include	"Saori.h"

#include	<cassert>
#include    <algorithm>
#include	"../_/Sender.h"
#include	"../_/CriticalSection.h"
#include	"dsstp.h"
#include	"get_browser_info.h"

SakuraDLLHost* SakuraDLLHost::m_dll = new obu;

static CriticalSection	gCS;
static CBrowserInfo *g_pBrowserInfo = NULL;

static int	gFrequency=0;
static int	gTimerEventID=0;

#define	FREQ_RATE	4

VOID CALLBACK TimerProc(
  HWND hwnd,     // handle of window for timer messages
  UINT uMsg,     // WM_TIMER message
  UINT idEvent,  // timer identifier
  DWORD dwTime   // current system time
) {
	Locker	locker(gCS);
	static	unsigned int	count = 0;
	static	int	theCoutner=0;
	static	wstring	lastURL=L"", lastTitle=L"";
	wstring	URL, Title;
	++count;

	if ( !g_pBrowserInfo->Get(URL, Title) )
		return;

	if ( count==1 ) {	// 初回でブラウザ情報が取得できた場合、１回目の移動までは検出しない
		lastURL=URL;
		lastTitle=Title;
	}

	if ( URL!=lastURL ) {
		theCoutner = 0;
		lastURL = URL;
		lastTitle = Title;
		
		std::deque<wstring>	refs;
		refs.push_back(URL);	// ref0
		refs.push_back(Title);	// ref1
		sendDirectSSTP_for_NOTIFY(L"obu", L"OnWebsiteVisit", refs);
	}
	else {
		++theCoutner;
		if ( gFrequency>0 && (theCoutner % gFrequency)==0 ) {
			std::deque<wstring>	refs;
			refs.push_back(URL);	// ref0
			refs.push_back(Title);	// ref1
			refs.push_back(itos(theCoutner/FREQ_RATE));	// ref2
			sendDirectSSTP_for_NOTIFY(L"obu", L"OnWebsiteStay", refs);
		}
	}

}

bool	obu::load(const wstring& iBaseFolder) {

	g_pBrowserInfo = new CBrowserInfo;

	gTimerEventID = ::SetTimer(NULL, NULL, 1000/FREQ_RATE, TimerProc);
	if ( gTimerEventID == 0 )
		return	false;
	return true;
}

bool	obu::unload() {
	Locker	locker(gCS);
	if ( gTimerEventID != 0 )
		::KillTimer(NULL, gTimerEventID);

	delete g_pBrowserInfo;
	g_pBrowserInfo = NULL;

	return true;
}


SRV	obu::request(std::deque<wstring>& iArguments, std::deque<wstring>& oValues) {
	Locker	locker(gCS);

	if ( iArguments.empty() ) {
		return	SRV(400);
	}
	
	wstring argCmd = iArguments[0];
	std::transform(argCmd.begin(), argCmd.end(), argCmd.begin(), towlower);

	wstring	result;
	if ( argCmd==L"setEvent" && iArguments.size()==2 ) {
		//gFrequency = stoi_internal(iArguments[1]);
	}
	else if ( argCmd==L"setfrequency" && iArguments.size()==2 ) {
		gFrequency = stoi_internal(iArguments[1])*FREQ_RATE;
	}
	else if ( argCmd==L"getfrequency" ) {
		result = itos(gFrequency/FREQ_RATE);
	}
	else if ( argCmd==L"geturllist" ) {
		std::vector< str_pair >	URL;
		if ( !g_pBrowserInfo->GetMulti(URL) )
			return	SRV(204);
		result = URL[0].first;

		for ( UINT i = 0 ; i < URL.size() ; ++i ) {
			oValues.push_back(URL[i].first);
		}
	}
	else if ( argCmd==L"gettitlelist" ) {
		std::vector< str_pair >	URL;
		if ( !g_pBrowserInfo->GetMulti(URL) )
			return	SRV(204);
		result = URL[0].second;

		for ( UINT i = 0 ; i < URL.size() ; ++i ) {
			oValues.push_back(URL[i].second);
		}
	}
	else if ( argCmd==L"geturl" ) {
		wstring	URL, Title;
		if ( !g_pBrowserInfo->Get(URL, Title) )
			return	SRV(204);
		result = URL;
	}
	else if ( argCmd==L"gettitle" ) {
		wstring	URL, Title;
		if ( !g_pBrowserInfo->Get(URL, Title) )
			return	SRV(204);
		result = Title;
	}
	else {
		return	SRV(400);
	}

	return	SRV(result);
}
