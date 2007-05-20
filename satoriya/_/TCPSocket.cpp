#include	"TCPSocket.h"
#include	<stdio.h> // for sprintf

inline string itos(int i)
{
	char buf[32];
	sprintf(buf, "%d", i);
	return buf;
}

#ifdef	_MSC_VER

	// WinSockを利用

	#include	<winsock2.h>

	int TCPSocket::sm_num_of_instance = 0;
	bool TCPSocket::sm_is_can_use_winsock2 = false;

	// 初期化
	void	TCPSocket::winsock2_startup() 
	{
		if ( sm_is_can_use_winsock2 )
		{
			return;
		}
		const string base = "TCPSocket::winsock2_makeup(): ";


		WSADATA winsock_data;
		WORD	request_winsock_version = MAKEWORD(1,1);
		if( WSAStartup(request_winsock_version, &winsock_data) != 0 ) 
		{
			throw base + "WSAStartup(): " + itos(::WSAGetLastError());
		}

		if ( winsock_data.wVersion != request_winsock_version ) 
		{
			throw base + "version_check: " + itos(::WSAGetLastError());
		}

		sm_is_can_use_winsock2 = true;
	}

	// お片づけ
	void	TCPSocket::winsock2_cleanup() 
	{
		if ( sm_is_can_use_winsock2 )
		{
			::WSACleanup();
			sm_is_can_use_winsock2 = false;
		}
	}

#else

	// UNIX Socketを利用

	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <unistd.h> // for close()

	struct hostent *gethostbyaddr();

	#define	INVALID_SOCKET	0xffffffff
	#define INADDR_NONE     0xffffffff
	#define	SOCKET_ERROR	-1 // connect失敗時の戻り値。windowsだと0なのだが……

#endif // _MSC_VER



// クラス初期化
TCPSocket::TCPSocket() 
{
	#ifdef _MSC_VER
		++sm_num_of_instance;
	#endif // _MSC_VER

	m_id = INVALID_SOCKET;
}

// クラス解放
TCPSocket::~TCPSocket() 
{
	this->disconnect();

	#ifdef _MSC_VER
		if ( --sm_num_of_instance == 0 )
		{
			TCPSocket::winsock2_cleanup();
		}
	#endif // _MSC_VER
}



// 接続
void	TCPSocket::connect(const char* iHostName, unsigned short iPortNumber)
{
	#ifdef _MSC_VER
		TCPSocket::winsock2_startup();
	#endif // _MSC_VER

	const string base = string() + "TCPSocket::connect(" + iHostName + ", " + itos(iPortNumber) + "): ";
	this->disconnect();

	// ソケット作成
	m_id = socket(AF_INET, SOCK_STREAM, 0);
	if( m_id==INVALID_SOCKET )
	{
		throw socket_error(base + "socket");
	}

	// サーバのIPアドレスを４バイトのulongとして取得
	unsigned long server_ip_addres;

//#ifdef _MSC_VER
	server_ip_addres = inet_addr((char*)iHostName);
	if ( server_ip_addres == INADDR_NONE )
/*#else
	if (inet_aton(iHostName, (in_addr*)&server_ip_addres) == 0 ) 
#endif */// _MSC_VER
	{
		// iHostNameがIPアドレスではない場合、
		// ホスト名であると判断し、IPアドレスへの変換を試みる
		struct hostent* the_hostent = gethostbyname(iHostName);
		if(the_hostent == NULL)
		{
			this->disconnect();
			throw socket_error(base + "gethostbyname("+iHostName+")" );
		}
		server_ip_addres = *((unsigned long *)(the_hostent->h_addr_list)[0]);
	}

	// 接続のための構造体を設定
	sockaddr_in server_sock_addres;
	memset((char*)(&server_sock_addres), 0, sizeof(sockaddr_in));
	server_sock_addres.sin_family = AF_INET;
	server_sock_addres.sin_addr.s_addr = server_ip_addres;
	server_sock_addres.sin_port = htons((unsigned short)iPortNumber);

	// 実際に接続
	if( ::connect(m_id, (struct sockaddr*)&server_sock_addres, sizeof(sockaddr_in)) == SOCKET_ERROR )
	{
		this->disconnect();
		throw socket_error(base + "connect" );
	}
}

// 切断
void	TCPSocket::disconnect() 
{
	if ( m_id == INVALID_SOCKET )
	{
		return;
	}

	shutdown(m_id, 2);
	#ifdef _MSC_VER
		closesocket(m_id);
	#else
		close(m_id);
	#endif // _MSC_VER

	m_id = INVALID_SOCKET;
}

// 送信
int TCPSocket::send(const char* iBuffer, int iLength) 
{
	int r = ::send(m_id, iBuffer, iLength, 0);
	if ( r == SOCKET_ERROR )
	{
		throw socket_error("TCPSocket::send: send");
	}
	return	r;
}

// 受信
int	TCPSocket::receive(char* oBuffer, int iLength) 
{
	int	r = recv(m_id, oBuffer, iLength, 0);
	if ( r == SOCKET_ERROR )
	{
		throw socket_error("TCPSocket::receive: recv");
	}
	return	r;
}

// 接続中か？
bool	TCPSocket::is_connect() 
{
	return	(m_id != INVALID_SOCKET);
}



#ifdef	_DEBUG
	#include	<iostream>
#endif

void	TCPSocket::send_line(const string& i_str)
{
#ifdef	_DEBUG
	std::cout << ">" << i_str << std::endl;
#endif

	this->send(
		(i_str+"\r\n").c_str(),
		i_str.size()+2);

}

string	TCPSocket::receive_line()
{ 
	string	s;
	while ( s.size() < 2 || s.substr( s.size()-2 ) != "\r\n" )
	{
		char buf[1024];
		int  size = this->receive(buf, 1024);
		s += string(buf, size);
	}

#ifdef	_DEBUG
	std::cout << "<"  << s.substr(0, s.size()-2) << std::endl;
#endif
	
	return s.substr(0, s.size()-2);
}


string TCPSocket::convert_hostname_to_IPaddress(string i_hostname) throw(socket_error)
{
	struct hostent* the_hostent = gethostbyname(i_hostname.c_str());
	if(the_hostent == NULL)
	{
		throw socket_error("convert_hostname_to_IPaddress: gethostbyname("+i_hostname+")" );
	}
	
	unsigned char ip[4];
	memcpy(ip, *(the_hostent->h_addr_list), 4);
	char buf[32];
	sprintf(buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	return buf;
}

/*
// for debug
// cl -GX TCPSocket.cpp ws2_32.lib
#include <stdio.h>
int main()
{
	//TCPSocket::winsock2_startup();
	try {
		printf("[%s]", TCPSocket::convert_hostname_to_IPaddress("www.google.com").c_str());
	}
	catch(...) {
	}
	
	return 0;
}
*/
