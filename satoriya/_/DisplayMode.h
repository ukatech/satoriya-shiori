#ifndef	DISPLAYMODE_H
#define	DISPLAYMODE_H

#include	<windows.h>

class	DisplayMode {
	DisplayMode();				// インスタンス生成を禁止

public:
	// 画面モードを変更
	static	BOOL	Change(DWORD iWidth, DWORD iHeight, DWORD iBits);
	// Changeによって変更され、Restoreによって戻されていなければTRUE
	static	BOOL	isChanged() { return mChangedFlag; }
	// 初期状態の画面モードに復帰
	static	void	Restore();
	// 現在の画面モードを取得
	static	void	Get(LPDWORD oWidth, LPDWORD oHeight, LPDWORD oBits);
	
	// 24->32->16->8 の順で存在する画面モードを検索、ビット数を返す。
	// いずれも存在しなければ 0 を返す。
	static	int		GetFullColorBits(int iWidth, int iHeight);
	
private:
	// 一度でも変更が為されたか
	static	BOOL	mChangedFlag;
	// 初回呼び出し前の画面モードを記憶
	static	DWORD	mWidth,mHeight,mBits;
};

#endif	//	DISPLAYMODE_H
