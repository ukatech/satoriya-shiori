#ifndef	WINDOW_H
#define	WINDOW_H

#include	<windows.h>
#include	<commctrl.h>
#include	<stdarg.h>
#include	<stdio.h>
#include	<assert.h>




// ウィンドウの基底クラス。
// ウィンドウハンドル、
// 共通のウィンドウプロシージャ、
// イベント（メッセージ）ハンドラ、
// 操作メソッド（主にAPIのラッパ）からなる。
class	Window {

private:

	//-------------------------------------------------------
	// ウィンドウプロシージャ

	// WNDCLASS::WndProcにこれを指定した上で、
	// WindowExtraAreaの先頭に
	// Windowクラスのthisポインタを格納することにより、
	// 各種のイベントハンドラが呼び出される。

	static	LRESULT WINAPI WindowProcedure(HWND, UINT, WPARAM, LPARAM);

	// 「タスクバー消滅後再生成」への対応用
	static UINT gTaskbarCreatedMessage;

protected:
	HWND	mWnd;	// ウィンドウハンドル

	//-------------------------------------------------------
	// イベントハンドラ。継承してオーバーロードして使う。
	//
	// falseを返した場合、同じメッセージをDefWindowProcに送る。

	virtual	bool	OnCreate(LPCREATESTRUCT iCreateStruct);
	virtual	bool	OnClose();
	virtual	bool	OnDestroy();

	virtual bool	OnPaint(HDC hDC);

	virtual	bool	OnKeyDown(int nVirtKey, long lKeyData);
	virtual	bool	OnKeyUp(int nVirtKey, long lKeyData);

	virtual bool	OnMouseMove(POINT iPoint, DWORD iKeys);
	virtual bool	OnLButtonDown(POINT iPoint, DWORD iKeys);
	virtual bool	OnLButtonUp(POINT iPoint, DWORD iKeys);
	virtual bool	OnLButtonDoubleClick(POINT iPoint, DWORD iKeys);
	virtual bool	OnRButtonDown(POINT iPoint, DWORD iKeys);
	virtual bool	OnRButtonUp(POINT iPoint, DWORD iKeys);
	virtual bool	OnRButtonDoubleClick(POINT iPoint, DWORD iKeys);
	virtual bool	OnMButtonDown(POINT iPoint, DWORD iKeys);
	virtual bool	OnMButtonUp(POINT iPoint, DWORD iKeys);
	virtual bool	OnMButtonDoubleClick(POINT iPoint, DWORD iKeys);
	virtual	bool	OnMouseWheel(WORD fwKeys, short zDelta, short xPos, short yPos);

	virtual	bool	OnHScroll(int nScrollCode, int nPos, HWND hwndScrollBar);
	virtual	bool	OnVScroll(int nScrollCode, int nPos, HWND hwndScrollBar);

	virtual bool	OnJoy1ButtonDown( WPARAM button, int x, int y );
	virtual bool	OnMCINotify(WORD wFlag, LONG lCallback);

	virtual	bool	OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCrl);
	virtual	bool	OnInitMenu(HMENU hMenu);
	virtual	bool	OnSysCommand(UINT uCmdType, WORD xPos, WORD yPos);
	virtual	bool	OnTimer(WPARAM wTimerID, TIMERPROC* tmprc);

	virtual	bool	OnDropFiles(HDROP hDrop);

	virtual	LRESULT	OnNCHitTest(POINT iPoint);
	virtual	bool	OnNCLButtonDoubleClick(UINT nHitTest, POINT iPoint);

	virtual bool	OnActivate(WORD iActive, BOOL iMinimized, HWND iWnd);
	virtual bool	OnActivateApp(BOOL iActive, DWORD iThreadID);
	virtual	bool	OnDisplayChange(int iWidth, int iHeight, int iColorBits);

	virtual	LRESULT	OnTaskbarRestart();

	virtual	bool OnDrawClipboard();
	virtual	bool OnChangeCBChain(HWND hWndRemove, HWND hWndNext);

	// WM_USER～0x7FFFの場合はこれを呼ぶ。引数は直渡し。
	virtual	LRESULT	OnUser(UINT message, WPARAM wParam, LPARAM lParam);

public:
	Window() : mWnd(NULL) {}
	virtual ~Window() {}

	WPARAM run(
		UINT i_wc_style,
		HINSTANCE i_h_instance,
		HICON i_h_icon,
		HICON i_h_small_icon,
		HCURSOR i_h_cursor,
		HBRUSH i_h_background_brush,
		LPCTSTR i_menu_name,
		LPCTSTR i_class_name,

		DWORD i_ex_style,
		LPCTSTR i_window_name,
		DWORD i_stlye,
		int i_x, int i_y, int i_width, int i_height,
		HWND i_h_parent_window,
		HMENU i_h_menu,
		
		int i_show_command,
		HACCEL i_h_accelerator
		)
	{
		assert(mWnd == NULL);

		WNDCLASSEX	wc;
		::ZeroMemory(&wc, sizeof(WNDCLASSEX));
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = i_wc_style;
		wc.lpfnWndProc = WindowProcedure;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 4;
		wc.hInstance = i_h_instance;
		wc.hIcon = i_h_icon;
		wc.hIconSm = i_h_small_icon;
		wc.hCursor = i_h_cursor;
		wc.hbrBackground = i_h_background_brush;
		wc.lpszMenuName = i_menu_name;
		wc.lpszClassName  = i_class_name;
		if ( ::RegisterClassEx(&wc)==0 )
		{
			return 1;
		}

		mWnd = ::CreateWindowEx(
			i_ex_style,
			i_class_name,
			i_window_name,
			i_stlye,
			i_x, i_y, i_width, i_height,
			i_h_parent_window,
			i_h_menu,
			i_h_instance,
			this);
		if ( mWnd == NULL )
		{
			return 1;
		}

		::ShowWindow(mWnd, i_show_command);
		::UpdateWindow(mWnd);

		MSG msg;
		while (::GetMessage(&msg, NULL, 0, 0)) 
		{
			if (i_h_accelerator == NULL || !::TranslateAccelerator(mWnd, i_h_accelerator, &msg)) 
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return msg.wParam;
	}

	//-------------------------------------------------------
	// 操作メソッド（主にAPIのラッパ）

	operator HWND() { return mWnd; }
	HINSTANCE	Instance() { ::GetWindowLong(mWnd, GWL_HINSTANCE); }
	BOOL	Show(int iShowCommand) { return ::ShowWindow(mWnd, iShowCommand); }
	LRESULT	SendMessage(UINT iMsg, WPARAM wParam=0, LPARAM lParam=0) { return ::SendMessage(mWnd, iMsg, wParam, lParam); }
	LRESULT	PostMessage(UINT iMsg, WPARAM wParam=0, LPARAM lParam=0) { return ::PostMessage(mWnd, iMsg, wParam, lParam); }
	BOOL	Destroy() { return ::DestroyWindow(mWnd); }
	BOOL	Invalidate(CONST RECT* iRect, BOOL iEraseBg) { return ::InvalidateRect(mWnd, iRect, iEraseBg); }
	BOOL	Update() { return ::UpdateWindow(mWnd); }
	HDC	GetDC() { return ::GetDC(mWnd); }
	VOID	ReleaseDC(HDC iDC) { ::ReleaseDC(mWnd, iDC); }
	RECT	GetRect() { RECT rc; ::GetWindowRect(mWnd, &rc); return rc; }
	LONG	SetLong(int nIndex, LONG lNewLong) { return ::SetWindowLong(mWnd, nIndex, lNewLong); }
	LONG	GetLong(int nIndex) { return ::GetWindowLong(mWnd, nIndex); }
	BOOL	SetMenu(HMENU hMenu) { return ::SetMenu(mWnd, hMenu); }
	HMENU	GetMenu() { return ::GetMenu(mWnd); }
	BOOL	DrawMenuBar() { return ::DrawMenuBar(mWnd); }
	BOOL	SetPos(HWND hwndInsertAfter, int x, int y, int cx, int cy, UINT fuFlags) { return ::SetWindowPos(mWnd, hwndInsertAfter, x, y, cx, cy, fuFlags); }
	RECT	GetClientRect() { RECT rc; ::GetClientRect(mWnd, &rc); return rc; }

	void	SetText(const char* format, ... );
	int		MesBox(UINT style, const char* title, const char* format, ... );
};


#endif	//	WINDOW_H
