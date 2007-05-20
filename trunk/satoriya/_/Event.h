#ifndef	EVENT_H
#define	EVENT_H
/*--------------------------------------------------------------------
	wrapper of event object
	
	1999/07/17	çÏê¨
--------------------------------------------------------------------*/

#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>

class	Event {
	HANDLE	m_hEvent;
public:
	Event( BOOL fInitialState=FALSE, BOOL fManualReset=TRUE ) {
		m_hEvent = ::CreateEvent( NULL, fManualReset, fInitialState, NULL );
	}
	~Event() { ::CloseHandle( m_hEvent ); }

	void	Set() {	::SetEvent( m_hEvent ); }
	void	Reset() { ::ResetEvent( m_hEvent ); }
	BOOL	isSet() { return ( ::WaitForSingleObject( m_hEvent, 0 ) == WAIT_OBJECT_0 ); }
	BOOL	WaitSet( DWORD dwTimeOut=INFINITE ) { return ( ::WaitForSingleObject( m_hEvent, dwTimeOut ) == WAIT_OBJECT_0 ); }

	operator HANDLE() { return m_hEvent; }
};

#endif	//	EVENT_H

