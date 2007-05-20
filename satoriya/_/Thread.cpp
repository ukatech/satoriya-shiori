#include	"Thread.h"

bool	Thread::create(void* iParam, bool iSuspendedCreate) {
	assert(mThread==NULL);

	// スレッド関数の本来の引数はメンバにコピーしておく。
	mParam=iParam;
	
	// スレッド作成
	mThread=::CreateThread(
		NULL,				// セキュリティ属性
		0,					// 初期スタックサイズ 0 で自動
		Thread::ThreadProc,	// スレッド関数
		(LPVOID)this,		// スレッド関数の引数
		iSuspendedCreate ? CREATE_SUSPENDED : 0,	// 即座に実行するか否か
		&mID );				// スレッドID受け取り

	return	(mThread!=NULL);
}

void	Thread::close() {
	if ( mThread == NULL )
		return;
	::CloseHandle(mThread);
	mThread = NULL;
}
