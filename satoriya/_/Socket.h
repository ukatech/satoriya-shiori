#ifndef	_adf2e6ed_6521_4ee5_8122_c523e9ccd2a7_
#define	_adf2e6ed_6521_4ee5_8122_c523e9ccd2a7_

#include	<vector>
using std::vector;

// ソケットクライアント。
// 返値false時はgetLastErrorで詳細を取得できる。
// 全インスタンスの使用前にmakeup, 全インスタンスの使用後にcleanupすること。
// 要ws2_32.lib。
class Socket {
public:
	static	bool makeup();
	static	void cleanup();
	static	int	getLastError();

	Socket();
	virtual ~Socket();

	// 接続
	bool	connect(const char* iHostName, unsigned short iPortNumber);
	// 切断
	void	disconnect();
	// 送信
	bool	send(const char* iBuffer, int iLength, int* oSendedBytes=NULL);
	// 受信
	bool	receive(char* oBuffer, int iLength, int* oReceivedBytes);

	// 接続中か？
	bool	isConnected();

private:
	unsigned int mID;
};


#endif	//	_adf2e6ed_6521_4ee5_8122_c523e9ccd2a7_
