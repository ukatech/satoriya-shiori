#ifndef	WIN32_H
#define	WIN32_H

#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>
#include	<assert.h>
#include	"Utilities.h"

//----------------------------------------------------------
// クリップボード

// 文字列を設定する。hWndはNULLでも構わない。
bool	SetClipBoard(HWND iWnd, const char* iFormat, ...);
#ifdef	_STRING_
std::string	GetClipBoard();
#endif

//----------------------------------------------------------
// プロセス・スレッド

BOOL	CreateThread( DWORD (* iCallBack)(LPVOID), LPVOID iParam);

// ShellExecute
BOOL	Start( LPSTR lpstrFileName );
// CreateProcess
BOOL	Start( LPCSTR lpszImageName, LPSTR lpszCommandLine );

//----------------------------------------------------------
// キーボード・マウス

// 指定のキーは押されているか？
inline	BOOL isKeyDown( int nVirtKey ) { return ( ::GetAsyncKeyState(nVirtKey) & 0x8000 ); }

// カーソル位置を設定・取得
POINT	GetCursorPos();
void	SetCursorPos( POINT pt );
// クライアント座標系でカーソル位置を設定・取得
POINT	GetCursorPos( HWND hWnd );
void	SetCursorPos( HWND hWnd, POINT pt );
// カーソルはクライアント領域内か？
BOOL	isCursorOnClient( HWND hWnd );

// マウスキャプチャの開始／終了
BOOL	StartCapture( HWND hWnd, const RECT* =NULL );
BOOL	isCapture( HWND hWnd );
void	EndCapture();
// 指定windowが持っていた場合に限り解放しTRUEを返す。
BOOL	EndCapture( HWND hWnd ); 

// マウスホイールからの入力をサポート。
// WinProc() { if ( message == msgMOUSEWHEEL ) message = WM_MOUSEWHEEL; ...
extern const UINT msgMOUSEWHEEL;

//----------------------------------------------------------
// 点

inline POINT MAKEPOINT( int x, int y ) { POINT	pt = { x, y }; return pt; }
inline POINT MAKEPOINT( LPARAM lParam ) { POINT	pt = { LOWORD(lParam), HIWORD(lParam) }; return	pt; }

inline POINT& operator += ( POINT& lhs, POINT& rhs ) { lhs.x+=rhs.x; lhs.y+=rhs.y; return lhs; }
inline POINT& operator -= ( POINT& lhs, POINT& rhs ) { lhs.x-=rhs.x; lhs.y-=rhs.y; return lhs; }
inline POINT operator + ( POINT& lhs, POINT& rhs ) { return MAKEPOINT( lhs.x+rhs.x, lhs.y+rhs.y ); }
inline POINT operator - ( POINT& lhs, POINT& rhs ) { return MAKEPOINT( lhs.x-rhs.x, lhs.y-rhs.y ); }
inline POINT& operator *= ( POINT& pt, int i ) { pt.x*=i; pt.y*=i; return pt; }
inline POINT& operator /= ( POINT& pt, int i ) { pt.x/=i; pt.y/=i; return pt; }
inline POINT operator - ( POINT& pt ) { return MAKEPOINT( -pt.x, -pt.y ); }

inline bool operator == (const POINT& lhs, const POINT& rhs) { return (lhs.x == rhs.x && lhs.y == rhs.y); }
inline bool operator != (const POINT& lhs, const POINT& rhs) { return (lhs.x != rhs.x || lhs.y != rhs.y); }

// iBaseを原点として、右回りに90度回転
POINT	RotateRight90(POINT iBase, POINT iPoint);
// iBaseを原点として、左回りに90度回転
POINT	RotateLeft90(POINT iBase, POINT iPoint);
// iBaseを原点として、180度回転
POINT	Rotate180(POINT iBase, POINT iPoint);

//----------------------------------------------------------
// 長方形

// 矩形の座標逆転を修正する。終点ずれ考慮。
RECT NormalizeRect( RECT rc );
// ２点から矩形を作成する。逆転と終点ずれを考慮。
RECT PointToRect( POINT pt1, POINT pt2 );
// 矩形の一点を取得。終点ずれ考慮。
inline POINT LeftTop( RECT rc ) { return MAKEPOINT(rc.left,rc.top); }
inline POINT RightTop( RECT rc ) { return MAKEPOINT(rc.right-1,rc.top); }
inline POINT LeftBottom( RECT rc ) { return MAKEPOINT(rc.left,rc.bottom-1); }
inline POINT RightBottom( RECT rc ) { return MAKEPOINT(rc.right-1,rc.bottom-1); }
// 点が矩形上にあるかどうか。終点ずれ考慮。
inline BOOL isOnRect( RECT rc, POINT pt ) { return ( pt.x >= rc.left && pt.x < rc.right && pt.y >= rc.top && pt.y < rc.bottom ); }
// 矩形を移動
inline RECT& MoveRect( RECT& rc, POINT pt ) { rc.left+=pt.x; rc.top+=pt.y;  rc.right+=pt.x; rc.bottom+=pt.y; return rc; }

// operators
inline RECT& operator *= ( RECT& rc, int i ) { rc.left*=i; rc.top*=i; rc.right*=i; rc.bottom*=i; return rc; }
inline RECT& operator /= ( RECT& rc, int i ) { rc.left/=i; rc.top/=i; rc.right/=i; rc.bottom/=i; return rc; }

inline RECT MAKERECT( int l, int t, int r, int b ) { RECT rc={l,t,r,b}; return rc; }
inline RECT MAKERECT( POINT pt1, POINT pt2 ) { RECT rc={pt1.x,pt1.y,pt2.x,pt2.y}; return rc; }
inline RECT MAKERECT( POINT pt, SIZE sz ) { RECT rc={ pt.x, pt.y, pt.x+sz.cx, pt.y+sz.cy }; return rc; }
inline SIZE MAKESIZE( int cx, int cy ) { SIZE size={ cx, cy }; return size; }
inline SIZE RECTSIZE( RECT rc ) { SIZE size={ rc.right - rc.left, rc.bottom - rc.top }; return size; }
inline RECT SIZERECT( SIZE siz ) { RECT rc={ 0, 0, siz.cx, siz.cy }; return rc; }

// 長方形を移動
inline void Move( RECT* rc, int x, int y ) { assert(rc!=NULL); rc->left+=x; rc->top+=y; rc->right+=x; rc->bottom+=y; }
// 移動後の長方形を取得
inline RECT Move( RECT rc, int x, int y ) { return MAKERECT( rc.left+x, rc.top+y, rc.right+x, rc.bottom+y ); }
// 長方形の幅・高さを得る
inline int Width(const RECT& rc) { return rc.right - rc.left; }
inline int Height(const RECT& rc) { return rc.bottom - rc.top; }
// 点が長方形の中にあるか
inline BOOL isOn( const RECT& rect, int x, int y ) { return (x >= rect.left && x < rect.right &&	y >= rect.top && y < rect.bottom ); }
// 長方形は空（大きさを持っていない）か
inline BOOL isEmpty( const RECT& rc ) { return ( rc.right <= rc.left || rc.bottom <= rc.top ); }
// rcChildがrcParentに収まるよう調整
inline void Clip( const RECT& rcParent, RECT* rcChild ) {
	assert( rcChild != NULL );
	if ( rcChild->left < rcParent.left ) rcChild->left = rcParent.left;
	if ( rcChild->top < rcParent.top ) rcChild->top = rcParent.top;
	if ( rcChild->right > rcParent.right ) rcChild->right = rcParent.right;
	if ( rcChild->bottom > rcParent.bottom ) rcChild->bottom = rcParent.bottom;
}

#define	OpenRect(rc)	rc.left, rc.top, rc.right, rc.bottom
#define	OpenPoint(pt)	pt.x, pt.y

//----------------------------------------------------------
// ウィンドウ操作

// クライアント領域のサイズを変更する。
// さらに、ウィンドウ位置が画面内に収まるよう調整する。
BOOL	ResizeClientRect( HWND hWnd, int w, int h );
inline	BOOL	ResizeClientRect( HWND hWnd, SIZE siz ) { return ResizeClientRect(hWnd, siz.cx, siz.cy ); }

// クライアント長方形を取得
inline	RECT	GetClientRect(HWND iWnd) { RECT rc; ::GetClientRect(iWnd, &rc); return rc; }
// クライアント長方形をスクリーン座標で返す。
RECT	GetClientRectOnScreen( HWND hWnd );
// 座標変換 for RECT
RECT	ClientToScreen(HWND iWnd, RECT iRect);
RECT	ScreenToClient(HWND iWnd, RECT iRect);
// ウィンドウ位置座標を取得
RECT	GetWindowRect( HWND hWnd );
POINT	GetWindowPoint( HWND hWnd );
// ウィンドウの大きさを取得
inline SIZE	GetSize(HWND iWnd) { RECT rc=GetWindowRect(iWnd); SIZE siz={Width(rc), Height(rc)}; return siz; }


//----------------------------------------------------------
// 時間

// DWORD値を時分秒ミリ秒に分解
void	DwordToSystemTime( DWORD dw, SYSTEMTIME* pst );
// SYSTEMTIME構造体をDWORD値に変換（ただし49日まで）
void	SystemTimeToDword( const SYSTEMTIME*pst, DWORD pdw );

// SYSTEMTIME構造体同士を比較。新しい日付が「大きい」とする。
bool operator < ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs );
bool operator > ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs );
bool operator <= ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs );
bool operator >= ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs );
bool operator == ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs );
bool operator != ( const SYSTEMTIME& lhs, const SYSTEMTIME& rhs );

//----------------------------------------------------------
// 報告

// ウィンドウメッセージをOutputに表示
void	PutWindowMessage( UINT message, WPARAM wParam, LPARAM lParam );

// 可変引数対応TextOut
void	TextOutF( HDC hDC, int xPos, int yPos, char* format, ... );

// 可変引数対応のOutputDebugString
void	DbgStr( const char* format, ... );
// 可変引数対応の簡易報告用メッセージボックス
void	MesBox( const char* format, ... );
// 可変引数対応の簡易報告用SetWindowText
void	SetWinText( HWND hWnd, const char* format, ... );

#endif	// WIN32_H
