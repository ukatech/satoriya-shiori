#include	"Utilities.h"
#include	<stdio.h>

//////////DEBUG/////////////////////////
#include "warning.h"
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

// 文字列中から指定の文字が最後に出現する位置を返す。
const wchar_t*	FindFinalChar( const wchar_t* start, wchar_t c ) {
	return	wcsrchr(start, c);
}


bool	CutExtention(wchar_t* iFileName) {
	assert(iFileName != NULL);
	wchar_t*	dot = FindFinalChar(iFileName, L'.');
	if ( dot == NULL )
		return	false;
	*dot=L'\0';
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
const wchar_t*	SkipDelimiter(const wchar_t* p) {
	assert(p!=NULL);
	while ( *p==L' ' || *p==L'\t' )
		p++;
	return	p;
}

