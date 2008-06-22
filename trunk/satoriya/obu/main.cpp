#include <iostream>
#include <string>
#include <ctime>
#include <windows.h>
using namespace std;

#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」

#include "get_browser_info.h"

int main()
{
	CBrowserInfo *pB = new CBrowserInfo;

	while (true) {
		vector< str_pair >  url;
		if ( !pB->GetMulti(url) ) {
			cout << "(Error)" << endl;
		}
		else {
			for ( int i = 0 ; i < url.size() ; ++i ) {
				cout << url[i].first << " | " << url[i].second << endl;
			}
		}
		cout << "---------------" << endl;
		Sleep(2000);
	}

	delete pB;

	return 0;
}