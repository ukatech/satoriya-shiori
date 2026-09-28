#include <string>
#include <vector>
#include "../_/charset.h"
using std::wstring;

#ifndef POSIX
#  include <windows.h>
#endif

// コンソールアプリケーションを呼び出す
inline wstring // エラーメッセージ。""なら正常終了
call_console_application(
	const wstring& i_command_line,   // コマンドと引数
	const wstring& i_boot_directory, // 実行時のディレクトリ
	wstring& o_stdout // 標準出力内容
	)
{
#ifdef POSIX
	return L""; // 未サポート。この関数は呼ばれない。

#else /* POSIX */   
	SECURITY_ATTRIBUTES	sa; 
	memset(&sa, 0, sizeof(sa)); 
	sa.nLength = sizeof(SECURITY_ATTRIBUTES); 
	sa.lpSecurityDescriptor = NULL; 
	sa.bInheritHandle = TRUE; 

	// HANDLEホルダ。
	class Handle
	{
		HANDLE mHandle;
	public:
		Handle() : mHandle(NULL) {}
		Handle(HANDLE iHandle) : mHandle(iHandle) {}
		~Handle() { ::CloseHandle(mHandle); }
		operator HANDLE() { return mHandle; }
		LPHANDLE p() { return &mHandle; }
	};

	Handle stdout_read, stdout_write;
	if ( !::CreatePipe(stdout_read.p(), stdout_write.p(), &sa, 0) )
	{
		return	L"CreatePipeで失敗。";
	}
	
	if ( !::DuplicateHandle(
		::GetCurrentProcess(), stdout_write, 
		::GetCurrentProcess(), NULL, 0, FALSE, DUPLICATE_SAME_ACCESS) )
	{
		return L"DuplicateHandleで失敗。";
	}

	STARTUPINFO si; 
	memset(&si, 0, sizeof(STARTUPINFO)); 
	si.cb = sizeof(STARTUPINFO); 
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = stdout_write; 

	PROCESS_INFORMATION	pi; 
	memset(&pi, 0, sizeof(pi)); 

	// CreateProcessは書き換え可能なバッファを要求する
	std::vector<wchar_t> command_line(i_command_line.begin(), i_command_line.end());
	command_line.push_back(L'\0');

	if ( !::CreateProcess(NULL, &command_line[0],
	 NULL, NULL, TRUE, 0, NULL, i_boot_directory.c_str(), &si, &pi))
	{ 
		return	L"CreateProcessで失敗。";
	}
	if ( ::WaitForSingleObject(pi.hProcess, 10000)==WAIT_TIMEOUT )
	{
		return	L"呼び出しタイムアウト。";
	}

	std::string out_bytes;
	while (true)
	{
		DWORD	dwResult;
		DWORD	dwBytesLeftThisMessage;
		PeekNamedPipe(stdout_read, NULL, 0, NULL, &dwResult, &dwBytesLeftThisMessage);
		if ( dwResult==0 && dwBytesLeftThisMessage==0 )
			break;
		if (dwResult > 0)
		{ 
			char szBuf[256];
			ReadFile(stdout_read, szBuf, sizeof(szBuf), &dwResult, NULL);
			out_bytes.append(szBuf, dwResult);
		}
	}

	::CloseHandle(pi.hThread);
	::CloseHandle(pi.hProcess);

	// 出力の文字コードは不明なので判定する
	o_stdout += MBtoW(out_bytes, CS_NULL);

	return	L"";
#endif /* POSIX */
}
