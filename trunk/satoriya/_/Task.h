#ifndef	TASK_H
#define	TASK_H

#define	NULL	0

class Task {

	Task	*parent, *next, *prev, *top_child, *bottom_child;
	int		child_num;

	virtual void	handle() {}	

public:
	void	insertNext(Task*);
	void	insertPrev(Task*);
	void	addChildTop(Task*);
	void	addChildBottom(Task*);
	Task*	getParent(Task*) { return parent; }
	Task*	getNext() { return next; }
	Task*	getPrev() { return prev; }
	Task*	getChildTop() { return top_child; }
	Task*	getChildBottom() { return bottom_child; }
	int		getChildNum() const { return child_num; }
	void	remove();	// 親兄弟を切り離す。子孫には影響なし。

	Task();
	virtual ~Task();
	void	run();
};

// コピペ用・継承サンプル
class EmptyTask : public Task {
	virtual void	handle() {}
public:
	EmptyTask() : Task() {}
	virtual ~EmptyTask() {}
};

#endif	//	TASK_H




#if 0	//----------------------------------------------------------------------

// コピペ用・継承サンプル分割版

// 宣言部
class EmptyTask : public Task {
	virtual void	handle();
public:
	EmptyTask();
	virtual ~EmptyTask();
};

// 実装部
EmptyTask::EmptyTask() : 
Task() {
}

EmptyTask::~EmptyTask() {
}

void	EmptyTask::handle() {
}

#endif
