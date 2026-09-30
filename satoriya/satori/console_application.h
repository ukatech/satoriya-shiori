#include <string>
#include <vector>
#include "../_/charset.h"
using std::wstring;

#ifndef POSIX
#  include <windows.h>
#endif

#ifndef POSIX
// パイプに溜まっている分を全部読んで out に足す
inline void read_available_from_pipe(HANDLE i_pipe, std::string& out)
{
	while (true)
	{
		DWORD	dwAvail = 0;
		if ( !::PeekNamedPipe(i_pipe, NULL, 0, NULL, &dwAvail, NULL) || dwAvail == 0 )
			break;
		char szBuf[256];
		DWORD	dwRead = 0;
		if ( !::ReadFile(i_pipe, szBuf, sizeof(szBuf), &dwRead, NULL) || dwRead == 0 )
			break;
		out.append(szBuf, dwRead);
	}
}
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
	// 終了を待つ間も標準出力を読み続ける。読まないと、パイプの容量（既定で4KB）を超える出力で
	// 子プロセスが書き込みで止まり、終了しないままタイムアウトになる。
	std::string out_bytes;
	bool timed_out = false;
	const DWORD start_tick = ::GetTickCount();
	while (true)
	{
		const DWORD wait = ::WaitForSingleObject(pi.hProcess, 50);
		read_available_from_pipe(stdout_read, out_bytes);
		if ( wait != WAIT_TIMEOUT )
			break;
		if ( ::GetTickCount() - start_tick > 10000 )
		{
			timed_out = true;
			break;
		}
	}

	if ( timed_out )
	{
		::TerminateProcess(pi.hProcess, 1);	// 応答しない子プロセスは残さない
	}
	::CloseHandle(pi.hThread);
	::CloseHandle(pi.hProcess);
	if ( timed_out )
	{
		return	L"呼び出しタイムアウト。";
	}

	// 出力の文字コードは不明なので判定する
	o_stdout += MBtoW(out_bytes, CS_NULL);

	return	L"";
#endif /* POSIX */
}
