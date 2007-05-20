#include	"bregexp.h"


HINSTANCE BRegExp::mHandle = NULL;
BRegExpCache BRegExp::mBRegExpCache;


int (*BMatch)(PCSTR, PCSTR, PCSTR, BREGEXP**, char*);
int (*BSubst)(PCSTR, PCSTR, PCSTR, BREGEXP**, char*);
int (*BTrans)(PCSTR, PCSTR, PCSTR, BREGEXP**, char*);
int (*BSplit)(PCSTR, PCSTR, PCSTR, int, BREGEXP**, char*);
void (*BRegfree)(BREGEXP*);
char *(*BRegexpVersion)(void);





bool BRegExp::load(string iPath) 
{
	if ( mHandle!=NULL )
		return	true; // already loaded

	// iPathが空でなく、その末尾が \ でないなら
	if ( !iPath.empty() && iPath[iPath.size()-1] != DIR_CHAR ) 
		iPath += DIR_CHAR; //  \ を付与

	mHandle = ::LoadLibrary((iPath + "bregexp.dll").c_str());
	if ( mHandle==NULL )
		return	false;

	(void*&)BMatch = ::GetProcAddress(mHandle, "BMatch");
	(void*&)BSubst = ::GetProcAddress(mHandle, "BSubst");
	(void*&)BSplit = ::GetProcAddress(mHandle, "BSplit");
	(void*&)BRegfree = ::GetProcAddress(mHandle, "BRegfree");
	if ( BMatch==NULL || BSubst==NULL || BSplit==NULL || BRegfree==NULL )
		return	false;

	return	true;
}

void	BRegExp::unload() {
	if ( mHandle != NULL )
		::FreeLibrary(mHandle);
}

#if 0
string	BRegExp::get(int iIndex) {

	if ( pLastRegExp==NULL )
		throw out_of_range("BRegExp::get() / uninitialized");
	
	if (bLastSplit) {
	} 
	else if (bLastReplace) {
		// replace
		if (iIndex == 0) {
			if (pLastRegExp->outp && pLastRegExp->outendp)
				return	string(pLastRegExp->outp, pLastRegExp->outendp);
		} 
		else 
			if (iIndex <= pLastRegExp->nparens) {
				if (pLastRegExp->startp[iIndex] && pLastRegExp->endp[iIndex])
					return	string(pLastRegExp->startp[iIndex], pLastRegExp->endp[iIndex]);
			} else
				throw out_of_range("BRegExp::get() / replace");
	} 
	else {
		// match
		if (iIndex > pLastRegExp->nparens)
			throw out_of_range("BRegExp::get() / match");

		if (pLastRegExp->startp[iIndex] && pLastRegExp->endp[iIndex])
			return	string(pLastRegExp->startp[iIndex], pLastRegExp->endp[iIndex]);
	}
	return	string();
}
#endif

void	BRegExp::ready(const string& iTarget, const string& iRE, const string& iFunction) 
{
	if ( mHandle == NULL )
		return;

	if ( iTarget != "" ) {
		strLastString = iTarget;
		nLastPos = 0;

		if ( iRE != "" ) {
			strLastRegExp = iRE;
			pLastRegExp = mBRegExpCache.get(iFunction.c_str(), strLastRegExp.c_str());
			nLastPos = 0;
		}
	}

	char cDelim = '/';
	if (strLastRegExp[0] != '/') {
		static const char theOpenBrackets[] = "({[<";
		static const char theCloseBrackets[] = ")}]>"; // ？ これでいいのか？
		cDelim = strLastRegExp[1];
		if (char *p = strchr(theOpenBrackets, cDelim))
			cDelim = theCloseBrackets[p - theOpenBrackets];
	}

	int i;
	for (i = strLastRegExp.length() - 1; i >= 0; --i)
		if (strLastRegExp[i] == cDelim)
			break;

	bool bPreHankaku = false;
	bCountMatch = false;
	for (; strLastRegExp[i]; ++i) {
		switch (strLastRegExp[i]) {
			case 'h': bPreHankaku = true; break; // 事前に半角に変換しておく
			case 'g': bCountMatch = true; break; // matchでマッチ回数を数える
		}
	}
	if (bPreHankaku) {
		/*char *szTmp = new char[strLastString.length() + 1];
		tohankaku(szTmp, strLastString.c_str());
		strLastString = szTmp;
		delete[] szTmp;*/
	}
}




int	BRegExp::match(
		const string& iTarget,			// 対象文字列
		const string& iRE,				// 正規表現。/abc/ とか
//		vector<string>*	oResult)		// [0]は一致した全体、[1]以降は各カッコ。
		vector< pair<string, int> >* oResult)	// [0]は一致した全体、[1]以降は各カッコ。secondはiTarget中の開始位置
{
	int nCount = 0;
	char msg[80];

	if ( !iRE.empty() && iRE[0] == 's' ) {
		// replace
		ready(iTarget, iRE, "replace");
		if (pLastRegExp == NULL)
			return	0;

		nCount = BSubst(
			strLastRegExp.c_str(),
			strLastString.c_str() + nLastPos,
			strLastString.c_str() + strLastString.length(),
			&pLastRegExp,
			msg);

		if (nCount)
			nLastPos = pLastRegExp->endp[0] - strLastString.c_str();
		bLastSplit = false;
		bLastReplace = true;
	}
	else {
		// match
		ready(iTarget, iRE, "match");
		if (pLastRegExp == NULL)
			return	0;

		if (bCountMatch) {
			while (bool bMatch = BMatch(strLastRegExp.c_str(), strLastString.c_str() + nLastPos, strLastString.c_str() + strLastString.length(), &pLastRegExp, msg) == 1) {
				nLastPos = pLastRegExp->endp[0] - strLastString.c_str();
				++nCount;
			}

		} else {
			bool bMatch = BMatch(strLastRegExp.c_str(), strLastString.c_str() + nLastPos, strLastString.c_str() + strLastString.length(), &pLastRegExp, msg) == 1;
			if (bMatch) {
				nLastPos = pLastRegExp->endp[0] - strLastString.c_str();
				nCount = 1;

				if ( oResult != NULL )
				{
					/*
					vector<string>& v = *oResult;
					for (int i=0;i <= pLastRegExp->nparens;i++)
						v.push_back( string(pLastRegExp->startp[i], pLastRegExp->endp[i]) );
					*/
					vector< pair<string, int> >& v = *oResult;
					for (int i=0;i <= pLastRegExp->nparens;i++)
						v.push_back( pair<string, int>(
							string(pLastRegExp->startp[i], pLastRegExp->endp[i]),
							pLastRegExp->startp[i] - pLastRegExp->startp[0]
							));
				}
			}
		}
		bLastSplit = false;
		bLastReplace = false;
	}
	return	nCount;
}

/*
int		BRegExp::split(const string& iTarget, const string& iRE) 
{
	ready(iTarget, iRE, "split");
	if (pLastRegExp == NULL )
		return	0;
	
	char msg[80];
	int nCount = BSplit(strLastRegExp.c_str(), strLastString.c_str(), strLastString.c_str() + strLastString.length(), 0, &pLastRegExp, msg);
	bLastSplit = true;
	bLastReplace = false;

	// 結果の取得
		// split
		//if (iIndex >= pLastRegExp->splitctr)
		//	throw out_of_range("BRegExp::get() / split");
		//if (pLastRegExp->splitp[iIndex * 2] && pLastRegExp->splitp[iIndex * 2 + 1])
		//	return	string(pLastRegExp->splitp[iIndex * 2], pLastRegExp->splitp[iIndex * 2 + 1]);

	return	nCount;
}
*/