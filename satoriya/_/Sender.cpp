#include	"Sender.h"
#ifdef POSIX
#  include      "Utilities.h"
#  include      <stdio.h>
#  include      <stdarg.h>
#else
#  include      "Utilities.h"
//#  include	<mbctype.h>		// for _ismbblead()
#endif

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


// グローバルオブジェクト
Sender::sender_stream	sender;

// staticメンバ
bool Sender::sm_sender_flag = true;
int Sender::nest_object::sm_nest = 0;
#ifndef POSIX
HWND Sender::sm_receiver_window = NULL;
#endif


bool	Sender::initialize()
{
#ifdef POSIX
	return true;
#else
	return (sm_receiver_window = ::FindWindow("れしば", "れしば")) != NULL;
#endif
}


// レシーバウィンドウにメッセージを送信
bool	Sender::send(const char* iFormat, ...)
{
	if ( !sm_sender_flag )
	{
		return	false;
	}

#ifdef POSIX
	// とりあえず標準エラーにでも。
	va_list theArgPtr;
	va_start(theArgPtr, iFormat);
	vfprintf(stderr, iFormat, theArgPtr);
	va_end(theArgPtr);
	fprintf(stderr, "\n");
	return false;
#else
	char	theBuf[sender_buf::MAX+1];
	va_list	theArgPtr;
	va_start(theArgPtr, iFormat);
	_vsnprintf(theBuf, sender_buf::MAX, iFormat, theArgPtr);
	va_end(theArgPtr);

	if ( sm_receiver_window==NULL )
	{
		// 初回のみ自動検索。毎回FindWindowは無駄すぎる
		static bool is_do_auto_initialize = false;
		if ( !is_do_auto_initialize )
		{
			bool b = initialize();
			is_do_auto_initialize = true;
			if ( !b )
			{
				return	false;
			}
		}
		else
		{
			return	false;
		}
	}

	const int nest = nest_object::count();
	if ( nest>0 )
	{
		char*	buf = new char[nest+1];
		int i;
		for ( i=0 ; i<nest ; ++i )
			buf[i]=' ';
		buf[i]='\0';
		COPYDATASTRUCT	cds = {1, nest+1, buf};
		::SendMessage(sm_receiver_window, WM_COPYDATA, NULL, (LPARAM)(&cds));
		delete [] buf;
	}

	// \\nを\r\nに置き換える
	/*char* p = theBuf;
	while ( (p=strstr(p, "\\n"))!=NULL )
	{
		*p++ = '\r';
		*p++ = '\n';
	}*/

	COPYDATASTRUCT	cds = {0,  strlen(theBuf)+1, &theBuf};
	::SendMessage(sm_receiver_window, WM_COPYDATA, NULL, (LPARAM)(&cds));

	//::OutputDebugString(theBuf);
	//::OutputDebugString("\n");

	return	false;
#endif
}


int Sender::sender_buf::overflow(int c)
{
	if ( c=='\n' || c=='\0' || c==EOF )
	{
		// 出力を行う
		send("%s", line);
		line[0]='\0';
		pos = 0;
	} 
	else
	{
		// バッファにためる
		line[pos++] = c;
		line[pos] = '\0';

		if ( pos+1>=MAX ) {
			if ( _ismbblead(c) ) {
				line[pos-1]='\0';
				send("%s", line);
				line[0]=c;
				line[1]='\0';
				pos = 1;
			} else {
				send("%s", line);
				line[0]='\0';
				pos = 0;
			}
		}
	}
	return	c;
}
