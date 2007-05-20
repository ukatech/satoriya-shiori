#include	"Font.h"
#include	<assert.h>

Font::Font() {
	init();
	mChanged=true;	// １回目は必ず作る
	mFont=NULL;
}

Font::~Font() {
	if ( mFont != NULL )
		::DeleteObject(mFont);
}

void	Font::init() {
	Bold(false);
	Italic(false);
	Strike(false);
	Underline(false);
	Color(255,255,255);
	Size(24);
	Face("ＭＳ ゴシック");
}


void	Font::Bold(bool iBold) {
	if ( mBold==iBold )
		return;
	mBold=iBold;
	mChanged=true;
}
void	Font::Italic(bool iItalic) {
	if ( mItalic==iItalic )
		return;
	mItalic=iItalic;
	mChanged=true;
}
void	Font::Strike(bool iStrike) {
	if ( mStrike==iStrike )
		return;
	mStrike=iStrike;
	mChanged=true;
}
void	Font::Underline(bool iUnderline) {
	if ( mUnderline==iUnderline )
		return;
	mUnderline=iUnderline;
	mChanged=true;
}

void	Font::Color(COLORREF iColor) {
	if ( mColor==iColor )
		return;
	mColor=iColor;
}
void	Font::Width(int iWidth) {
	if ( mSize.cx==iWidth/2 )
		return;
	mSize.cx=iWidth/2;
	mChanged=true;
}
void	Font::Height(int iHeight) {
	if ( mSize.cy==iHeight )
		return;
	mSize.cy=iHeight;
	mChanged=true;
}
void	Font::Size(int iSize) {
	if ( mSize.cx==iSize/2 && mSize.cy==iSize )
		return;
	mSize.cx=iSize/2;
	mSize.cy=iSize;
	mChanged=true;
}
void	Font::Size(SIZE iSize) {
	if ( mSize.cx==iSize.cx && mSize.cy==iSize.cy )
		return;
	mSize=iSize;
	mChanged=true;
}
void	Font::Face(LPCTSTR iFace) {
	assert(lstrlen(iFace)<LF_FACESIZE);
	if ( lstrcmp(mFace, iFace)==0 )
		return;
	lstrcpy(mFace, iFace);
	mChanged=true;
}

bool	Font::update() {

	if ( !mChanged )
		return	true;	// 変更されて無い

	// 変更されたので作り直し。
	if ( mFont != NULL )
		::DeleteObject(mFont);

	LOGFONT	theLogFont;
	::ZeroMemory(&theLogFont, sizeof(LOGFONT));
	theLogFont.lfWidth = mSize.cx;// フォント表示域の横幅
	theLogFont.lfHeight = mSize.cy;	// フォント表示域の高さ
	theLogFont.lfEscapement = 0;	// 縦傾き
	theLogFont.lfOrientation = 0;	// 横傾き
	theLogFont.lfWeight = mBold ? FW_BOLD : 0;	// フォントの太さ（ 0の場合はデフォルト）
	theLogFont.lfItalic = mItalic;	// 斜体指定
	theLogFont.lfUnderline = mUnderline;	// 下線指定
	theLogFont.lfStrikeOut = mStrike;	// 取り消し線指定
	theLogFont.lfCharSet = SHIFTJIS_CHARSET;		// キャラクタセット
	theLogFont.lfOutPrecision = OUT_DEFAULT_PRECIS;	// 出力精度
	theLogFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;// クリッピング方法
	theLogFont.lfQuality = DEFAULT_QUALITY;			// 外観のクオリティ
	theLogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_ROMAN;	// ピッチと種類
	::lstrcpy(theLogFont.lfFaceName, mFace);
	mFont = ::CreateFontIndirect(&theLogFont);
	if ( mFont == NULL )
		return	false;

	mChanged = false;
	return	true;
}



