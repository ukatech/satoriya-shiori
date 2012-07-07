/*----------------------------------------------------------------------------

  Light SSTP Client
  Copyright(C) 2001 Koumei Hashimoto, all rights reserved.

  dsstp.cpp

----------------------------------------------------------------------------*/
#include "dsstp.h"
#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」

/*----------------------------------------------------------------------------
	ローカル定義
----------------------------------------------------------------------------*/
#define MY_DIRECTSSTP_PORT	9801
#define MY_EXIST_FILEMAP	"Sakura"

/*----------------------------------------------------------------------------
	lstrchr()
	マルチバイトstrchr
----------------------------------------------------------------------------*/
LPSTR lstrchr(LPCSTR sz, CHAR ch)
{
	return (LPSTR)_mbschr((LPBYTE)sz, (UINT)ch);
}

/*----------------------------------------------------------------------------
	CheckSakuraFileMapping()
	ファイルマッピングオブジェクトの存在確認
----------------------------------------------------------------------------*/
BOOL CheckSakuraFileMapping(HWND hParentWnd, vector<HWND>& vec)
{
	HANDLE hFileMap;
	LPVOID lpBasePtr;
	LPSTR lpBuffer;
	LPSTR lpType1;
	LPSTR lpType2;
	CHAR szTemp[200];
	DWORD dwSize;
	HWND hWnd = NULL;
	BOOL bRet;

	vec.clear();

	// ファイルマップを開く
	hFileMap = OpenFileMapping(FILE_MAP_READ, FALSE, MY_EXIST_FILEMAP);
	if(hFileMap == NULL) 
	{
		// 存在しない
		//MessageBox(hParentWnd, "ファイルマッピングオブジェクトが開けません。", "TestSSTP", MB_ICONSTOP);
		return FALSE;
	}

	// ベースアドレス取得
	lpBasePtr = (LPVOID)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
	if(lpBasePtr == NULL)
	{
		// 失敗
		//MessageBox(hParentWnd, "ファイルマッピングオブジェクトが操作できません。", "TestSSTP", MB_ICONSTOP);
		CloseHandle(hFileMap);
		return FALSE;
	}
	lpBuffer = (LPSTR)lpBasePtr;

	// データ読み込み
	bRet = TRUE;
	try
	{
		CopyMemory(&dwSize, lpBuffer, sizeof(DWORD));
		lpBuffer += sizeof(DWORD);

		while(*lpBuffer)
		{
			// エントリ解析
			lpType1 = lstrchr(lpBuffer, '.');
			lpType1++;
			lpType2 = lstrchr(lpType1, '\01');
			lpType2++;
			lstrcpyn(szTemp, lpType1, lpType2 - lpType1);

			// エントリの種類ごとに分岐
			if(lstrcmpi(szTemp, "hwnd") == 0)
			{
				// データ取得
				lpType1 = lstrchr(lpType2, '\r');
				lpType1++;
				lstrcpyn(szTemp, lpType2, lpType1 - lpType2);
				hWnd = (HWND)atoi(szTemp);

				vec.push_back(hWnd);
			}

			// \r\n まで１エントリ
			lpBuffer = lstrchr(lpBuffer, '\n');
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
BOOL sendDirectSSTP_for_NOTIFY(string client, string id, deque<string>& refs) 
//BOOL DirectSSTPSendMessage(HWND hParentWnd, LPCSTR szClient, LPCSTR szMessage, LPCSTR szOption)
{
	HWND hParentWnd=NULL;
	LPCSTR szClient=client.c_str();
	LPCSTR szOption="";
	CHAR szBuffer[100];
	COPYDATASTRUCT cds;
	DWORD dwRet;
	vector<HWND> vec;
	vector<HWND>::iterator it;
	string strSendBuffer;

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

	wsprintf(szBuffer, "%d", hParentWnd);

	strSendBuffer = "NOTIFY SSTP/1.5\r\n";
	strSendBuffer += "Sender: ";
	strSendBuffer += szClient;
	strSendBuffer += "\r\n";
	strSendBuffer += "Event: ";
	strSendBuffer += id;
	strSendBuffer += "\r\n";
	int	n='0';
	for (deque<string>::iterator i=refs.begin() ; i!=refs.end() ; ++i, ++n) {
		strSendBuffer += "Reference";
		strSendBuffer += (char)n;
		strSendBuffer += ": ";
		strSendBuffer += *i;
		strSendBuffer += "\r\n";
	}
	strSendBuffer += "Charset: Shift_JIS";
	strSendBuffer += "\r\n";
	strSendBuffer += "\r\n";

	// 送信
	cds.dwData = MY_DIRECTSSTP_PORT;
	cds.cbData = strSendBuffer.size();
	cds.lpData = (LPVOID)strSendBuffer.c_str();
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
#define MY_LOCALHOST	"127.0.0.1"
#define MY_SAKURA_MUTEX	"sakura"
#define MY_SSP_MUTEX	"ssp"


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

