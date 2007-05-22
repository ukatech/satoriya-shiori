#include "SakuraFMO.h"
#include "../_/FMO.h"
#include "../_/Sender.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


bool SakuraFMO::update()
{
	vector<char> v;
	{
		FMO	fmo;
		if ( !fmo.open(FILE_MAP_READ, FALSE, "Sakura") ) {
			sender << "FMO can't open." << endl;
			return	false;
		}

		LPVOID	p = fmo.map();
		if ( p==NULL ) {
			sender << "FMO can't mapping." << endl;
			return	false;
		}

		int size = *((long*)p);
		v.resize(size+1);
		v[size] = '\0';
		memcpy(&v[0], p, size);
		fmo.unmap(p);
		fmo.close();

		/*sender << 
			"SakuraFMO::update() - - -" << endl <<
			string((char*)p,size) << endl <<
			"- - - - - - - - - - - - -" << endl;*/
	}


	strvec	lines;
	split(&v[4], CRLF, lines);
	for( strvec::iterator i=lines.begin() ; i!=lines.end() ; ++i ) {
		strvec	MD5andDATA;
		if ( split(*i, ".", MD5andDATA, 2) != 2 )
		{
			continue;
		}

		strvec	ENTRYandVALUE;
		static const char BYTE1[2] = {1,0};
		if ( split(MD5andDATA[1], BYTE1, ENTRYandVALUE) != 2 )
		{
			continue;
		}

		((*this)[ MD5andDATA[0] ])[ ENTRYandVALUE[0] ] = ENTRYandVALUE[1];
	}

	// log
	for( const_iterator i=begin() ; i!=end() ; ++i )
	{
		const strmap&	m = i->second;
		sender << i->first << endl;
		for( strmap::const_iterator j=m.begin() ; j!=m.end() ; ++j )
		{
			sender << "@" << j->first << ":" << j->second << endl;
		}
	}
	
	return	true;
}



