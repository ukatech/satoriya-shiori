#include	"satori.h"

#include	<fstream>
#include	<cassert>
#ifdef POSIX
#  include      "Utilities.h"
#else
#  include	<mbctype.h>
#endif

#include "random.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

void	diet_script(string& ioScript) {
	//replace(ioScript, "\\h", "\\0");
	//replace(ioScript, "\\u", "\\1");
	erase(ioScript, "\\_w[0]");
	int	count;
	do {
		count=0;
		count += replace(ioScript, "\\1\\0", "\\0");
		count += replace(ioScript, "\\0\\0", "\\0");
		count += replace(ioScript, "\\0\\1", "\\1");
		count += replace(ioScript, "\\1\\1", "\\1");
		count += replace(ioScript, "\\n\\e", "\\e");
		count += replace(ioScript, "\\n[half]\\e", "\\e");
		count += replace(ioScript, "\\n\\-", "\\-");
		count += replace(ioScript, "\\n[half]\\-", "\\-");
		count += replace(ioScript, "\\e\\-", "\\-");
		count += replace(ioScript, "\\e\\e", "\\e");
	} while (count>0);

	while ( compare_tail(ioScript, "\\n") )
		ioScript.erase(ioScript.size()-2,2);
		//ioScript.assign(ioScript.substr(0, ioScript.size()-2));
}

bool	Satori::FindEventTalk(string& ioevent) {

	static	strmap	replace_map;
	static	bool	isinit=false;
	if ( !isinit ) {
		replace_map["OnBoot"]="起動";
		replace_map["OnClose"]="終了";
		replace_map["OnFirstBoot"]="初回";
		replace_map["OnGhostChanged"]="他のゴーストから変更";
		replace_map["OnGhostChanging"]="他のゴーストへ変更";
		//replace_map["OnMouseDoubleClick"]="OnTalk";
		replace_map["初回"]="OnBoot";
		replace_map["他のゴーストから変更"]="OnBoot";
		replace_map["他のゴーストへ変更"]="OnClose";
		replace_map["OnVanishSelecting"]="消滅指示";
		replace_map["OnVanishCancel"]="消滅撤回";
		replace_map["OnVanishSelected"]="消滅決定";
		replace_map["OnVanishButtonHold"]="消滅中断";
		replace_map["OnTalk"]="";
		//replace_map[""]="";
		isinit = true;
	}

	while (true) {
		if ( talks.is_exist(ioevent) )
			return	true;	// イベントが存在、それに決定
		if ( replace_map.find(ioevent) == replace_map.end() )
			return	false;	// 置き換え対象がもう無い
		sender << "event replaced " << ioevent <<  " → " <<  replace_map[ioevent] << endl;
		ioevent = replace_map[ioevent];
	}
}




string	Satori::GetSentence(const string& name)
{
	string script, sentence=name;

	/*++m_nest_count;

	if ( m_nest_limit > 0 && m_nest_count > m_nest_limit ) {
		sender << "呼び出し回数超過：" << name << endl;
		--m_nest_count;
		return string("（" + name + "）");
	}*/

	// トークをさくらスクリプトに変換
	const Talk *pTalk = GetSentenceInternal(sentence);
	if ( pTalk ) {
		Sender::nest_object smo(2); 
		script = SentenceToSakuraScriptExec(*pTalk);
		sender << "return: " << script << "" << endl;
	}
	return	script;
}



// 自動挿入ウェイト
// 2=一般 1=里々 0=無効
#define	character_wait_clear(wait_quantity)	\
	if ( mRequestID == "OnHeadlinesense.OnFind" ) { chars_spoken = 0;	} \
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
		result += "\\_w[" + itos(next_wait_value) + "]"; \
		next_wait_value = 0; \
	}


string Satori::SentenceToSakuraScriptExec(const Talk& vec)
{
	string jump_to;
	string result;
	ptrdiff_t ip = 0;
	const Talk* pVec = &vec;
	bool comAndMode = mRequestID!="OnCommunicate";

	//実行環境初期化
	string allresult = "\\1";

	question_num = 0;	// 選択肢番号
	chars_spoken = 0;	// 喋った字数
	next_wait_value = 0; // ウェイト処理

	int jumpcount = 0;

	speaker = 1;	// 本体は 0 うにゅうは 1
	bool jump_inited = true;

	//return禁止
	kakko_replace_history.push(strvec()); // カッコの前方参照用

	while ( TRUE ) {
		if ( speaker != 1 && ! jump_inited ) {
			allresult += "\\1";
			speaker = 1;	// 本体は 0 うにゅうは 1
			jump_inited = true;
		}

		result = "";
		jump_to = "";

		kakko_replace_history.top().clear();
		int resp = SentenceToSakuraScriptInternal(*pVec,result,jump_to,ip);

		allresult += result;

		//resp == 0で全実行終了
		if ( ! resp ) {
			break;
		}

		//それ以外はどこかにジャンプを示す
		if ( resp == 1 ) {
			string jump = jump_to;
			m_escaper.unescape(jump);

			const Talk* pTR = GetSentenceInternal(jump);
			if ( ! pTR ) {
				sender << "＞" << jump_to << " not found." << endl;
			}
			else {
				pVec = pTR;
				ip = 0;
				jump_inited = false;
			}
		}
		else if ( resp == 2 ) {
			string jump = jump_to;
			m_escaper.unescape(jump);

			const Talk* pTR = talks.communicate_search(jump, comAndMode);
			if ( ! pTR ) {
				sender << "≫" << jump_to << " not found." << endl;
			}
			else {
				pVec = pTR;
				ip = 0;
				jump_inited = false;
			}
		}
		++jumpcount;

		if ( m_jump_limit > 0 && jumpcount >= m_jump_limit ) {
			sender << "ジャンプ回数超過" << endl;
			break;
		}
	}

	kakko_replace_history.pop(1);
	//return禁止終了

	return allresult;
}

int Satori::SentenceToSakuraScriptInternal(const strvec &vec,string &result,string &jump_to,ptrdiff_t &ip)
{
	// 再帰管理
	static	int nest_count=0;
	++nest_count;
	//DBG(sender << "enter SentenceToSakuraScriptInternal, nest-count: " << nest_count << ", vector_size: " << vec.size() << endl);

	if ( m_nest_limit > 0 && nest_count > m_nest_limit ) {
		sender << "呼び出し回数超過" << endl;
		--nest_count;
		return 0;
	}

	static const int basewait=3;

	strvec::const_iterator it = vec.begin();
	std::advance(it,ip);

	for ( ; it != vec.end() ; ++it) {
		const char*	p = it->c_str();
		//DBG(sender << nest_count << " '" << p << "'" << endl);

		if ( it==vec.begin() && strncmp(p, "→", 2)==0 ) {
			p+=2;
			updateGhostsInfo();	// ゴースト情報を更新

			if ( ghosts_info.size()>=2 ) {	// そもそも自分以外にゴーストはいるのか。
				string	temp = p;
				vector<strmap>::iterator i=ghosts_info.begin(); ++i; // 自分は飛ばす
				for ( ; i!=ghosts_info.end() ; ++i ) { 
					string	name = (*i)["name"];
					sender << "ghost: " << name <<endl;
					if ( compare_head(temp, name) ) {// 相手を特定
						mCommunicateFor = name;
						p += mCommunicateFor.size();
						break;
					}
				}

				
				if ( i==ghosts_info.end() ) {	// 特定しなかった場合
					// ランダム
					//int n = random(ghosts_info.size()-1))+1;
					//assert( n>=1 && n < ghosts_info.size());
					mCommunicateFor = (ghosts_info[1])["name"];

					// あかん、隣で起動している～～にならん
				}
			}
		}

		// 選択肢	\q?[id,string]
		if ( strncmp(p, "＿", 2)==0 ) {
			if ( strlen(p)>1023 )
				continue;
			char	buf[1024];
			strncpy(buf, p+2, sizeof(buf) / sizeof(buf[0]));

			char*	choiced = buf;
			char*	id = (char*)strstr_hz(buf, "\t"); // 選択肢ラベルとジャンプ先の区切り

			result += append_at_choice_start;
			if ( id == NULL ) {
				string	str=UnKakko(choiced);
				//result += string("\\q")+itos(question_num++)+"["+str+"]["+str+"]";
				result += "\\q["+str+","+str+"]";
			} else {
				*id++='\0';
				while ( *id=='\t' ) ++id; // 選択肢ラベルとジャンプ先の区切り
				//result += string("\\q")+itos(question_num++)+"["+UnKakko(id)+"]["+UnKakko(choiced)+"]";
				result += "\\q["+UnKakko(choiced)+","+UnKakko(id)+"]";
			}
			result += append_at_choice_end;

			continue;
		}

		// ちょっと微妙な存在意義。
		if ( strncmp(p, "\\s", 2)==0 ) {	
			if ( !is_speaked(speaker) ) {
				if ( surface_changed_before_speak.find(speaker) == surface_changed_before_speak.end() ) {
					surface_changed_before_speak.insert(map<int,bool>::value_type(speaker,is_speaked_anybody()) );
				}
			}
		}

		// ジャンプ
		if ( strncmp(p, "＞", 2)==0 || strncmp(p, "≫", 2)==0 ) {
			strvec	words;
			split(p+2, "\t", words, 2); // ジャンプ先とジャンプ条件の区切り

			if ( words.size()>=2 ) {
				string	r;
				if ( !calculate(words[1], r) )
					break;
				if ( zen2int(r) == 0 ) {
					sender << "＊計算結果が０だったため、続行します。" << endl;
					continue;
				}
			}

			if ( words.size() >= 1 ) {
				jump_to = UnKakko(words[0].c_str(),false,true);
			}
			else {
				jump_to.erase();
			}

			if ( strncmp(p, "≫", 2)==0 ) {
				ip = std::distance(vec.begin(),it) + 1;
				--nest_count;
				return 2;
			}
			else {
				ip = std::distance(vec.begin(),it) + 1;
				--nest_count;
				return 1;
			} 
		}

		// 変数を設定
		if ( strncmp(p, "＄", 2)==0 ) {
			const char* v;
			string	value;
			bool	do_calc=false;
			p+=2;
			if ( (v=strstr_hz(p, "\t"))!=NULL ) { // 変数名と変数に設定する内容の区切り
				value = UnKakko(v+1,false,true);
			}
			else if ( (v=strstr_hz(p, "＝"))!=NULL || (v=strstr_hz(p, "="))!=NULL ) {
				value = UnKakko(v+((*v=='=') ? 1 : 2),false,true);
				do_calc=true;
			}
			else {
				//v = p+strlen(p);
				//value="";
				result += "\\n※　＄による変数代入文には、タブによる区切りか、＝による計算式が必要です　※\\n\\n[half]'" + value +"'";
				break;
			}

			string	key(p, v-p);
			key = UnKakko(key.c_str(),false,true);

			if ( key=="" ) {
				result += "＄"; // ＄そのまま表示
				speaked_speaker.insert(speaker);
			}
			else if ( aredigits(zen2han(key)) ) {
				sender << "＄" << key << "　数字のみの変数名は扱えません." << endl;
				erase_var(key);	// 存在抹消
			}
			else if ( value=="" ) {
				sender << "＄" << key << "／cleared." << endl;
				erase_var(key);	// 存在抹消
				system_variable_operation(key, "", &result);//存在抹消したものがシステム変数かも！
			}
			else {
				bool isOverwritten;
				bool isSysValue;

				// "0"は代入先を先に参照する時、エラーを返さないように。
				string *pstr = GetValue(key,isSysValue,true,&isOverwritten,"0");

				if ( do_calc ) {
					if ( !calculate(value, value) )
						break;
					if ( aredigits(value) ) {
						value = int2zen(stoi(value));
					}
				}

				sender << "＄" << key << "＝" << value << "／" << 
					(isOverwritten ? "written." : "overwritten.")<< endl;

				if ( pstr ) { *pstr = value; }
				system_variable_operation(key, value, &result);
			}
			continue;
		}

		// 通常処理
		while ( p[0] != '\0' ) {
			string	c=get_a_chr(p);	// 全角半角問わず一文字取得し、pを一文字すすめる

			if ( c=="（" ) {	// 何かを取得・挿入
				character_wait_exec;
				result += KakkoSection(p);
			}
			else if ( c=="\xff" ) {	//内部特殊表現
				c=get_a_chr(p);
				if ( c=="\x01" ) { //0xff0x01＝スコープ切り替え　後に半角数値が1文字続く
					c=get_a_chr(p);
					int speaker_tmp = stoi(c.c_str());
					if ( is_speaked(speaker) && speaker != speaker_tmp ) {
						result += append_at_scope_change;
						chars_spoken += 2;
					}
					speaker = speaker_tmp;
					character_wait_clear(2);
					if ( speaker == 0 ) {
						result += "\\0";
					}
					else if ( speaker == 1 ) {
						result += "\\1";
					}
					else {
						result += "\\p[" + c + "]";
					}
				}
			}
			else if ( c=="：" ) {	// スコープ切り替え - ここは二人を想定。
				if ( is_speaked(speaker) ) {
					result += append_at_scope_change;
					chars_spoken += 2;
				}
				speaker = (speaker==0) ? 1 : 0;
				character_wait_clear(2);
				result += (speaker ? "\\1" : "\\0");
			}
			else if ( c=="\\" || c=="%" ) {	// さくらスクリプトの解釈、というか解釈のスキップ。

				if (*p=='\\'||*p=='%') {	// エスケープされた\, %
					result += c + *p++;
					continue;
				}

				const char*	start=p;
				string	cmd="",opt="";

				while (!_ismbblead(*p) && (isalpha(*p)||isdigit(*p)||*p=='!'||*p=='-'||*p=='*'||*p=='&'||*p=='?'||*p=='_'))
					++p;
				cmd.assign(start, p-start);

				if ( cmd == "_?" || cmd == "_!" ) { //エスケープ処理 この間に自動タグ挿入はしない
					const char* e1 = strstr(p,"\\_?");
					if ( ! e1 ) { e1 = strstr(p,"\\_!"); }

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

				if (*p=='[') {
					const char* opt_start = ++p;
					while (*p!=']') {
						if (p[0]=='\\' && p[1]==']')	// エスケープされた]
							++p;
						p += _ismbblead(*p) ? 2 : 1;
					}
					opt.assign(opt_start, p++ -opt_start);
					opt=UnKakko(opt.c_str());
				}

				if ( cmd=="n" ) {
					// 改行
					character_wait_clear(2);
				}
				else if ( ( (cmd=="0" || cmd=="h") && speaker==1) || ( (cmd=="1" || cmd=="u") && speaker==0) ) {
					// スコープ切り替え
					if ( is_speaked(speaker) ) {
						result += append_at_scope_change_with_sakura_script;
						chars_spoken += 2;
					}
					speaker = stoi(cmd);
					character_wait_clear(2);
				}
				else if ( cmd=="p" && aredigits(opt) ) {
					int spktmp = stoi(opt);
					if ( speaker != spktmp ) {
						// スコープ切り替えonSSP/CROW
						if ( is_speaked(speaker) ) {
							result += append_at_scope_change_with_sakura_script;
							chars_spoken += 2;
						}
						speaker = spktmp;
						character_wait_clear(2);
					}
				}
				else if ( cmd=="s" ) {
					//サーフィス切り替えの前にウェイトは済ませておくこと
					character_wait_exec;
				}
				else if ( cmd=="_q" ) {
					if ( ! is_quick_section ) { //これからクイックセクションなのでウエイトを全部消化
						character_wait_exec;
					}
					is_quick_section = ! is_quick_section;
				}

				if ( opt!="" ) {
					//sender << "ss_cmd: " << c << "," << cmd << "," << opt << endl;
					result += c + cmd + "[" + opt + "]";
				} else {
					//sender << "ss_cmd: " << c << "," << cmd << endl;
					result += c + cmd;
				}


				//result += string(start, p-start);
			}
			else {	// 通常の一文字
				character_wait_exec;

				speaked_speaker.insert(speaker);
				result += c;
				chars_spoken += c.size();
				if ( c=="。" || c=="、" ) {
					character_wait_clear(c=="、"?1:2);
				}
			}

		}
		character_wait_clear(2);
		result += "\\n";
	}

	//DBG(sender << "leave SentenceToSakuraScriptInternal, nest-count: " << nest_count << endl);
	--nest_count;
	return 0;
}

#undef	DBG
#define	DBG(a)	a

// 指定された名前に該当する候補からランダムで一つ選び、
// さくらスクリプトに展開して返す。
// 再帰をカウントしてるので不用意なreturnは厳禁。
const Talk* Satori::GetSentenceInternal(string& ioSentenceName)
{
	// 再帰管理
	static	int nest_count=0;
	++nest_count;

	// ランダムトークが予約されていた場合の特殊処理。ただし１回のトーク生成で１回だけ。
	static	bool	reserved_talk_processed;
	if ( nest_count==1 )
		reserved_talk_processed=false;
	if ( !reserved_talk_processed && (ioSentenceName=="" || ioSentenceName=="OnTalk") )
	{
		reserved_talk_processed = true;

		string	reserved_talk_name;	// 今回話すべきトークがあれば、その名前になる
		if ( !reserved_talk.empty() ) {
			// 予約トークの添字を1ずつデクリメント
			map<int, string>::iterator	it = reserved_talk.begin();
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

		if ( ioSentenceName=="OnTalk" && talks.is_exist("OnTalk") ) {	
			// OnTalkであり、かつ定義されてる場合
			if ( reserved_talk_name.empty() ) {	// 話すべきトークはない＝ランダムトークでいい
				mRequestMap["Reference0"]=mReferences[0]="0";
				mRequestMap["Reference1"]=mReferences[1]="";
			}
			else {	// 今回、話すべき予約トークが存在する
				mRequestMap["Reference0"]=mReferences[0]="1";
				mRequestMap["Reference1"]=mReferences[1]=reserved_talk_name;
			}
		}
		else {
			// 無名か、OnTalkであったとしても定義されてない場合
			if ( !reserved_talk_name.empty() )	// 今回、話すべき予約トークが存在する
				ioSentenceName = reserved_talk_name;	// イベント名を予約トークで置き換える
		}
	}

	// mapから指定名を持つトーク群を検索
	sender << "＊" << ioSentenceName;
	const Talk* talk = talks.select(ioSentenceName, *this);
	if ( talk == NULL )
	{
		sender << " not matched." << endl; // 条件に一致するものがなかった。
		--nest_count;
		return NULL;
	}
	sender << endl;
	--nest_count;
	return talk;
}
