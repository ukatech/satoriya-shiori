#ifndef	TEXTPOSITIONMANAGER_H
#define	TEXTPOSITIONMANAGER_H

#include	"../Canvas/Shape.h"

class TextPositionManager {

	CPoint	mBasePoint;	// 描画範囲の右上座標
	CSize	mSize;		// 描画範囲の広さ
	CSize	mFormat;	// 描画可能文字数（範囲に対しての分母）

	CPoint	mPosition;	// 現在の描画位置

public:
	TextPositionManager();
	~TextPositionManager();

	// 表示域を設定
	void	BasePoint(int iX, int iY);
	void	Size(int iWidth, int iHeight);
	void	Area(int iL, int iT, int iR, int iB) { BasePoint(iL,iT); Size(iR-iL, iB-iT); }
	void	Margin(CRect iOut, int iL, int iT, int iR, int iB) { Area(iOut.l+iL, iOut.t+iT, iOut.r-iR, iOut.b-iB); }
	// 表示字数を設定
	void	Format(int iWidth, int iHeight);

	// 位置を原点に戻す
	void	Home();		
	// 位置を改行する
	void	Return();	
	// 原点にいる？
	bool	isHome() { return mPosition.x==0 && mPosition.y==0; }

	// 行末と禁則を考慮して、改行処理。
	void	Before(const char* p);
	// 次の文字を表示すべき位置を返す。
	// 表示時に使うフォント（や記号等）の大きさを引数として渡す。
	CPoint	Position(CSize iFontSize);
	// 文字位置をインクリメント。
	int		After(const char* p);
};

#endif	//	TEXTPOSITIONMANAGER_H
