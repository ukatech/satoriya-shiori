#include	"Dialog.h"
#ifdef	_DEBUG
#include	"Win32.h"
#endif

Dialog*	Dialog::sm_pDialog = NULL;

//---------------------------------------------------------------------
int	
Dialog::Run( HINSTANCE hInstance, WORD idDialog, HWND hwndOwner ) {
	m_hInstance = hInstance;
	m_hOwner = hwndOwner;
	m_fModeless = FALSE;
	return	::DialogBoxParam( 
		hInstance, 
		MAKEINTRESOURCE(idDialog),
		hwndOwner,
		DialogProcedure,
		(LPARAM)this );
}

//---------------------------------------------------------------------
HWND
Dialog::RunAsync( HINSTANCE hInstance, WORD idDialog, HWND hwndOwner ) {
	m_hInstance = hInstance;
	m_hOwner = hwndOwner;
	m_fModeless = TRUE;
	m_hDlg = ::CreateDialogParam( 
		hInstance, 
		MAKEINTRESOURCE(idDialog),
		hwndOwner,
		DialogProcedure,
		(LPARAM)this );
	return	m_hDlg;
}

//---------------------------------------------------------------------
void	
Dialog::SetText( int id, const char* format, ... ) {
	char	buf[65536];
	va_list	argptr;
	va_start( argptr, format );
	_vsnprintf( buf, 65535, format, argptr );
	va_end( argptr );
	::SetDlgItemText( m_hDlg, id, buf );
}

//---------------------------------------------------------------------
BOOL	
Dialog::DialogProcedure( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam ) {

//#ifdef	_DEBUG
//	PutWindowMessage( message, wParam, lParam );
//#endif

	switch ( message ) {

	case WM_INITDIALOG:
		sm_pDialog = (Dialog*)lParam;
		sm_pDialog->m_hDlg = hDlg;
		return	sm_pDialog->OnInitDialog( (HWND)wParam, (LONG)lParam );
	case WM_SYSCOMMAND:
		return	sm_pDialog->OnSysCommand( wParam, LOWORD(lParam), HIWORD(lParam) );
	case WM_COMMAND:
		return	sm_pDialog->OnCommand( HIWORD(wParam), LOWORD(wParam), (HWND)lParam );
	case WM_HSCROLL:
		return	sm_pDialog->OnHScroll( (int)LOWORD(wParam), (int)HIWORD(wParam), (HWND)lParam );
	case WM_VSCROLL:
		return	sm_pDialog->OnVScroll( (int)LOWORD(wParam), (int)HIWORD(wParam), (HWND)lParam );
	case WM_KEYDOWN:
		return	sm_pDialog->OnKeyDown( (int)wParam, (long)lParam );
	case MM_JOY1BUTTONDOWN:
		return	sm_pDialog->OnJoy1ButtonDown( wParam, LOWORD(lParam), HIWORD(lParam) );
	case WM_COPYDATA:
		return	sm_pDialog->OnCopyData( (HWND)wParam, (PCOPYDATASTRUCT)lParam );
	case WM_CLOSE:
		return	sm_pDialog->OnClose();
	case WM_DESTROY:
		return	sm_pDialog->OnDestroy();

	default:
		return	FALSE;
	}
	return	TRUE;
}

BOOL	
Dialog::OnSysCommand( UINT uCmdType, WORD xPos, WORD yPos ) {
/*	if ( uCmdType == SC_SCREENSAVE )
		if ( ::GetParent(m_hDlg) != NULL ) {
			::SendMessage( ::GetParent(m_hDlg),
				WM_SYSCOMMAND, (WPARAM)uCmdType, MAKELPARAM(xPos,yPos) );
			return	TRUE;
		}
*/
	return FALSE;
}

