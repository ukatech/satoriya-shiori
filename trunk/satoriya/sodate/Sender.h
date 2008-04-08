#ifndef	SENDER_H
#define	SENDER_H

#include	<iostream>
using namespace std;

class sender_buf : public streambuf {
public:
	sender_buf() { line[0]='\0'; pos=0; }
	virtual int overflow(int c=EOF);

	enum const_value {
		MAX=8191
	};
private:
	char	line[MAX+1];
	int		pos;
};

class sender_stream : public ostream {
	sender_buf	buf;
public:
	sender_stream() : ostream(&buf) {}
};


extern	sender_stream	sender;
bool	Send(const char* iFormat, ...);

extern	bool	sender_flag;

#endif	// SENDER_H
