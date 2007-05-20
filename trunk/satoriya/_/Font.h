#ifndef	FONT_H
#define	FONT_H

#include	<windows.h>

class Font {

	// フォントの各種設定
	bool		mBold, mItalic, mStrike, mUnderline;
	COLORREF	mColor;
	SIZE		mSize;
	TCHAR		mFace[LF_FACESIZE];

	// フォント情報の変更有無。
	// 描画時、変更があればmFontを作り直す。
	bool		mChanged;

	// 作成済みフォントのハンドル
	HFONT		mFont;	
	
public:
	Font();
	~Font();
	void	init();		// 設定を初期状態に戻す
	// 設定を反映してmFontを最新状態にする。TextOut前には忘れずに実行しましょう。
	bool	update();	
	void	del() { if (mFont!=NULL) ::DeleteObject(mFont); }
	HFONT	getFont() { return mFont; }
	COLORREF	getColor() const { return mColor; }

	bool	Bold() const { return mBold; }
	bool	Italic() const { return mItalic; }
	bool	Strike() const { return mStrike; }
	bool	Underline() const { return mUnderline; }
	SIZE	Size() const { return mSize; }
	const char*	Face() const { return mFace; }

	void	Bold(bool iBold);
	void	Italic(bool iItalic);
	void	Strike(bool iStrike);
	void	Underline(bool iUnderline);
	void	Color(COLORREF iColor);
	void	Color(int iR, int iG, int iB) { Color(RGB(iR,iG,iB)); }
	void	Size(int iSize);
	void	Size(SIZE iSize);
	void	Height(int iHeight);
	void	Width(int iWidth);
	void	Face(LPCTSTR iFace);
};

#endif	//	FONT_H
