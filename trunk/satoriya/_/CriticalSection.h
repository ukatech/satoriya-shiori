#ifndef	CRITICAL_SECTION_H
#define	CRITICAL_SECTION_H

#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>
#include	<assert.h>

// クリティカルセクションらっぱ
class	CriticalSection {
	CRITICAL_SECTION	cs;
public:
	CriticalSection() { ::InitializeCriticalSection(&cs); }
	~CriticalSection() { ::DeleteCriticalSection(&cs); }
	
	void	Enter() { ::EnterCriticalSection(&cs); }
	void	Leave() { ::LeaveCriticalSection(&cs); }
};

class	Locker {
	CriticalSection&	mCS;
public:
	Locker( CriticalSection& iCS ) : mCS(iCS) { mCS.Enter(); }
	~Locker() { mCS.Leave(); }
};

#endif	//	CRITICAL_SECTION_H
