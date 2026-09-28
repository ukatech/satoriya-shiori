#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」

#include	"Sender.h"
#include	"charset.h"
#include      <locale.h>
#include      <stdio.h>
#include      <stdarg.h>

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


static const wchar_t TAMA_CLASSNAME[] = L"TamaWndClass";
static const wchar_t RECV_CLASSNAME[] = L"れしば";

int Sender::nest_object::sm_nest = 0;

Sender& GetSender()
{
	static Sender send;
	return send;
}

Sender::Sender()
{
	delay_send_event_max = 0;
	delay_send_string_max = 200;

	sm_sender_flag = true;
	sm_buffering_flag = false;

	is_do_auto_initialize = false;
	nest_object::sm_nest = 0;
#ifndef POSIX
	sm_receiver_window = NULL;
	sm_receiver_mode = SenderConst::MODE_RECEIVER;
#endif
}

Sender::~Sender()
{
	flush();

	send_to_window(SenderConst::E_END,L"");
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
			send_to_window(SenderConst::E_UTF8,L"");
		}
	}

	return sm_receiver_window != NULL;
#endif
}

#ifndef POSIX
void Sender::set_receiver_window(HWND hwnd)
{
	sm_receiver_window = hwnd;
	sm_receiver_mode = SenderConst::MODE_TAMA;

	//指定されたウィンドウを使い、FindWindowでの自動探索はしない（NULLなら無効）
	is_do_auto_initialize = true;

	if ( sm_receiver_window ) {
		send_to_window(SenderConst::E_UTF8,L"");
	}
}
#endif

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
bool Sender::send(int mode,const wchar_t* iString)
{
	const int nest = nest_object::count();
	wchar_t *theBuf = buffer_to_send;
	
	if ( nest>0 ) {
		int nest_limited = nest;
		if ( nest_limited > SenderConst::NEST_MAX ) {
			nest_limited = SenderConst::NEST_MAX;
		}
		for ( int i = 0 ; i < nest_limited ; ++i ) {
			buffer_to_send[i] = L' ';
		}
		theBuf += nest_limited;
	}
	
	wcsncpy(theBuf, iString, SenderConst::MAX);
	theBuf[SenderConst::MAX] = L'\0';

	// \\nを\r\nに置き換える
	/*char* p = theBuf;
	while ( (p=strstr(p, "\\n"))!=NULL )
	{
		*p++ = '\r';
		*p++ = '\n';
	}*/

	//::OutputDebugString(theBuf);
	//::OutputDebugString("\n");

	add_delay_text(mode, buffer_to_send);
	
	if ( ! sm_buffering_flag ) {
		flush();
	}

	return false;
}

bool Sender::send_to_window(const int mode,const wchar_t* theBuf)
{
#ifdef POSIX
	fputs(WtoUTF8(theBuf).c_str(), stderr);
	fputs("\n", stderr);
	return true;
#else
	if ( !auto_init() ) { return false; }
	
	COPYDATASTRUCT cds;
	DWORD ret_dword = 0;

	std::string mbstr;
	std::wstring wstr;

	if ( sm_receiver_mode == SenderConst::MODE_RECEIVER ) {
		// れしばはSJISのみ対応
		mbstr = WtoSJIS(theBuf);
		cds.dwData = 0;
		cds.cbData = mbstr.size()+1;
		cds.lpData = const_cast<char*>(mbstr.c_str());
	}
	else /*MODE_TAMA*/ {
		cds.dwData = mode;

		// ログ行には改行をつける（空行も送る）。E_END以降は制御用なのでそのまま
		// 改行はYAYAと同じくLFのみ（tamacはテキストモードで出力するため）
		wstr = theBuf;
		if ( mode < SenderConst::E_END ) {
			wstr += L"\n";
		}

		cds.cbData = (wstr.size()+1) * sizeof(wchar_t);
		cds.lpData = const_cast<wchar_t*>(wstr.c_str());
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

sender_buf::int_type sender_buf::overflow(int_type c)
{
	if ( traits_type::eq_int_type(c, traits_type::eof()) || c==L'\n' || c==L'\0' )
	{
		// 出力を行う
		GetSender().send(SenderConst::E_I,line);
		line[0]=L'\0';
		pos = 0;
	}
	else if ( c == SenderConst::FLUSH_MARK )
	{
		//skip
	}
	else
	{
		// バッファにためる
		line[pos++] = (wchar_t)c;
		line[pos] = L'\0';

		if ( pos+1>=SenderConst::MAX ) {
			if ( IsHighSurrogate((wchar_t)c) ) {
				line[pos-1]=L'\0';
				GetSender().send(SenderConst::E_I,line);
				line[0]=(wchar_t)c;
				line[1]=L'\0';
				pos = 1;
			} else {
				GetSender().send(SenderConst::E_I,line);
				line[0]=L'\0';
				pos = 0;
			}
		}
	}
	return	c;
}

void error_buf::set_log_mode(bool is_log)
{
	if ( is_log == false ) {
		for (std::vector<wstring>::iterator i=log_data.begin() ; i!=log_data.end() ; ++i) {
#ifdef POSIX
			std::wcerr << L"error - SATORI : " << *i << std::endl;
#else
			::MessageBox(NULL, i->c_str(), L"error - SATORI", MB_OK|MB_SYSTEMMODAL);
#endif
		}
		log_data.clear();
	}
	log_mode = is_log;
}

void error_buf::send(const wchar_t *str)
{
	if ( ! str || ! *str ) { return; }

	GetSender().send(SenderConst::E_E,str);

	log_tmp_buffer.push_back(wstring(str));
}

void error_buf::flush(void)
{
	if ( log_mode ) {
		wstring out;
		for (std::vector<wstring>::iterator i=log_tmp_buffer.begin() ; i!=log_tmp_buffer.end() ; ++i) {
			out += *i;
			out += L" ";
		}
		log_tmp_buffer.clear();

		log_data.push_back(out);
	}
	else {
		wstring out;
		for (std::vector<wstring>::iterator i=log_tmp_buffer.begin() ; i!=log_tmp_buffer.end() ; ++i) {
			out += *i;
			out += L"\r\n";
		}
		log_tmp_buffer.clear();

#ifdef POSIX
        std::wcerr << L"error - SATORI : " << out << std::endl;
#else
        ::MessageBox(NULL, out.c_str(), L"error - SATORI", MB_OK|MB_SYSTEMMODAL);
#endif
	}
}

error_buf::int_type error_buf::overflow(int_type c)
{
	if ( traits_type::eq_int_type(c, traits_type::eof()) || c==L'\n' || c==L'\0' )
	{
		send(line);
		line[0]=L'\0';
		pos = 0;
	}
	else if ( c == SenderConst::FLUSH_MARK )
	{
		flush();
	}
	else
	{
		// バッファにためる
		line[pos++] = (wchar_t)c;
		line[pos] = L'\0';

		if ( pos+1>=SenderConst::MAX ) {
			if ( IsHighSurrogate((wchar_t)c) ) {
				line[pos-1]=L'\0';
				send(line);
				line[0]=(wchar_t)c;
				line[1]=L'\0';
				pos = 1;
			} else {
				send(line);
				line[0]=L'\0';
				pos = 0;
			}
		}
	}
	return	c;
}

void Sender::add_delay_text(int mode, const wchar_t* text)
{
	if (delay_send_list.empty()) {
		next_event();
	}
	if (sm_sender_flag)
	{
		if ( delay_send_list.rbegin()->size() > delay_send_string_max ) {
			flush_latest_event();
		}
		delay_send_list.rbegin()->push_back(delay_text(mode, text));
	}
}

void Sender::next_event()
{
	std::list<delay_text_list>::reverse_iterator it = delay_send_list.rbegin();

	if (it == delay_send_list.rend())
	{
		//何も入ってない
		delay_send_list.push_back(delay_text_list());
	}
	else
	{
		if (it->empty())
		{
			return;	//空ならいいや
		}
		else
		{
			//満タンなら消す。
			while (delay_send_list.size() > delay_send_event_max)
			{
				delay_send_list.pop_front();
			}

			//別のイベントとして用意
			delay_send_list.push_back(delay_text_list());
		}
	}
}

void Sender::flush_latest_event()
{
	std::list<delay_text_list>::reverse_iterator it = delay_send_list.rbegin();
	if (auto_init())
	{
		for (delay_text_list::iterator st = it->begin(); st != it->end(); st++)
		{
			send_to_window(st->first, st->second.c_str());
		}
	}
	it->clear();
}

void Sender::flush()
{
#ifndef POSIX
	if (auto_init())
	{
		for (std::list<delay_text_list>::iterator it = delay_send_list.begin(); it != delay_send_list.end(); it++)
		{
			for (delay_text_list::iterator st = it->begin(); st != it->end(); st++)
			{
				send_to_window(st->first, st->second.c_str());
			}
		}
	}
#endif // not(POSIX)

	//1つだけ残してパージ
	if ( ! delay_send_list.empty() ) {
		while ( delay_send_list.size() > 1 ) {
			delay_send_list.pop_back();
		}
		delay_send_list.rbegin()->clear();
	}
}

void Sender::delete_last_request()
{
	if ( ! delay_send_list.empty() ) {
		delay_send_list.rbegin()->clear();
	}
}

