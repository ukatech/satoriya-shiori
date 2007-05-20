#ifndef	_adf2e6ed_6521_4ee5_8122_c523e9ccd2a7_
#define	_adf2e6ed_6521_4ee5_8122_c523e9ccd2a7_


// 利用するライブラリ
#include <string>
#include <exception>
#include <stdexcept>
using std::string;


// TCPSocketのメソッドが使用する例外オブジェクト
struct socket_error : public std::runtime_error
{
	socket_error(string i_str) : runtime_error(i_str) {}
	virtual ~socket_error() throw() {}
};


// ソケットクライアント
class TCPSocket {
public:

	TCPSocket();
	virtual ~TCPSocket();

	// 接続
	void	connect(const char* iHostName, unsigned short iPortNumber);
	void	connect(string iHostName, unsigned short iPortNumber) { TCPSocket::connect(iHostName.c_str(), iPortNumber); }
	// 切断
	void	disconnect();
	// 送信　戻り値はsended octet[s]
	int		send(const char* iBuffer, int iLength);
	// 受信　戻り値はreceived octet[s]
	int		receive(char* oBuffer, int iLength);

	// 接続中か？
	bool	is_connect();

	// 行ベースのやりとり用。CRLFを自動で付与または削除する
	void	send_line(const string& i_str);
	string	receive_line();
	
	// ホスト名をIPアドレスへと変換（名前解決）する。できなかったら例外を投げる。
	static string convert_hostname_to_IPaddress(string i_hostname) throw (socket_error);

private:
	unsigned int m_id;

	#ifdef _MSC_VER
		static int sm_num_of_instance;
		static bool sm_is_can_use_winsock2;

		static void winsock2_startup();
		static void winsock2_cleanup();
	#endif // _MSC_VER
};


#endif	//	_adf2e6ed_6521_4ee5_8122_c523e9ccd2a7_
