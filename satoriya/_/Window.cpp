#include	"Window.h"
#include	"Win32.h"

// くるくる対応
#include	<zmouse.h>
extern const UINT msgMOUSEWHEEL;	// require ../_/Win32.cpp


UINT	Window::gTaskbarCreatedMessage;

#define	Handle(message, handler) \
	case message: if ( p->handler ) return 0; else break

//---------------------------------------------------------------------
LRESULT	Window::WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

#ifdef	_DEBUG
	/*
	char	buf[20];
	sprintf(buf, "0x%x : ", hWnd);
	OutputDebugString(buf);
	*/
	//PutWindowMessage(message, wParam, lParam);
#endif

	if ( message==WM_CREATE )
	{
		LPCREATESTRUCT	pc = (LPCREATESTRUCT)lParam;
		Window*	pw = (Window*)(pc->lpCreateParams);
		pw->mWnd = hWnd;
		::SetWindowLong(hWnd, 0, (long)pw);
		bool b = pw->OnCreate(pc);

		gTaskbarCreatedMessage = ::RegisterWindowMessage(TEXT("TaskbarCreated"));

		return	b ? 0 : -1;
	}

	// マウスホイール対応
	if ( message == msgMOUSEWHEEL ) {
		message = WM_MOUSEWHEEL;
		DbgStr("MouseWheel fwKeys:%04x, zDelta:%d, xPos:%d, yPos:%d",
			LOWORD(wParam), (short)HIWORD(wParam),
			(short)LOWORD(lParam), (short)HIWORD(lParam) );
	}

	Window*	p=(Window*)(::GetWindowLong(hWnd, 0));
	if ( p == NULL )
		return	::DefWindowProc(hWnd, message, wParam, lParam);

	if ( message >= WM_USER && message <= 0x7FFF )
		return	p->OnUser(message, wParam, lParam);
	if ( message == gTaskbarCreatedMessage )
		return  p->OnTaskbarRestart();

	switch ( message ) {

	case WM_PAINT:
		{
			PAINTSTRUCT	thePS;
			HDC			theDC = ::BeginPaint(hWnd, &thePS);
			if (theDC != NULL) {
				p->OnPaint(theDC);
				::EndPaint(hWnd, &thePS);
			}
		}
		return	0;

	case WM_NCHITTEST:
		return	p->OnNCHitTest(MAKEPOINT(lParam));

	Handle(WM_ACTIVATE, OnActivate(LOWORD(wParam), HIWORD(wParam), (HWND)lParam));
	Handle(WM_ACTIVATEAPP, OnActivateApp(BOOL(wParam), (DWORD)lParam));
	Handle(WM_DISPLAYCHANGE, OnDisplayChange(LOWORD(lParam), HIWORD(lParam), wParam));
	/*case WM_DISPLAYCHANGE:
		p->OnDisplayChange(LOWORD(lParam), HIWORD(lParam), wParam);
		break;*/


	Handle(WM_COMMAND, OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam));
	Handle(WM_INITMENU, OnInitMenu(HMENU(wParam)));
	Handle(WM_CLOSE, OnClose());
	Handle(WM_DESTROY, OnDestroy());

	Handle(MM_MCINOTIFY, OnMCINotify(WORD(wParam), LONG(lParam)));

	Handle(WM_LBUTTONDOWN, OnLButtonDown(MAKEPOINT(lParam), wParam));
	Handle(WM_MBUTTONDOWN, OnMButtonDown(MAKEPOINT(lParam), wParam));
	Handle(WM_RBUTTONDOWN, OnRButtonDown(MAKEPOINT(lParam), wParam));
	Handle(WM_LBUTTONUP, OnLButtonUp(MAKEPOINT(lParam), wParam));
	Handle(WM_MBUTTONUP, OnMButtonUp(MAKEPOINT(lParam), wParam));
	Handle(WM_RBUTTONUP, OnRButtonUp(MAKEPOINT(lParam), wParam));
	Handle(WM_LBUTTONDBLCLK, OnLButtonDoubleClick(MAKEPOINT(lParam), wParam));
	Handle(WM_MBUTTONDBLCLK, OnMButtonDoubleClick(MAKEPOINT(lParam), wParam));
	Handle(WM_RBUTTONDBLCLK, OnRButtonDoubleClick(MAKEPOINT(lParam), wParam));
	Handle(WM_MOUSEMOVE, OnMouseMove(MAKEPOINT(lParam), wParam));
	Handle(WM_MOUSEWHEEL, OnMouseWheel(LOWORD(wParam), (short)HIWORD(wParam), (short)LOWORD(lParam), (short)HIWORD(lParam)));
	Handle(WM_NCLBUTTONDBLCLK, OnNCLButtonDoubleClick(wParam, MAKEPOINT(lParam)));

	Handle(WM_KEYDOWN, OnKeyDown((int)wParam, (long)lParam));
	Handle(WM_KEYUP, OnKeyUp((int)wParam, (long)lParam));
	Handle(WM_DROPFILES, OnDropFiles(HDROP(wParam)));
	
	Handle(WM_TIMER, OnTimer(wParam, (TIMERPROC*)lParam));

	Handle(WM_DRAWCLIPBOARD, OnDrawClipboard());
	Handle(WM_CHANGECBCHAIN, OnChangeCBChain((HWND)wParam, (HWND)lParam));

	default:
		break;
	}
	return	::DefWindowProc(hWnd, message, wParam, lParam);
}

bool	Window::OnCreate(LPCREATESTRUCT iCreateStruct) { 
	return true; 
}
bool	Window::OnClose() {
	Destroy();
	return true;
}
bool	Window::OnDestroy() {
	::PostQuitMessage(0);
	return true;
}
bool	Window::OnPaint(HDC hDC) { return false; }
bool	Window::OnKeyDown(int nVirtKey, long lKeyData) { return false; }
bool	Window::OnKeyUp(int nVirtKey, long lKeyData) { return false; }
bool	Window::OnLButtonDown(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnMButtonDown(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnRButtonDown(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnMouseMove(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnLButtonUp(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnMButtonUp(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnRButtonUp(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnLButtonDoubleClick(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnMButtonDoubleClick(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnRButtonDoubleClick(POINT iPoint, DWORD iKeys) { return false; }
bool	Window::OnHScroll(int nScrollCode, int nPos, HWND hwndScrollBar) { return false; }
bool	Window::OnVScroll(int nScrollCode, int nPos, HWND hwndScrollBar) { return false; }
bool	Window::OnJoy1ButtonDown( WPARAM button, int x, int y ) { return false; }
bool	Window::OnMCINotify(WORD wFlag, LONG lCallback) { return false; }
bool	Window::OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCrl) { return false; }
bool	Window::OnInitMenu(HMENU hMenu) { return false; }
bool	Window::OnSysCommand(UINT uCmdType, WORD xPos, WORD yPos) { return false; }
bool	Window::OnTimer(WPARAM wTimerID, TIMERPROC* tmprc) { return false; }
bool	Window::OnDropFiles(HDROP hDrop) { return false; }
bool	Window::OnActivateApp(BOOL iActive, DWORD iThreadID)  { return false; }
bool	Window::OnActivate(WORD iActive, BOOL iMinimized, HWND iWnd)  { return false; }
bool	Window::OnDisplayChange(int iWidth, int iHeight, int iColorBits)  { return false; }
bool	Window::OnNCLButtonDoubleClick(UINT nHitTest, POINT iPoint) { return false; }

LRESULT	Window::OnNCHitTest(POINT iPoint) { return ::DefWindowProc(mWnd, WM_NCHITTEST, 0, MAKELPARAM(iPoint.x, iPoint.y)); }
LRESULT	Window::OnUser(UINT message, WPARAM wParam, LPARAM lParam) { return ::DefWindowProc(mWnd, message, wParam, lParam); }
LRESULT	Window::OnTaskbarRestart() { return 0; }

bool Window::OnDrawClipboard() { return false; }
bool Window::OnChangeCBChain(HWND hWndRemove, HWND hWndNext) { return false; } 

// zDeltaが-1以下なら手前、1以上なら奥に回転。
bool	Window::OnMouseWheel(WORD fwKeys, short zDelta, short xPos, short yPos) {
	return	false;
}

void	Window::SetText(const char* format, ... ) {
	char	buf[1024];
	va_list	argptr;
	va_start(argptr, format);
	_vsnprintf(buf, 1023, format, argptr);
	va_end(argptr);
	::SetWindowText(mWnd, buf);
}

int		Window::MesBox(UINT style, const char* title, const char* format, ... ) {
	char	buf[1024];
	va_list	argptr;
	va_start( argptr, format );
	_vsnprintf( buf, 1023, format, argptr );
	va_end( argptr );
	return	::MessageBox(mWnd, buf, title, style);
}


