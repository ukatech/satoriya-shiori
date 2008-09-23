#include	"satori.h"
#include	<iostream>
using namespace std;

#include <windows.h>

class Ghost {
	Satori	mSatori;
public:
	Ghost() {
		char buf[MAX_PATH+1];
		const int len = ::GetCurrentDirectory(MAX_PATH, buf);
		strcpy(buf+len, "\\");
		if ( !mSatori.load(buf) )
			throw ;
	}
	~Ghost() {
		mSatori.unload();
	}
	string	request(string iCommandLine)
	{
		return ((SakuraDLLHost*)(&mSatori))->request( string() +
			"GET SHIORI/3.0\r\n"
			"ID: " + iEvent + "\r\n"
			"Sender: embryo\r\n"
			"SecurityLevel: local\r\n"
			"\r\n"
		);
	}

};


#include	<crtdbg.h>
int main() {
	
#ifdef	_DEBUG
	//メモリリークの検出(_DEBUGが定義されていた場合のみ有効です。)
	int tmpDbgFlag;
	tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	//tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpDbgFlag);

	//_CrtSetBreakAlloc(45109);
#endif



	Ghost* g = new Ghost;

	cout << g->request("OnBoot") << endl;
	cout << g->request("OnDoubleClick") << endl;
	//cout << ghost.request("OnNadeCopy") << endl;

	delete g;

	return	0;
}


/*
class A
{
	string m;
public:
	A(string i) : m(i) { cout << "A(" << m << ")" << endl; }
	~A() { cout << "~A(" << m << ")" << endl; }
};

A foo(A i_a)
{
	A the_a("ローカル変数");
	return A("戻り値");
}


	/*{
		foo(A("引数"));
		return 0;
	}*/

	/*
	{
		Families<string> fs;
		fs.add_element("名前", "中身", "条件式");
	}*/

	//for ( int i=0 ; i<10 ; ++i )


	//cout << ghost.request("get") << endl;
	//cout << ghost.request("うんこ") << endl;
/*	
	cout << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << endl;
	cout << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << endl;
	cout << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << endl;
	cout << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << endl;
	cout << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << ghost.request("たんご") << endl;
*/
 

