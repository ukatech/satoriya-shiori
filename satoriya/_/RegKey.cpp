#include	"RegKey.h"
#include	<assert.h>

BOOL	RegKey::Create( HKEY hParentKey, LPCTSTR lpszSubKey ) {
	assert( hParentKey != NULL && lpszSubKey != NULL );
	Close();

	DWORD	dwDisposition;
	if ( ::RegCreateKeyEx( 
			hParentKey, lpszSubKey,
			0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
			NULL, &m_hKey, &dwDisposition ) !=  ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

BOOL	RegKey::Open( HKEY hParentKey, LPCTSTR lpszSubKey ) {
	assert( hParentKey != NULL && lpszSubKey != NULL );
	Close();

	if ( ::RegOpenKeyEx( 
			hParentKey, lpszSubKey,
			0, KEY_ALL_ACCESS, &m_hKey ) !=  ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

BOOL	RegKey::DeleteSubKey( LPCTSTR lpszSubKey ) {
	assert( lpszSubKey != NULL );
	assert( isOpened() );

	if ( ::RegDeleteKey( m_hKey, lpszSubKey ) != ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

void	RegKey::Close() {
	if ( isOpened() ) {
		::RegCloseKey( m_hKey );
		m_hKey = NULL;
	}
}

void	RegKey::Flush() {
	if ( isOpened() ) {
		::RegFlushKey( m_hKey );
	}
}

BOOL	RegKey::SetValue( LPCTSTR lpValueName, DWORD dwValue ) {
	assert( lpValueName != NULL );
	if ( ::RegSetValueEx( m_hKey, lpValueName, 0,
		REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD) ) != ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

BOOL	RegKey::SetValue( LPCTSTR lpValueName, LPCTSTR lpValue ) {
	assert( lpValueName != NULL && lpValue != NULL );
	if ( ::RegSetValueEx( m_hKey, lpValueName, 0,
		REG_SZ, (LPBYTE)lpValue, ::lstrlen(lpValue)+1 ) != ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

BOOL	RegKey::SetValue( LPCTSTR lpValueName, LPCVOID lpBuffer, DWORD dwSize ) {
	assert( lpValueName != NULL && lpBuffer != NULL && dwSize > 0 );
	if ( ::RegSetValueEx( m_hKey, lpValueName, 0,
		REG_BINARY, (LPBYTE)lpBuffer, dwSize ) != ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

BOOL	RegKey::GetValue( LPCTSTR lpValueName, LPDWORD pdwBuffer ) {
	assert( lpValueName != NULL && pdwBuffer != NULL );
	DWORD	dwSize = sizeof(DWORD);
	if ( ::RegQueryValueEx( m_hKey, lpValueName, 0,
		NULL, (LPBYTE)pdwBuffer, &dwSize ) != ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

BOOL	RegKey::GetValue( LPCTSTR lpValueName, LPTSTR lpBuffer, DWORD dwBufSize ) {
	assert( lpValueName != NULL && lpBuffer != NULL );
	if ( ::RegQueryValueEx( m_hKey, lpValueName, 0,
		NULL, (LPBYTE)lpBuffer, &dwBufSize ) != ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

BOOL	RegKey::GetValueSize( LPCTSTR lpValueName, LPDWORD pdwSize ) {
	assert( lpValueName != NULL && pdwSize != NULL );

	if ( ::RegQueryValueEx( m_hKey, lpValueName,
			NULL, NULL, NULL, pdwSize ) != ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

BOOL	RegKey::GetValue( LPCTSTR lpValueName, LPVOID lpBuffer, DWORD dwSize ) {
	assert( lpValueName != NULL && lpBuffer != NULL );

	if ( ::RegQueryValueEx( m_hKey, lpValueName, NULL, NULL,
			(LPBYTE)lpBuffer, &dwSize ) != ERROR_SUCCESS )
		return	FALSE;
	return	TRUE;
}

template<class T>
BOOL	RegKey::SetValueT( LPCTSTR iName, const T* iT ) {
	assert(iName != NULL);
	assert(iT != NULL);
	return SetValue(iName, iT, sizeof(T) );
}

template<class T>
BOOL	RegKey::GetValueT( LPCTSTR iName, T* oT ) {
	assert(iName != NULL);
	assert(oT != NULL);
	return GetValue(iName, oT, sizeof(T) );
}

BOOL	RegKey::DeleteValue( LPCTSTR lpValueName ) {
	assert( lpValueName != NULL );
	return	( ::RegDeleteValue( m_hKey, lpValueName ) == ERROR_SUCCESS );
}


