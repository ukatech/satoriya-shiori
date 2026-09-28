#include	"satori.h"

#include	<fstream>
#include	<cassert>

#include "random.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

void	diet_script(wstring& ioScript) {
	//replace(ioScript, "\\h", "\\0");
	//replace(ioScript, "\\u", "\\1");
	erase_all(ioScript, L"\\_w[0]");
	int	count;
	do {
		count=0;
		count += replace(ioScript, L"\\1\\0", L"\\0");
		count += replace(ioScript, L"\\0\\0", L"\\0");
		count += replace(ioScript, L"\\0\\1", L"\\1");
		count += replace(ioScript, L"\\1\\1", L"\\1");
		count += replace(ioScript, L"\\n\\e", L"\\e");
		count += replace(ioScript, L"\\n[half]\\e", L"\\e");
		count += replace(ioScript, L"\\n\\-", L"\\-");
		count += replace(ioScript, L"\\n[half]\\-", L"\\-");
		count += replace(ioScript, L"\\e\\-", L"\\-");
		count += replace(ioScript, L"\\e\\e", L"\\e");
	} while (count>0);

	while ( compare_tail(ioScript, L"\\n") )
		ioScript.erase(ioScript.size()-2,2);
		//ioScript.assign(ioScript.substr(0, ioScript.size()-2));
}

bool	Satori::FindEventTalk(wstring& ioevent) {

	static	strmap	replace_map;
	static	bool	isinit=false;
	if ( !isinit ) {
		replace_map[L"OnBoot"]=L"起動";
		replace_map[L"OnClose"]=L"終了";
		replace_map[L"OnFirstBoot"]=L"初回";
		replace_map[L"OnGhostChanged"]=L"他のゴーストから変更";
		replace_map[L"OnGhostChanging"]=L"他のゴーストへ変更";
		//replace_map["OnMouseDoubleClick"]="OnTalk";
		replace_map[L"初回"]=L"OnBoot";
		replace_map[L"他のゴーストから変更"]=L"OnBoot";
		replace_map[L"他のゴーストへ変更"]=L"OnClose";
		replace_map[L"OnVanishSelecting"]=L"消滅指示";
		replace_map[L"OnVanishCancel"]=L"消滅撤回";
		replace_map[L"OnVanishSelected"]=L"消滅決定";
		replace_map[L"OnVanishButtonHold"]=L"消滅中断";
		replace_map[L"OnTalk"]=L"";
		//replace_map[""]="";
		isinit = true;
	}

	while (true) {
		if ( talks.is_exist(ioevent) )
			return	true;	// イベントが存在、それに決定
		if ( replace_map.find(ioevent) == replace_map.end() )
			return	false;	// 置き換え対象がもう無い
		GetSender().sender() << L"event replaced " << ioevent <<  L" → " <<  replace_map[ioevent] << std::endl;
		ioevent = replace_map[ioevent];
	}
}




wstring	Satori::GetSentence(const wstring& name)
{
	wstring script, sentence=name;

	// トークをさくらスクリプトに変換
	const Talk *pTalk = GetSentenceInternal(sentence);
	if ( pTalk ) {
		Sender::nest_object smo(2); 
		script = SentenceToSakuraScriptExec(*pTalk);
		GetSender().sender() << L"return: " << script << L"" << std::endl;
	}
	return	script;
}



// 自動挿入ウェイト
// 2=一般 1=里々 0=無効
#define	character_wait_clear(wait_quantity)	\
	if ( mRequestID == L"OnHeadlinesense.OnFind" || mRequestID == L"OnTranslate" ) { chars_spoken = 0;	} \
	else if( chars_spoken > 0 ) { \
		if ( ! is_quick_section ) { \
			if ( type_of_auto_insert_wait >= 2 ) { \
				next_wait_value += (50*3+random(100))*(wait_quantity)*rate_of_auto_insert_wait/100; \
			} \
			else if ( type_of_auto_insert_wait == 1 ) { \
				next_wait_value += chars_spoken*basewait*rate_of_auto_insert_wait/100; \
			} \
		} \
		chars_spoken = 0; \
	}

// 実際の挿入処理
#define character_wait_exec \
	if ( next_wait_value ) { \
		result += L"\\_w[" + itos(next_wait_value) + L"]"; \
		next_wait_value = 0; \
	}


wstring Satori::SentenceToSakuraScriptExec(const Talk& vec)
{
	wstring jump_to;
	wstring result;
	std::ptrdiff_t ip = 0;
	const Talk* pVec = &vec;
	bool comAndMode = mRequestID!=L"OnCommunicate";

	//実行環境初期化
	wstring allresult = L"\\1";

	//question_num = 0;	// 選択肢番号
	chars_spoken = 0;	// 喋った字数
	next_wait_value = 0; // ウェイト処理

	int jumpcount = 0;

	speaker = 1;	// 本体は 0 うにゅうは 1
	bool jump_inited = true;

	//return禁止
	kakko_replace_history.push(strvec()); // カッコの前方参照用

	while ( TRUE ) {
		if ( speaker != 1 && ! jump_inited ) {
			allresult += L"\\1";
			speaker = 1;	// 本体は 0 うにゅうは 1
			jump_inited = true;
		}

		result = L"";
		jump_to = L"";

		kakko_replace_history.top().clear();
		int resp = SentenceToSakuraScriptInternal(*pVec,result,jump_to,ip);

		allresult += result;

		//resp == 0で全実行終了
		if ( ! resp ) {
			break;
		}

		//それ以外はどこかにジャンプを示す
		if ( resp == 1 ) {
			wstring jump = jump_to;
			m_escaper.unescape(jump);

			const Talk* pTR = GetSentenceInternal(jump);
			if ( ! pTR ) {
				GetSender().sender() << L"＞" << jump_to << L" not found." << std::endl;
			}
			else {
				pVec = pTR;
				ip = 0;
				jump_inited = false;
			}
		}
		else if ( resp == 2 || resp == 3 ) {
			wstring jump = jump_to;
			m_escaper.unescape(jump);

			FamilyComSearchType search_type = type_of_communicate_search;
			if (resp == 3)
			{
				//タグ検索モード
				search_type = COMSEARCH_TAG;	//タグ検索モードを設定
			}

			const Talk* pTR = talks.communicate_search(jump, comAndMode,search_type, *this);
			if ( ! pTR ) {
				if (resp == 3)
				{
					GetSender().sender() << L"≧" << jump_to << L" not found." << std::endl;
				}
				else
				{
					GetSender().sender() << L"≫" << jump_to << L" not found." << std::endl;
				}
			}
			else {
				pVec = pTR;
				ip = 0;
				jump_inited = false;
			}
		}
		++jumpcount;

		if ( m_jump_limit > 0 && jumpcount >= m_jump_limit ) {
			GetSender().sender() << L"ジャンプ回数超過" << std::endl;
			break;
		}
	}

	kakko_replace_history.pop(1);
	//return禁止終了

	return allresult;
}

int Satori::SentenceToSakuraScriptInternal(const strvec &vec,wstring &result,wstring &jump_to, std::ptrdiff_t &ip)
{
	// 再帰管理
	static	int nest_count=0;
	++nest_count;
	//DBG(GetSender().sender() << "enter SentenceToSakuraScriptInternal, nest-count: " << nest_count << ", vector_size: " << vec.size() << std::endl);

	if ( m_nest_limit > 0 && nest_count > m_nest_limit ) {
		GetSender().sender() << L"呼び出し回数超過" << std::endl;
		--nest_count;
		return 0;
	}

	static const int basewait=3;

	strvec::const_iterator it = vec.begin();
	std::advance(it,ip);

	wstring line;
	wstring kakko_result;

	for ( ; it != vec.end() ; ++it) {
		line = *it;
		const wchar_t*	p = line.c_str();
		//DBG(GetSender().sender() << nest_count << " '" << p << "'" << std::endl);

		if ( it==vec.begin() && *p==L'→' ) {
			p+=1;
			//updateGhostsInfo();	// ゴースト情報を更新 -> いらない

			if ( otherghostname.size()>=1 ) {	// そもそも自分以外にゴーストはいるのか 自分自身はotherghostnameには含まない
				wstring	temp = p;
				std::set<wstring>::iterator i = otherghostname.begin();
				for ( ; i != otherghostname.end() ; ++i ) { 
					wstring	name = *i;
					GetSender().sender() << L"ghost: " << name <<std::endl;
					if ( compare_head(temp, name) ) {// 相手を特定
						mCommunicateFor = name;
						p += mCommunicateFor.size();
						break;
					}
				}

				
				if ( i==otherghostname.end() ) {	// 特定しなかった場合
					// ランダム
					//int n = random(ghosts_info.size()-1))+1;
					//assert( n>=1 && n < ghosts_info.size());
					mCommunicateFor = *otherghostname.begin();

					// あかん、隣で起動している～～にならん
				}
			}
		}

		// 選択肢	\q?[id,string]
		if ( *p==L'＿' ) {
			size_t len = wcslen(p);
			if ( len <= 1 ) {
				GetSender().sender() << L"選択肢記法の後に文字列が存在しないので、無視して続行します。" << std::endl;
				continue;
			}
			if ( len>1023 ) {
				GetSender().sender() << L"選択肢記法が長すぎるので、無視して続行します。" << std::endl;
				continue;
			}
			wchar_t	buf[1024];
			wcsncpy(buf, p+1, sizeof(buf) / sizeof(buf[0]));

			wchar_t*	choiced = buf;
			wchar_t*	id = (wchar_t*)strstr_hz(buf, L"\t"); // 選択肢ラベルとジャンプ先の区切り

			result += append_at_choice_start;
			if ( id == NULL ) {
				wstring	str=UnKakko(choiced);
				//result += string("\\q")+itos(question_num++)+"["+str+"]["+str+"]";
				result += L"\\q["+str+L","+str+L"]";
			} else {
				*id++=L'\0';
				while ( *id==L'\t' ) ++id; // 選択肢ラベルとジャンプ先の区切り
				//result += string("\\q")+itos(question_num++)+"["+UnKakko(id)+"]["+UnKakko(choiced)+"]";
				result += L"\\q["+UnKakko(choiced)+L","+UnKakko(id)+L"]";
			}
			result += append_at_choice_end;

			continue;
		}

		// ジャンプ
		if ( *p==L'＞' || *p==L'≫' || *p==L'≧' ) {
			strvec	words;
			split(p+1, L"\t", words, 2); // ジャンプ先とジャンプ条件の区切り

			if ( words.size()>=2 ) {
				wstring	r;
				if ( !calculate(words[1], r) ) {
					GetSender().sender() << L"計算式が異常なので、無視して続行します。" << std::endl;
					continue;
				}
				if ( zen2int(r) == 0 ) {
					GetSender().sender() << L"計算結果が０だったため、続行します。" << std::endl;
					continue;
				}
			}

			if ( words.size() >= 1 ) {
				jump_to = UnKakko(words[0].c_str(),false,true);
			}
			else {
				jump_to.erase();
			}

			if ( *p==L'≫' ) {
				ip = std::distance(vec.begin(),it) + 1;
				--nest_count;
				return 2;
			}
			else if (*p==L'≧'){
				ip = std::distance(vec.begin(), it) + 1;
				--nest_count;
				return 3;
			}
			else {
				ip = std::distance(vec.begin(),it) + 1;
				--nest_count;
				return 1;
			} 
		}

		// 変数を設定
		if ( *p==L'＄' ) {
			const wchar_t* v;
			wstring	value;
			bool	do_calc=false;
			p+=1;

			if ( (v=strstr_hz(p, L"\t"))!=NULL ) { // 変数名と変数に設定する内容の区切り
				value = UnKakko(v+1,false,true);
			}
			else if ( (v=strstr_hz(p, L"＝"))!=NULL || (v=strstr_hz(p, L"="))!=NULL ) {
				value = UnKakko(v+1,false,true);
				do_calc=true;
			}
			else {
				//v = p+strlen(p);
				//value="";
				result += L"\\n※　＄による変数代入文には、タブによる区切りか、＝による計算式が必要です　※\\n\\n[half]'" + value +L"'";
				break;
			}

			wstring	key(p, v-p);
			key = UnKakko(key.c_str(),false,true);

			if ( key==L"" ) {
				result += L"＄"; // ＄そのまま表示
				speaked_speaker.insert(speaker);
			}
			else {
				if ( ! SubstVariable(key,value,result,do_calc) ) {
					break;
				}
			}
			continue;
		}

		// 通常処理

		// 括弧置換後の文字列に余分な処理（スコープ切り替え）を行わないようにするための細工
		// このポインタより後ろは処理可
		const wchar_t *p_do_not_process_end = p;

		while ( p[0] != L'\0' ) {
			bool do_process = (p >= p_do_not_process_end);

			wstring	c=get_a_chr(p);	// 全角半角問わず一文字取得し、pを一文字すすめる

			if ( do_process && (c==L"（") ) {	// 何かを取得・挿入
				character_wait_exec;
				const wchar_t *pe = p;
				kakko_result = KakkoSection(pe);

				//括弧分を引く
				p -= c.size();

				//括弧位置を確認して置き換え
				wstring::size_type index = p - line.c_str();
				wstring::size_type size  = pe - p;

				line.replace(index,size,kakko_result);

				//ポインタ再初期化 (replaceで無効になってるよ)
				p = line.c_str() + index;
				p_do_not_process_end = p + kakko_result.size();
			}
			else if ( c==INTERNAL_MARK_STR ) {	//内部特殊表現 (カッコ遅延評価もあるのでdo_process判定はスキップ)
				c = get_a_chr(p);

				if ( c == L"\x01" || c == L"\x02" ) {
					wstring cmd = c;

					wstring param;

					while (true) {
						c=get_a_chr(p);
						if ( c==INTERNAL_MARK_STR ) { break; }
						param += c;
					}

					if ( cmd == L"\x01" ) { //スコープ切り替え
						int speaker_tmp = stoi_internal(param.c_str());
						if ( is_speaked(speaker) && speaker != speaker_tmp ) {
							result += append_at_scope_change;
							chars_spoken += 1;
						}
						speaker = speaker_tmp;
						character_wait_clear(2);
						character_wait_exec;	// スコープ切り替えタグの前にウエイトを吐き出す
						if ( speaker == 0 ) {
							result += L"\\0";
						}
						else if ( speaker == 1 ) {
							result += L"\\1";
						}
						else {
							result += L"\\p[" + itos(speaker_tmp) + L"]";
						}
					}
					else if ( cmd == L"\x02" ) { //サーフェス加算のための遅延評価
						int s = stoi_internal(param.c_str());
						if ( s != -1 ) { // -1は「消し」なので特別扱い
							s += surface_add_value[speaker];
						}

						//ここをいじったら\sタグ処理部も更新すること

						//サーフィス切り替えの前にウェイトは済ませておくこと
						character_wait_exec;

						//トーク前喋りチェック
						if ( !is_speaked(speaker) ) {
							if ( surface_changed_before_speak.find(speaker) == surface_changed_before_speak.end() ) {
								surface_changed_before_speak.insert(std::map<int,bool>::value_type(speaker,is_speaked_anybody()) );
							}
						}

						result += L"\\s[" + itos(s) + L"]";
					}
				}

			}
			else if ( do_process && (c==L"：") ) {	// スコープ切り替え - ここは二人を想定。
				if ( is_speaked(speaker) ) {
					result += append_at_scope_change;
					chars_spoken += 1;
				}
				speaker = (speaker==0) ? 1 : 0;
				character_wait_clear(2);
				character_wait_exec;	// スコープ切り替えタグの前にウエイトを吐き出す
				result += (speaker ? L"\\1" : L"\\0");
			}
			else if ( c==L"\\" ) {	// さくらスクリプトの解釈、というか解釈のスキップ。

				if ( *p==L'\\' ) {	// エスケープ
					result += c + *p++;
					continue;
				}

				const wchar_t*	start=p;
				wstring	cmd=L"",opt=L"";

				//複数個のアンダースコアと、1個の文字
				while (*p==L'_') {
					++p;
				}
				if (*p < 0x80 && (iswalpha(*p)||iswdigit(*p)||*p==L'!'||*p==L'-'||*p==L'*'||*p==L'&'||*p==L'?'||*p==L'+')) {
					wchar_t c = *p;
					++p;
					if (c == L'w' || c == L's' || c == L'p') {
						if ( iswdigit(*p) ) {
							++p;
						}
					}
				}
				cmd.assign(start, p-start);
				
				if ( cmd == L"_?" || cmd == L"_!" ) { //エスケープ処理 この間に自動タグ挿入はしない
					const wchar_t* e1 = wcsstr(p,L"\\_?");
					if ( ! e1 ) { e1 = wcsstr(p,L"\\_!"); }

					if ( e1 ) {
						character_wait_exec;
						result += c;
						result += cmd;

						opt.assign(p,e1-p+3);
						opt = UnKakko(opt.c_str());

						p = e1 + 3; //endtag

						result += opt;
						continue;
					}
				}

				if (*p==L'[') {
					const wchar_t* opt_start = ++p;
					while (*p && *p!=L']') {
						if (p[0]==L'\\' && p[1]==L']')	// エスケープされた]
							++p;
						++p;
					}
					opt.assign(opt_start, p-opt_start);
					if ( *p ) { ++p; }
					opt=UnKakko(opt.c_str());
				}

				if ( cmd==L"n" ) {
					// 改行
					character_wait_clear(2);
				}
				else if ( (cmd==L"0" || cmd==L"h") || (cmd==L"1" || cmd==L"u") || (cmd==L"p" && aredigits(opt)) ) {
					int spktmp;

					if (cmd==L"0" || cmd==L"h") {
						spktmp = 0;
					}
					else if (cmd==L"1" || cmd==L"u") {
						spktmp = 1;
					}
					else {
						spktmp = stoi_internal(opt);
					}
					
					if ( speaker != spktmp ) {
						// スコープ切り替え
						if ( is_speaked(speaker) ) {
							result += append_at_scope_change_with_sakura_script;
							chars_spoken += 1;
						}
						speaker = spktmp;
						character_wait_clear(2);
						character_wait_exec;	// スコープ切り替えタグの前にウエイトを吐き出す
					}
				}
				else if ( cmd==L"s" ) { //ここをいじったらINTERNAL_MARK 0x02 (内部特殊表現) も更新すること

					//サーフィス切り替えの前にウェイトは済ませておくこと
					character_wait_exec;

					//トーク前喋りチェック
					if ( !is_speaked(speaker) ) {
						if ( surface_changed_before_speak.find(speaker) == surface_changed_before_speak.end() ) {
							surface_changed_before_speak.insert(std::map<int,bool>::value_type(speaker,is_speaked_anybody()) );
						}
					}
				}
				else if ( cmd==L"_q" ) {
					if ( ! is_quick_section ) { //これからクイックセクションなのでウエイトを全部消化
						character_wait_exec;
					}
					is_quick_section = ! is_quick_section;
				}

				if ( opt!=L"" ) {
					//GetSender().sender() << "ss_cmd: " << c << "," << cmd << "," << opt << std::endl;
					result += c + cmd + L"[" + opt + L"]";
				} else {
					//GetSender().sender() << "ss_cmd: " << c << "," << cmd << std::endl;
					result += c + cmd;
				}


				//result += string(start, p-start);
			}
			else if ( c==L"%" ) {	// さくらスクリプトの解釈、というか解釈のスキップ。

				if ( *p==L'%' || *p==L'*' ) {	// エスケープされた%か、%*
					result += c + *p++;
					continue;
				}

				wstring cmd=L"";

				static const wchar_t* tag_pattern[] = {
					L"month",L"day",L"hour",L"minute",L"second",L"username",L"selfname",L"selfname2",L"keroname",L"screenwidth",L"screenheight",
					L"exh",L"et",L"wronghour",L"ms",L"mz",L"mc",L"mh",L"mt",L"me",L"mp",L"m?",L"dms",L"lastghostname",L"lastobjectname",L"property"
				};
				static unsigned int tag_pattern_count = sizeof(tag_pattern)/sizeof(tag_pattern[0]);

				for ( unsigned int tg = 0 ; tg < tag_pattern_count ; ++tg ) {
					int len = wcslen(tag_pattern[tg]);

					if ( wcsncmp(p,tag_pattern[tg],len) == 0 ) {
						cmd = tag_pattern[tg];
						p += len;
					}
				}

				wstring opt=L"";

				if ( (cmd==L"property") && (*p==L'[') ) {
					const wchar_t* opt_start = ++p;
					while (*p && *p!=L']') {
						if (p[0]==L'\\' && p[1]==L']')	// エスケープされた]
							++p;
						++p;
					}
					opt.assign(opt_start, p-opt_start);
					if ( *p ) { ++p; }
					opt=UnKakko(opt.c_str());
				}

				if ( opt!=L"" ) {
					//GetSender().sender() << "ss_cmd: " << c << "," << cmd << "," << opt << std::endl;
					result += c + cmd + L"[" + opt + L"]";
				} else {
					//GetSender().sender() << "ss_cmd: " << c << "," << cmd << std::endl;
					result += c + cmd;
				}

				//result += string(start, p-start);
			}
			else {	// 通常の一文字
				character_wait_exec;

				speaked_speaker.insert(speaker);
				result += c;
				chars_spoken += 1;
				if ( c==L"。" || c==L"、" ) {
					character_wait_clear(c==L"、"?1:2);
				}
			}

		}
		character_wait_clear(2);

		if ( auto_newline_enable_onetime ) { //onetimeのほうのチェックだけで良い
			result += L"\\n";
		}
	}

	//DBG(GetSender().sender() << "leave SentenceToSakuraScriptInternal, nest-count: " << nest_count << std::endl);
	--nest_count;
	return 0;
}

bool Satori::SubstVariable(const wstring &key,wstring &value,wstring &result,bool do_calc)
{
	if ( aredigits(zen2han(key)) ) {
		GetSender().sender() << L"＄" << key << L"　数字のみの変数名は扱えません." << std::endl;
		erase_var(key);	// 存在抹消
	}
	else if ( value==L"" ) {
		GetSender().sender() << L"＄" << key << L"／cleared." << std::endl;
		erase_var(key);	// 存在抹消
		system_variable_operation(key, L"", &result);//存在抹消したものがシステム変数かも！
	}
	else {
		if ( words.is_exist(key) ) {
			GetSender().sender() << L"変数「" << key << L"」と同じ名前の単語群があります。トラブルの元なので避けましょう。" << std::endl;
		}
		if ( talks.is_exist(key) ) {
			GetSender().sender() << L"変数「" << key << L"」と同じ名前の文があります。トラブルの元なので避けましょう。" << std::endl;
		}

		if ( system_variable_operation(key, value, &result) >= 0 ) {
			bool isOverwritten;
			bool isSysValue;

			// "0"は代入先を先に参照する時、エラーを返さないように。
			wstring *pstr = GetValue(key,isSysValue,true,&isOverwritten,L"0");

			if ( do_calc ) {
				if ( !calculate(value, value) ) {
					return false;
				}
				if ( aredigits(value) ) {
					value = int2zen(stoi_internal(value));
				}
			}

			GetSender().sender() << L"＄" << key << L"＝" << value << L"／" << 
				(isOverwritten ? L"written." : L"overwritten.")<< std::endl;

			if ( pstr ) { *pstr = value; }
		}
	}
	return true;
}

#undef	DBG
#define	DBG(a)	a

// 指定された名前に該当する候補からランダムで一つ選び、
// さくらスクリプトに展開して返す。
// 再帰をカウントしてるので不用意なreturnは厳禁。
const Talk* Satori::GetSentenceInternal(wstring& ioSentenceName)
{
	// 再帰管理
	static	int nest_count=0;
	++nest_count;

	// ランダムトークが予約されていた場合の特殊処理。ただし１回のトーク生成で１回だけ。
	static	bool	reserved_talk_processed;
	if ( nest_count==1 )
		reserved_talk_processed=false;
	if ( !reserved_talk_processed && (ioSentenceName==L"" || ioSentenceName==L"OnTalk") )
	{
		reserved_talk_processed = true;

		wstring	reserved_talk_name;	// 今回話すべきトークがあれば、その名前になる
		if ( !reserved_talk.empty() ) {
			// 予約トークの添字を1ずつデクリメント
			std::map<int, wstring>::iterator	it = reserved_talk.begin();
			while ( it!=reserved_talk.end() )
			{
				reserved_talk[it->first-1] = it->second;
				reserved_talk.erase(it++);
			}

			// 先頭のトークが話すべき順になっていれば、名前を取得した上、キューアウト。
			it=reserved_talk.begin();
			if ( it->first<=0 ) {
				reserved_talk_name = it->second;
				reserved_talk.erase(it);	// 今回のは削除
			}
		}

		if ( ioSentenceName==L"OnTalk" && talks.is_exist(L"OnTalk") ) {	
			// OnTalkであり、かつ定義されてる場合
			if (mReferences.size() < 2)
			{
				mReferences.resize(2);
			}
			if ( reserved_talk_name.empty() ) {	// 話すべきトークはない＝ランダムトークでいい
				mRequestMap[L"Reference0"]=mReferences[0]=L"0";
				mRequestMap[L"Reference1"]=mReferences[1]=L"";
			}
			else {	// 今回、話すべき予約トークが存在する
				mRequestMap[L"Reference0"]=mReferences[0]=L"1";
				mRequestMap[L"Reference1"]=mReferences[1]=reserved_talk_name;
			}
		}
		else {
			// 無名か、OnTalkであったとしても定義されてない場合
			if ( !reserved_talk_name.empty() )	// 今回、話すべき予約トークが存在する
				ioSentenceName = reserved_talk_name;	// イベント名を予約トークで置き換える
		}
	}

	// mapから指定名を持つトーク群を検索
	GetSender().sender() << L"＊" << ioSentenceName << std::endl;
	const Talk* talk = talks.select(ioSentenceName, *this);
	if ( talk == NULL )
	{
		GetSender().sender() << L" not matched." << std::endl; // 条件に一致するものがなかった。
		--nest_count;
		return NULL;
	}
	GetSender().sender() << std::endl;
	--nest_count;
	return talk;
}

