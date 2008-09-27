#ifndef	SENDER_H
#define	SENDER_H

/*ログ関連一式*/

#include	<iostream>
#include	<vector>
#include      <stdio.h>
using namespace std;

#ifndef POSIX
#include <windows.h> // for HWND
#endif


class Sender
{
#ifndef POSIX
	// 出力先ウィンドウ
	static HWND sm_receiver_window;
#endif

	class sender_buf : public streambuf
	{
	public:
		sender_buf() { line[0]='\0'; pos=0; }
		virtual int overflow(int c=EOF);

		enum const_value {
			MAX=8191
		};
	private:
		char	line[MAX+1];
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

		enum const_value {
			MAX=8191
		};
	private:
		void send(const std::string &str);

		char	line[MAX+1];
		int		pos;

		bool    log_mode;
		std::vector<string> log_data;
	};

	static bool	sm_sender_flag;		// 動作有効可否

	Sender();
public:

	class sender_stream : public ostream
	{
		sender_buf	buf;
	public:
		sender_stream() : ostream(&buf) {}
	};

	class error_stream : public ostream
	{
		error_buf	buf;
	public:
		error_stream() : ostream(&buf) {}

		void set_log_mode(bool is_log) { buf.set_log_mode(is_log); }
		bool get_log_mode(void) { return buf.get_log_mode(); }

		const std::vector<string> & get_log(void) { return buf.get_log(); }
		void clear_log(void) { buf.clear_log(); }
	};

	// このクラスのインスタンスのライフタイムに合わせ、ログ表示の際にネストを行う
	class nest_object
	{
		static int sm_nest;
		int m_nest;
	public:
		nest_object(int i_nest) : m_nest(i_nest) { sm_nest += m_nest; }
		~nest_object() { sm_nest -= m_nest; }
		static int count() { return sm_nest; }
	};


	static bool initialize();
	static bool send(const char* iFormat, ...);

	static void validate(bool i_flag=true) { sm_sender_flag = i_flag; }
	static bool is_validated() { return sm_sender_flag; }
};

// ログ出力ストリーム
extern	Sender::sender_stream	sender;
extern  Sender::error_stream    errsender;

#endif	// SENDER_H
