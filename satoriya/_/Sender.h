#ifndef	SENDER_H
#define	SENDER_H

/*ログ関連一式*/

#include	<iostream>
#include	<vector>
#include	<list>
#include      <stdio.h>
#include      <wchar.h>
using std::string;

#ifndef POSIX
#include <windows.h> // for HWND
#endif

namespace SenderConst {
	enum const_value {
		//バッファサイズ
		MAX=8191,
		NEST_MAX=20,

		//ログモード
		MODE_RECEIVER = 100,
		MODE_TAMA = 101,

		//AYA由来ログコード
		E_I = 0,	/* 標準のメッセージ */
		E_F = 1,	/* フェータルエラー */
		E_E = 2,	/* エラー */
		E_W = 3,	/* ワーニング */
		E_N = 4,	/* 注記 */
		E_END = 5,	/* ログの終了 */
		E_NO_EMPTY_LOG_ID_LIMIT = 5,
		E_SJIS = 16,	/* マルチバイト文字コード＝SJIS */
		E_UTF8 = 17,	/* マルチバイト文字コード＝UTF-8 */
		E_DEFAULT = 32,	/* マルチバイト文字コード＝OSデフォルトのコード */
	};
};

class sender_buf : public std::basic_streambuf<char>
{
public:
	sender_buf() { line[0]='\0'; pos=0; }
	virtual int overflow(int c=EOF);

private:
	char	line[SenderConst::MAX+1];
	int		pos;
};

class error_buf : public std::basic_streambuf<char>
{
public:
	error_buf() { line[0]='\0'; pos=0; log_mode=true; } //初期logmodeはtrueでないとload時をカバーできない
	virtual int overflow(int c=EOF);

	void set_log_mode(bool is_log);
	bool get_log_mode(void) { return log_mode; }

	const std::vector<string> & get_log(void) { return log_data; }
	void clear_log(void) { log_data.clear(); }

private:
	void send(const char* str);
	void flush(void);

	char	line[SenderConst::MAX+1];
	int		pos;

	bool    log_mode;

	std::vector<string> log_data;
	std::vector<string> log_tmp_buffer;
};

class sender_stream : public std::basic_ostream<char>
{
	sender_buf buf;
public:
	sender_stream() : std::basic_ostream<char>( &buf ) {}
};

class error_stream : public std::basic_ostream<char>
{
	error_buf buf;
public:
	error_stream() : std::basic_ostream<char>( &buf ) {}

	void set_log_mode(bool is_log) { buf.set_log_mode(is_log); }
	bool get_log_mode(void) { return buf.get_log_mode(); }

	const std::vector<string> & get_log(void) { return buf.get_log(); }
	void clear_log(void) { buf.clear_log(); }
};

class Sender;

extern Sender& GetSender();

class Sender
{
#ifdef POSIX
#	define HWND int
#endif
	// 出力先ウィンドウ
	HWND sm_receiver_window;
	int  sm_receiver_mode;

	sender_stream	send_stream;
	error_stream	err_stream;

	std::list< std::list<std::string> > delay_send_list;
	int delay_send_event_max;	//遅延送信最大数。
	int delay_send_string_max;  //イベント内にためこむ文字列リスト最大数

	void add_delay_text(const char* text);

	char buffer_to_send[SenderConst::MAX+SenderConst::NEST_MAX+1];
	wchar_t buffer_to_sendw[SenderConst::MAX+SenderConst::NEST_MAX+3]; //\r\n分増やす

	bool sm_sender_flag;		// 動作有効可否
	bool sm_buffering_flag;		// バッファリングモード
	bool is_do_auto_initialize;

	Sender();

	bool initialize();
	bool send_to_window(const int mode,const char* text);

	bool auto_init();

	friend Sender& GetSender();
public:
	~Sender();

	// このクラスのインスタンスのライフタイムに合わせ、ログ表示の際にネストを行う
	class nest_object
	{
		friend Sender;

		static int sm_nest;
		int m_nest;
	public:
		nest_object(int i_nest) : m_nest(i_nest) { sm_nest += m_nest; }
		~nest_object() { sm_nest -= m_nest; }
		static int count() { return sm_nest; }
	};

	bool reinit(bool isEnable);
	bool send(int mode,const char* iString);

	void validate(bool i_flag=true) { sm_sender_flag = i_flag; }
	bool is_validated() { return sm_sender_flag; }

	void next_event();	//遅延送信イベントを更新する
	void set_delay_save_count(int count)
	{ 
		if (count < 0)
		{
			count = 0;
		}
		delay_send_event_max = count; 
	}
	int get_delay_save_count(){ return delay_send_event_max; }

	error_stream&  errsender()  { return err_stream; }
	sender_stream& sender()     { return send_stream; }

	void buffering(bool i_flag = true) { sm_buffering_flag = i_flag; }

	void flush();
	void flush_latest_event();
	void delete_last_request();
};

// buffering ON
class SenderEnableBuffering
{
private:
	Sender& s;
public:
	SenderEnableBuffering(Sender& sp) : s(sp) { s.buffering(true); }
	~SenderEnableBuffering() { s.flush(); s.buffering(false); }
};

namespace satori {
	template<class CharT, class Traits>
		std::basic_ostream<CharT, Traits>& endl(std::basic_ostream<CharT, Traits>& os) {
		return os << '\n' << '\xfe' << std::flush;
	}
}

#endif	// SENDER_H
