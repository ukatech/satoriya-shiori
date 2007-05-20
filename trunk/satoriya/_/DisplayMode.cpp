#include	"DisplayMode.h"

// 指定の解像度・色数に一致するDevModeを検索、
// 存在すればoDevModeに格納して true を返す。見つからなければ false を返す。
static BOOL	GetDevMode( DEVMODE* oDevMode, DWORD iX, DWORD iY, DWORD iBits ) {
	DEVMODE	theDevMode;
	for (int i=0;
		 ::EnumDisplaySettings(NULL, i, &theDevMode);
		 i++ ) {
		if ( theDevMode.dmBitsPerPel == iBits &&
			theDevMode.dmPelsWidth == iX &&
			theDevMode.dmPelsHeight == iY ) {
			*oDevMode = theDevMode;
			return	TRUE;
		}
	}
	return	FALSE;
}

BOOL	DisplayMode::mChangedFlag = FALSE;
DWORD	DisplayMode::mWidth = 0;
DWORD	DisplayMode::mHeight = 0;
DWORD	DisplayMode::mBits = 0;

// 画面モードを変更
BOOL	DisplayMode::Change(DWORD iX, DWORD iY, DWORD iBits) {
	if ( !mChangedFlag )
		Get(&mWidth, &mHeight, &mBits);	// 画面モードの初期状態を保存

	// DevModeを取得
	DEVMODE	theDevMode;
	if ( !GetDevMode( &theDevMode, iX, iY, iBits ) )
		return	FALSE;	// 指定の画面モードが存在しない

	// 画面モードを変更
	OutputDebugString("DisplayMode::Change -- 画面モードを切り換えます。\n");
	::ChangeDisplaySettings(&theDevMode, 0 );// CDS_UPDATEREGISTRY);
	OutputDebugString("DisplayMode::Change -- 画面モードを切り換えました。\n");
	mChangedFlag = TRUE;
	return	TRUE;
}

// 初期状態の画面モードに復帰
void	DisplayMode::Restore() {
	if ( !mChangedFlag )
		return;	// 画面モードは変更されていない

	DEVMODE	theDevMode;
	GetDevMode( &theDevMode, mWidth, mHeight, mBits );
	OutputDebugString("DisplayMode::Restore -- 画面モードを元に戻します。\n");
	::ChangeDisplaySettings(&theDevMode, 0 );// CDS_UPDATEREGISTRY);
	OutputDebugString("DisplayMode::Restore -- 画面モードを元に戻しました。\n");
	mChangedFlag = FALSE;
}

// 現在の画面モードを取得
void	DisplayMode::Get(LPDWORD oX, LPDWORD oY, LPDWORD oBits) {
	if ( oBits != NULL ) {
		HDC	hDC = ::GetDC(NULL);
		*oBits = ::GetDeviceCaps(hDC, BITSPIXEL);
		::ReleaseDC(NULL, hDC);
	}
	if ( oX != NULL )
		*oX = ::GetSystemMetrics(SM_CXSCREEN);
	if ( oY != NULL )
		*oY = ::GetSystemMetrics(SM_CYSCREEN);
}


int		DisplayMode::GetFullColorBits(int iWidth, int iHeight) {

	int	bpp=0;
	DEVMODE	theDevMode;
	for (int i=0 ; ::EnumDisplaySettings(NULL, i, &theDevMode); i++ ) {
		if ( theDevMode.dmPelsWidth != (unsigned)iWidth ) continue;
		if ( theDevMode.dmPelsHeight != (unsigned)iHeight ) continue;

		if ( theDevMode.dmBitsPerPel==24 ) {
			return	24;	// もうこれで決定
		}
		else if ( theDevMode.dmBitsPerPel==32 ) {
			bpp=32;
		}
		else if ( theDevMode.dmBitsPerPel==16 && bpp!=32 ) {
			bpp=16;
		}
		else if ( theDevMode.dmBitsPerPel==8 && bpp==0 ) {
			bpp=8;
		}
	}
	return	bpp;
}


