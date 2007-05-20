#include	<cassert>
#include	<string>
#include	<sstream>
using namespace std;
#include	"charset.h"
#include    <locale.h>

// 大文字小文字を問わず比較
static const char* stristr(const char* p, const char* substr) {
	assert(p!=NULL);
	for(;*p;++p) {
		for(int i=0;substr[i];++i)
			if (tolower(substr[i])!=tolower(p[i]))
				break;
		if(substr[i]=='\0')
			return	p;
	}
	return	NULL;
}


typedef	unsigned short	word;
typedef	unsigned char	byte;


word sjis2jis(word ch){
    byte leader  = (ch >> 8);
    byte trailer = (ch & 0xFF);

    if(leader <= 0x9F)  leader -= 0x71;
    else                leader -= 0xB1;
    leader = (leader << 1) + 1;

    if(trailer > 0x7F)  trailer --;
    if(trailer >= 0x9E){
        trailer -= 0x7D;
        leader ++;
    } else {
        trailer -= 0x1F;
    }
    return (leader << 8) | trailer;
}

word jis2sjis(word ch){
    byte leader  = (ch >> 8);
    byte trailer = (ch & 0xFF);

    if((leader & 0x01) != 0)    trailer += 0x1F;
    else                        trailer += 0x7D;
    if(trailer >= 0x7F)         trailer ++;

    leader = ((leader - 0x21) >> 1) + 0x81;
    if(leader > 0x9F)          leader += 0x40;
    return (leader << 8) | trailer;
}

inline word euc2jis(word ch){ return ch & 0x7F7F; }
inline word jis2euc(word ch){ return ch | 0x8080; }
inline word sjis2euc(word ch){ return jis2euc(sjis2jis(ch)); }
inline word euc2sjis(word ch){ return jis2sjis(euc2jis(ch)); }

#define JIS_ESC 27

string	jis2euc(const string& in)
{
	stringstream	out;
	const char* p=in.c_str();
	bool	escaped=false;
	while ( p[0]!='\0' )
	{
		if (0) {}
		else if ( p[0]==JIS_ESC && p[1]=='$' && p[2]=='@' ) { p+=3; escaped=true; }
		else if ( p[0]==JIS_ESC && p[1]=='$' && p[2]=='B' ) { p+=3; escaped=true; }
		else if ( p[0]==JIS_ESC && p[1]=='&' && p[2]=='@' ) { p+=3; escaped=true; }
		else if ( p[0]==JIS_ESC && p[1]=='$' && p[2]=='(' && p[3]=='D' ) { p+=4; escaped=true; }
		else if ( p[0]==JIS_ESC && p[1]=='(' && p[2]=='J' ) { p+=3; escaped=false; }
		else if ( p[0]==JIS_ESC && p[1]=='(' && p[2]=='H' ) { p+=3; escaped=false; }
		else if ( p[0]==JIS_ESC && p[1]=='(' && p[2]=='B' ) { p+=3; escaped=false; }
		else if ( p[0]==JIS_ESC && p[1]=='(' && p[2]=='I' ) { p+=3; escaped=false; }

		if ( escaped && p[1]!='\0' )
		{
			word	wd = (byte(p[0])<<8) + byte(p[1]);
			wd = jis2euc(wd);
			out.put(wd/256);
			out.put(wd%256);
			p+=2;
		}
		else
		{
			out.put(*p++);
		}
	}
	return	out.str();
}

string	euc2sjis(const string& in) {
	stringstream	out;
	const char* p=in.c_str();
	while ( p[0]!='\0' ) {
		if ( isascii(p[0]) || p[0]=='\n' || p[1]=='\0' )
			out.put(*p++);
		else {
			word	wd = (byte(p[0])<<8) + byte(p[1]);
			wd = euc2sjis(wd);
			out.put(wd/256);
			out.put(wd%256);
			p+=2;
		}
	}
	return	out.str();
}

string  sjis2euc(const string& in) {
	stringstream	out;
	const char* p=in.c_str();
	while ( p[0]!='\0' ) {
		if ( isascii(p[0]) || p[0]=='\n' || p[1]=='\0' )
			out.put(*p++);
		else {
			word	wd = (byte(p[0])<<8) + byte(p[1]);
			wd = sjis2euc(wd);
			out.put(wd/256);
			out.put(wd%256);
			p+=2;
		}
	}
	return	out.str();
}


string	jis2sjis(const string& in) {
	stringstream	out;
	const char* p=in.c_str();
	bool	escaped=false;


	while ( p[0]!='\0' ) {

		if (0) {}
		else if ( p[0]==0x1b && p[1]=='$' && p[2]=='@' ) { p+=3; escaped=true; }
		else if ( p[0]==0x1b && p[1]=='$' && p[2]=='B' ) { p+=3; escaped=true; }
		else if ( p[0]==0x1b && p[1]=='&' && p[2]=='@' ) { p+=3; escaped=true; }
		else if ( p[0]==0x1b && p[1]=='$' && p[2]=='B' ) { p+=3; escaped=true; }
		else if ( p[0]==0x1b && p[1]=='$' && p[2]=='(' && p[3]=='D' ) { p+=4; escaped=true; }
		else if ( p[0]==0x1b && p[1]=='(' && p[2]=='J' ) { p+=3; escaped=false; }
		else if ( p[0]==0x1b && p[1]=='(' && p[2]=='H' ) { p+=3; escaped=false; }
		else if ( p[0]==0x1b && p[1]=='(' && p[2]=='B' ) { p+=3; escaped=false; }
		else if ( p[0]==0x1b && p[1]=='(' && p[2]=='I' ) { p+=3; escaped=false; }

		if ( !escaped || p[1]=='\0' )
			out.put(*p++);
		else {
			word	wd = (byte(p[0])<<8) + byte(p[1]);
			wd = jis2sjis(wd);
			out.put(wd/256);
			out.put(wd%256);
			p+=2;
		}
	}
	return	out.str();
}





inline bool area(byte c, int min, int max) { return ( c>=min && c<=max ); }


inline int stricmp_head(const char* lhs, const char* rhs) {
	return _strnicmp(lhs, rhs, strlen(rhs));
}

// 文字コード自動判別
CharactorSet	getCharactorSet(const char* const iString) {

	// UTFのBOMによる判別。UTF-32には未対応。UTF-16では必須
	{
		unsigned char* bom = (unsigned char*)iString;
		if ( bom[0]==0xFE && bom[1]==0xFF )
			return	CS_UTF16BE;
		if ( bom[0]==0xFF && bom[1]==0xFE )
			return	CS_UTF16LE;
		if ( bom[0]==0xEF && bom[1]==0xBB && bom[2]==0xBF )
			return	CS_UTF8;
	}

//	_wstristr　UTF-16のcharset指定は読めない……

	// charset指定っぽい文字列があればそれを使う。ただし一番先頭にくるものを採用
	const char* enc;
	if ( 
		(enc=stristr(iString, "charset=")) != NULL ||
		(enc=stristr(iString, "encoding=")) != NULL
		) 
	{
		while ( isalpha(*enc) ) ++enc;
		while ( *enc=='=' || *enc=='\"' || *enc==' ' || *enc=='\'') ++enc;
		if ( stricmp_head(enc, "iso-2022-jp")==0 )
			return	CS_JIS;
		else if ( stricmp_head(enc, "shift_jis")==0 )
			return	CS_SJIS;
		else if ( stricmp_head(enc, "x-sjis")==0 )
			return	CS_SJIS;
		else if ( stricmp_head(enc, "x-euc-jp")==0 )
			return	CS_EUC;
		else if ( stricmp_head(enc, "euc-jp")==0 )
			return	CS_EUC;
		else if ( stricmp_head(enc, "utf")==0 ) // -8 じゃなくてもとりあえず。
			return	CS_UTF8;
	}

	// jisのエスケープシーケンスがあればjisに確定
	const char* p = iString;
	while (*p!=NULL) {
		if ( p[0]==0x1b && p[1]=='$' && p[2]=='@' ) return CS_JIS;
		else if ( p[0]==0x1b && p[1]=='$' && p[2]=='B' ) return CS_JIS;
		else if ( p[0]==0x1b && p[1]=='&' && p[2]=='@' ) return CS_JIS;
		else if ( p[0]==0x1b && p[1]=='$' && p[2]=='B' ) return CS_JIS;
		else if ( p[0]==0x1b && p[1]=='$' && p[2]=='(' && p[3]=='D' ) return CS_JIS;
		else if ( p[0]==0x1b && p[1]=='(' && p[2]=='J' ) return CS_JIS;
		else if ( p[0]==0x1b && p[1]=='(' && p[2]=='H' ) return CS_JIS;
		else if ( p[0]==0x1b && p[1]=='(' && p[2]=='B' ) return CS_JIS;
		else if ( p[0]==0x1b && p[1]=='(' && p[2]=='I' ) return CS_JIS;
		else if ( byte(p[0])>0xa0 && byte(p[0])<0xe0 ) break;	// jisには出現しないコード
		else ++p;
	}

	// eucかsjisかの確実な判別
	p = iString;
	while (*p!=NULL) {
		if ( isascii(p[0]) )
			++p;
		else if ( area(byte(p[0]),0x81,0x9f) )
			return	CS_SJIS;
		else if ( area(byte(p[0]),0xa1,0xdf) && ( isascii(p[1]) || area(byte(p[1]),0x80,0xa0) ) )
			return	CS_SJIS;
		else if ( area(byte(p[0]),0xf0,0xfe) )
			return	CS_EUC;
		else
			++p;
	}

	// eucかsjisか推測 → 無意味。確定できる文字が無いからここにきているのだ。
	/*int	n_sjis=0, n_euc=0;
	p = iString;
	while (*p!='\0') {
		int	n=1;
		if ( area(p[1],0x80,0xFC) && (area(p[0],0x81,0x9f) || area(p[0],0xe0,0xef)) ) {	// sjis
			++n_sjis;
			n=2;
		}
		if ( area(p[0],0xa1,0xfe) && area(p[1],0xa1,0xfe) )	{// euc
			++n_euc;
			n=2;
		}
		p+=n;
	}
	if ( n_sjis>n_euc )
		return	CS_SJIS;
	else if ( n_euc>n_sjis )
		return	CS_EUC;
	*/

	return	CS_NULL;	// わかんなかった
}



// 改行コードを \r\n に統一（未確認）
string	 unification_return_code_for_windows(const string& in) {
	stringstream	out;
	const char* p=in.c_str();
	while ( p[0]!='\0' ) {
		if ( p[0]=='\r' ) {
			if ( p[1]=='\n' ) {
				out.put(*p++);
				out.put(*p++);
			}
			else {
				out.put(*p++);
				out.put('\n');
			}
		}
		else if ( p[0]=='\n' ) {
			out.put('\r');
			out.put(*p++);
		}
		else
			out.put(*p++);
	}
	return	out.str();
}



// Shift-JISの１バイト目たりうる文字か？
inline bool is_sjis1st(byte c) { return ((unsigned int) (c ^ 0x20) - 0xa1 < 0x3c); }

// JISコードのエスケープ文字列
static const char* SHIFT2B = "\x1b\x24\x42"; // ２バイトへ
static const char* SHIFT1B = "\x1b\x28\x42"; // １バイトへ

string sjis2jis(const string& in)
{
	string out;
	bool shift = false;
	for(int i=0; i < in.size(); ++i)
	{
		byte c0 = in[i];
		byte c1 = in[i+1];

		if (is_sjis1st(c0))
		{
			// 2bytes
			if ( !shift )
			{
				out += SHIFT2B;
				shift = true;
			}
			++i;
			out += ((c0 - (c0 < 0xa0 ? 0x70 : 0xb0)) << 1) - (c1 < 0x9f);
			out += c1 - ((c1 < 0x9f) ? (c1 > 0x7f ? 0x20 : 0x1f) : 0x7e);
		}
		else
		{
			// ASCII
			if ( shift )
			{
				out += SHIFT1B;
				shift = false;
			}
			out += c0;
		}
	}

	// シフト状態のままなら戻しておく
	if ( shift )
	{
		out += SHIFT1B;
	}
	return out;
}

string euc2jis(const string& in)
{
	//return sjis2jis( euc2sjis(in) );

	string out;
	bool shift = false;
	for(int i=0; i < in.size(); ++i)
	{
		byte c0 = in[i];
		byte c1 = in[i+1];
		
		//if ( (c0 >= 0x00 && c0 <=0x7F) || c1 == '\0' ) // c0 >= 0x00 is always true
		if ( c0 <=0x7F || c1 == '\0' )
		{
			// ASCII
			if ( shift )
			{
				out += SHIFT1B;
				shift = false;
			}
			out += c0;
		}
		else
		{
			// 2bytes
			if ( !shift )
			{
				out += SHIFT2B;
				shift = true;
			}
			++i;

			// EUC半角カナをEUC全角カナに変換する（JISでは半角カナを扱えないため）
			if ( c0 == 0x8E && c1 >= 0xA0 && c1 <= 0xDF )
			{
				static const word euckana_han2zen[16*4] = {	0xA1A1,0xA1A3,0xA1D6,0xA1D7,0xA1A2,0xA1A6,0xA5F2,0xA5A1,0xA5A3,0xA5A5,0xA5A7,0xA5A9,0xA5E3,0xA5E5,0xA5E7,0xA5C3,0xA1BC,0xA5A2,0xA5A4,0xA5A6,0xA5A8,0xA5AA,0xA5AB,0xA5AD,0xA5AF,0xA5B1,0xA5B3,0xA5B5,0xA5B7,0xA5B9,0xA5BB,0xA5BD,0xA5BF,0xA5C1,0xA5C4,0xA5C6,0xA5C8,0xA5CA,0xA5CB,0xA5CC,0xA5CD,0xA5CE,0xA5CF,0xA5D2,0xA5D5,0xA5D8,0xA5DB,0xA5DE,0xA5DF,0xA5E0,0xA5E1,0xA5E2,0xA5E4,0xA5E6,0xA5E8,0xA5E9,0xA5EA,0xA5EB,0xA5EC,0xA5ED,0xA5EF,0xA5F3,0xA1AB,0xA1AC };
				word w = euckana_han2zen[c1 - 0xA0];
				c0 = w >> 8;
				c1 = w & 0x00ff;
			}

			out += c0 & 0x7F;
			out += c1 & 0x7F;
		}
	}

	// シフト状態のままなら戻しておく
	if ( shift )
	{
		out += SHIFT1B;
	}
	return out;


}



//
// http://kamoland.com/comp/unicode.htmlから拝借。
// 戻り値はfreeで解放しなければならない模様。
//

#include	<windows.h>

class CUnicodeF {
public:
    static char* utf8_to_sjis(const char *pUtf8Str, int *nBytesOut);
    static char* sjis_to_utf8(const char *pAnsiStr, int *nBytesOut);
    static wchar_t* sjis_to_utf16be(const char *pAnsiStr, int *nBytesOut);
    static char* utf16be_to_utf8(const wchar_t *pUcsStr, int *nBytesOut);
    static char* utf16be_to_sjis(const wchar_t *pUcsStr, int *nBytesOut);
    static wchar_t* utf8_to_utf16be(const char *pUtf8Str, int *nNumOut, BOOL bBigEndian);

private:
    static int utf16be_to_utf8_sub( char *pUtf8, const wchar_t *pUcs2, int nUcsNum, BOOL bCountOnly);
    static int utf8_to_utf16be_sub( wchar_t *pUcs2, const char *pUtf8, int nUtf8Num, BOOL bCountOnly, BOOL bBigEndian);
};

//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>	// for mbstowcs(), wcstombs()
//#include <locale.h>	// for setlocale()

char *CUnicodeF::utf8_to_sjis(const char *pUtf8Str, int *nBytesOut)
{
    int nNum, nBytes;

    wchar_t *pwcWork = utf8_to_utf16be( pUtf8Str, &nNum, TRUE);
    char *pcSjis = utf16be_to_sjis( pwcWork, &nBytes);
    free( pwcWork);

    *nBytesOut = nBytes;
    return pcSjis;
}

char *CUnicodeF::sjis_to_utf8(const char *pAnsiStr, int *nBytesOut)
{
    int nNum, nBytes;

    wchar_t *pwcWork = sjis_to_utf16be( pAnsiStr, &nNum);
    char *pcUtf8 = utf16be_to_utf8( pwcWork, &nBytes);
    free( pwcWork);

    *nBytesOut = nBytes;
    return pcUtf8;
}


char *CUnicodeF::utf16be_to_sjis(const wchar_t *pUcsStr, int *nBytesOut)
{
    char *pAnsiStr = NULL;
    int nLen;

    if (!pUcsStr) return NULL;

    setlocale(LC_ALL, "Japanese");// これがないとUnicodeに変換されない！

    nLen = wcslen( pUcsStr);

    if ( pUcsStr[0] == 0xfeff || pUcsStr[0] == 0xfffe) {
        pUcsStr++; // 先頭にBOM(byte Order Mark)があれば，スキップする
        nLen--;
    }

    pAnsiStr = (char *)calloc((nLen+1), sizeof(wchar_t));
    if (!pAnsiStr) return NULL;

    // 1文字ずつ変換する。
    // まとめて変換すると、変換不能文字への対応が困難なので
    int nRet, i, nMbpos = 0;
    char *pcMbchar = new char[MB_CUR_MAX];

    for ( i=0; i < nLen; i++) {
        nRet = wctomb( pcMbchar, pUcsStr[i]);
        switch ( nRet) {
        case 1:
            pAnsiStr[nMbpos++] = pcMbchar[0];
            break;

        case 2:
            pAnsiStr[nMbpos++] = pcMbchar[0];
            pAnsiStr[nMbpos++] = pcMbchar[1];
            break;

        default: // 変換不能
            pAnsiStr[nMbpos++] = ' ';
            break;
        }
    }
    pAnsiStr[nMbpos] = '\0';

    delete [] pcMbchar;

    *nBytesOut = nMbpos;

    return pAnsiStr;
}

wchar_t *CUnicodeF::sjis_to_utf16be(const char *pAnsiStr, int *nBytesOut)
{
    int len;
    wchar_t *pUcsStr = NULL;

    if (!pAnsiStr) return NULL;

    setlocale(LC_ALL, "Japanese");  // これがないとUnicodeに変換されない！

    len = strlen( pAnsiStr);
    *nBytesOut = sizeof(wchar_t)*(len);

    pUcsStr = (wchar_t *)calloc(*nBytesOut + 2, 1);
    if (!pUcsStr) return NULL;

    mbstowcs(pUcsStr, pAnsiStr, len+1);

    return pUcsStr;
}

char *CUnicodeF::utf16be_to_utf8(const wchar_t *pUcsStr, int *nBytesOut)
{
    int nUcsNum;
    char *pUtf8Str;

    nUcsNum = wcslen(pUcsStr);

    *nBytesOut = utf16be_to_utf8_sub( NULL, pUcsStr, nUcsNum, TRUE);

    pUtf8Str = (char *)calloc(*nBytesOut + 3, 1);
    utf16be_to_utf8_sub( pUtf8Str, pUcsStr, nUcsNum, FALSE);

    return pUtf8Str;
}

// Unicode(UTF-16) -> UTF-8 下請け
int CUnicodeF::utf16be_to_utf8_sub( char *pUtf8, const wchar_t *pUcs2, int nUcsNum, BOOL bCountOnly)
{
    int nUcs2, nUtf8 = 0;

    for ( nUcs2=0; nUcs2 < nUcsNum; nUcs2++) {
        if ( (unsigned short)pUcs2[nUcs2] <= 0x007f) {
            if ( bCountOnly == FALSE) {
                pUtf8[nUtf8] = (pUcs2[nUcs2] & 0x007f);
            }
            nUtf8 += 1;
        } else if ( (unsigned short)pUcs2[nUcs2] <= 0x07ff) {
            if ( bCountOnly == FALSE) {
                pUtf8[nUtf8] = ((pUcs2[nUcs2] & 0x07C0) >> 6 ) | 0xc0; // 2002.08.17 修正
                pUtf8[nUtf8+1] = (pUcs2[nUcs2] & 0x003f) | 0x80;
            }
            nUtf8 += 2;
        } else {
            if ( bCountOnly == FALSE) {
                pUtf8[nUtf8] = ((pUcs2[nUcs2] & 0xf000) >> 12) | 0xe0; // 2002.08.04 修正
                pUtf8[nUtf8+1] = ((pUcs2[nUcs2] & 0x0fc0) >> 6) | 0x80;
                pUtf8[nUtf8+2] = (pUcs2[nUcs2] & 0x003f) | 0x80;
            }
            nUtf8 += 3;
        }
    }
    if ( bCountOnly == FALSE) {
        pUtf8[nUtf8] = '\0';
    }

    return nUtf8;
}


wchar_t *CUnicodeF::utf8_to_utf16be(const char *pUtf8Str, int *nNumOut, BOOL bBigEndian)
{
    int nUtf8Num;
    wchar_t *pUcsStr;

    nUtf8Num = strlen(pUtf8Str); // UTF-8文字列には，'\0' がない
    *nNumOut = utf8_to_utf16be_sub( NULL, pUtf8Str, nUtf8Num, TRUE, bBigEndian);

    pUcsStr = (wchar_t *)calloc((*nNumOut + 1), sizeof(wchar_t));
    utf8_to_utf16be_sub( pUcsStr, pUtf8Str, nUtf8Num, FALSE, bBigEndian);

    return pUcsStr;
}

// UTF-8 -> Unicode(UCS-2) 下請け
int CUnicodeF::utf8_to_utf16be_sub( wchar_t *pUcs2, const char *pUtf8, int nUtf8Num,
                          BOOL bCountOnly, BOOL bBigEndian)
{
    int nUtf8, nUcs2 = 0;
    char cHigh, cLow;

    for ( nUtf8=0; nUtf8 < nUtf8Num;) {
        if ( ( pUtf8[nUtf8] & 0x80) == 0x00) { // 最上位ビット = 0
            if ( bCountOnly == FALSE) {
                pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x7f);
            }
            nUtf8 += 1;
        } else if ( ( pUtf8[nUtf8] & 0xe0) == 0xc0) { // 上位3ビット = 110
            if ( bCountOnly == FALSE) {
                pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x1f) << 6;
                pUcs2[nUcs2] |= ( pUtf8[nUtf8+1] & 0x3f);
            }
            nUtf8 += 2;
        } else {
            if ( bCountOnly == FALSE) {
                pUcs2[nUcs2] = ( pUtf8[nUtf8] & 0x0f) << 12;
                pUcs2[nUcs2] |= ( pUtf8[nUtf8+1] & 0x3f) << 6;
                pUcs2[nUcs2] |= ( pUtf8[nUtf8+2] & 0x3f);
            }
            nUtf8 += 3;
        }

        if ( bCountOnly == FALSE) {
            if ( !bBigEndian) {
                // リトルエンディアンにする処理
                cHigh = (pUcs2[nUcs2] & 0xff00) >> 8;
                cLow = (pUcs2[nUcs2] & 0x00ff);
                pUcs2[nUcs2] = (cLow << 8) | cHigh;
            }
        }

        nUcs2 += 1;
    }
    if ( bCountOnly == FALSE) {
        pUcs2[nUcs2] = L'\0';
    }

    return nUcs2;
}


string	UTF8toSJIS(const string& str) {
	// BOMスキップ。UTF-16についてはライブラリでやってくれてるみたい。
	const char* p = str.c_str();
	if ( p[0]==0xEF && p[1]==0xBB && p[2]==0xBF )
		p += 3;

	int	count;
	char* buf = CUnicodeF::utf8_to_sjis(p, &count);
	string	ret(buf, count);
	free(buf);
	return	ret;
}

string	UTF16BEtoSJIS(const wchar_t* mbp) {
	int	count;
	char* buf = CUnicodeF::utf16be_to_sjis(mbp, &count);
	string	ret(buf, count);
	free(buf);
	return	ret;
}

string	UTF16LEtoSJIS(const wchar_t* mbp) {

	// エンディアンを変更
/*    setlocale(LC_ALL, "Japanese");// これがないとUnicodeに変換されない！
    int len = wcslen(mbp);

	int n=0;
	for ( char* p = (char*)mbp ; n<len ; p+=2, ++n ) {
		char temp = p[0];
		p[0] = p[1];
		p[1] = temp;
	}
*/
	int	count;
	char* buf = CUnicodeF::utf16be_to_sjis(mbp, &count);
	string	ret(buf, count);
	free(buf);
	return	ret;
}
