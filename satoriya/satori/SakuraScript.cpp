#include "SakuraScript.h"
#include <mbctype.h> // for _ismbblead

string sakura_script_parts::serialize() const
{
	string r;
	for ( const_iterator it=begin() ; it!=end() ; ++it )
	{
		r += it->command;
		r += it->value;
	}
	return r;
}


void sakura_script_parts::unserialize(string i_script)
{
	string	fragment;
	const char* p = i_script.c_str();
	while (*p)
	{
		string	c=get_a_chr(p);	// 全角半角問わず一文字取得し、pを一文字すすめる

		if ( c=="\\" || c=="%" )
		{
			if (*p=='\\'||*p=='%')	// エスケープされた\, %
			{
				fragment += (c + *p++);
				continue;
			}

			if ( !fragment.empty() )
			{
				this->push_back( sakura_script_part("", fragment) );
				fragment = "";
			}
			
			string command, option;
			{
				const char*	start=p;
				while (!_ismbblead(*p) && (isalpha(*p)||isdigit(*p)||*p=='!'||*p=='*'||*p=='&'||*p=='?'||*p=='_'))
				{
					++p;
				}
				command.assign(start, p - start);
			}
	
			if (*p=='[')
			{
				const char* start = ++p;
				while (*p!=']')
				{
					if (p[0]=='\\' && p[1]==']')
					{	
						++p; // エスケープされた]
					}
					p += _ismbblead(*p) ? 2 : 1;
				}
				option.assign(start, p++ - start);
			}

			if ( option=="" )
			{
				this->push_back( sakura_script_part(c + command, "") );
			}
			else
			{
				this->push_back( sakura_script_part(c + command, option) );
			}
		}
		else
		{
			fragment += c;
		}
	}
	
	if ( !fragment.empty() )
	{
		this->push_back( sakura_script_part("", fragment) );
	}

}
