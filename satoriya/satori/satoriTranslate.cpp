#include	"satori.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


bool	Satori::Translate(wstring& ioScript) {

	if ( ioScript.empty() )
		return	false;

	const bool is_OnTranslate = (mRequestID==L"OnTranslate");
	const bool is_AnchorEnable = mRequestID.empty() || (mRequestID.compare(0,2,L"On") == 0); //reqidがempty＝さとりてcall

	// さくらスクリプトとそれ以外を分割して処理を加える
	std::vector<wstring>	vec;
	wstring	acum;
	bool	content=false;	// 文に中身があるのか
	bool	is_first_question = true; // 選択分岐記録の消去処理用。
	int	last_speaker=0;
	const wchar_t* p = ioScript.c_str();
	while (*p) {
		wstring	c=get_a_chr(p);	// 全角半角問わず一文字取得し、pを一文字すすめる
		
		if ( c==L"\\" || c==L"%" ) {
			if (*p==L'\\'||*p==L'%') {	// エスケープされた\, %
				acum += c + *p++;
				continue;
			}
			
			const wchar_t*	start=p;
			wstring	cmd=L"",opt=L"";
	
			while (*p < 0x80 && (iswalpha(*p)||iswdigit(*p)||*p==L'!'||*p==L'*'||*p==L'&'||*p==L'?'||*p==L'_'))
				++p;
			cmd.assign(start, p-start);
	
			if (*p==L'[') {
				const wchar_t* opt_start = ++p;
				while (*p && *p!=L']') {
					if (p[0]==L'\\' && p[1]==L']')	// エスケープされた]
						++p;
					++p;
				}
				opt.assign(opt_start, p-opt_start);
				if ( *p ) { ++p; }
			}
			
			// 選択分岐ラベルに対する特殊処理
			if ( !is_OnTranslate && 
				cmd==L"q" && opt!=L"" && count(opt, L",")>0 && 
				mRequestID!=L"OnHeadlinesense.OnFind") {

				// 選択分岐があるスクリプトであれば、その初回で選択分岐記録をクリア
				if ( is_first_question ) {
					question_record.clear();	
					is_first_question = false;
				}

				// 選択分岐を記録
				{
					strvec	vec;
					split(opt, L",", vec);
					if ( vec.size()==1 )	// countとsplitの罠。
						vec.push_back(L"");	// 
					wstring	label=vec[0], id=vec[1];

					if ( false == compare_head(id, L"On") &&
						 false == compare_head(id, L"http://") &&
						 false == compare_head(id, L"https://") 
						)
					{ 
						// Onで始まるものはOnChoiceSelectを経由されないため、対象外とする
						if (!compare_head(id, L"script:") && !compare_head(id, L"\"script:"))
						{
							//script: も対象外

							int	count = question_record.size() + 1;
							question_record[id] = std::pair<int, wstring>(count, label);

							//idも分離
							strvec vec_id;
							if ( id.size() == 0 ) {
								vec_id.push_back(L"");
							}
							else {
								split(id,L"\1",vec_id);
							}

							// ラベルＩＤに書き戻し
							opt = label + L"," + vec_id[0] + byte1_dlmt + label + byte1_dlmt + itos(count);
							for (int i = 2; i < vec.size(); ++i)
								opt += wstring(L",") + vec[i];
						}
					}
				}
			}
			else if ( cmd==L"0" || cmd==L"h" ) { last_speaker=0; }
			else if ( cmd==L"1" || cmd==L"u" ) { last_speaker=1; }
			else if ( cmd==L"p" && aredigits(opt) ) {
				last_speaker=stoi_internal(opt);
				if ( mIsMateria || last_speaker<=1 ) {
					cmd = (opt==L"0") ? L"0" : L"1";
					opt = L"";
				}
			}
			else if ( cmd==L"s" && !opt.empty() ) {
				last_talk_exiting_surface[last_speaker]=stoi_internal(opt);
			}
			else if ( cmd.size()==2 && cmd[0]==L's' && iswdigit(cmd[1]) ) {
				last_talk_exiting_surface[last_speaker]=cmd[1]-L'0';
			}
						
			if ( !acum.empty() )
				vec.push_back(acum);

			if ( opt==L"" )
				vec.push_back(c + cmd);
			else
				vec.push_back(c + cmd + L"[" + opt + L"]");
			acum=L"";

			static	std::set<wstring>	nc_cmd;	// 有効と数えないさくらスクリプト群
			static	bool	initialized=false;
			if (!initialized) {
				initialized=true;
				nc_cmd.insert(L"0"); nc_cmd.insert(L"1"); nc_cmd.insert(L"h"); nc_cmd.insert(L"u"); nc_cmd.insert(L"p");
				nc_cmd.insert(L"n"); nc_cmd.insert(L"w"); nc_cmd.insert(L"_w"); nc_cmd.insert(L"e");
			}
			if ( nc_cmd.find(cmd)==nc_cmd.end() )
				content = true;

		}
		else {
			content = true;
			acum += c;
		}
	}
	if ( !acum.empty() )
		vec.push_back(acum);

	if (!content)
		return	false;	// 中身の無いスクリプト（実行してもしなくても一緒）と判断。

	ioScript=L"";
	wstring repstr;

	for (std::vector<wstring>::iterator i=vec.begin() ; i!=vec.end() ; ++i) {
		if ( i->at(0)!=L'\\' && i->at(0)!=L'%' ) {
			// さくらスクリプト以外の文への処理

			// アンカー挿入
			if ( auto_anchor_enable_onetime ) { //onetimeとの比較だけでよい
				if ( is_AnchorEnable && !is_OnTranslate ) {
					wstring::size_type n = i->size();
					for ( wstring::size_type c=0 ; c<n ; ++c ) {
						for ( std::vector<wstring>::iterator j=anchors.begin() ; j!=anchors.end() ; ++j ) {
							if ( n - c >= j->size() ) {
								if ( i->compare(c,j->size(),*j) == 0 ) {
									repstr = L"\\_a[";
									repstr += *j;
									repstr += L"]";
									repstr += *j;
									repstr += L"\\_a";

									i->replace(c,j->size(),repstr);
									c += repstr.size();
									n = i->size();
									break;
								}
							}
						}
						if ( c < n && IsHighSurrogate(i->at(c)) ) {
							++c;
						}
					}
				}
			}
		}
		ioScript += *i;
	}


	// 事後置き換え辞書を適用
	if ( !is_OnTranslate ) {
		for ( strmap::iterator di=replace_after_dic.begin() ; di!=replace_after_dic.end() ; ++di ) {
			replace(ioScript, di->first, di->second);
		}
	}

	diet_script(ioScript);	// ラストダイエット

	// エスケープしてあった文字を戻す
	m_escaper.unescape(ioScript);

	return	true;
}

