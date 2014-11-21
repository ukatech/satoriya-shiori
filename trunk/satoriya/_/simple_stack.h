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
		this->push_back(iT);
	}
	T	pop() {
		assert(this->size()>0);
		T	theT = this->back();
		this->pop_back();
		return	theT;
	}
	void	pop(int num) {
		assert(num>=0 && num<=this->size());
		while ( num-- > 0 ) {
			this->pop_back();
		}
	}
	T&	top() {
		return	this->back();
	}
	T&	from_top(int len) {
		assert(len>=0 && len<this->size());
		return	(*this)[this->size()-1-len];
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
