#include	"Thread.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

bool	Thread::create(void* iParam, bool iSuspendedCreate) {
	assert(mThread==NULL);

	// スレッド関数の本来の引数はメンバにコピーしておく。
	mParam=iParam;
	
	// スレッド作成。ThreadMain が delete this しても mThread への代入が先に済むよう、止めた状態で作る
	mThread=::CreateThread(
		NULL,				// セキュリティ属性
		0,					// 初期スタックサイズ 0 で自動
		Thread::ThreadProc,	// スレッド関数
		(LPVOID)this,		// スレッド関数の引数
		CREATE_SUSPENDED,
		&mID );				// スレッドID受け取り

	if ( mThread==NULL )
		return	false;
	if ( !iSuspendedCreate )
		::ResumeThread(mThread);	// ここから先は this に触れない
	return	true;
}

void	Thread::close() {
	if ( mThread == NULL )
		return;
	::CloseHandle(mThread);
	mThread = NULL;
}
