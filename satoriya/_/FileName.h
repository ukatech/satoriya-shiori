#ifndef	FILENAME_H
#define	FILENAME_H

#include	<windows.h>
#include	"File.h"	// for isExist
class	FileName {
	TCHAR	mFileName[MAX_PATH+1];
public:
	FileName() { mFileName[0]='\0'; }
	FileName(const char* iFileName) { assert(strlen(iFileName)<=MAX_PATH); strcpy(mFileName, iFileName); }
	FileName&	operator=(const char* iFileName) { assert(strlen(iFileName)<=MAX_PATH); strcpy(mFileName, iFileName); return *this; }
	bool	operator==(const char* iFileName) { assert(strlen(iFileName)<=MAX_PATH); return strcmp(mFileName, iFileName)==0; }
	bool	operator!=(const char* iFileName) { assert(strlen(iFileName)<=MAX_PATH); return strcmp(mFileName, iFileName)!=0; }
	operator char*() { return mFileName; }
	operator [](int n) { return mFileName[n]; }
	const char* c_str() { return mFileName; }
	bool	isValid() { return mFileName[0] != '\0'; }
	bool	isExist() { return ::isExist(mFileName); }
};


#endif	//	FILENAME_H
