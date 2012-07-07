#include	"Sender.h"
#ifdef POSIX
#  include      "Utilities.h"
#else
#  include      "Utilities.h"
//#  include	<mbctype.h>		// for _ismbblead()
#endif
#include      <locale.h>
#include      <stdio.h>
#include      <stdarg.h>

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」

// グローバルオブジェクト
Sender::sender_stream	sender;
Sender::error_stream	errsender;

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
	DWORD ret_dword = 0;
	
	if ( nest>0 )
	{
		char*	buf = new char[nest+1];
		int i;
		for ( i=0 ; i<nest ; ++i )
			buf[i]=' ';
		buf[i]='\0';
		COPYDATASTRUCT	cds = {1, nest+1, buf};
		::SendMessageTimeout(sm_receiver_window, WM_COPYDATA, NULL, (LPARAM)(&cds),SMTO_BLOCK|SMTO_ABORTIFHUNG,5000,&ret_dword);
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
	::SendMessageTimeout(sm_receiver_window, WM_COPYDATA, NULL, (LPARAM)(&cds),SMTO_BLOCK|SMTO_ABORTIFHUNG,5000,&ret_dword);

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

void Sender::error_buf::send(const std::string &line)
{
	if ( ! line.length() ) { return; }
	if ( log_mode ) {
		log_data.push_back(line);
	}
	else {
#ifdef POSIX
        cerr << "error - SATORI : " << line << endl;
#else
        ::MessageBox(NULL, line.c_str(), "error - SATORI", MB_OK|MB_SYSTEMMODAL);
#endif
	}
}

int Sender::error_buf::overflow(int c)
{
	if ( c=='\n' || c=='\0' || c==EOF )
	{
		send(line);
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
				send(line);
				line[0]=c;
				line[1]='\0';
				pos = 1;
			} else {
				send(line);
				line[0]='\0';
				pos = 0;
			}
		}
	}
	return	c;
}
