#include	"FlowSystem.h"
#include	"Task.h"
#include	<assert.h>
#include	<windows.h>	// for OutputDebugString

FlowSystem&	FlowSystem::I=mInstance;
FlowSystem	FlowSystem::mInstance;

FlowSystem::FlowSystem() :
	mTask(NULL),
	mID(EMPTYTASK),	
	mRequestedID(EMPTYTASK)
{
}

FlowSystem::~FlowSystem() {
	initialize();
}

void	FlowSystem::initialize() {
	mID=EMPTYTASK;
	mRequestedID=EMPTYTASK;
	delete mTask;
	mTask=NULL;
}

void	FlowSystem::requestChange(ID iID) {
	mRequestedID = iID;
}

void	FlowSystem::run() {

	// 要求されているフローIDが現在のIDと異なれば、
	// 現在のフロー（に対応したルートタスク、ひいてはタスクツリー）を作り直す
	if ( mID != mRequestedID )
		remake();

	// メモリ不足なら投げる筈なのでNULLになるのは何らかのバグ
	assert(mTask != NULL);	

	mTask->run();
}

#if 0
#include	"FlowSelect.h"
#include	"testrender.h"
void	FlowSystem::remake() {

	mID = mRequestedID;
	delete mTask;

	switch ( mID ) {

	// ここでフローの一覧を定義。
	case EMPTYTASK: mTask=new EmptyTask(); break;

	case FLOWSELECT: mTask=new FlowSelect(); break;

	case TESTRENDER: mTask=new TestRender(); break;

	default:
		::OutputDebugString("\nフローIDに対応する処理が定義されていません。\n");
		break;
	}
}
#endif
