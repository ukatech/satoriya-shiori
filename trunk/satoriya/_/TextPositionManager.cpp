#include	"TextPositionManager.h"
#include	<assert.h>
#include	<mbctype.h>	// for _ismbblead,_ismbbtrail
#include	<string.h>	// for NULL

TextPositionManager::TextPositionManager() {
}

TextPositionManager::~TextPositionManager() {
}

void	TextPositionManager::BasePoint(int iX, int iY) {
	mBasePoint = CPoint(iX, iY);
}

void	TextPositionManager::Size(int iWidth, int iHeight) {
	mSize = CSize(iWidth, iHeight);
}

void	TextPositionManager::Format(int iWidth, int iHeight) {
	mFormat = CSize(iWidth, iHeight);
}

void	TextPositionManager::Return() {
	mPosition.x = 0;
	mPosition.y++;
}

void	TextPositionManager::Home() {
	mPosition.x = 0;
	mPosition.y = 0;
}


// 行末と禁則を考慮して、改行処理。
void	TextPositionManager::Before(const char* p) {
	int	len = _ismbblead(*p) ? 2 : 1;

	const char kinsoku[] = "！？。、";
	if ( len == 2 )
		for ( const char* pk=kinsoku ; *pk!='\0' ; pk+=2 )
			if ( strncmp(p, pk, 2) == 0 )
				return;

	if ( mPosition.x >= mFormat.w-2 )	// -2 は禁則予定部分
		Return();
}

// 次の文字を表示すべき位置を返す。
// 表示時に使うフォント（や記号等）の大きさを引数として渡す。
CPoint	TextPositionManager::Position(CSize iFontSize) {

	assert(mFormat.w > 0);
	assert(mFormat.h > 0);

	// 一文字の割り当て領域
	double oneW = mSize.w / (double)mFormat.w;
	double oneH = mSize.h / (double)mFormat.h;

	//             基点　　　　　表示範囲の中央点　　　　フォントサイズ
	int	x = int(mBasePoint.x + (mPosition.x+1)*oneW - iFontSize.w);
	int	y = int(mBasePoint.y + (mPosition.y+0.5)*oneH - iFontSize.h/2.0);

	return	CPoint(x, y);
}

// 文字位置をインクリメント。
int		TextPositionManager::After(const char* p) {
	int	len = _ismbblead(*p) ? 2 : 1;
	mPosition.x += len;
	return	len;
}

