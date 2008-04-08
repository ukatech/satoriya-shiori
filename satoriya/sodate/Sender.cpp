#include	"Sender.h"
#include	<windows.h>
#include	<mbctype.h>		// for _ismbblead()
#include	<iostream>
#include	<fstream>
#include	<string>
using namespace std;

sender_stream	sender;
bool	sender_flag = true;

extern	string	base_folder;

// レシーバウィンドウにメッセージを送信
bool	Send(const char* iFormat, ...) {
	char	theBuf[sender_buf::MAX+1];
	va_list	theArgPtr;
	va_start(theArgPtr, iFormat);
	_vsnprintf(theBuf, sender_buf::MAX, iFormat, theArgPtr);
	va_end(theArgPtr);

	if ( !sender_flag )
		return	false;

	ios_base::openmode	mode = ios::out;
	static	bool	isinit = false;
	if ( isinit )
		mode |= ios::app;
	else
		isinit = true;

	ofstream	logfile( (base_folder+"\\sodate_log.txt").c_str(), mode);
	if ( logfile.is_open() ) {
		logfile << theBuf << endl;
		logfile.close();
	}

	return	false;
}


int sender_buf::overflow(int c) {

	if ( c>=0 && c<=255 ) {
		char	buf[2]={c,0};
		cout << buf;
	}

	if ( c=='\n' || c=='\0' || c==EOF ) {
		// 出力を行う
		Send("%s", line);
		line[0]='\0';
		pos = 0;
	} 
	else {
		// バッファにためる
		line[pos++] = c;
		line[pos] = '\0';

		if ( pos+1>=MAX ) {
			if ( _ismbblead(c) ) {
				line[pos-1]='\0';
				Send("%s", line);
				line[0]=c;
				line[1]='\0';
				pos = 1;
			} else {
				Send("%s", line);
				line[0]='\0';
				pos = 0;
			}
		}
	}
	return	c;
}
