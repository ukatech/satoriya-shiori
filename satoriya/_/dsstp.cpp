/*----------------------------------------------------------------------------

  Light SSTP Client
  Copyright(C) 2001 Koumei Hashimoto, all rights reserved.

  dsstp.cpp

----------------------------------------------------------------------------*/
#include "dsstp.h"
#include "charset.h"
#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/*----------------------------------------------------------------------------
	ローカル定義
----------------------------------------------------------------------------*/
#define MY_DIRECTSSTP_PORT	9801
#define MY_EXIST_FILEMAP	L"Sakura"

/*----------------------------------------------------------------------------
	CheckSakuraFileMapping()
	ファイルマッピングオブジェクトの存在確認
----------------------------------------------------------------------------*/
BOOL CheckSakuraFileMapping(HWND hParentWnd, vector<HWND>& vec)
{
	HANDLE hFileMap;
	LPVOID lpBasePtr;
	const char* lpBuffer;
	const char* lpType1;
	const char* lpType2;
	char szTemp[200];
	DWORD dwSize;
	HWND hWnd = NULL;
	BOOL bRet;

	vec.clear();

	// ファイルマップを開く
	hFileMap = OpenFileMapping(FILE_MAP_READ, FALSE, MY_EXIST_FILEMAP);
	if(hFileMap == NULL)
	{
		// 存在しない
		return FALSE;
	}

	// ベースアドレス取得
	lpBasePtr = (LPVOID)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
	if(lpBasePtr == NULL)
	{
		// 失敗
		CloseHandle(hFileMap);
		return FALSE;
	}
	lpBuffer = (const char*)lpBasePtr;

	// データ読み込み
	// 区切り文字はすべてASCIIで、ここで取り出すのはhwnd(数値)だけなので、バイト列のまま扱う。
	bRet = TRUE;
	try
	{
		CopyMemory(&dwSize, lpBuffer, sizeof(DWORD));

		// 先頭の DWORD が全体の大きさ。終端の NUL が無くても読み過ぎないよう、写像の範囲内で写し取ってから解析する
		MEMORY_BASIC_INFORMATION mbi;
		if ( ::VirtualQuery(lpBasePtr, &mbi, sizeof(mbi)) == 0 || dwSize > mbi.RegionSize ) {
			dwSize = 0;
		}
		const std::string theData = ( dwSize > sizeof(DWORD) ) ? std::string(lpBuffer + sizeof(DWORD), dwSize - sizeof(DWORD)) : std::string();
		lpBuffer = theData.c_str();

		while(lpBuffer && *lpBuffer)
		{
			// エントリ解析
			lpType1 = strchr(lpBuffer, '.');
			if ( !lpType1 ) { break; }
			lpType1++;
			lpType2 = strchr(lpType1, '\01');
			if ( !lpType2 ) { break; }
			lpType2++;
			lstrcpynA(szTemp, lpType1, min((int)sizeof(szTemp), lpType2 - lpType1));

			// エントリの種類ごとに分岐
			if(lstrcmpiA(szTemp, "hwnd") == 0)
			{
				// データ取得
				lpType1 = strchr(lpType2, '\r');
				if ( !lpType1 ) { break; }
				lpType1++;
				lstrcpynA(szTemp, lpType2, min((int)sizeof(szTemp), lpType1 - lpType2));
				hWnd = (HWND)atoi(szTemp);

				vec.push_back(hWnd);
			}

			// \r\n まで１エントリ
			lpBuffer = strchr(lpBuffer, '\n');
			if(lpBuffer)
			{
				lpBuffer++;
			}
		}

		bRet = vec.size() > 0 ? TRUE : FALSE;
	}
	catch(...)
	{
		bRet = FALSE;
	}

	UnmapViewOfFile(lpBasePtr);
	CloseHandle(hFileMap);
	return bRet;
}



/*----------------------------------------------------------------------------
	DirectSSTPSendMessage()
	DirectSSTP メッセージ送信
----------------------------------------------------------------------------*/
BOOL sendDirectSSTP_for_NOTIFY(wstring client, wstring id, deque<wstring>& refs) 
//BOOL DirectSSTPSendMessage(HWND hParentWnd, LPCSTR szClient, LPCSTR szMessage, LPCSTR szOption)
{
	HWND hParentWnd=NULL;
	const wchar_t* szClient=client.c_str();
	COPYDATASTRUCT cds;
	DWORD dwRet;
	vector<HWND> vec;
	vector<HWND>::iterator it;
	wstring strSendBuffer;

	// 存在をチェック
	if(!CheckSakuraMutex())
	{
		//MessageBox(hParentWnd, "SSTPサーバーが起動していません。", "TestSSTP", MB_ICONSTOP);
		return FALSE;
	}

	// ウィンドウ取得
	if(!CheckSakuraFileMapping(hParentWnd, vec))
	{
		//MessageBox(hParentWnd, "DirectSSTPが使用できません。", "TestSSTP", MB_ICONSTOP);
		return FALSE;
	}

	strSendBuffer = L"NOTIFY SSTP/1.5\r\n";
	strSendBuffer += L"Charset: UTF-8\r\n";
	strSendBuffer += L"Sender: ";
	strSendBuffer += szClient;
	strSendBuffer += L"\r\n";
	strSendBuffer += L"Event: ";
	strSendBuffer += id;
	strSendBuffer += L"\r\n";
	int	n=0;
	for (deque<wstring>::iterator i=refs.begin() ; i!=refs.end() ; ++i, ++n) {
		wchar_t	num[16];
		strSendBuffer += L"Reference";
		strSendBuffer += _itow(n, num, 10);	// 10個目以降も Reference10, Reference11, ...
		strSendBuffer += L": ";
		strSendBuffer += *i;
		strSendBuffer += L"\r\n";
	}
	strSendBuffer += L"\r\n";

	// 送信
	const std::string sendBytes = WtoUTF8(strSendBuffer);
	cds.dwData = MY_DIRECTSSTP_PORT;
	cds.cbData = sendBytes.size();
	cds.lpData = (LPVOID)sendBytes.c_str();
	for(it = vec.begin(); it != vec.end(); it++)
	{
		// WM_COPYDATA は汎用なので、HWND_BROADCAST してはいけません。
		::SendMessageTimeout(*it, WM_COPYDATA, (WPARAM)hParentWnd, (LPARAM)&cds,
			SMTO_ABORTIFHUNG|SMTO_BLOCK, 1000, &dwRet);
	}

	return TRUE;
}

/*----------------------------------------------------------------------------
	ローカル定義
----------------------------------------------------------------------------*/
#define MY_LOCALHOST	L"127.0.0.1"
#define MY_SAKURA_MUTEX	L"sakura"
#define MY_SSP_MUTEX	L"ssp"


/*----------------------------------------------------------------------------
	CheckSakuraMutex()
	SSTP サーバー存在確認
----------------------------------------------------------------------------*/
BOOL CheckSakuraMutex()
{
	//return TRUE;
	HANDLE hMutex;

	hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MY_SAKURA_MUTEX);
	if(hMutex == NULL) 
	{
		hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MY_SSP_MUTEX);
		if(hMutex == NULL) 
		{
			return FALSE;
		}

		CloseHandle(hMutex);
		return TRUE;
	}

	CloseHandle(hMutex);
	return TRUE;
}

