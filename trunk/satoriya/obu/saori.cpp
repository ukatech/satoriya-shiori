// saori.cpp - obu.dll
#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」
#include	"Saori.h"

#include	<fstream>
#include	<strstream>
#include	<cassert>
#include    <algorithm>
#include	"../_/Sender.h"
#include	"../_/CriticalSection.h"
#include	"dsstp.h"
#include	"get_browser_info.h"

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
	static	string	lastURL="", lastTitle="";
	string	URL, Title;
	static	set<string>	visitedURL;
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
		
		deque<string>	refs;
		refs.push_back(URL);	// ref0
		refs.push_back(Title);	// ref1
		sendDirectSSTP_for_NOTIFY("obu", "OnWebsiteVisit", refs);
	}
	else {
		++theCoutner;
		if ( gFrequency>0 && (theCoutner % gFrequency)==0 ) {
			deque<string>	refs;
			refs.push_back(URL);	// ref0
			refs.push_back(Title);	// ref1
			refs.push_back(itos(theCoutner/FREQ_RATE));	// ref2
			sendDirectSSTP_for_NOTIFY("obu", "OnWebsiteStay", refs);
		}
	}

}

/*
list< list<string> >	gList;
bool	read_dictionary(strvec& vec) {
	gList.clear();

	strvec::iterator i=vec.begin();
	while ( i!=vec.end() ) {

		// 空行スキップ
		while ( i->empty() )
			if ( ++i == vec.end() )
				return	!gList.empty();

		gList.push_back( list<string>() );
		list<string>::iterator j=gList.back();
		while ( i!=vec.end() && !i->empty() ) {
		}
	} while ( i!=vec.end() );
	// 空行区切りのlist<multimap<string, string> >
}
*/

bool	Saori::load(const string& iBaseFolder) {

	/*strvec	vec;
	if ( !strvec_from_file(vec, iBaseFolder+"\\obu.txt") )
		return	false;*/

	// コメント削除
	//strvec::iterator i=vec.begin();

	//read_dictionary(vec);
	g_pBrowserInfo = new CBrowserInfo;

	gTimerEventID = ::SetTimer(NULL, NULL, 1000/FREQ_RATE, TimerProc);
	if ( gTimerEventID == 0 )
		return	false;
	return true;
}

bool	Saori::unload() {
	Locker	locker(gCS);
	if ( gTimerEventID != 0 )
		::KillTimer(NULL, gTimerEventID);

	delete g_pBrowserInfo;

	return true;
}


int	Saori::request(deque<string>& iArguments, string& oResult, deque<string>& oValues) {
	Locker	locker(gCS);

	if ( iArguments.empty() ) {
		return	400;
	}
	
	std::string argCmd = iArguments[0];
	std::transform(argCmd.begin(), argCmd.end(), argCmd.begin(), tolower);

	if ( argCmd=="setEvent" && iArguments.size()==2 ) {
		//gFrequency = stoi(iArguments[1]);
	}
	else if ( argCmd=="setfrequency" && iArguments.size()==2 ) {
		gFrequency = stoi(iArguments[1])*FREQ_RATE;
	}
	else if ( argCmd=="getfrequency" ) {
		oResult = itos(gFrequency/FREQ_RATE);
	}
	else if ( argCmd=="geturllist" ) {
		std::vector< str_pair >	URL;
		if ( !g_pBrowserInfo->GetMulti(URL) )
			return	204;
		oResult = URL[0].first;

		for ( UINT i = 0 ; i < URL.size() ; ++i ) {
			oValues.push_back(URL[i].first);
		}
	}
	else if ( argCmd=="gettitlelist" ) {
		std::vector< str_pair >	URL;
		if ( !g_pBrowserInfo->GetMulti(URL) )
			return	204;
		oResult = URL[0].second;

		for ( UINT i = 0 ; i < URL.size() ; ++i ) {
			oValues.push_back(URL[i].second);
		}
	}
	else if ( argCmd=="geturl" ) {
		string	URL, Title;
		if ( !g_pBrowserInfo->Get(URL, Title) )
			return	204;
		oResult = URL;
	}
	else if ( argCmd=="gettitle" ) {
		string	URL, Title;
		if ( !g_pBrowserInfo->Get(URL, Title) )
			return	204;
		oResult = Title;
	}
	else {
		return	400;
	}

	return	200;
}

