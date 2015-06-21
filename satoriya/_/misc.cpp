
#include	<assert.h>
#include	<ctype.h>
#include	<stddef.h>

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

// ‘å•¶š¬•¶š‚ğ–â‚í‚¸”äŠr
static const char* stristr(const char* p, const char* substr) {
	assert(p!=NULL);
	for(;*p;++p) {
		for(int i=0;substr[i];++i)
			if (tolower(substr[i])!=tolower(p[i]))
				break;
		if(substr[i]=='\0')
			return	p;
	}
	return	NULL;
}

