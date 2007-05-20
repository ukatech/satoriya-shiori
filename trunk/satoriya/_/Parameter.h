#ifndef	PARAMETER_H
#define	PARAMETER_H


/*-----------------------------------------------
	パラメータファイルアクセサ Parameter
	
パラメータファイルの書式：
・デリミタはspace, comma, tab。
・空行は無視
・一単語の行はセクション名
・colon以降行末までコメント

例：

たこ
いち		100, 100
おおきさ	64,80
ぱたーん	0,0,640, 480	; たこさんは大きめなの。
ながさ		2

いか
いち		100, 100
おおきさ	64,80
ぱたーん	0,0,640, 480
ながさ		2

-----------------------------------------------*/

#pragma warning( disable : 4786 ) // 「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」
#include	<map>
#include	<string>
using namespace std;

#include	"../Canvas/Shape.h"
#include	"../Canvas/Color24.h"

typedef	map<string, string>	strmap;

class	Parameter {

	map<string, strmap>	mData;
	strmap*				mSection;

	// 一行に対して処理。loadFileの補助関数。
	void	processLine(char* iLine);

public:
	Parameter();
	~Parameter();
	void	clear();

	// ファイル読んで
	bool	loadFile(const char* iFileName);
	// セクション選んで
	bool	selectSection(const char* iSection);
	// 値を取得
	bool	getValue(const char* iKey, int& oInt);
	bool	getValue(const char* iKey, string& oString);
	bool	getValue(const char* iKey, CRect& oRect);
	bool	getValue(const char* iKey, CSize& oSize);
	bool	getValue(const char* iKey, CPoint& oPoint);
	bool	getValue(const char* iKey, Color24& oColor);
};




#endif	//	PARAMETER_H
