#ifndef	SIMPLE_STACK_H
#define	SIMPLE_STACK_H

#include	<deque>
#include	<cassert>
using namespace std;

template<class T>
class simple_stack {

	deque<T>	mDeque;

public:

	void	push(const T& iT) {
		mDeque.push_back(iT);
	}
	T	pop() {
		assert(mDeque.size()>0);
		T	theT = mDeque.back();
		mDeque.pop_back();
		return	theT;
	}
	void	pop(int num) {
		assert(num>=0 && num<=mDeque.size());
		while ( num-- > 0 )
			mDeque.pop_back();
	}
	T&	top() {
		return	mDeque.back();
	}
	T&	from_top(int len) {
		assert(len>=0 && len<mDeque.size());
		return	mDeque[mDeque.size()-1-len];
	}
	int	size() { 
		return	mDeque.size();
	}
	bool	empty() {
		return	mDeque.size()==0;
	}
	void	clear() {
		mDeque.clear();
	}
	T&	operator[](int n) {
		assert(n>=0 && n<mDeque.size());
		return	mDeque[n];
	}


	typedef	typename deque<T>::iterator	iterator;
	iterator	begin() { return mDeque.begin(); }
	iterator	end() { return mDeque.end(); }
};

#ifdef	_OSTREAM_
template<class T>
inline ostream& operator<<(ostream& stream, simple_stack<T>& ary) {
	for ( int i=0 ; i<ary.size() ; i++ ) {
		stream << (ary[i]);
		if ( i < ary.size()-1 )
			stream << ", ";
	}
	return	stream;
}
#endif


#endif	// SIMPLE_STACK_H
