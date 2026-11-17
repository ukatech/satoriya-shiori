#ifndef	SENDER_H
#define	SENDER_H

/*ログ関連一式*/

#include	<iostream>
#include	<vector>
#include	<list>
#include	<utility>
#include      <stdio.h>
#include      <wchar.h>
using std::wstring;

#ifdef POSIX
// ウィンドウハンドルと整数の変換用。POSIX では long がポインタと同じ大きさ
typedef long INT_PTR;
#else
#include <windows.h> // for HWND

// SendMessageTimeout の結果の型。64bit では DWORD_PTR（古い SDK には DWORD_PTR が無い）
#ifdef GWLP_USERDATA
typedef DWORD_PTR SMTO_RESULT;
#else
typedef DWORD SMTO_RESULT;
#endif
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
		E_J = 5,	/* その他 */
		E_END = 6,	/* ログの終了 */
		E_SJIS = 16,	/* マルチバイト文字コード＝SJIS */
		E_UTF8 = 17,	/* マルチバイト文字コード＝UTF-8 */
		E_DEFAULT = 32,	/* マルチバイト文字コード＝OSデフォルトのコード */
	};

	// satori::endl がストリームに流す区切り（私用領域の文字）
	static const wchar_t FLUSH_MARK = 0xE0FE;
};

class sender_buf : public std::basic_streambuf<wchar_t>
{
public:
	sender_buf() { line[0]=L'\0'; pos=0; }
	virtual int_type overflow(int_type c=traits_type::eof());

private:
	wchar_t	line[SenderConst::MAX+1];
	int		pos;
};

class error_buf : public std::basic_streambuf<wchar_t>
{
public:
	error_buf() { line[0]=L'\0'; pos=0; log_mode=true; } //初期logmodeはtrueでないとload時をカバーできない
	virtual int_type overflow(int_type c=traits_type::eof());

	void set_log_mode(bool is_log);
	bool get_log_mode(void) { return log_mode; }

	const std::vector<wstring> & get_log(void) { return log_data; }
	void clear_log(void) { log_data.clear(); }

private:
	void send(const wchar_t* str);
	void flush(void);

	wchar_t	line[SenderConst::MAX+1];
	int		pos;

	bool    log_mode;

	std::vector<wstring> log_data;
	std::vector<wstring> log_tmp_buffer;
};

class sender_stream : public std::basic_ostream<wchar_t>
{
	sender_buf buf;
public:
	sender_stream() : std::basic_ostream<wchar_t>( &buf ) {}
};

class error_stream : public std::basic_ostream<wchar_t>
{
	error_buf buf;
public:
	error_stream() : std::basic_ostream<wchar_t>( &buf ) {}

	void set_log_mode(bool is_log) { buf.set_log_mode(is_log); }
	bool get_log_mode(void) { return buf.get_log_mode(); }

	const std::vector<wstring> & get_log(void) { return buf.get_log(); }
	void clear_log(void) { buf.clear_log(); }
};

class Sender;

// 現在のインスタンスの Sender を返す。何も選ばれていなければ、DLL に1つだけの既定の Sender。
extern Sender& GetSender();

// GetSender() が返す Sender を差し替える（NULL で既定の Sender に戻す）。
// マルチインスタンスのホストが、呼び出しの間だけインスタンス専用の Sender を選ぶために使う。
// 戻り値は直前に選ばれていたもの（既定なら NULL）。
extern Sender* SetCurrentSender(Sender* i_sender);

// Set_loghandler で渡されるログのコールバック（YAYA互換）
typedef void (*sender_loghandler)(const wchar_t *str, int mode, int id);

class Sender
{
#ifdef POSIX
#	define HWND int
#endif
	// 出力先ウィンドウ
	HWND sm_receiver_window;
	int  sm_receiver_mode;

	// 設定されていればウィンドウの代わりにこれを呼ぶ
	sender_loghandler sm_loghandler;

	sender_stream	send_stream;
	error_stream	err_stream;

	// 1行分のログ（first=ログコード、second=文字列）
	typedef std::pair<int, std::wstring> delay_text;
	typedef std::list<delay_text> delay_text_list;

	std::list<delay_text_list> delay_send_list;
	int delay_send_event_max;	//遅延送信最大数。
	int delay_send_string_max;  //イベント内にためこむ文字列リスト最大数

	void add_delay_text(int mode, const wchar_t* text);

	wchar_t buffer_to_send[SenderConst::MAX+SenderConst::NEST_MAX+1];

	bool sm_sender_flag;		// 動作有効可否
	bool sm_buffering_flag;		// バッファリングモード
	bool is_do_auto_initialize;

	bool initialize();
	bool send_to_window(const int mode,const wchar_t* text);

	bool auto_init();

	// ログ表示のネストの深さ（nest_object が増減する）。実体ごとの Sender が持つので、実体をまたいだ入れ子でも混ざらない
	int m_nest_count;

	friend Sender& GetSender();
public:
	// 通常は GetSender() の既定の1つを使う。マルチインスタンスのホストだけが、インスタンスごとに作る。
	Sender();
	~Sender();

	// このクラスのインスタンスのライフタイムに合わせ、ログ表示の際にネストを行う。
	// 作った時点の GetSender()（現在の実体の Sender）の深さを増減する。
	class nest_object
	{
		Sender* m_sender;
		int m_nest;

		nest_object(const nest_object&);
		nest_object& operator=(const nest_object&);
	public:
		nest_object(int i_nest);
		~nest_object();
	};

	// ログ表示のネストの深さ。nest_object が増減に使う
	void add_nest(int i_delta) { m_nest_count += i_delta; }
	int get_nest() const { return m_nest_count; }

	bool reinit(bool isEnable);
#ifndef POSIX
	// チェックツール（tama/tamac）が logsend で渡したウィンドウを送信先にする
	void set_receiver_window(HWND hwnd);
#endif
	// ログをコールバックで受け取る（YAYAのSet_loghandler互換、NULLで解除）
	void set_loghandler(sender_loghandler handler);
	bool send(int mode,const wchar_t* iString);

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
		os.put(L'\n');
		os.put(SenderConst::FLUSH_MARK);
		return os << std::flush;
	}
}

#endif	// SENDER_H
