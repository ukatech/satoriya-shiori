#ifndef	SIMPLE_STACK_H
#define	SIMPLE_STACK_H

#include	<vector>
#include	<cassert>
using namespace std;

template<class T>
class simple_stack : public std::vector<T>
{
public:

	void	push(const T& iT) {
		push_back(iT);
	}
	T	pop() {
		assert(size()>0);
		T	theT = back();
		pop_back();
		return	theT;
	}
	void	pop(int num) {
		assert(num>=0 && num<=size());
		while ( num-- > 0 ) {
			pop_back();
		}
	}
	T&	top() {
		return	back();
	}
	T&	from_top(int len) {
		assert(len>=0 && len<size());
		return	(*this)[size()-1-len];
	}
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
