#ifndef	THREAD_H
#define	THREAD_H

#include	<windows.h>
#include	<assert.h>

class	Thread {
private:
	HANDLE	mThread;	// スレッドハンドル
	DWORD	mID;		// スレッドのID
	
	// スレッドのメイン関数（純粋仮想）
	virtual	DWORD	ThreadMain()=0;
	// 仮スレッド関数（_this->ThreadMainを呼び出すだけ）
	static	DWORD WINAPI ThreadProc(LPVOID iThis) { return reinterpret_cast<Thread*>(iThis)->ThreadMain(); }

protected:
	LPVOID	mParam;	// スレッド起動時の引数

public:
	Thread() : mThread(NULL), mID(0), mParam(NULL) {}
	virtual	~Thread() {}

	// 作成
	virtual	bool	create(void* iParam=NULL, bool iSuspendedCreate=false);
	// ハンドルを閉じる
	virtual	void	close();

	// 優先度設定
	virtual	BOOL	setPriority(int nPriority) { return ::SetThreadPriority(mThread, nPriority); }
	// 強制中断
	virtual	BOOL	terminate(DWORD dwExitCode=-1) { return ::TerminateThread(mThread, dwExitCode); }
	// 終了待ち
	virtual	DWORD	waitEnd(DWORD dwTimeOut=INFINITE) { return ::WaitForSingleObject(mThread, dwTimeOut); };
	// 終了を確認
	virtual	DWORD	isEnd() { return ( ::WaitForSingleObject(mThread, 0 ) == WAIT_OBJECT_0 );}
};

#endif	//	THREAD_H

