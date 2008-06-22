
#pragma once

#include <windows.h>
#include <DDEML.H>

#include	<string>
#include	<vector>
#include	<utility>

typedef std::pair<std::string,std::string> str_pair;

class CBrowserInfo {
private:
	DWORD m_dwDDEID;
	HSZ m_hszTopic;
	HSZ m_hszListWindows;
	bool m_first;
	int m_search_num;
	int m_server_num;
	std::string m_infoStrBuf;

	bool GetMultiImpl(const char *iDDE_ServerName,
		std::vector< str_pair > & URLV,
		bool isActiveOnly);

public:
	CBrowserInfo();
	~CBrowserInfo();

	bool Get(std::string& URL, std::string& Title);
	bool GetMulti(std::vector< str_pair > & URLV);
};
