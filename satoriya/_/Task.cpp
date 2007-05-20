#include	"Task.h"
#include	<assert.h>

Task::Task() : 
	parent(NULL), 
	next(NULL), 
	prev(NULL), 
	top_child(NULL), 
	bottom_child(NULL), 
	child_num(0) {
}

Task::~Task() { 
	remove(); 
	while ( top_child != NULL )
		delete top_child;
}

void	Task::run() {
	this->handle();
	Task	*now=top_child, *next;
	while ( now != NULL ) {
		next = now->next;
		now->run();
		now = next;
	}
}

//---------------------------------------------------------------------------------
// 以下は樹構造部分。

void	Task::remove() {

	// 自分の子供には触らない。

	// 親があれば切り離す
	if ( parent ) {
		// 親の先頭子供が自分であれば、次の子供にゆだねる。
		// 自分が唯一の子供だった場合は、親のポインタは NULL になる。
		if ( parent->top_child == this )
			parent->top_child = next;
		// 末尾も同様に。
		if ( parent->bottom_child == this )
			parent->bottom_child = prev;
		--(parent->child_num);

		parent = NULL;
	}

	// 前後があれば詰めあわせる
	if ( next )
		next->prev = prev;
	if ( prev )
		prev->next = next;
	next = prev = NULL;	// NULLにするのは最後ね。
}

void	Task::addChildTop(Task* p) {
	assert( p != NULL );

	// 新入りの親兄弟を切り離し、自分を親とする
	p->remove();
	p->parent = this;
	
	if ( top_child == NULL ) 
		top_child = bottom_child = p;	// 初子供
	else {
		assert( top_child->prev == NULL );

		// すでに子供がいる
		// 既存先頭の手前に追加し、新子供を先頭に。
		top_child->prev = p;
		p->next = top_child;
		top_child = p;
	} 
	++child_num;
}

void	Task::addChildBottom(Task* p) {
	assert( p != NULL );

	// 新入りの親兄弟を切り離し、自分を親とする
	p->remove();
	p->parent = this;
	
	if ( bottom_child == NULL ) 
		bottom_child = top_child = p;	// 初子供
	else {
		assert( bottom_child->next == NULL );

		// すでに子供がいる
		// 既存末尾の手前に追加し、新子供を末尾に。
		bottom_child->next = p;
		p->prev = bottom_child;
		bottom_child = p;
	} 
	++child_num;
}

void	Task::insertNext(Task* p) {
	assert( p != NULL );
	assert( parent != NULL );	// 兄弟をいじるにはまず親が要る

	// 子供以外を切り離す
	p->remove();
	// 親は共通化
	p->parent = parent;
	++(parent->child_num);
	// 自分が末弟だった場合、pに立場を譲る
	if ( parent->bottom_child == this )
		parent->bottom_child = p;

	// 前後関係
	p->next = next;
	p->prev = this;
	if ( next )
		next->prev = p;
	next = p;

}

void	Task::insertPrev(Task* p) {
	assert( p != NULL );
	assert( parent != NULL );	// 兄弟をいじるにはまず親が要る

	// 子供以外を切り離す
	p->remove();
	// 親は共通化
	p->parent = parent;
	++(parent->child_num);
	// 自分が長兄だった場合、pに立場を譲る
	if ( parent->top_child == this )
		parent->top_child = p;

	// 前後関係
	p->prev = prev;
	p->next = this;
	if ( prev )
		prev->next = p;
	prev = p;
}

