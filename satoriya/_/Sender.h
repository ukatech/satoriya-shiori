#ifndef	SENDER_H
#define	SENDER_H

/*ログ関連一式*/

#include	<iostream>
#include	<vector>
#include      <stdio.h>
#include      <wchar.h>
using namespace std;

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

class sender_buf : public streambuf
{
public:
	sender_buf() { line[0]='\0'; pos=0; }
	virtual int overflow(int c=EOF);

private:
	char	line[SenderConst::MAX+1];
	int		pos;
};

class error_buf : public streambuf
{
public:
	error_buf() { line[0]='\0'; pos=0; log_mode=false; }
	virtual int overflow(int c=EOF);

	void set_log_mode(bool is_log) { log_mode = is_log; }
	bool get_log_mode(void) { return log_mode; }

	const std::vector<string> & get_log(void) { return log_data; }
	void clear_log(void) { log_data.clear(); }

private:
	void send(const std::string &str);

	char	line[SenderConst::MAX+1];
	int		pos;

	bool    log_mode;
	std::vector<string> log_data;
};

class sender_stream : public ostream
{
	sender_buf buf;
public:
	sender_stream() : ostream( &buf ) {}
};

class error_stream : public ostream
{
	error_buf buf;
public:
	error_stream() : ostream( &buf ) {}

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

	char buffer_to_send[SenderConst::MAX+SenderConst::NEST_MAX+1];
	wchar_t buffer_to_sendw[SenderConst::MAX+SenderConst::NEST_MAX+3]; //\r\n分増やす

	bool sm_sender_flag;		// 動作有効可否
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
	bool send(int mode,const char* iFormat, ...);

	void validate(bool i_flag=true) { sm_sender_flag = i_flag; }
	bool is_validated() { return sm_sender_flag; }

	error_stream&  errsender()  { return err_stream; }
	sender_stream& sender()     { return send_stream; }

};


#endif	// SENDER_H
