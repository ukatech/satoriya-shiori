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

static const char TAMA_CLASSNAME[] = "TamaWndClass";
static const char RECV_CLASSNAME[] = "れしば";

int Sender::nest_object::sm_nest = 0;

Sender& GetSender()
{
	static Sender send;
	return send;
}

Sender::Sender()
{
	sm_sender_flag = true;
	is_do_auto_initialize = false;
	nest_object::sm_nest = 0;
#ifndef POSIX
	sm_receiver_window = NULL;
	sm_receiver_mode = SenderConst::MODE_RECEIVER;
#endif
}

Sender::~Sender()
{
	send_to_window(SenderConst::E_END,"");
}

bool Sender::initialize()
{
#ifdef POSIX
	return true;
#else
	sm_receiver_window = ::FindWindow(RECV_CLASSNAME, RECV_CLASSNAME);
	sm_receiver_mode = SenderConst::MODE_RECEIVER;

	if ( ! sm_receiver_window ) {
		sm_receiver_window = ::FindWindow(TAMA_CLASSNAME, NULL);
		sm_receiver_mode = SenderConst::MODE_TAMA;

		if ( sm_receiver_window ) {
			send_to_window(SenderConst::E_SJIS,"");
		}
	}

	return sm_receiver_window != NULL;
#endif
}

bool Sender::reinit(bool isEnable)
{
	if(isEnable)
	{
		//フラグを下ろして次に再スキャン
		is_do_auto_initialize = false;
	}
	else
	{
		//フラグを立ててNULLにすれば無効
		is_do_auto_initialize = true;
		sm_receiver_window = NULL;
	}
	return true;
}

// れしば自動探索
bool Sender::auto_init()
{
	if ( !sm_sender_flag ) { return false; }

	if ( sm_receiver_window==NULL )
	{
		// 初回のみ自動検索。毎回FindWindowは無駄すぎる
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

	return true;
}

// レシーバウィンドウにメッセージを送信
bool Sender::send(int mode,const char* iFormat, ...)
{
	if ( !auto_init() ) { return false; }

	const int nest = nest_object::count();
	char *theBuf = buffer_to_send;
	
	if ( nest>0 ) {
		int nest_limited = nest;
		if ( nest_limited > SenderConst::NEST_MAX ) {
			nest_limited = SenderConst::NEST_MAX;
		}
		for ( int i = 0 ; i < nest_limited ; ++i ) {
			buffer_to_send[i] = ' ';
		}
		theBuf += nest_limited;
	}
	
	va_list	theArgPtr;
	va_start(theArgPtr, iFormat);
	_vsnprintf(theBuf, SenderConst::MAX, iFormat, theArgPtr);
	va_end(theArgPtr);

	// \\nを\r\nに置き換える
	/*char* p = theBuf;
	while ( (p=strstr(p, "\\n"))!=NULL )
	{
		*p++ = '\r';
		*p++ = '\n';
	}*/

	send_to_window(mode,buffer_to_send);

	//::OutputDebugString(theBuf);
	//::OutputDebugString("\n");

	return	false;
}

bool Sender::send_to_window(const int mode,const char* theBuf)
{
#ifdef POSIX
	fprintf(stderr, theBuf);
	fprintf(stderr, "\n");
#else
	if ( !auto_init() ) { return false; }
	
	COPYDATASTRUCT cds;
	DWORD ret_dword = 0;

	size_t theBuf_len = strlen(theBuf);

	if ( sm_receiver_mode == SenderConst::MODE_RECEIVER ) {
		cds.dwData = 0;
		cds.cbData = theBuf_len+1;
		cds.lpData = const_cast<char*>(theBuf);
	}
	else /*MODE_TAMA*/ {
		cds.dwData = mode;

		if ( theBuf_len == 0 ) {
			if ( mode <= SenderConst::E_NO_EMPTY_LOG_ID_LIMIT ) {
				return false;
			}
		}

		int size = ::MultiByteToWideChar(932/*SJIS*/,0,theBuf,strlen(theBuf),buffer_to_sendw,sizeof(buffer_to_sendw) / sizeof(buffer_to_sendw[0]));
		buffer_to_sendw[size] = 0;

		if ( theBuf_len > 0 ) {
			wcscat(buffer_to_sendw,L"\r\n");
		}

		cds.cbData = (wcslen(buffer_to_sendw)+1) * sizeof(wchar_t);
		cds.lpData = buffer_to_sendw;
	}

	if ( ::SendMessageTimeout(sm_receiver_window, WM_COPYDATA, NULL, (LPARAM)(&cds),SMTO_BLOCK|SMTO_ABORTIFHUNG,5000,&ret_dword) == 0 ) {
		if ( ::GetLastError() == ERROR_INVALID_WINDOW_HANDLE ) {
			reinit(false);
			return false;
		}
	}
	return true;
#endif
}

int sender_buf::overflow(int c)
{
	if ( c=='\n' || c=='\0' || c==EOF )
	{
		// 出力を行う
		GetSender().send(SenderConst::E_I,line);
		line[0]='\0';
		pos = 0;
	} 
	else
	{
		// バッファにためる
		line[pos++] = c;
		line[pos] = '\0';

		if ( pos+1>=SenderConst::MAX ) {
			if ( _ismbblead(c) ) {
				line[pos-1]='\0';
				GetSender().send(SenderConst::E_I,line);
				line[0]=c;
				line[1]='\0';
				pos = 1;
			} else {
				GetSender().send(SenderConst::E_I,line);
				line[0]='\0';
				pos = 0;
			}
		}
	}
	return	c;
}

void error_buf::send(const std::string &line)
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

int error_buf::overflow(int c)
{
	if ( c=='\n' || c=='\0' || c==EOF )
	{
		GetSender().send(SenderConst::E_W,line);
		line[0]='\0';
		pos = 0;
	} 
	else
	{
		// バッファにためる
		line[pos++] = c;
		line[pos] = '\0';

		if ( pos+1>=SenderConst::MAX ) {
			if ( _ismbblead(c) ) {
				line[pos-1]='\0';
				GetSender().send(SenderConst::E_W,line);
				line[0]=c;
				line[1]='\0';
				pos = 1;
			} else {
				GetSender().send(SenderConst::E_W,line);
				line[0]='\0';
				pos = 0;
			}
		}
	}
	return	c;
}
