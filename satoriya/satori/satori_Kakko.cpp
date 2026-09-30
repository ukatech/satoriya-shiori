#include	"satori.h"
#include	"../_/Utilities.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


void	add_characters(const wchar_t* p, int& chars_spoken) {
	// さくらスクリプトとそれ以外を分割して処理を加える
	while (*p) {
		if (*p==L'\\'||*p==L'%') {
			++p;
			if (*p==L'\\'||*p==L'%')	// エスケープされた\, %
				continue;
			while ( *p < 0x80 && (iswalpha(*p)||iswdigit(*p)||*p==L'!'||*p==L'*'||*p==L'&'||*p==L'?'||*p==L'_') )
				++p;
			if (*p==L'[') {
				p += 1;
				while ( *p && *p!=L']' ) {
					if (p[0]==L'\\' && p[1]==L']') {	// エスケープされた]
						p += 2;
					}
					else {
						get_a_chr(p);
					}
				}
			}
		}
		else {
			get_a_chr(p);
			chars_spoken += 1;
		}
	}
}


// R・H・A・S・Cを判定
bool Satori::IsArrayValue(const wstring &iName,int &ref,wchar_t &firstChar)
{
	if ( ((iName[0]==L'R' || iName[0]==L'H' || iName[0]==L'A' || iName[0]==L'S' || iName[0]==L'C') && iName.size() >= 2) ||
		((iName[0]==L'Ｒ' || iName[0]==L'Ｈ' || iName[0]==L'Ａ' || iName[0]==L'Ｓ' || iName[0]==L'Ｃ') && iName.size() >= 2) ) {

		wstring hankaku=zen2han(iName);

		if ( aredigits(hankaku.c_str()+1) ) {
			firstChar = hankaku[0];
			ref = stoi_internal(hankaku.c_str()+1);
			return true;
		}
	}
	return false;
}


// 変数取得
wstring* Satori::GetValue(const wstring &iName,bool &oIsSysValue,bool iIsExpand,bool *oIsExpanded,const wchar_t *pDefault)
{
	if ( oIsExpanded ) { *oIsExpanded = false; }
	oIsSysValue = false;

	int ref;
	wchar_t firstChar;

	if ( IsArrayValue(iName,ref,firstChar) ) {
		if ( firstChar==L'R' ) {
			oIsSysValue = true;

			// Event通知時の引数取得
			if (ref>=0 && ref<mReferences.size()) {
				return &(mReferences[ref]);
			}
			else {
				if ( iIsExpand && ref >= 0 ) {
					if ( ref >= MAX_ARRAY_INDEX ) {
						GetSender().sender() << L"＄" << iName << L"　添字が大きすぎます（" << MAX_ARRAY_INDEX << L"未満にしてください）。" << std::endl;
						return NULL;
					}
					mReferences.resize(ref+1);
					if ( oIsExpanded ) { *oIsExpanded = true; }
					return &(mReferences[ref]);
				}
				return NULL;
			}
		}
		else if ( firstChar==L'H' ) {
			oIsSysValue = true;

			// 過去の置き換え履歴を参照
			if ( kakko_replace_history.empty() ) { return NULL; }

			strvec&	khr = kakko_replace_history.top();

			ref -= 1; //いっこまえでないと履歴にならん！

			if ( ref>=0 && ref < khr.size() ) {
				return &(khr[ref]);
			}
			else {
				return NULL;
			}
		}
		else if ( firstChar==L'A' ) {
			oIsSysValue = true;

			if ( mCallStack.empty() ) { return NULL; }

			// callによる呼び出しの引数を参照S
			strvec&	v = mCallStack.top();
			if ( ref >= 0 && ref < v.size() ) {
				return &(v[ref]);
			}
			else {
				return NULL;
			}
		}
		else if ( firstChar==L'S' ) {
			oIsSysValue = true;

			// SAORIなどコール時の結果処理
			if (ref>=0 && ref<mKakkoCallResults.size()) {
				return &(mKakkoCallResults[ref]);
			}
			else {
				if ( iIsExpand && ref >= 0 ) {
					if ( ref >= MAX_ARRAY_INDEX ) {
						GetSender().sender() << L"＄" << iName << L"　添字が大きすぎます（" << MAX_ARRAY_INDEX << L"未満にしてください）。" << std::endl;
						return NULL;
					}
					mKakkoCallResults.resize(ref+1);
					if ( oIsExpanded ) { *oIsExpanded = true; }
					return &(mKakkoCallResults[ref]);
				}
				return NULL;
			}
		}
		else if ( firstChar==L'C' ) {
			oIsSysValue = true;

			if ( 0 <= ref && ref < mLoopCounters.size() ) {
				return &(mLoopCounters.from_top(ref));
			}
			else {
				return NULL;
			}
		}
	}

	if ( variables.find(iName) != variables.end() ) {
		// 変数名であれば変数の内容を返す
		return &(variables[iName]);
	}

	if ( iIsExpand ) {
		variables[iName] = wstring(pDefault);
		if ( oIsExpanded ) { *oIsExpanded = true; }
		return &(variables[iName]);
	}
	return NULL;
}


// 引数に渡されたものを何かの名前であるとし、置き換え対象があれば置き換える。
bool	Satori::Call(const wstring& iName, wstring& oResult, bool for_calc, bool for_non_talk, bool use_arg_callstack)
{
	if ( for_calc ) {
		for_non_talk = true;
	}

	++m_nest_count;

	if ( m_nest_limit > 0 && m_nest_count > m_nest_limit ) {
		GetSender().sender() << L"呼び出し回数超過：" << iName << std::endl;
		oResult = L"（" + iName + L"）";
		--m_nest_count;
		return false;
	}

	bool r = CallReal(iName,oResult,for_calc,for_non_talk, use_arg_callstack);
	--m_nest_count;

	if ( r && oResult.empty() ) {
		if ( for_calc ) {
			oResult = L"０";
		}
	}
	return r;
}

bool	Satori::CallReal(const wstring& iName, wstring& oResult, bool for_calc, bool for_non_talk, bool use_arg_callstack)
{
	simple_stack<strvec>::size_type stack_size_before_call = kakko_replace_history.size();

	// 名前の解決の順番：SAORI・内蔵関数 → 単語群 → 文 → 変数 → 内蔵変数
	const Word* w;
	bool isSysValue;
	wstring *pstr;

	if ( CallFunction(iName, oResult, use_arg_callstack) ) {
		// SAORI・内蔵関数を呼んだ。返値はカッコ展開済み。
	}
	else if ( (w = words.select(iName, *this)) != NULL )
	{
		// 単語を選択した
		GetSender().sender() << L"＠" << iName << std::endl;

		if ( talks.is_exist(iName) ) {
			GetSender().sender() << L"同じ名前「" << iName << L"」の単語群と文があります。トラブルの元なので避けましょう。" << std::endl;
		}

		// 展開中に「単語の追加」などで単語群が変わると w が指す先が無効になるので、コピーしてから展開する
		const Word word_copy = *w;
		oResult = UnKakko( word_copy.c_str() );
	}
	else if ( talks.is_exist(iName) ) {
		// ＊に定義があれば文を取得
		oResult = GetSentence(iName);
	}
	else if ( (pstr = GetValue(iName,isSysValue)) != NULL || isSysValue ) {
		// 変数名であれば変数の内容を返す
		oResult = pstr ? *pstr : L"";
	}
	else if ( !GetBuiltinValue(iName, oResult) ) {
		// 見つからなかった。
		GetSender().sender() << L"（" << iName << L"） not found." << std::endl;
		return	false;
	}

	if ( stack_size_before_call != 0 && stack_size_before_call <= kakko_replace_history.size() ) {
		kakko_replace_history[stack_size_before_call-1].push_back(oResult);
	}
	GetSender().sender() << L"（" << iName << L"）→" << oResult << L"" << std::endl;
	return	true;
}
