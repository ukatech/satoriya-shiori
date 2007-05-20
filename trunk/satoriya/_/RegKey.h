#ifndef	REGKEY_H
#define	REGKEY_H

#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>

class	RegKey {

	HKEY	m_hKey;
public:
	RegKey() : m_hKey(NULL) {}
	~RegKey() { Close(); }

	BOOL	Create( HKEY hParentKey, LPCTSTR lpszSubKey );
	BOOL	Open( HKEY hParentKey, LPCTSTR lpszSubKey );
	BOOL	DeleteSubKey( LPCTSTR lpszSubKey );

	BOOL	SetValue( LPCTSTR lpValueName, DWORD dwValue );
	BOOL	SetValue( LPCTSTR lpValueName, LPCTSTR lpszValue );
	BOOL	SetValue( LPCTSTR lpValueName, LPCVOID lpBuffer, DWORD dwSize );

	BOOL	GetValue( LPCTSTR lpValueName, LPDWORD dwBuffer );
	BOOL	GetValue( LPCTSTR lpValueName, LPTSTR lpszBuffer, DWORD dwBufSize );
	BOOL	GetValueSize( LPCTSTR lpValueName, LPDWORD dwSize );
	BOOL	GetValue( LPCTSTR lpValueName, LPVOID lpBuffer, DWORD dwSize );

	BOOL	DeleteValue( LPCTSTR lpValueName );

	// ÉNÉâÉXÇèúÇ¢ÇΩîCà”å^Ç sizeof T ÇóòópÇµÇƒäiî[
	template<class T>
	BOOL	SetValueT( LPCTSTR iName, const T* iT );
	template<class T>
	BOOL	GetValueT( LPCTSTR iName, T* oT );

	void	Flush();
	void	Close();

	operator HKEY() { return m_hKey; }
	BOOL	isOpened() { return ( m_hKey != NULL ); }
};

#endif	// REGKEY_H
