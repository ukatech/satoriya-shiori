#include <string>
using namespace std;

#ifndef POSIX
#  include <windows.h>
#endif

// コンソールアプリケーションを呼び出す
inline string // エラーメッセージ。""なら正常終了
call_console_application(
	const string& i_command_line,   // コマンドと引数
	const string& i_boot_directory, // 実行時のディレクトリ
	string& o_stdout // 標準出力内容
	)
{
#ifdef POSIX
	return ""; // 未サポート。この関数は呼ばれない。

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
		return	"CreatePipeで失敗。";
	}
	
	if ( !::DuplicateHandle(
		::GetCurrentProcess(), stdout_write, 
		::GetCurrentProcess(), NULL, 0, FALSE, DUPLICATE_SAME_ACCESS) )
	{
		return "DuplicateHandleで失敗。";
	}

	STARTUPINFO si; 
	memset(&si, 0, sizeof(STARTUPINFO)); 
	si.cb = sizeof(STARTUPINFO); 
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.hStdOutput = stdout_write; 

	PROCESS_INFORMATION	pi; 
	memset(&pi, 0, sizeof(pi)); 

	char	command_line[4096];
	strcpy(command_line, i_command_line.c_str());

	if ( !::CreateProcess(NULL, command_line,
	 NULL, NULL, TRUE, 0, NULL, i_boot_directory.c_str(), &si, &pi))
	{ 
		return	"CreateProcessで失敗。";
	}
	if ( ::WaitForSingleObject(pi.hProcess, 10000)==WAIT_TIMEOUT )
	{
		return	"呼び出しタイムアウト。";
	}

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
			ReadFile(stdout_read, szBuf, sizeof(szBuf) - 1, &dwResult, NULL); 
			szBuf[dwResult] = '\0'; 
			o_stdout += szBuf;
		}
	} 
	return	"";
#endif /* POSIX */
}
