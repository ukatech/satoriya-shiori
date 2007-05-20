#ifndef	VCOUT_H
#define	VCOUT_H


#include	<iostream>
using namespace std;

#include	<windows.h>
#include	<mbctype.h>		// for _ismbblead()


class vc_debugstring_buf : public streambuf {
	int	knj;	// 2byte•¶š‚Ì•Û
public:
	vc_debugstring_buf() : knj(0) {}
	virtual int overflow(int c=EOF) {
		char	buf[3];
		if (knj != 0) {
			// 2byte•¶š‚Ì2byte–Ú
			buf[0] = (char)knj;
			buf[1] = (char)c;
			buf[2] = '\0';
			::OutputDebugString(buf);
			knj = 0;
		}
		else if ( _ismbblead(c) ) {
			knj = c;
		}
		else {
			buf[0] = (char)c;
			buf[1] = '\0';
			::OutputDebugString(buf);
		}
		return	c;
	}
};

class vc_debugstring_stream : public ostream {
	vc_debugstring_buf	buf;
public:
	vc_debugstring_stream() : ostream(&buf) {}
};

//extern	vc_debugstring_stream	vcout;
static	vc_debugstring_stream	vcout;

#endif	// VCOUT_H
