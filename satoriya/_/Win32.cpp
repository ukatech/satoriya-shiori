#include	"Win32.h"
#include	<windows.h>
#include	<stdio.h>
#include	<zmouse.h>	// for MouseWheel
#include	<stdarg.h>	// for va_arg

bool	SetClipBoard(HWND iWnd, const char* iFormat, ...) {
	assert(iFormat != NULL);

	char	theString[1024];
	va_list	theArgPtr;
	va_start(theArgPtr, iFormat);
	_vsnprintf(theString, 1023, iFormat, theArgPtr);
	va_end(theArgPtr);

	int	len = strlen(theString)+1;
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
	::SetClipboardData(CF_TEXT, hGlobal);
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
	sprintf( buf, "WM_0x%x\t\t\t( %x, %x )\n", message, wParam, lParam );
	::OutputDebugString(buf);
#else
	switch ( message ) {

	case 0x0019: lstrcpy( buf, "WM_CTLCOLOR" );  break;
	case 0x0132: lstrcpy( buf, "WM_CTLCOLORMSGBOX" );  break;
	case 0x0133: lstrcpy( buf, "WM_CTLCOLOREDIT" );  break;
	case 0x0134: lstrcpy( buf, "WM_CTLCOLORLISTBOX" );  break;
	case 0x0135: lstrcpy( buf, "WM_CTLCOLORBTN" );  break;
	case 0x0136: lstrcpy( buf, "WM_CTLCOLORDLG" );  break;
	case 0x0137: lstrcpy( buf, "WM_CTLCOLORSCROLLBAR" );  break;
	case 0x0138: lstrcpy( buf, "WM_CTLCOLORSTATIC" );  break;

	case 0x0000: lstrcpy( buf, "WM_NULL" );  break;
	case 0x0001: lstrcpy( buf, "WM_CREATE" );  break;
	case 0x0002: lstrcpy( buf, "WM_DESTROY" );  break;
	case 0x0003: lstrcpy( buf, "WM_MOVE" );  break;
	case 0x0005: lstrcpy( buf, "WM_SIZE" );  break;
	case 0x0006: lstrcpy( buf, "WM_ACTIVATE" );  break;
	case 0x0007: lstrcpy( buf, "WM_SETFOCUS" );  break;
	case 0x0008: lstrcpy( buf, "WM_KILLFOCUS" );  break;
	case 0x000A: lstrcpy( buf, "WM_ENABLE" );  break;
	case 0x000B: lstrcpy( buf, "WM_SETREDRAW" );  break;
	case 0x000C: lstrcpy( buf, "WM_SETTEXT" );  break;
	case 0x000D: lstrcpy( buf, "WM_GETTEXT" );  break;
	case 0x000E: lstrcpy( buf, "WM_GETTEXTLENGTH" );  break;
	case 0x000F: lstrcpy( buf, "WM_PAINT" );  break;
	case 0x0010: lstrcpy( buf, "WM_CLOSE" );  break;
	case 0x0011: lstrcpy( buf, "WM_QUERYENDSESSION" );  break;
	case 0x0012: lstrcpy( buf, "WM_QUIT" );  break;
	case 0x0013: lstrcpy( buf, "WM_QUERYOPEN" );  break;
	case 0x0014: lstrcpy( buf, "WM_ERASEBKGND" );  break;
	case 0x0015: lstrcpy( buf, "WM_SYSCOLORCHANGE" );  break;
	case 0x0016: lstrcpy( buf, "WM_ENDSESSION" );  break;
	case 0x0018: lstrcpy( buf, "WM_SHOWWINDOW" );  break;
	case 0x001A: lstrcpy( buf, "WM_WININICHANGE" );  break;
	case 0x001B: lstrcpy( buf, "WM_DEVMODECHANGE" );  break;
	case 0x001C: lstrcpy( buf, "WM_ACTIVATEAPP" );  break;
	case 0x001D: lstrcpy( buf, "WM_FONTCHANGE" );  break;
	case 0x001E: lstrcpy( buf, "WM_TIMECHANGE" );  break;
	case 0x001F: lstrcpy( buf, "WM_CANCELMODE" );  break;
	case 0x0020: lstrcpy( buf, "WM_SETCURSOR" );  break;
	case 0x0021: lstrcpy( buf, "WM_MOUSEACTIVATE" );  break;
	case 0x0022: lstrcpy( buf, "WM_CHILDACTIVATE" );  break;
	case 0x0023: lstrcpy( buf, "WM_QUEUESYNC" );  break;
	case 0x0024: lstrcpy( buf, "WM_GETMINMAXINFO" );  break;
	case 0x0026: lstrcpy( buf, "WM_PAINTICON" );  break;
	case 0x0027: lstrcpy( buf, "WM_ICONERASEBKGND" );  break;
	case 0x0028: lstrcpy( buf, "WM_NEXTDLGCTL" );  break;
	case 0x002A: lstrcpy( buf, "WM_SPOOLERSTATUS" );  break;
	case 0x002B: lstrcpy( buf, "WM_DRAWITEM" );  break;
	case 0x002C: lstrcpy( buf, "WM_MEASUREITEM" );  break;
	case 0x002D: lstrcpy( buf, "WM_DELETEITEM" );  break;
	case 0x002E: lstrcpy( buf, "WM_VKEYTOITEM" );  break;
	case 0x002F: lstrcpy( buf, "WM_CHARTOITEM" );  break;
	case 0x0030: lstrcpy( buf, "WM_SETFONT" );  break;
	case 0x0031: lstrcpy( buf, "WM_GETFONT" );  break;
	case 0x0032: lstrcpy( buf, "WM_SETHOTKEY" );  break;
	case 0x0033: lstrcpy( buf, "WM_GETHOTKEY" );  break;
	case 0x0037: lstrcpy( buf, "WM_QUERYDRAGICON" );  break;
	case 0x0039: lstrcpy( buf, "WM_COMPAREITEM" );  break;
	case 0x003D: lstrcpy( buf, "WM_GETOBJECT" );  break;
	case 0x0041: lstrcpy( buf, "WM_COMPACTING" );  break;
	case 0x0044: lstrcpy( buf, "WM_COMMNOTIFY" );  break;
	case 0x0046: lstrcpy( buf, "WM_WINDOWPOSCHANGING" );  break;
	case 0x0047: lstrcpy( buf, "WM_WINDOWPOSCHANGED" );  break;
	case 0x0048: lstrcpy( buf, "WM_POWER" );  break;
	case 0x004A: lstrcpy( buf, "WM_COPYDATA" );  break;
	case 0x004B: lstrcpy( buf, "WM_CANCELJOURNAL" );  break;
	case 0x004E: lstrcpy( buf, "WM_NOTIFY" );  break;
	case 0x0050: lstrcpy( buf, "WM_INPUTLANGCHANGEREQUEST" );  break;
	case 0x0051: lstrcpy( buf, "WM_INPUTLANGCHANGE" );  break;
	case 0x0052: lstrcpy( buf, "WM_TCARD" );  break;
	case 0x0053: lstrcpy( buf, "WM_HELP" );  break;
	case 0x0054: lstrcpy( buf, "WM_USERCHANGED" );  break;
	case 0x0055: lstrcpy( buf, "WM_NOTIFYFORMAT" );  break;
	case 0x007B: lstrcpy( buf, "WM_CONTEXTMENU" );  break;
	case 0x007C: lstrcpy( buf, "WM_STYLECHANGING" );  break;
	case 0x007D: lstrcpy( buf, "WM_STYLECHANGED" );  break;
	case 0x007E: lstrcpy( buf, "WM_DISPLAYCHANGE" );  break;
	case 0x007F: lstrcpy( buf, "WM_GETICON" );  break;
	case 0x0080: lstrcpy( buf, "WM_SETICON" );  break;
	case 0x0081: lstrcpy( buf, "WM_NCCREATE" );  break;
	case 0x0082: lstrcpy( buf, "WM_NCDESTROY" );  break;
	case 0x0083: lstrcpy( buf, "WM_NCCALCSIZE" );  break;
	case 0x0084: lstrcpy( buf, "WM_NCHITTEST" );  break;
	case 0x0085: lstrcpy( buf, "WM_NCPAINT" );  break;
	case 0x0086: lstrcpy( buf, "WM_NCACTIVATE" );  break;
	case 0x0087: lstrcpy( buf, "WM_GETDLGCODE" );  break;
	case 0x0088: lstrcpy( buf, "WM_SYNCPAINT" );  break;
	case 0x00A0: lstrcpy( buf, "WM_NCMOUSEMOVE" );  break;
	case 0x00A1: lstrcpy( buf, "WM_NCLBUTTONDOWN" );  break;
	case 0x00A2: lstrcpy( buf, "WM_NCLBUTTONUP" );  break;
	case 0x00A3: lstrcpy( buf, "WM_NCLBUTTONDBLCLK" );  break;
	case 0x00A4: lstrcpy( buf, "WM_NCRBUTTONDOWN" );  break;
	case 0x00A5: lstrcpy( buf, "WM_NCRBUTTONUP" );  break;
	case 0x00A6: lstrcpy( buf, "WM_NCRBUTTONDBLCLK" );  break;
	case 0x00A7: lstrcpy( buf, "WM_NCMBUTTONDOWN" );  break;
	case 0x00A8: lstrcpy( buf, "WM_NCMBUTTONUP" );  break;
	case 0x00A9: lstrcpy( buf, "WM_NCMBUTTONDBLCLK" );  break;
	//case 0x0100: lstrcpy( buf, "WM_KEYFIRST" );  break;
	case 0x0100: lstrcpy( buf, "WM_KEYDOWN" );  break;
	case 0x0101: lstrcpy( buf, "WM_KEYUP" );  break;
	case 0x0102: lstrcpy( buf, "WM_CHAR" );  break;
	case 0x0103: lstrcpy( buf, "WM_DEADCHAR" );  break;
	case 0x0104: lstrcpy( buf, "WM_SYSKEYDOWN" );  break;
	case 0x0105: lstrcpy( buf, "WM_SYSKEYUP" );  break;
	case 0x0106: lstrcpy( buf, "WM_SYSCHAR" );  break;
	case 0x0107: lstrcpy( buf, "WM_SYSDEADCHAR" );  break;
	case 0x0108: lstrcpy( buf, "WM_KEYLAST" );  break;
	case 0x010D: lstrcpy( buf, "WM_IME_STARTCOMPOSITION" );  break;
	case 0x010E: lstrcpy( buf, "WM_IME_ENDCOMPOSITION" );  break;
	//case 0x010F: lstrcpy( buf, "WM_IME_COMPOSITION" );  break;
	case 0x010F: lstrcpy( buf, "WM_IME_KEYLAST" );  break;
	case 0x0110: lstrcpy( buf, "WM_INITDIALOG" );  break;
	case 0x0111: lstrcpy( buf, "WM_COMMAND" );  break;
	case 0x0112: lstrcpy( buf, "WM_SYSCOMMAND" );  break;
	case 0x0113: lstrcpy( buf, "WM_TIMER" );  break;
	case 0x0114: lstrcpy( buf, "WM_HSCROLL" );  break;
	case 0x0115: lstrcpy( buf, "WM_VSCROLL" );  break;
	case 0x0116: lstrcpy( buf, "WM_INITMENU" );  break;
	case 0x0117: lstrcpy( buf, "WM_INITMENUPOPUP" );  break;
	case 0x011F: lstrcpy( buf, "WM_MENUSELECT" );  break;
	case 0x0120: lstrcpy( buf, "WM_MENUCHAR" );  break;
	case 0x0121: lstrcpy( buf, "WM_ENTERIDLE" );  break;
	case 0x0122: lstrcpy( buf, "WM_MENURBUTTONUP" );  break;
	case 0x0123: lstrcpy( buf, "WM_MENUDRAG" );  break;
	case 0x0124: lstrcpy( buf, "WM_MENUGETOBJECT" );  break;
	case 0x0125: lstrcpy( buf, "WM_UNINITMENUPOPUP" );  break;
	case 0x0126: lstrcpy( buf, "WM_MENUCOMMAND" );  break;
	//case 0x0132: lstrcpy( buf, "WM_CTLCOLORMSGBOX" );  break;
	//case 0x0133: lstrcpy( buf, "WM_CTLCOLOREDIT" );  break;
	//case 0x0134: lstrcpy( buf, "WM_CTLCOLORLISTBOX" );  break;
	//case 0x0135: lstrcpy( buf, "WM_CTLCOLORBTN" );  break;
	//case 0x0136: lstrcpy( buf, "WM_CTLCOLORDLG" );  break;
	//case 0x0137: lstrcpy( buf, "WM_CTLCOLORSCROLLBAR" );  break;
	//case 0x0138: lstrcpy( buf, "WM_CTLCOLORSTATIC" );  break;
	//case 0x0200: lstrcpy( buf, "WM_MOUSEFIRST" );  break;
	case 0x0200: lstrcpy( buf, "WM_MOUSEMOVE" );  break;
	case 0x0201: lstrcpy( buf, "WM_LBUTTONDOWN" );  break;
	case 0x0202: lstrcpy( buf, "WM_LBUTTONUP" );  break;
	case 0x0203: lstrcpy( buf, "WM_LBUTTONDBLCLK" );  break;
	case 0x0204: lstrcpy( buf, "WM_RBUTTONDOWN" );  break;
	case 0x0205: lstrcpy( buf, "WM_RBUTTONUP" );  break;
	case 0x0206: lstrcpy( buf, "WM_RBUTTONDBLCLK" );  break;
	case 0x0207: lstrcpy( buf, "WM_MBUTTONDOWN" );  break;
	case 0x0208: lstrcpy( buf, "WM_MBUTTONUP" );  break;
	case 0x0209: lstrcpy( buf, "WM_MBUTTONDBLCLK" );  break;
	case 0x020A: lstrcpy( buf, "WM_MOUSEWHEEL" );  break;
	//case 0x020A: lstrcpy( buf, "WM_MOUSELAST" );  break;
	//case 0x0209: lstrcpy( buf, "WM_MOUSELAST" );  break;
	case 0x0210: lstrcpy( buf, "WM_PARENTNOTIFY" );  break;
	case 0x0211: lstrcpy( buf, "WM_ENTERMENULOOP" );  break;
	case 0x0212: lstrcpy( buf, "WM_EXITMENULOOP" );  break;
	case 0x0213: lstrcpy( buf, "WM_NEXTMENU" );  break;
	case 0x0214: lstrcpy( buf, "WM_SIZING" );  break;
	case 0x0215: lstrcpy( buf, "WM_CAPTURECHANGED" );  break;
	case 0x0216: lstrcpy( buf, "WM_MOVING" );  break;
	case 0x0218: lstrcpy( buf, "WM_POWERBROADCAST" );  break;	// r_winuser pbt
	case 0x0219: lstrcpy( buf, "WM_DEVICECHANGE" );  break;
	case 0x0220: lstrcpy( buf, "WM_MDICREATE" );  break;
	case 0x0221: lstrcpy( buf, "WM_MDIDESTROY" );  break;
	case 0x0222: lstrcpy( buf, "WM_MDIACTIVATE" );  break;
	case 0x0223: lstrcpy( buf, "WM_MDIRESTORE" );  break;
	case 0x0224: lstrcpy( buf, "WM_MDINEXT" );  break;
	case 0x0225: lstrcpy( buf, "WM_MDIMAXIMIZE" );  break;
	case 0x0226: lstrcpy( buf, "WM_MDITILE" );  break;
	case 0x0227: lstrcpy( buf, "WM_MDICASCADE" );  break;
	case 0x0228: lstrcpy( buf, "WM_MDIICONARRANGE" );  break;
	case 0x0229: lstrcpy( buf, "WM_MDIGETACTIVE" );  break;
	case 0x0230: lstrcpy( buf, "WM_MDISETMENU" );  break;
	case 0x0231: lstrcpy( buf, "WM_ENTERSIZEMOVE" );  break;
	case 0x0232: lstrcpy( buf, "WM_EXITSIZEMOVE" );  break;
	case 0x0233: lstrcpy( buf, "WM_DROPFILES" );  break;
	case 0x0234: lstrcpy( buf, "WM_MDIREFRESHMENU" );  break;
	case 0x0281: lstrcpy( buf, "WM_IME_SETCONTEXT" );  break;
	case 0x0282: lstrcpy( buf, "WM_IME_NOTIFY" );  break;
	case 0x0283: lstrcpy( buf, "WM_IME_CONTROL" );  break;
	case 0x0284: lstrcpy( buf, "WM_IME_COMPOSITIONFULL" );  break;
	case 0x0285: lstrcpy( buf, "WM_IME_SELECT" );  break;
	case 0x0286: lstrcpy( buf, "WM_IME_CHAR" );  break;
	case 0x0288: lstrcpy( buf, "WM_IME_REQUEST" );  break;
	case 0x0290: lstrcpy( buf, "WM_IME_KEYDOWN" );  break;
	case 0x0291: lstrcpy( buf, "WM_IME_KEYUP" );  break;
	case 0x02A1: lstrcpy( buf, "WM_MOUSEHOVER" );  break;
	case 0x02A3: lstrcpy( buf, "WM_MOUSELEAVE" );  break;
	case 0x0300: lstrcpy( buf, "WM_CUT" );  break;
	case 0x0301: lstrcpy( buf, "WM_COPY" );  break;
	case 0x0302: lstrcpy( buf, "WM_PASTE" );  break;
	case 0x0303: lstrcpy( buf, "WM_CLEAR" );  break;
	case 0x0304: lstrcpy( buf, "WM_UNDO" );  break;
	case 0x0305: lstrcpy( buf, "WM_RENDERFORMAT" );  break;
	case 0x0306: lstrcpy( buf, "WM_RENDERALLFORMATS" );  break;
	case 0x0307: lstrcpy( buf, "WM_DESTROYCLIPBOARD" );  break;
	case 0x0308: lstrcpy( buf, "WM_DRAWCLIPBOARD" );  break;
	case 0x0309: lstrcpy( buf, "WM_PAINTCLIPBOARD" );  break;
	case 0x030A: lstrcpy( buf, "WM_VSCROLLCLIPBOARD" );  break;
	case 0x030B: lstrcpy( buf, "WM_SIZECLIPBOARD" );  break;
	case 0x030C: lstrcpy( buf, "WM_ASKCBFORMATNAME" );  break;
	case 0x030D: lstrcpy( buf, "WM_CHANGECBCHAIN" );  break;
	case 0x030E: lstrcpy( buf, "WM_HSCROLLCLIPBOARD" );  break;
	case 0x030F: lstrcpy( buf, "WM_QUERYNEWPALETTE" );  break;
	case 0x0310: lstrcpy( buf, "WM_PALETTEISCHANGING" );  break;
	case 0x0311: lstrcpy( buf, "WM_PALETTECHANGED" );  break;
	case 0x0312: lstrcpy( buf, "WM_HOTKEY" );  break;
	case 0x0317: lstrcpy( buf, "WM_PRINT" );  break;
	case 0x0318: lstrcpy( buf, "WM_PRINTCLIENT" );  break;
	case 0x0358: lstrcpy( buf, "WM_HANDHELDFIRST" );  break;
	case 0x035F: lstrcpy( buf, "WM_HANDHELDLAST" );  break;
	case 0x0360: lstrcpy( buf, "WM_AFXFIRST" );  break;
	case 0x037F: lstrcpy( buf, "WM_AFXLAST" );  break;
	case 0x0380: lstrcpy( buf, "WM_PENWINFIRST" );  break;
	case 0x038F: lstrcpy( buf, "WM_PENWINLAST" );  break;
	case 0x8000: lstrcpy( buf, "WM_APP" );  break;
	case 0x0400: lstrcpy( buf, "WM_USER" );  break;

	//case 0x0001: lstrcpy( buf, "MWMO_WAITALL" );  break;
	//case 0x0002: lstrcpy( buf, "MWMO_ALERTABLE" );  break;
	case 0x0004: lstrcpy( buf, "MWMO_INPUTAVAILABLE" );  break;

	//case 0x000c: lstrcpy( buf, "WM_HELP" );  break;

	default: sprintf( buf, "WM_(0x%x)", message );  break;
	}
	if ( message >= WM_USER && message <= 0x7FFF )
		sprintf( buf, "WM_USER+%d", message-WM_USER );

	LPSTR	ptr = buf+lstrlen(buf);
	if ( message == WM_MOUSEWHEEL ) {
		// ホイール
		sprintf( ptr, "\t\t\t( fwKeys:%x, zDelta:%d, x:%d, y:%d )\n", LOWORD(wParam), (short) HIWORD(wParam), LOWORD(lParam), HIWORD(lParam) );
	}
	else if ( message >= WM_MOUSEMOVE && message <= WM_MOUSEWHEEL ) {
		// マウス関連メッセージ
		sprintf( ptr, "\t\t\t( fwKeys:%x, x:%d, y:%d )\n", wParam, LOWORD(lParam), HIWORD(lParam) );
	}
	else if ( message == WM_ACTIVATE ) {
		if ( LOWORD(wParam) == WA_ACTIVE )
			ptr += sprintf( ptr, "\t\t\t( WA_ACTIVE, ");
		else if ( LOWORD(wParam) == WA_CLICKACTIVE )
			ptr += sprintf( ptr, "\t\t\t( WA_CLICKACTIVE, ");
		else if ( LOWORD(wParam) == WA_INACTIVE )
			ptr += sprintf( ptr, "\t\t\t( WA_INACTIVE, ");
		ptr += sprintf( ptr, "%s, ", ((BOOL)HIWORD(wParam)) ? "true" : "false"  );
		if ( (HWND)lParam == NULL )
			ptr += sprintf( ptr, "NULL )\n" );
		else {
			ptr += sprintf( ptr, "\"" );
			ptr += ::GetWindowText( (HWND)lParam, ptr, 256 );
			ptr += sprintf( ptr, "\" )\n" );
		}
	}
	else if ( message == WM_NCACTIVATE )
		ptr += sprintf( ptr, "\t\t\t( %s )\n", ((BOOL)wParam) ? "true" : "false"  );
	else if ( message == WM_MOVE )
		ptr += sprintf( ptr, "\t\t\t( x:%d, y:%d )\n", LOWORD(lParam), HIWORD(lParam) );
	else if ( message == WM_SIZE ) {
		ptr += sprintf( ptr, "\t\t\t( " );
		if ( wParam == SIZE_MAXIMIZED )
			ptr += sprintf( ptr, "SIZE_MAXIMIZED" );
		else if ( wParam == SIZE_MINIMIZED )
			ptr += sprintf( ptr, "SIZE_MINIMIZED" );
		else if ( wParam == SIZE_RESTORED )
			ptr += sprintf( ptr, "SIZE_RESTORED" );
		else if ( wParam == SIZE_MAXHIDE )
			ptr += sprintf( ptr, "SIZE_MAXHIDE" );
		else if ( wParam == SIZE_MAXSHOW )
			ptr += sprintf( ptr, "SIZE_MAXSHOW" );
		ptr += sprintf( ptr, ", w:%d, h:%d )\n", LOWORD(lParam), HIWORD(lParam) );
	}
	else if ( message == WM_WINDOWPOSCHANGED || message == WM_WINDOWPOSCHANGING ) {
		// WINDOWPOS構造体を使用するメッセージ
		LPWINDOWPOS	p = (LPWINDOWPOS)lParam;
		ptr += sprintf( ptr, "\t\t\t( x:%d, y:%d, cx:%d, cy:%d )\n",
			p->x, p->y, p->cx, p->cy );
		ptr += sprintf( ptr, "flags : " );
		if ( p->flags & SWP_DRAWFRAME )
			ptr += sprintf( ptr, "SWP_DRAWFRAME " );
		if ( p->flags & SWP_FRAMECHANGED )
			ptr += sprintf( ptr, "SWP_HIDEWINDOW " );
		if ( p->flags & SWP_NOACTIVATE )
			ptr += sprintf( ptr, "SWP_NOACTIVATE " );
		if ( p->flags & SWP_NOCOPYBITS )
			ptr += sprintf( ptr, "SWP_NOCOPYBITS " );
		if ( p->flags & SWP_NOMOVE )
			ptr += sprintf( ptr, "SWP_NOMOVE " );
		if ( p->flags & SWP_NOOWNERZORDER )
			ptr += sprintf( ptr, "SWP_NOOWNERZORDER " );
		if ( p->flags & SWP_NOSIZE )
			ptr += sprintf( ptr, "SWP_NOSIZE " );
		if ( p->flags & SWP_NOREDRAW )
			ptr += sprintf( ptr, "SWP_NOREDRAW " );
		if ( p->flags & SWP_NOZORDER )
			ptr += sprintf( ptr, "SWP_NOZORDER " );
		if ( p->flags & SWP_SHOWWINDOW )
			ptr += sprintf( ptr, "SWP_SHOWWINDOW " );
		ptr += sprintf( ptr, "\n" );
	}
	else if ( message == WM_KEYDOWN || message == WM_KEYUP || message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) {
		// 仮想キーコードの表示
		ptr += sprintf( ptr, "\t\t\t( VK_" );
		int	vk = (int)wParam;
		switch ( vk ) {
		case 0x01: ptr += sprintf( ptr, "LBUTTON" ); break;
		case 0x02: ptr += sprintf( ptr, "RBUTTON" ); break;
		case 0x03: ptr += sprintf( ptr, "CANCEL" ); break;
		case 0x04: ptr += sprintf( ptr, "MBUTTON" ); break;    /* NOT contiguous with L & RBUTTON */
		case 0x08: ptr += sprintf( ptr, "BACK" ); break;
		case 0x09: ptr += sprintf( ptr, "TAB" ); break;
		case 0x0C: ptr += sprintf( ptr, "CLEAR" ); break;
		case 0x0D: ptr += sprintf( ptr, "RETURN" ); break;
		case 0x10: ptr += sprintf( ptr, "SHIFT" ); break;
		case 0x11: ptr += sprintf( ptr, "CONTROL" ); break;
		case 0x12: ptr += sprintf( ptr, "MENU" ); break;
		case 0x13: ptr += sprintf( ptr, "PAUSE" ); break;
		case 0x14: ptr += sprintf( ptr, "CAPITAL" ); break;
		case 0x15: ptr += sprintf( ptr, "KANA" ); break;
		//case 0x15: ptr += sprintf( ptr, "HANGEUL" ); break;  /* old name - should be here for compatibility */
		//case 0x15: ptr += sprintf( ptr, "HANGUL" ); break;
		case 0x17: ptr += sprintf( ptr, "JUNJA" ); break;
		case 0x18: ptr += sprintf( ptr, "FINAL" ); break;
		//case 0x19: ptr += sprintf( ptr, "HANJA" ); break;
		case 0x19: ptr += sprintf( ptr, "KANJI" ); break;
		case 0x1B: ptr += sprintf( ptr, "ESCAPE" ); break;
		case 0x1C: ptr += sprintf( ptr, "CONVERT" ); break;
		case 0x1D: ptr += sprintf( ptr, "NONCONVERT" ); break;
		case 0x1E: ptr += sprintf( ptr, "ACCEPT" ); break;
		case 0x1F: ptr += sprintf( ptr, "MODECHANGE" ); break;
		case 0x20: ptr += sprintf( ptr, "SPACE" ); break;
		case 0x21: ptr += sprintf( ptr, "PRIOR" ); break;
		case 0x22: ptr += sprintf( ptr, "NEXT" ); break;
		case 0x23: ptr += sprintf( ptr, "END" ); break;
		case 0x24: ptr += sprintf( ptr, "HOME" ); break;
		case 0x25: ptr += sprintf( ptr, "LEFT" ); break;
		case 0x26: ptr += sprintf( ptr, "UP" ); break;
		case 0x27: ptr += sprintf( ptr, "RIGHT" ); break;
		case 0x28: ptr += sprintf( ptr, "DOWN" ); break;
		case 0x29: ptr += sprintf( ptr, "SELECT" ); break;
		case 0x2A: ptr += sprintf( ptr, "PRINT" ); break;
		case 0x2B: ptr += sprintf( ptr, "EXECUTE" ); break;
		case 0x2C: ptr += sprintf( ptr, "SNAPSHOT" ); break;
		case 0x2D: ptr += sprintf( ptr, "INSERT" ); break;
		case 0x2E: ptr += sprintf( ptr, "DELETE" ); break;
		case 0x2F: ptr += sprintf( ptr, "HELP" ); break;
		/* 0 thru 9 are the same as ASCII '0' thru '9' (0x30 - 0x39) */
		/* A thru Z are the same as ASCII 'A' thru 'Z' (0x41 - 0x5A) */
		case 0x5B: ptr += sprintf( ptr, "LWIN" ); break;
		case 0x5C: ptr += sprintf( ptr, "RWIN" ); break;
		case 0x5D: ptr += sprintf( ptr, "APPS" ); break;
		case 0x60: ptr += sprintf( ptr, "NUMPAD0" ); break;
		case 0x61: ptr += sprintf( ptr, "NUMPAD1" ); break;
		case 0x62: ptr += sprintf( ptr, "NUMPAD2" ); break;
		case 0x63: ptr += sprintf( ptr, "NUMPAD3" ); break;
		case 0x64: ptr += sprintf( ptr, "NUMPAD4" ); break;
		case 0x65: ptr += sprintf( ptr, "NUMPAD5" ); break;
		case 0x66: ptr += sprintf( ptr, "NUMPAD6" ); break;
		case 0x67: ptr += sprintf( ptr, "NUMPAD7" ); break;
		case 0x68: ptr += sprintf( ptr, "NUMPAD8" ); break;
		case 0x69: ptr += sprintf( ptr, "NUMPAD9" ); break;
		case 0x6A: ptr += sprintf( ptr, "MULTIPLY" ); break;
		case 0x6B: ptr += sprintf( ptr, "ADD" ); break;
		case 0x6C: ptr += sprintf( ptr, "SEPARATOR" ); break;
		case 0x6D: ptr += sprintf( ptr, "SUBTRACT" ); break;
		case 0x6E: ptr += sprintf( ptr, "DECIMAL" ); break;
		case 0x6F: ptr += sprintf( ptr, "DIVIDE" ); break;
		case 0x70: ptr += sprintf( ptr, "F1" ); break;
		case 0x71: ptr += sprintf( ptr, "F2" ); break;
		case 0x72: ptr += sprintf( ptr, "F3" ); break;
		case 0x73: ptr += sprintf( ptr, "F4" ); break;
		case 0x74: ptr += sprintf( ptr, "F5" ); break;
		case 0x75: ptr += sprintf( ptr, "F6" ); break;
		case 0x76: ptr += sprintf( ptr, "F7" ); break;
		case 0x77: ptr += sprintf( ptr, "F8" ); break;
		case 0x78: ptr += sprintf( ptr, "F9" ); break;
		case 0x79: ptr += sprintf( ptr, "F10" ); break;
		case 0x7A: ptr += sprintf( ptr, "F11" ); break;
		case 0x7B: ptr += sprintf( ptr, "F12" ); break;
		case 0x7C: ptr += sprintf( ptr, "F13" ); break;
		case 0x7D: ptr += sprintf( ptr, "F14" ); break;
		case 0x7E: ptr += sprintf( ptr, "F15" ); break;
		case 0x7F: ptr += sprintf( ptr, "F16" ); break;
		case 0x80: ptr += sprintf( ptr, "F17" ); break;
		case 0x81: ptr += sprintf( ptr, "F18" ); break;
		case 0x82: ptr += sprintf( ptr, "F19" ); break;
		case 0x83: ptr += sprintf( ptr, "F20" ); break;
		case 0x84: ptr += sprintf( ptr, "F21" ); break;
		case 0x85: ptr += sprintf( ptr, "F22" ); break;
		case 0x86: ptr += sprintf( ptr, "F23" ); break;
		case 0x87: ptr += sprintf( ptr, "F24" ); break;
		case 0x90: ptr += sprintf( ptr, "NUMLOCK" ); break;
		case 0x91: ptr += sprintf( ptr, "SCROLL" ); break;
		/*
		 * VK_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
		 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
		 * No other API or message will distinguish left and right keys in this way.
		 */
		case 0xA0: ptr += sprintf( ptr, "LSHIFT" ); break;
		case 0xA1: ptr += sprintf( ptr, "RSHIFT" ); break;
		case 0xA2: ptr += sprintf( ptr, "LCONTROL" ); break;
		case 0xA3: ptr += sprintf( ptr, "RCONTROL" ); break;
		case 0xA4: ptr += sprintf( ptr, "LMENU" ); break;
		case 0xA5: ptr += sprintf( ptr, "RMENU" ); break;
		#if(WINVER >= 0x0400)
		case 0xE5: ptr += sprintf( ptr, "PROCESSKEY" ); break;
		#endif /* WINVER >= 0x0400 */
		case 0xF6: ptr += sprintf( ptr, "ATTN" ); break;
		case 0xF7: ptr += sprintf( ptr, "CRSEL" ); break;
		case 0xF8: ptr += sprintf( ptr, "EXSEL" ); break;
		case 0xF9: ptr += sprintf( ptr, "EREOF" ); break;
		case 0xFA: ptr += sprintf( ptr, "PLAY" ); break;
		case 0xFB: ptr += sprintf( ptr, "ZOOM" ); break;
		case 0xFC: ptr += sprintf( ptr, "NONAME" ); break;
		case 0xFD: ptr += sprintf( ptr, "PA1" ); break;
		case 0xFE: ptr += sprintf( ptr, "OEM_CLEAR" ); break;
		default:
			if ( vk >= '0' && vk <= '9' )
				ptr += sprintf( ptr, "%c", vk );
			else if ( vk >= 'A' && vk <= 'Z' )
				ptr += sprintf( ptr, "%c", vk );
			else
				ptr += sprintf( ptr, "(0x%x)", vk );
			break;
		}

		ptr += sprintf( ptr, ", %x )\n", lParam );
	}
	else {
		// それ以外の一般形
		sprintf( ptr, "\t\t\t( %x, %x )\n", wParam, lParam );
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
void	TextOutF( HDC hDC, int xPos, int yPos, char* format, ... ) {
/*	char	buf[256];
	va_list	argptr;
	va_start( argptr, format );
	_vsnprintf( buf, 255, format, argptr );
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
void	DbgStr( const char* format, ... ) {
	char	buf[256];
	va_list	argptr;
	va_start( argptr, format );
	_vsnprintf( buf, 255, format, argptr );
	va_end( argptr );
	::OutputDebugString(buf);
}

// 可変引数対応の簡易報告用メッセージボックス
void	MesBox( const char* format, ... ) {
	char	buf[256];
	va_list	argptr;
	va_start( argptr, format );
	_vsnprintf( buf, 255, format, argptr );
	va_end( argptr );
	::MessageBox( NULL, buf, "notice", MB_OK|MB_SYSTEMMODAL );
}

// 可変引数対応の簡易報告用SetWindowText
void	SetWinText( HWND hWnd, const char* format, ... ) {
	char	buf[256];
	va_list	argptr;
	va_start( argptr, format );
	_vsnprintf( buf, 255, format, argptr );
	va_end( argptr );
	::SetWindowText( hWnd, buf );
}
