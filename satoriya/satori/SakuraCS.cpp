#include "SakuraCS.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

wstring cut_token(wstring& io_target, const wstring& i_delimiter)
{
	wstring word;
	cut_token(io_target, i_delimiter, word);
	return word;
}

bool cut_token(wstring& io_target, const wstring& i_delimiter, wstring& o_token)
{
	wstring::size_type pos = io_target.find(i_delimiter);
	if ( pos == wstring::npos )
	{
		o_token = io_target;
		io_target = L"";
		return false;
	}
	else
	{
		o_token = io_target.substr(0, pos);
		pos += i_delimiter.size();
		io_target = io_target.substr(pos);
		return true;
	}
}

std::string	find_charset_header(const std::string& i_request)
{
	static const char header[] = "\r\ncharset:";
	const std::string::size_type header_len = sizeof(header) - 1;
	for ( std::string::size_type pos = 0 ; pos + header_len <= i_request.size() ; ++pos )
	{
		std::string::size_type n = 0;
		for ( ; n < header_len ; ++n ) {
			char c = i_request[pos+n];
			if ( c >= 'A' && c <= 'Z' ) { c = c - 'A' + 'a'; }
			if ( c != header[n] ) { break; }
		}
		if ( n == header_len ) {
			std::string::size_type start = pos + header_len;
			std::string::size_type end = i_request.find("\r\n", start);
			if ( end == std::string::npos ) { end = i_request.size(); }
			while ( start < end && (i_request[start] == ' ' || i_request[start] == '\t') ) { ++start; }
			while ( end > start && (i_request[end-1] == ' ' || i_request[end-1] == '\t') ) { --end; }
			return i_request.substr(start, end - start);
		}
	}
	return std::string();
}

