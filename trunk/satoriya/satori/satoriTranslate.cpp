#include	"satori.h"
#ifdef POSIX
#  include "Utilities.h"
#else
#  include	<mbctype.h>
#endif

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


bool	Satori::Translate(string& ioScript) {

	if ( ioScript.empty() )
		return	false;

	const bool is_OnTranslate = (mRequestID=="OnTranslate");
	const bool is_AnchorEnable = mRequestID.empty() || (mRequestID.compare(0,2,"On") == 0); //reqidがempty＝さとりてcall

	// さくらスクリプトとそれ以外を分割して処理を加える
	vector<string>	vec;
	string	acum;
	bool	content=false;	// 文に中身があるのか
	bool	is_first_question = true; // 選択分岐記録の消去処理用。
	int	last_speaker=0;
	const char* p = ioScript.c_str();
	while (*p) {
		string	c=get_a_chr(p);	// 全角半角問わず一文字取得し、pを一文字すすめる
		
		if ( c=="\\" || c=="%" ) {
			if (*p=='\\'||*p=='%') {	// エスケープされた\, %
				acum += c + *p++;
				continue;
			}
			
			const char*	start=p;
			string	cmd="",opt="";
	
			while (!_ismbblead(*p) && (isalpha(*p)||isdigit(*p)||*p=='!'||*p=='*'||*p=='&'||*p=='?'||*p=='_'))
				++p;
			cmd.assign(start, p-start);
	
			if (*p=='[') {
				const char* opt_start = ++p;
				while (*p!=']') {
					if (p[0]=='\\' && p[1]==']')	// エスケープされた]
						++p;
					p += _ismbblead(*p) ? 2 : 1;
				}
				opt.assign(opt_start, p++ -opt_start);
			}
			
			// 選択分岐ラベルに対する特殊処理
			if ( !is_OnTranslate && 
				cmd=="q" && opt!="" && count(opt, ",")>0 && 
				mRequestID!="OnHeadlinesense.OnFind") {

				// 選択分岐があるスクリプトであれば、その初回で選択分岐記録をクリア
				if ( is_first_question ) {
					question_record.clear();	
					is_first_question = false;
				}

				// 選択分岐を記録
				{
					strvec	vec;
					split(opt, ",", vec);
					if ( vec.size()==1 )	// countとsplitの罠。
						vec.push_back("");	// 
					string	label=vec[0], id=vec[1];

					if ( false == compare_head(id, "On") &&
						 false == compare_head(id, "http://") &&
						 false == compare_head(id, "https://") 
						)
					{ 
						// Onで始まるものはOnChoiceSelectを経由されないため、対象外とする

						int	count = question_record.size()+1;
						question_record[id] = pair<int,string>(count, label);

						// ラベルＩＤに書き戻し
						opt = label+","+id+byte1_dlmt+label+byte1_dlmt+itos(count);
						for (int i=2;i<vec.size();++i)
							opt += string(",") + vec[i];
					}
				}
			}
			else if ( cmd=="0" || cmd=="h" ) { last_speaker=0; }
			else if ( cmd=="1" || cmd=="u" ) { last_speaker=1; }
			else if ( cmd=="p" && aredigits(opt) ) {
				last_speaker=stoi(opt);
				if ( mIsMateria || last_speaker<=1 ) {
					cmd = (opt=="0") ? "0" : "1";
					opt = "";
				}
			}
			else if ( cmd=="s" && !opt.empty() ) {
				last_talk_exiting_surface[last_speaker]=stoi(opt);
			}
			else if ( cmd.size()==2 && cmd[0]=='s' && isdigit(cmd[1]) ) {
				last_talk_exiting_surface[last_speaker]=cmd[1]-'0';
			}
						
			if ( !acum.empty() )
				vec.push_back(acum);

			if ( opt=="" )
				vec.push_back(c + cmd);
			else
				vec.push_back(c + cmd + "[" + opt + "]");
			acum="";

			static	set<string>	nc_cmd;	// 有効と数えないさくらスクリプト群
			static	bool	initialized=false;
			if (!initialized) {
				initialized=true;
				nc_cmd.insert("0"); nc_cmd.insert("1"); nc_cmd.insert("h"); nc_cmd.insert("u"); nc_cmd.insert("p");
				nc_cmd.insert("n"); nc_cmd.insert("w"); nc_cmd.insert("_w"); nc_cmd.insert("e");
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

	ioScript="";
	string repstr;

	for (vector<string>::iterator i=vec.begin() ; i!=vec.end() ; ++i) {
		if ( i->at(0)!='\\' && i->at(0)!='%' ) {
			// さくらスクリプト以外の文への処理

			// アンカー挿入
			if ( auto_anchor_enable_onetime ) { //onetimeとの比較だけでよい
				if ( is_AnchorEnable && !is_OnTranslate ) {
					string::size_type n = i->size();
					for ( string::size_type c=0 ; c<n ; ++c ) {
						for ( vector<string>::iterator j=anchors.begin() ; j!=anchors.end() ; ++j ) {
							if ( n - c >= j->size() ) {
								if ( i->compare(c,j->size(),*j) == 0 ) {
									repstr = "\\_a[";
									repstr += *j;
									repstr += "]";
									repstr += *j;
									repstr += "\\_a";

									i->replace(c,j->size(),repstr);
									c += repstr.size();
									n = i->size();
									break;
								}
							}
						}
						if ( c < n && _ismbblead(i->at(c)) ) {
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

