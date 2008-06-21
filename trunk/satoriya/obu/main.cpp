#include <iostream>
#include <string>
#include <ctime>
#include <windows.h>
using namespace std;

bool	get_browser_info(string&,string&);

int main()
{
	while (true) {
		string url, title;
		if ( !get_browser_info(url, title) )
		{
			cout << "(Error)" << endl;
		}
		else
		{
			//cout << url << "," << title << endl;
			cout << url << endl;
		}
		Sleep(1000);
	}

	return 0;
}