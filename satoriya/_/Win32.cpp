#include	"Win32.h"
#include	<windows.h>
#include	<stdio.h>
#include	<zmouse.h>	// for MouseWheel
#include	<stdarg.h>	// for va_arg

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

bool	SetClipBoard(HWND iWnd, const wchar_t* iFormat, ...) {
	assert(iFormat != NULL);

	wchar_t	theString[1024];
	va_list	theArgPtr;
	va_start(theArgPtr, iFormat);
	_vsnwprintf(theString, 1023, iFormat, theArgPtr);
	va_end(theArgPtr);
	theString[1023] = L'\0';	// 入りきらないときは終端が付かない

	int	len = (wcslen(theString)+1) * sizeof(wchar_t);
	HGLOBAL	hGlobal = ::GlobalAlloc(GHND, len);
	if ( hGlobal == NULL )
		return	false;
	LPVOID	pMemory = ::GlobalLock(hGlobal);
	if ( pMemory == NULL )
		return	false;
	::CopyMemory(pMemory, theString, len);
	::GlobalUnlock(hGlobal);
	
	::OpenClipboard(NULL);
	::EmptyClipboard();
	::SetClipboardData(CF_UNICODETEXT, hGlobal);
	::CloseClipboard();
	return	true;
}




const UINT msgMOUSEWHEEL =
	(((GetVersion() & 0x80000000) && LOBYTE(LOWORD(GetVersion()) == 4)) ||
	(!(GetVersion() & 0x80000000) && LOBYTE(LOWORD(GetVersion()) == 3)))
	? RegisterWindowMessage(MSH_MOUSEWHEEL) : 0;

RECT NormalizeRect( RECT rc ) {
	if ( rc.left > rc.right ) {
		int temp = rc.left;
		rc.left = rc.right-1;
		rc.right = temp+1;
	}
	if ( rc.top > rc.bottom ) {
		int temp = rc.top;
		rc.top = rc.bottom-1;
		rc.bottom = temp+1;
	}
	return	rc;
}

RECT PointToRect( POINT pt1, POINT pt2 ) {
	if ( pt1.x > pt2.x )
		Swap( &(pt1.x), &(pt2.x) );
	if ( pt1.y > pt2.y )
		Swap( &(pt1.y), &(pt2.y) );
	return	MAKERECT( pt1.x, pt1.y, pt2.x+1, pt2.y+1 );
}

BOOL	StartCapture( HWND hWnd, const RECT* pRect ) {
	if ( ::GetCapture() == hWnd )
		return	FALSE;
	// 矩形を取得
	RECT rc = ( pRect == NULL ) ? 
		GetClientRectOnScreen( hWnd ) :
		*pRect;
	::SetCapture(hWnd);
	::ClipCursor(&rc);	// マウスの移動範囲を制限
	return	TRUE;
}

BOOL	isCapture( HWND hWnd ) {
	return ( ::GetCapture() == hWnd );
}

BOOL	EndCapture( HWND hWnd ) {
	if ( ::GetCapture() != hWnd )
		return	FALSE;
	::ClipCursor(NULL);	// マウスの移動制限を解除
	::ReleaseCapture();
	return	TRUE;
}

void	EndCapture() {
	::ClipCursor(NULL);	// マウスの移動制限を解除
	::ReleaseCapture();
}

void
PutWindowMessage( UINT message, WPARAM wParam, LPARAM lParam )
{
	TCHAR	buf[1024];
#ifndef	_DEBUG
	swprintf( buf, L"WM_0x%x\t\t\t( %x, %x )\n", message, wParam, lParam );
	::OutputDebugString(buf);
#else
	switch ( message ) {

	case 0x0019: lstrcpy( buf, L"WM_CTLCOLOR" );  break;
	case 0x0132: lstrcpy( buf, L"WM_CTLCOLORMSGBOX" );  break;
	case 0x0133: lstrcpy( buf, L"WM_CTLCOLOREDIT" );  break;
	case 0x0134: lstrcpy( buf, L"WM_CTLCOLORLISTBOX" );  break;
	case 0x0135: lstrcpy( buf, L"WM_CTLCOLORBTN" );  break;
	case 0x0136: lstrcpy( buf, L"WM_CTLCOLORDLG" );  break;
	case 0x0137: lstrcpy( buf, L"WM_CTLCOLORSCROLLBAR" );  break;
	case 0x0138: lstrcpy( buf, L"WM_CTLCOLORSTATIC" );  break;

	case 0x0000: lstrcpy( buf, L"WM_NULL" );  break;
	case 0x0001: lstrcpy( buf, L"WM_CREATE" );  break;
	case 0x0002: lstrcpy( buf, L"WM_DESTROY" );  break;
	case 0x0003: lstrcpy( buf, L"WM_MOVE" );  break;
	case 0x0005: lstrcpy( buf, L"WM_SIZE" );  break;
	case 0x0006: lstrcpy( buf, L"WM_ACTIVATE" );  break;
	case 0x0007: lstrcpy( buf, L"WM_SETFOCUS" );  break;
	case 0x0008: lstrcpy( buf, L"WM_KILLFOCUS" );  break;
	case 0x000A: lstrcpy( buf, L"WM_ENABLE" );  break;
	case 0x000B: lstrcpy( buf, L"WM_SETREDRAW" );  break;
	case 0x000C: lstrcpy( buf, L"WM_SETTEXT" );  break;
	case 0x000D: lstrcpy( buf, L"WM_GETTEXT" );  break;
	case 0x000E: lstrcpy( buf, L"WM_GETTEXTLENGTH" );  break;
	case 0x000F: lstrcpy( buf, L"WM_PAINT" );  break;
	case 0x0010: lstrcpy( buf, L"WM_CLOSE" );  break;
	case 0x0011: lstrcpy( buf, L"WM_QUERYENDSESSION" );  break;
	case 0x0012: lstrcpy( buf, L"WM_QUIT" );  break;
	case 0x0013: lstrcpy( buf, L"WM_QUERYOPEN" );  break;
	case 0x0014: lstrcpy( buf, L"WM_ERASEBKGND" );  break;
	case 0x0015: lstrcpy( buf, L"WM_SYSCOLORCHANGE" );  break;
	case 0x0016: lstrcpy( buf, L"WM_ENDSESSION" );  break;
	case 0x0018: lstrcpy( buf, L"WM_SHOWWINDOW" );  break;
	case 0x001A: lstrcpy( buf, L"WM_WININICHANGE" );  break;
	case 0x001B: lstrcpy( buf, L"WM_DEVMODECHANGE" );  break;
	case 0x001C: lstrcpy( buf, L"WM_ACTIVATEAPP" );  break;
	case 0x001D: lstrcpy( buf, L"WM_FONTCHANGE" );  break;
	case 0x001E: lstrcpy( buf, L"WM_TIMECHANGE" );  break;
	case 0x001F: lstrcpy( buf, L"WM_CANCELMODE" );  break;
	case 0x0020: lstrcpy( buf, L"WM_SETCURSOR" );  break;
	case 0x0021: lstrcpy( buf, L"WM_MOUSEACTIVATE" );  break;
	case 0x0022: lstrcpy( buf, L"WM_CHILDACTIVATE" );  break;
	case 0x0023: lstrcpy( buf, L"WM_QUEUESYNC" );  break;
	case 0x0024: lstrcpy( buf, L"WM_GETMINMAXINFO" );  break;
	case 0x0026: lstrcpy( buf, L"WM_PAINTICON" );  break;
	case 0x0027: lstrcpy( buf, L"WM_ICONERASEBKGND" );  break;
	case 0x0028: lstrcpy( buf, L"WM_NEXTDLGCTL" );  break;
	case 0x002A: lstrcpy( buf, L"WM_SPOOLERSTATUS" );  break;
	case 0x002B: lstrcpy( buf, L"WM_DRAWITEM" );  break;
	case 0x002C: lstrcpy( buf, L"WM_MEASUREITEM" );  break;
	case 0x002D: lstrcpy( buf, L"WM_DELETEITEM" );  break;
	case 0x002E: lstrcpy( buf, L"WM_VKEYTOITEM" );  break;
	case 0x002F: lstrcpy( buf, L"WM_CHARTOITEM" );  break;
	case 0x0030: lstrcpy( buf, L"WM_SETFONT" );  break;
	case 0x0031: lstrcpy( buf, L"WM_GETFONT" );  break;
	case 0x0032: lstrcpy( buf, L"WM_SETHOTKEY" );  break;
	case 0x0033: lstrcpy( buf, L"WM_GETHOTKEY" );  break;
	case 0x0037: lstrcpy( buf, L"WM_QUERYDRAGICON" );  break;
	case 0x0039: lstrcpy( buf, L"WM_COMPAREITEM" );  break;
	case 0x003D: lstrcpy( buf, L"WM_GETOBJECT" );  break;
	case 0x0041: lstrcpy( buf, L"WM_COMPACTING" );  break;
	case 0x0044: lstrcpy( buf, L"WM_COMMNOTIFY" );  break;
	case 0x0046: lstrcpy( buf, L"WM_WINDOWPOSCHANGING" );  break;
	case 0x0047: lstrcpy( buf, L"WM_WINDOWPOSCHANGED" );  break;
	case 0x0048: lstrcpy( buf, L"WM_POWER" );  break;
	case 0x004A: lstrcpy( buf, L"WM_COPYDATA" );  break;
	case 0x004B: lstrcpy( buf, L"WM_CANCELJOURNAL" );  break;
	case 0x004E: lstrcpy( buf, L"WM_NOTIFY" );  break;
	case 0x0050: lstrcpy( buf, L"WM_INPUTLANGCHANGEREQUEST" );  break;
	case 0x0051: lstrcpy( buf, L"WM_INPUTLANGCHANGE" );  break;
	case 0x0052: lstrcpy( buf, L"WM_TCARD" );  break;
	case 0x0053: lstrcpy( buf, L"WM_HELP" );  break;
	case 0x0054: lstrcpy( buf, L"WM_USERCHANGED" );  break;
	case 0x0055: lstrcpy( buf, L"WM_NOTIFYFORMAT" );  break;
	case 0x007B: lstrcpy( buf, L"WM_CONTEXTMENU" );  break;
	case 0x007C: lstrcpy( buf, L"WM_STYLECHANGING" );  break;
	case 0x007D: lstrcpy( buf, L"WM_STYLECHANGED" );  break;
	case 0x007E: lstrcpy( buf, L"WM_DISPLAYCHANGE" );  break;
	case 0x007F: lstrcpy( buf, L"WM_GETICON" );  break;
	case 0x0080: lstrcpy( buf, L"WM_SETICON" );  break;
	case 0x0081: lstrcpy( buf, L"WM_NCCREATE" );  break;
	case 0x0082: lstrcpy( buf, L"WM_NCDESTROY" );  break;
	case 0x0083: lstrcpy( buf, L"WM_NCCALCSIZE" );  break;
	case 0x0084: lstrcpy( buf, L"WM_NCHITTEST" );  break;
	case 0x0085: lstrcpy( buf, L"WM_NCPAINT" );  break;
	case 0x0086: lstrcpy( buf, L"WM_NCACTIVATE" );  break;
	case 0x0087: lstrcpy( buf, L"WM_GETDLGCODE" );  break;
	case 0x0088: lstrcpy( buf, L"WM_SYNCPAINT" );  break;
	case 0x00A0: lstrcpy( buf, L"WM_NCMOUSEMOVE" );  break;
	case 0x00A1: lstrcpy( buf, L"WM_NCLBUTTONDOWN" );  break;
	case 0x00A2: lstrcpy( buf, L"WM_NCLBUTTONUP" );  break;
	case 0x00A3: lstrcpy( buf, L"WM_NCLBUTTONDBLCLK" );  break;
	case 0x00A4: lstrcpy( buf, L"WM_NCRBUTTONDOWN" );  break;
	case 0x00A5: lstrcpy( buf, L"WM_NCRBUTTONUP" );  break;
	case 0x00A6: lstrcpy( buf, L"WM_NCRBUTTONDBLCLK" );  break;
	case 0x00A7: lstrcpy( buf, L"WM_NCMBUTTONDOWN" );  break;
	case 0x00A8: lstrcpy( buf, L"WM_NCMBUTTONUP" );  break;
	case 0x00A9: lstrcpy( buf, L"WM_NCMBUTTONDBLCLK" );  break;
	//case 0x0100: lstrcpy( buf, "WM_KEYFIRST" );  break;
	case 0x0100: lstrcpy( buf, L"WM_KEYDOWN" );  break;
	case 0x0101: lstrcpy( buf, L"WM_KEYUP" );  break;
	case 0x0102: lstrcpy( buf, L"WM_CHAR" );  break;
	case 0x0103: lstrcpy( buf, L"WM_DEADCHAR" );  break;
	case 0x0104: lstrcpy( buf, L"WM_SYSKEYDOWN" );  break;
	case 0x0105: lstrcpy( buf, L"WM_SYSKEYUP" );  break;
	case 0x0106: lstrcpy( buf, L"WM_SYSCHAR" );  break;
	case 0x0107: lstrcpy( buf, L"WM_SYSDEADCHAR" );  break;
	case 0x0108: lstrcpy( buf, L"WM_KEYLAST" );  break;
	case 0x010D: lstrcpy( buf, L"WM_IME_STARTCOMPOSITION" );  break;
	case 0x010E: lstrcpy( buf, L"WM_IME_ENDCOMPOSITION" );  break;
	//case 0x010F: lstrcpy( buf, "WM_IME_COMPOSITION" );  break;
	case 0x010F: lstrcpy( buf, L"WM_IME_KEYLAST" );  break;
	case 0x0110: lstrcpy( buf, L"WM_INITDIALOG" );  break;
	case 0x0111: lstrcpy( buf, L"WM_COMMAND" );  break;
	case 0x0112: lstrcpy( buf, L"WM_SYSCOMMAND" );  break;
	case 0x0113: lstrcpy( buf, L"WM_TIMER" );  break;
	case 0x0114: lstrcpy( buf, L"WM_HSCROLL" );  break;
	case 0x0115: lstrcpy( buf, L"WM_VSCROLL" );  break;
	case 0x0116: lstrcpy( buf, L"WM_INITMENU" );  break;
	case 0x0117: lstrcpy( buf, L"WM_INITMENUPOPUP" );  break;
	case 0x011F: lstrcpy( buf, L"WM_MENUSELECT" );  break;
	case 0x0120: lstrcpy( buf, L"WM_MENUCHAR" );  break;
	case 0x0121: lstrcpy( buf, L"WM_ENTERIDLE" );  break;
	case 0x0122: lstrcpy( buf, L"WM_MENURBUTTONUP" );  break;
	case 0x0123: lstrcpy( buf, L"WM_MENUDRAG" );  break;
	case 0x0124: lstrcpy( buf, L"WM_MENUGETOBJECT" );  break;
	case 0x0125: lstrcpy( buf, L"WM_UNINITMENUPOPUP" );  break;
	case 0x0126: lstrcpy( buf, L"WM_MENUCOMMAND" );  break;
	//case 0x0132: lstrcpy( buf, "WM_CTLCOLORMSGBOX" );  break;
	//case 0x0133: lstrcpy( buf, "WM_CTLCOLOREDIT" );  break;
	//case 0x0134: lstrcpy( buf, "WM_CTLCOLORLISTBOX" );  break;
	//case 0x0135: lstrcpy( buf, "WM_CTLCOLORBTN" );  break;
	//case 0x0136: lstrcpy( buf, "WM_CTLCOLORDLG" );  break;
	//case 0x0137: lstrcpy( buf, "WM_CTLCOLORSCROLLBAR" );  break;
	//case 0x0138: lstrcpy( buf, "WM_CTLCOLORSTATIC" );  break;
	//case 0x0200: lstrcpy( buf, "WM_MOUSEFIRST" );  break;
	case 0x0200: lstrcpy( buf, L"WM_MOUSEMOVE" );  break;
	case 0x0201: lstrcpy( buf, L"WM_LBUTTONDOWN" );  break;
	case 0x0202: lstrcpy( buf, L"WM_LBUTTONUP" );  break;
	case 0x0203: lstrcpy( buf, L"WM_LBUTTONDBLCLK" );  break;
	case 0x0204: lstrcpy( buf, L"WM_RBUTTONDOWN" );  break;
	case 0x0205: lstrcpy( buf, L"WM_RBUTTONUP" );  break;
	case 0x0206: lstrcpy( buf, L"WM_RBUTTONDBLCLK" );  break;
	case 0x0207: lstrcpy( buf, L"WM_MBUTTONDOWN" );  break;
	case 0x0208: lstrcpy( buf, L"WM_MBUTTONUP" );  break;
	case 0x0209: lstrcpy( buf, L"WM_MBUTTONDBLCLK" );  break;
	case 0x020A: lstrcpy( buf, L"WM_MOUSEWHEEL" );  break;
	//case 0x020A: lstrcpy( buf, "WM_MOUSELAST" );  break;
	//case 0x0209: lstrcpy( buf, "WM_MOUSELAST" );  break;
	case 0x0210: lstrcpy( buf, L"WM_PARENTNOTIFY" );  break;
	case 0x0211: lstrcpy( buf, L"WM_ENTERMENULOOP" );  break;
	case 0x0212: lstrcpy( buf, L"WM_EXITMENULOOP" );  break;
	case 0x0213: lstrcpy( buf, L"WM_NEXTMENU" );  break;
	case 0x0214: lstrcpy( buf, L"WM_SIZING" );  break;
	case 0x0215: lstrcpy( buf, L"WM_CAPTURECHANGED" );  break;
	case 0x0216: lstrcpy( buf, L"WM_MOVING" );  break;
	case 0x0218: lstrcpy( buf, L"WM_POWERBROADCAST" );  break;	// r_winuser pbt
	case 0x0219: lstrcpy( buf, L"WM_DEVICECHANGE" );  break;
	case 0x0220: lstrcpy( buf, L"WM_MDICREATE" );  break;
	case 0x0221: lstrcpy( buf, L"WM_MDIDESTROY" );  break;
	case 0x0222: lstrcpy( buf, L"WM_MDIACTIVATE" );  break;
	case 0x0223: lstrcpy( buf, L"WM_MDIRESTORE" );  break;
	case 0x0224: lstrcpy( buf, L"WM_MDINEXT" );  break;
	case 0x0225: lstrcpy( buf, L"WM_MDIMAXIMIZE" );  break;
	case 0x0226: lstrcpy( buf, L"WM_MDITILE" );  break;
	case 0x0227: lstrcpy( buf, L"WM_MDICASCADE" );  break;
	case 0x0228: lstrcpy( buf, L"WM_MDIICONARRANGE" );  break;
	case 0x0229: lstrcpy( buf, L"WM_MDIGETACTIVE" );  break;
	case 0x0230: lstrcpy( buf, L"WM_MDISETMENU" );  break;
	case 0x0231: lstrcpy( buf, L"WM_ENTERSIZEMOVE" );  break;
	case 0x0232: lstrcpy( buf, L"WM_EXITSIZEMOVE" );  break;
	case 0x0233: lstrcpy( buf, L"WM_DROPFILES" );  break;
	case 0x0234: lstrcpy( buf, L"WM_MDIREFRESHMENU" );  break;
	case 0x0281: lstrcpy( buf, L"WM_IME_SETCONTEXT" );  break;
	case 0x0282: lstrcpy( buf, L"WM_IME_NOTIFY" );  break;
	case 0x0283: lstrcpy( buf, L"WM_IME_CONTROL" );  break;
	case 0x0284: lstrcpy( buf, L"WM_IME_COMPOSITIONFULL" );  break;
	case 0x0285: lstrcpy( buf, L"WM_IME_SELECT" );  break;
	case 0x0286: lstrcpy( buf, L"WM_IME_CHAR" );  break;
	case 0x0288: lstrcpy( buf, L"WM_IME_REQUEST" );  break;
	case 0x0290: lstrcpy( buf, L"WM_IME_KEYDOWN" );  break;
	case 0x0291: lstrcpy( buf, L"WM_IME_KEYUP" );  break;
	case 0x02A1: lstrcpy( buf, L"WM_MOUSEHOVER" );  break;
	case 0x02A3: lstrcpy( buf, L"WM_MOUSELEAVE" );  break;
	case 0x0300: lstrcpy( buf, L"WM_CUT" );  break;
	case 0x0301: lstrcpy( buf, L"WM_COPY" );  break;
	case 0x0302: lstrcpy( buf, L"WM_PASTE" );  break;
	case 0x0303: lstrcpy( buf, L"WM_CLEAR" );  break;
	case 0x0304: lstrcpy( buf, L"WM_UNDO" );  break;
	case 0x0305: lstrcpy( buf, L"WM_RENDERFORMAT" );  break;
	case 0x0306: lstrcpy( buf, L"WM_RENDERALLFORMATS" );  break;
	case 0x0307: lstrcpy( buf, L"WM_DESTROYCLIPBOARD" );  break;
	case 0x0308: lstrcpy( buf, L"WM_DRAWCLIPBOARD" );  break;
	case 0x0309: lstrcpy( buf, L"WM_PAINTCLIPBOARD" );  break;
	case 0x030A: lstrcpy( buf, L"WM_VSCROLLCLIPBOARD" );  break;
	case 0x030B: lstrcpy( buf, L"WM_SIZECLIPBOARD" );  break;
	case 0x030C: lstrcpy( buf, L"WM_ASKCBFORMATNAME" );  break;
	case 0x030D: lstrcpy( buf, L"WM_CHANGECBCHAIN" );  break;
	case 0x030E: lstrcpy( buf, L"WM_HSCROLLCLIPBOARD" );  break;
	case 0x030F: lstrcpy( buf, L"WM_QUERYNEWPALETTE" );  break;
	case 0x0310: lstrcpy( buf, L"WM_PALETTEISCHANGING" );  break;
	case 0x0311: lstrcpy( buf, L"WM_PALETTECHANGED" );  break;
	case 0x0312: lstrcpy( buf, L"WM_HOTKEY" );  break;
	case 0x0317: lstrcpy( buf, L"WM_PRINT" );  break;
	case 0x0318: lstrcpy( buf, L"WM_PRINTCLIENT" );  break;
	case 0x0358: lstrcpy( buf, L"WM_HANDHELDFIRST" );  break;
	case 0x035F: lstrcpy( buf, L"WM_HANDHELDLAST" );  break;
	case 0x0360: lstrcpy( buf, L"WM_AFXFIRST" );  break;
	case 0x037F: lstrcpy( buf, L"WM_AFXLAST" );  break;
	case 0x0380: lstrcpy( buf, L"WM_PENWINFIRST" );  break;
	case 0x038F: lstrcpy( buf, L"WM_PENWINLAST" );  break;
	case 0x8000: lstrcpy( buf, L"WM_APP" );  break;
	case 0x0400: lstrcpy( buf, L"WM_USER" );  break;

	//case 0x0001: lstrcpy( buf, "MWMO_WAITALL" );  break;
	//case 0x0002: lstrcpy( buf, "MWMO_ALERTABLE" );  break;
	case 0x0004: lstrcpy( buf, L"MWMO_INPUTAVAILABLE" );  break;

	//case 0x000c: lstrcpy( buf, "WM_HELP" );  break;

	default: swprintf( buf, L"WM_(0x%x)", message );  break;
	}
	if ( message >= WM_USER && message <= 0x7FFF )
		swprintf( buf, L"WM_USER+%d", message-WM_USER );

	LPSTR	ptr = buf+lstrlen(buf);
	if ( message == WM_MOUSEWHEEL ) {
		// ホイール
		sprintf( ptr, L"\t\t\t( fwKeys:%x, zDelta:%d, x:%d, y:%d )\n", LOWORD(wParam), (short) HIWORD(wParam), LOWORD(lParam), HIWORD(lParam) );
	}
	else if ( message >= WM_MOUSEMOVE && message <= WM_MOUSEWHEEL ) {
		// マウス関連メッセージ
		sprintf( ptr, L"\t\t\t( fwKeys:%x, x:%d, y:%d )\n", wParam, LOWORD(lParam), HIWORD(lParam) );
	}
	else if ( message == WM_ACTIVATE ) {
		if ( LOWORD(wParam) == WA_ACTIVE )
			ptr += sprintf( ptr, L"\t\t\t( WA_ACTIVE, ");
		else if ( LOWORD(wParam) == WA_CLICKACTIVE )
			ptr += sprintf( ptr, L"\t\t\t( WA_CLICKACTIVE, ");
		else if ( LOWORD(wParam) == WA_INACTIVE )
			ptr += sprintf( ptr, L"\t\t\t( WA_INACTIVE, ");
		ptr += sprintf( ptr, L"%s, ", ((BOOL)HIWORD(wParam)) ? L"true" : L"false"  );
		if ( (HWND)lParam == NULL )
			ptr += sprintf( ptr, L"NULL )\n" );
		else {
			ptr += sprintf( ptr, L"\"" );
			ptr += ::GetWindowText( (HWND)lParam, ptr, 256 );
			ptr += sprintf( ptr, L"\" )\n" );
		}
	}
	else if ( message == WM_NCACTIVATE )
		ptr += sprintf( ptr, L"\t\t\t( %s )\n", ((BOOL)wParam) ? L"true" : L"false"  );
	else if ( message == WM_MOVE )
		ptr += sprintf( ptr, L"\t\t\t( x:%d, y:%d )\n", LOWORD(lParam), HIWORD(lParam) );
	else if ( message == WM_SIZE ) {
		ptr += sprintf( ptr, L"\t\t\t( " );
		if ( wParam == SIZE_MAXIMIZED )
			ptr += sprintf( ptr, L"SIZE_MAXIMIZED" );
		else if ( wParam == SIZE_MINIMIZED )
			ptr += sprintf( ptr, L"SIZE_MINIMIZED" );
		else if ( wParam == SIZE_RESTORED )
			ptr += sprintf( ptr, L"SIZE_RESTORED" );
		else if ( wParam == SIZE_MAXHIDE )
			ptr += sprintf( ptr, L"SIZE_MAXHIDE" );
		else if ( wParam == SIZE_MAXSHOW )
			ptr += sprintf( ptr, L"SIZE_MAXSHOW" );
		ptr += sprintf( ptr, L", w:%d, h:%d )\n", LOWORD(lParam), HIWORD(lParam) );
	}
	else if ( message == WM_WINDOWPOSCHANGED || message == WM_WINDOWPOSCHANGING ) {
		// WINDOWPOS構造体を使用するメッセージ
		LPWINDOWPOS	p = (LPWINDOWPOS)lParam;
		ptr += sprintf( ptr, L"\t\t\t( x:%d, y:%d, cx:%d, cy:%d )\n",
			p->x, p->y, p->cx, p->cy );
		ptr += sprintf( ptr, L"flags : " );
		if ( p->flags & SWP_DRAWFRAME )
			ptr += sprintf( ptr, L"SWP_DRAWFRAME " );
		if ( p->flags & SWP_FRAMECHANGED )
			ptr += sprintf( ptr, L"SWP_HIDEWINDOW " );
		if ( p->flags & SWP_NOACTIVATE )
			ptr += sprintf( ptr, L"SWP_NOACTIVATE " );
		if ( p->flags & SWP_NOCOPYBITS )
			ptr += sprintf( ptr, L"SWP_NOCOPYBITS " );
		if ( p->flags & SWP_NOMOVE )
			ptr += sprintf( ptr, L"SWP_NOMOVE " );
		if ( p->flags & SWP_NOOWNERZORDER )
			ptr += sprintf( ptr, L"SWP_NOOWNERZORDER " );
		if ( p->flags & SWP_NOSIZE )
			ptr += sprintf( ptr, L"SWP_NOSIZE " );
		if ( p->flags & SWP_NOREDRAW )
			ptr += sprintf( ptr, L"SWP_NOREDRAW " );
		if ( p->flags & SWP_NOZORDER )
			ptr += sprintf( ptr, L"SWP_NOZORDER " );
		if ( p->flags & SWP_SHOWWINDOW )
			ptr += sprintf( ptr, L"SWP_SHOWWINDOW " );
		ptr += sprintf( ptr, L"\n" );
	}
	else if ( message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) {
		// 仮想キーコードの表示
		ptr += sprintf( ptr, L"\t\t\t( VK_" );
		int	vk = (int)wParam;
		switch ( vk ) {
		case 0x01: ptr += sprintf( ptr, L"LBUTTON" ); break;
		case 0x02: ptr += sprintf( ptr, L"RBUTTON" ); break;
		case 0x03: ptr += sprintf( ptr, L"CANCEL" ); break;
		case 0x04: ptr += sprintf( ptr, L"MBUTTON" ); break;    /* NOT contiguous with L & RBUTTON */
		case 0x08: ptr += sprintf( ptr, L"BACK" ); break;
		case 0x09: ptr += sprintf( ptr, L"TAB" ); break;
		case 0x0C: ptr += sprintf( ptr, L"CLEAR" ); break;
		case 0x0D: ptr += sprintf( ptr, L"RETURN" ); break;
		case 0x10: ptr += sprintf( ptr, L"SHIFT" ); break;
		case 0x11: ptr += sprintf( ptr, L"CONTROL" ); break;
		case 0x12: ptr += sprintf( ptr, L"MENU" ); break;
		case 0x13: ptr += sprintf( ptr, L"PAUSE" ); break;
		case 0x14: ptr += sprintf( ptr, L"CAPITAL" ); break;
		case 0x15: ptr += sprintf( ptr, L"KANA" ); break;
		//case 0x15: ptr += sprintf( ptr, "HANGEUL" ); break;  /* old name - should be here for compatibility */
		//case 0x15: ptr += sprintf( ptr, "HANGUL" ); break;
		case 0x17: ptr += sprintf( ptr, L"JUNJA" ); break;
		case 0x18: ptr += sprintf( ptr, L"FINAL" ); break;
		//case 0x19: ptr += sprintf( ptr, "HANJA" ); break;
		case 0x19: ptr += sprintf( ptr, L"KANJI" ); break;
		case 0x1B: ptr += sprintf( ptr, L"ESCAPE" ); break;
		case 0x1C: ptr += sprintf( ptr, L"CONVERT" ); break;
		case 0x1D: ptr += sprintf( ptr, L"NONCONVERT" ); break;
		case 0x1E: ptr += sprintf( ptr, L"ACCEPT" ); break;
		case 0x1F: ptr += sprintf( ptr, L"MODECHANGE" ); break;
		case 0x20: ptr += sprintf( ptr, L"SPACE" ); break;
		case 0x21: ptr += sprintf( ptr, L"PRIOR" ); break;
		case 0x22: ptr += sprintf( ptr, L"NEXT" ); break;
		case 0x23: ptr += sprintf( ptr, L"END" ); break;
		case 0x24: ptr += sprintf( ptr, L"HOME" ); break;
		case 0x25: ptr += sprintf( ptr, L"LEFT" ); break;
		case 0x26: ptr += sprintf( ptr, L"UP" ); break;
		case 0x27: ptr += sprintf( ptr, L"RIGHT" ); break;
		case 0x28: ptr += sprintf( ptr, L"DOWN" ); break;
		case 0x29: ptr += sprintf( ptr, L"SELECT" ); break;
		case 0x2A: ptr += sprintf( ptr, L"PRINT" ); break;
		case 0x2B: ptr += sprintf( ptr, L"EXECUTE" ); break;
		case 0x2C: ptr += sprintf( ptr, L"SNAPSHOT" ); break;
		case 0x2D: ptr += sprintf( ptr, L"INSERT" ); break;
		case 0x2E: ptr += sprintf( ptr, L"DELETE" ); break;
		case 0x2F: ptr += sprintf( ptr, L"HELP" ); break;
		/* 0 thru 9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
		/* A thru Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
		case 0x5B: ptr += sprintf( ptr, L"LWIN" ); break;
		case 0x5C: ptr += sprintf( ptr, L"RWIN" ); break;
		case 0x5D: ptr += sprintf( ptr, L"APPS" ); break;
		case 0x60: ptr += sprintf( ptr, L"NUMPAD0" ); break;
		case 0x61: ptr += sprintf( ptr, L"NUMPAD1" ); break;
		case 0x62: ptr += sprintf( ptr, L"NUMPAD2" ); break;
		case 0x63: ptr += sprintf( ptr, L"NUMPAD3" ); break;
		case 0x64: ptr += sprintf( ptr, L"NUMPAD4" ); break;
		case 0x65: ptr += sprintf( ptr, L"NUMPAD5" ); break;
		case 0x66: ptr += sprintf( ptr, L"NUMPAD6" ); break;
		case 0x67: ptr += sprintf( ptr, L"NUMPAD7" ); break;
		case 0x68: ptr += sprintf( ptr, L"NUMPAD8" ); break;
		case 0x69: ptr += sprintf( ptr, L"NUMPAD9" ); break;
		case 0x6A: ptr += sprintf( ptr, L"MULTIPLY" ); break;
		case 0x6B: ptr += sprintf( ptr, L"ADD" ); break;
		case 0x6C: ptr += sprintf( ptr, L"SEPARATOR" ); break;
		case 0x6D: ptr += sprintf( ptr, L"SUBTRACT" ); break;
		case 0x6E: ptr += sprintf( ptr, L"DECIMAL" ); break;
		case 0x6F: ptr += sprintf( ptr, L"DIVIDE" ); break;
		case 0x70: ptr += sprintf( ptr, L"F1" ); break;
		case 0x71: ptr += sprintf( ptr, L"F2" ); break;
		case 0x72: ptr += sprintf( ptr, L"F3" ); break;
		case 0x73: ptr += sprintf( ptr, L"F4" ); break;
		case 0x74: ptr += sprintf( ptr, L"F5" ); break;
		case 0x75: ptr += sprintf( ptr, L"F6" ); break;
		case 0x76: ptr += sprintf( ptr, L"F7" ); break;
		case 0x77: ptr += sprintf( ptr, L"F8" ); break;
		case 0x78: ptr += sprintf( ptr, L"F9" ); break;
		case 0x79: ptr += sprintf( ptr, L"F10" ); break;
		case 0x7A: ptr += sprintf( ptr, L"F11" ); break;
		case 0x7B: ptr += sprintf( ptr, L"F12" ); break;
		case 0x7C: ptr += sprintf( ptr, L"F13" ); break;
		case 0x7D: ptr += sprintf( ptr, L"F14" ); break;
		case 0x7E: ptr += sprintf( ptr, L"F15" ); break;
		case 0x7F: ptr += sprintf( ptr, L"F16" ); break;
		case 0x80: ptr += sprintf( ptr, L"F17" ); break;
		case 0x81: ptr += sprintf( ptr, L"F18" ); break;
		case 0x82: ptr += sprintf( ptr, L"F19" ); break;
		case 0x83: ptr += sprintf( ptr, L"F20" ); break;
		case 0x84: ptr += sprintf( ptr, L"F21" ); break;
		case 0x85: ptr += sprintf( ptr, L"F22" ); break;
		case 0x86: ptr += sprintf( ptr, L"F23" ); break;
		case 0x87: ptr += sprintf( ptr, L"F24" ); break;
		case 0x90: ptr += sprintf( ptr, L"NUMLOCK" ); break;
		case 0x91: ptr += sprintf( ptr, L"SCROLL" ); break;
		/*
		 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
		 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
		 * No other API or message will distinguish left and right keys in this way.
		 */
		case 0xA0: ptr += sprintf( ptr, L"LSHIFT" ); break;
		case 0xA1: ptr += sprintf( ptr, L"RSHIFT" ); break;
		case 0xA2: ptr += sprintf( ptr, L"LCONTROL" ); break;
		case 0xA3: ptr += sprintf( ptr, L"RCONTROL" ); break;
		case 0xA4: ptr += sprintf( ptr, L"LMENU" ); break;
		case 0xA5: ptr += sprintf( ptr, L"RMENU" ); break;
		#if(WINVER >= 0x0400)
		case 0xE5: ptr += sprintf( ptr, L"PROCESSKEY" ); break;
		#endif /* WINVER >= 0x0400 */
		case 0xF6: ptr += sprintf( ptr, L"ATTN" ); break;
		case 0xF7: ptr += sprintf( ptr, L"CRSEL" ); break;
		case 0xF8: ptr += sprintf( ptr, L"EXSEL" ); break;
		case 0xF9: ptr += sprintf( ptr, L"EREOF" ); break;
		case 0xFA: ptr += sprintf( ptr, L"PLAY" ); break;
		case 0xFB: ptr += sprintf( ptr, L"ZOOM" ); break;
		case 0xFC: ptr += sprintf( ptr, L"NONAME" ); break;
		case 0xFD: ptr += sprintf( ptr, L"PA1" ); break;
		case 0xFE: ptr += sprintf( ptr, L"OEM_CLEAR" ); break;
		default:
			if ( vk >= L'0' && vk <= L'9' )
				ptr += sprintf( ptr, L"%c", vk );
			else if ( vk >= L'A' && vk <= L'Z' )
				ptr += sprintf( ptr, L"%c", vk );
			else
				ptr += sprintf( ptr, L"(0x%x)", vk );
			break;
		}

		ptr += sprintf( ptr, L", %x )\n", lParam );
	}
	else {
		// それ以外の一般形
		sprintf( ptr, L"\t\t\t( %x, %x )\n", wParam, lParam );
	}
	
	::OutputDebugString(buf);
#endif	// _DEBUG
}


// クライアント領域のサイズを変更する。
// さらに、ウィンドウ位置が画面内に収まるよう調整する。
BOOL	ResizeClientRect( HWND hWnd, int w, int h ) {

	int		xPos, yPos, width, height;
	RECT	rectWindow, rectDesktop, rectClient;

	// デスクトップ、ウィンドウ全体、クライアント領域、
	// それぞれの矩形を取得
	GetWindowRect( hWnd, &rectWindow );
	GetClientRect( hWnd, &rectClient );
	GetWindowRect( GetDesktopWindow(), &rectDesktop );

	// 幅と高さを計算
	width = w + 
				(rectClient.left - rectWindow.left) +
				(rectWindow.right - rectClient.right);
	height = h + 
				(rectClient.top - rectWindow.top) +
				(rectWindow.bottom - rectClient.bottom);

	xPos = rectWindow.left;
	if ( xPos + width > rectDesktop.right )
		xPos = rectDesktop.right - width;
	yPos = rectWindow.top;
	if ( yPos + height > rectDesktop.bottom )
		yPos = rectDesktop.bottom - height;

	if ( xPos < 0 )
		xPos = 0;
	if ( yPos < 0 )
		yPos = 0;

	return	MoveWindow( hWnd, xPos,	yPos, width, height, TRUE );
}

// iBaseを原点として、右回りに90度回転
POINT	RotateRight90(POINT iBase, POINT iPoint) {
	iPoint -= iBase;
	Swap(&iPoint.x, &iPoint.y);
	ReverseSign(iPoint.y);
	iPoint += iBase;
	return	iPoint;
}

// iBaseを原点として、左回りに90度回転
POINT	RotateLeft90(POINT iBase, POINT iPoint) {
	iPoint -= iBase;
	Swap(&iPoint.x, &iPoint.y);
	ReverseSign(iPoint.x);
	iPoint += iBase;
	return	iPoint;
}

// iBaseを原点として、180度回転
POINT	Rotate180(POINT iBase, POINT iPoint) {
	iPoint.x = Reverse(iBase.x, iPoint.x);
	iPoint.y = Reverse(iBase.y, iPoint.y);
	return	iPoint;
}


// クライアント長方形をスクリーン座標で返す。
RECT	GetClientRectOnScreen( HWND hWnd ) {
	RECT	rect;
	::GetClientRect( hWnd, &rect );
	::ClientToScreen( hWnd, (LPPOINT)&rect );
	::ClientToScreen( hWnd, ((LPPOINT)&rect)+1 );
	return	rect;
}

RECT	ClientToScreen(HWND iWnd, RECT iRect) {
	::ClientToScreen(iWnd, (LPPOINT)&iRect );
	::ClientToScreen(iWnd, ((LPPOINT)&iRect)+1 );
	return	iRect;
}

RECT	ScreenToClient(HWND iWnd, RECT iRect) {
	::ScreenToClient(iWnd, (LPPOINT)&iRect );
	::ScreenToClient(iWnd, ((LPPOINT)&iRect)+1 );
	return	iRect;
}

// カーソル位置を設定・取得
void	SetCursorPos( POINT pt ) {
	::SetCursorPos( pt.x, pt.y );
}

POINT	GetCursorPos() {
	POINT	pt;
	::GetCursorPos(&pt);
	return	pt;
}

// クライアント座標系でカーソル位置を設定・取得
void	SetCursorPos( HWND hWnd, POINT pt ) {
	::ClientToScreen( hWnd, &pt );
	::SetCursorPos( pt.x, pt.y );
}

POINT	GetCursorPos( HWND hWnd ) {
	POINT	pt;
	::GetCursorPos(&pt);
	::ScreenToClient( hWnd, (LPPOINT)&pt );
	return	pt;
}

// カーソルはクライアント領域内か？
BOOL	isCursorOnClient(HWND hWnd) {
	POINT	pt = GetCursorPos(hWnd);
	RECT	rect = GetClientRect(hWnd);
	return	isOn(rect, pt.x, pt.y);
}

// ウィンドウ位置座標を取得
RECT	GetWindowRect( HWND hWnd ) {
	RECT	rc = {0,0,0,0};
	::GetWindowRect( hWnd, &rc );
	return	rc;
}
POINT	GetWindowPoint( HWND hWnd ) {
	RECT	rc = {0,0,0,0};
	::GetWindowRect( hWnd, &rc );
	return	MAKEPOINT(rc.left,rc.top);
}

// DWORD値を時分秒ミリ秒に分解
void	DwordToSystemTime( DWORD dw, SYSTEMTIME* pst ) {
	assert( pst != NULL );
	::ZeroMemory( pst, sizeof(SYSTEMTIME) );

	pst->wMilliseconds = WORD( dw%1000 );
	dw /= 1000;
	pst->wSecond = WORD( dw%60 );
	dw /= 60;
	pst->wMinute = WORD( dw%60 );
	dw /= 60;
	pst->wHour = WORD(dw);
}

// 可変引数対応TextOut
void	TextOutF( HDC hDC, int xPos, int yPos, wchar_t* format, ... ) {
/*	char	buf[256];
	va_list	argptr;
	va_start( argptr, format );
	_vsnwprintf( buf, 255, format, argptr );
	va_end( argptr );
	::TextOut( hDC, xPos, yPos, buf, ::lstrlen(buf) );*/
}

int	CompareSystemTime(  const SYSTEMTIME& lhs, const SYSTEMTIME& rhs ) {
	// 最終更新日付を比較
	if ( lhs.wYear > rhs.wYear )	return	1;
	else if ( lhs.wYear < rhs.wYear )	return	-1;
	if ( lhs.wMonth > rhs.wMonth )	return	1;
	else if ( lhs.wMonth < rhs.wMonth )	return	-1;
	if ( lhs.wDay > rhs.wDay )	return	1;
	else if ( lhs.wDay < rhs.wDay )	return	-1;
	if ( lhs.wHour > rhs.wHour )	return	1;
	else if ( lhs.wHour < rhs.wHour )	return	-1;
	if ( lhs.wMinute > rhs.wMinute )	return	1;
	else if ( lhs.wMinute < rhs.wMinute )	return	-1;
	if ( lhs.wSecond > rhs.wSecond )	return	1;
	else if ( lhs.wSecond < rhs.wSecond )	return	-1;
	if ( lhs.wMilliseconds > rhs.wMilliseconds )	return	1;
	else if ( lhs.wMilliseconds < rhs.wMilliseconds )	return	-1;
	// 完全一致
	return	0;
}
bool operator < ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs ) {	return ( CompareSystemTime( lhs, rhs ) < 0 ); }
bool operator > ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs ) {	return ( CompareSystemTime( lhs, rhs ) > 0 ); }
bool operator <= ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs ) {	return ( CompareSystemTime( lhs, rhs ) <= 0 ); }
bool operator >= ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs ) {	return ( CompareSystemTime( lhs, rhs ) >= 0 ); }
bool operator == ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs ) {	return ( CompareSystemTime( lhs, rhs ) == 0 ); }
bool operator != ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs ) {	return ( CompareSystemTime( lhs, rhs ) != 0 ); }




//------------------------------------------------------------

// 可変引数対応のOutputDebugString
void	DbgStr( const wchar_t* format, ... ) {
	wchar_t	buf[256];
	va_list	argptr;
	va_start( argptr, format );
	_vsnwprintf( buf, 255, format, argptr );
	va_end( argptr );
	buf[255] = L'\0';
	::OutputDebugString(buf);
}

// 可変引数対応の簡易報告用メッセージボックス
void	MesBox( const wchar_t* format, ... ) {
	wchar_t	buf[256];
	va_list	argptr;
	va_start( argptr, format );
	_vsnwprintf( buf, 255, format, argptr );
	va_end( argptr );
	buf[255] = L'\0';
	::MessageBox( NULL, buf, L"notice", MB_OK|MB_SYSTEMMODAL );
}

// 可変引数対応の簡易報告用SetWindowText
void	SetWinText( HWND hWnd, const wchar_t* format, ... ) {
	wchar_t	buf[256];
	va_list	argptr;
	va_start( argptr, format );
	_vsnwprintf( buf, 255, format, argptr );
	va_end( argptr );
	buf[255] = L'\0';
	::SetWindowText( hWnd, buf );
}
