#include	"Parameter.h"
#include	<fstream>
#include	<strstream>
#include	<string>
using namespace std;


Parameter::Parameter() : mData() {
	clear(); 
}
Parameter::~Parameter() {
	mData.clear(); 
}
void	Parameter::clear() { 
	mData.clear(); 
	mData[""] = strmap();
	mSection = &(mData[""]);
}

// ファイル読み
bool	Parameter::loadFile(const char* iFileName) {

	ifstream	in(iFileName);
	if ( !in.is_open() )
		return	false;
	clear();

	while ( in.peek() != EOF ) {
		// １行読み込み
		strstream	line;
		int	c;
		while ( (c=in.get()) != '\n' && c!=EOF)
			line.put(c);
		line.put('\0');

		// 行ストリームを固定、各行に対し処理
		processLine(line.str());
		// 行ストリームの固定を解除
		line.rdbuf()->freeze(0);
	}
	in.close();

	// 無名セクションを選んでおく
	selectSection("");
	return	true;
}

// デリミタ飛ばし
static inline void skipDelimiter(const char*& p) {
	while ( *p==' ' || *p=='\t' || *p==',' )
		++p;
}
// デリミタ飛ばし、文字列読み込み、デリミタ飛ばし
static inline string readString(const char*& p) {
	skipDelimiter(p);
	strstream	word;
	while ( *p!=' ' && *p!='\t' && *p!=',' && *p!='\0')
		word.put(*p++);
	word.put('\0');
	string	str = word.str();
	word.rdbuf()->freeze(0);
	skipDelimiter(p);
	return	str;
}

// 読み込んだファイルの各行を処理
void	Parameter::processLine(char* p) {

	// コメント ; 以降は消去
	char*	comment_mark = strchr(p, ';');
	if ( comment_mark != NULL )
		*comment_mark = '\0';

	// 空行チェック
	skipDelimiter(p);
	if ( *p=='\0' )
		return;	

	// 先頭の単語を取得
	string	theFront = readString(p);

	if ( *p=='\0' ) {
		// 一単語だけなら「セクション名」
		map<string, strmap>::iterator i = mData.find(theFront);
		if ( i == mData.end() ) {
			mData[theFront] = strmap();
			i = mData.find(theFront);
		}
		mSection = &(i->second);
	}
	else {
		// 複数の単語があれば、先頭単語をキー、以降を値とする
		(*mSection)[theFront] = p;
	}
}

// セクション選んで
bool	Parameter::selectSection(const char* iSection) {
	assert(iSection != NULL);
	map<string, strmap>::iterator i = mData.find(iSection);
	if ( i == mData.end() )
		return	false;	// セクションが存在しない

	mSection = &(i->second);
	return	true;
}


// 値を取得
#define	get_value_common	\
	assert(iKey != NULL);	\
	strmap::iterator i = mSection->find(iKey);	\
	if ( i==mSection->end() )	\
		return	false;	\
	else NULL \

#define	null_false	if ( *p=='\0' ) return false; else NULL

bool	Parameter::getValue(const char* iKey, int& oInt) {
	get_value_common;
	oInt = atoi(i->second.c_str());
	return	true;
}

bool	Parameter::getValue(const char* iKey, string& oString) {
	get_value_common;
	oString = i->second;
	return	true;
}

bool	Parameter::getValue(const char* iKey, CRect& oRect) {
	get_value_common;
	const char* p = i->second.c_str();
	oRect.l = atoi(readString(p).c_str()); null_false;
	oRect.t = atoi(readString(p).c_str()); null_false;
	oRect.r = atoi(readString(p).c_str()); null_false;
	oRect.b = atoi(readString(p).c_str());
	return	true;
}

bool	Parameter::getValue(const char* iKey, CSize& oSize) {
	get_value_common;
	const char* p = i->second.c_str();
	oSize.w = atoi(readString(p).c_str()); null_false;
	oSize.h = atoi(readString(p).c_str());
	return	true;
}

bool	Parameter::getValue(const char* iKey, CPoint& oPoint) {
	get_value_common;
	const char* p = i->second.c_str();
	oPoint.x = atoi(readString(p).c_str()); null_false;
	oPoint.y = atoi(readString(p).c_str());
	return	true;
}

bool	Parameter::getValue(const char* iKey, Color24& oColor) {
	get_value_common;
	const char* p = i->second.c_str();
	oColor.R(atoi(readString(p).c_str())); null_false;
	oColor.G(atoi(readString(p).c_str())); null_false;
	oColor.B(atoi(readString(p).c_str()));
	return	true;
}
