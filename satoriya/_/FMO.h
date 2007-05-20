

#include	<windows.h>
#include	<assert.h>

// 読むだけ。書けない。困ったもんだ。
class	FMO {
	HANDLE	mHandle;
public:
	FMO() { mHandle=NULL; }
	~FMO() { close(); }
	bool	isValid() { return (mHandle!=NULL); }

	BOOL	create(DWORD Protect, DWORD MaxSize, LPCTSTR lpszName) {
		assert(mHandle==NULL);
		mHandle = ::CreateFileMapping((HANDLE)0xffffffff, NULL, Protect, 0, MaxSize, lpszName);
		return	( mHandle != NULL );
	}
	BOOL	open(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCTSTR lpszName) {
		assert(mHandle==NULL);
		mHandle = ::OpenFileMapping(dwDesiredAccess, bInheritHandle, lpszName);
		return	( mHandle != NULL );
	}
	LPVOID	map(DWORD fdwAccess=FILE_MAP_READ, DWORD dwOffsetHigh=0, DWORD dwOffsetLow=0, DWORD cbMap=0) {
		assert(mHandle!=NULL);
		return	::MapViewOfFile(mHandle, fdwAccess, dwOffsetHigh, dwOffsetLow, cbMap);
	}
	BOOL	unmap(LPVOID lpBaseAddres) {
		assert(mHandle!=NULL);
		return	::UnmapViewOfFile(lpBaseAddres);
	}
	void	close() {
		if ( mHandle != NULL ) {
			::CloseHandle(mHandle);
			mHandle=NULL;
		}
	}
};


#if	0

template<class T, class U>
class	FMOmap {

	// FMOの名前。プロセス間で共有して用いる。
	FMO	mFMO;
	string	mName;

	// lockしてunlockするまで有効。
	map<T,U>	mMap;

private:
	bool	read() {
		if ( !mFMO.Open(FILE_MAP_READ, FALSE, mName.c_str()) )
			return	false;

		LPVOID	pfm = fmo.Map();
		if ( pfm==NULL ) {
			sender << "FMO can't mapping." << endl;
			return	false;
		}

		long	size = *((long*)pfm);
		char*	p = new char[size+1];
		memcpy(p, pfm, size);
		p[size]='\0';
		if ( !fmo.Unmap(pfm) )
			return	false;
		fmo.Close();

		static const char	ret_dlmt[3] = { 13, 10, 0 };
		static const char	byte1_dlmt[2] = { 1, 0 };
		strvec	lines;
		split(p+4, ret_dlmt, lines);
		for( strvec::iterator i=lines.begin() ; i!=lines.end() ; ++i ) {
			strvec	MD5andDATA;
			if ( split(*i, ".", MD5andDATA, 2) != 2 )
				continue;

			strvec	ENTRYandVALUE;
			if ( split(MD5andDATA[1], byte1_dlmt, ENTRYandVALUE) != 2 )
				continue;

			(SakuraFMO[ MD5andDATA[0] ])[ ENTRYandVALUE[0] ] = ENTRYandVALUE[1];
		}

	}
	bool	write() {
	}

public:
	FMOmap(const string& iName) : mName(iName) {}
	~FMOmap() {}

	void	set(const T& t, const U& u) {
	}
	U	get(const T& t) {

	}
	void	erase(const T& t) {
	}
	void	clear() {
	}
};


#endif

