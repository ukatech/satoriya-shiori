#include	"Utilities.h"
#include	<stdio.h>

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

/*char*	FindFinalChar(char* start, char c) {
	return	const_cast<char*>(FindFinalChar(start,c));
}*/

// 文字列中から指定の1byte文字が最後に出現する位置を返す。
const char*	FindFinalChar( const char* start, char c ) {
	const char*	last = NULL, *ptr = start;
	while ( *ptr != '\0' ) {
		if ( *ptr == c )
			last = ptr;
		mbinc(ptr);
	}
	return	last;
}


bool	CutExtention(char* iFileName) {
	assert(iFileName != NULL);
	char*	dot = FindFinalChar(iFileName, '.');
	if ( dot == NULL )
		return	false;
	*dot='\0';
	return	true;
}
/*
void	SetExtention(char* iFileName, const char* iNewExtention) {
	assert(iFileName != NULL);
	assert(iNewExtention != NULL);
	char*	dot = FindFinalChar(iFileName, '.');
	if ( dot == NULL )
		dot = iFileName + strlen(iFileName);
	sprintf(dot, ".%s", iNewExtention);
}
*/


// iStringの位置から半角スペース及び半角タブを飛ばした位置を返す
const char*	SkipDelimiter(const char* p) {
	assert(p!=NULL);
	while ( *p==' ' || *p=='\t' )
		p++;
	return	p;
}

