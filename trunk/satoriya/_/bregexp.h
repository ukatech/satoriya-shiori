#pragma warning( disable : 4786 ) //「デバッグ情報内での識別子切捨て」
#pragma warning( disable : 4503 ) //「装飾された名前の長さが限界を越えました。名前は切り捨てられます。」
#ifndef for
#define for if(0);else for
#endif	// for

#include <string>
#include <map>
#include	<vector>
#include	<windows.h>
using namespace std;

typedef struct bregexp {
  const char *outp;    // BSubst 置換データの先頭ポインタ
  const char *outendp; // BSubst 置換データの最終ポインタ+1
  const int  splitctr; // BSplit 配列数
  const char **splitp; // BSplit データポインタ
  int  rsv1;           // リザーブ　自由に使用可能 
  char *parap;         // パターンデータポインタ
  char *paraendp;      // パターンデータポインタ+1
  char *transtblp;     // BTrans 変換テーブルポインタ
  char **startp;       // マッチしたデータの先頭ポインタ
  char **endp;         // マッチしたデータの最終ポインタ+1
  int nparens;         // パターンの中の() の数。 $1,$2, を調べるときに使用
} BREGEXP;

typedef const char* PCSTR;


extern int (*BMatch)(PCSTR, PCSTR, PCSTR, BREGEXP**, char*);
extern int (*BSubst)(PCSTR, PCSTR, PCSTR, BREGEXP**, char*);
extern int (*BTrans)(PCSTR, PCSTR, PCSTR, BREGEXP**, char*);
extern int (*BSplit)(PCSTR, PCSTR, PCSTR, int, BREGEXP**, char*);
extern void (*BRegfree)(BREGEXP*);
extern char *(*BRegexpVersion)(void);



class BRegExpCache {
private:
	map<string, BREGEXP*> mCache;

public:
	~BRegExpCache() {
		for ( map<string, BREGEXP*>::iterator i=mCache.begin() ; i!=mCache.end() ; ++i )
			if ( i->second != NULL )
				BRegfree(i->second);
	}

	BREGEXP*	get(const char *szKind, const char *szReg) {

		// キャッシュを検索
		map<string, BREGEXP*>::iterator i = mCache.find(szReg);
		if ( i != mCache.end() )
			return	i->second; // 見つかった。それを使う。

		// 新規にコンパイル
		BREGEXP*	pBRegExp = NULL;
		char msg[80], p[] = " ";
		if (stricmp(szKind, "match") == 0)
			BMatch(szReg, p, p + 1, &pBRegExp, msg);
		else if (stricmp(szKind, "replace") == 0)
			BSubst(szReg, p, p + 1, &pBRegExp, msg);
		else
			BSplit(szReg, p, p + 1, 0, &pBRegExp, msg);

		if ( pBRegExp != NULL && pBRegExp->parap != NULL )
			mCache[string(pBRegExp->parap, pBRegExp->paraendp)] = pBRegExp;	// 結果をキャッシュに追加

		return	pBRegExp;
	}
};




class	BRegExp {
public:
	// bregexp.dllをロード/アンロードする
	static bool load(string iPath="");
	static bool isloaded() { return mHandle != NULL; }
	static void unload();

	int // 一致有無0/1。gオプション付きなら一致数。
	match(
		const string& iTarget,			// 対象文字列
		const string& iRE,				// 正規表現。/abc/ とか
		//vector<string>*	oResult=NULL);	// [0]は一致した全体、[1]以降は各カッコ。
		vector< pair<string, int> >* oResult=NULL);	// [0]は一致した全体、[1]以降は各カッコ。secondはiTarget中の開始位置

	int // 一致有無0/1。gオプション付きなら一致数。
	replace(
		const string& iTarget,			// 対象文字列
		const string& iRE,				// 正規表現 s/abc/あいう/ とか
		vector<string>*	oResult = NULL,	// [0]は一致した全体、[1]以降は各カッコ。
		string*	oReplaced = NULL);		// 置換後文字列。

	int // 分割数。一致数より1多い。
	split(
		const string& iTarget,	 // 対象文字列
		const string& iRE,		 // 正規表現 /,/ とか /\t/ とか
		vector<string>*	oResult);// 分割結果

private:
	static HINSTANCE mHandle;
	static BRegExpCache mBRegExpCache;

// ここより下は要再考……

	string strLastString;
	string strLastRegExp;
	BREGEXP *pLastRegExp;
	int nLastPos;
	bool bLastSplit;
	bool bLastReplace;
	bool bCountMatch;

	void	ready(const string& iTarget, const string& iRE, const string& iFunction);
public:
	BRegExp() : pLastRegExp(NULL) {}

};

