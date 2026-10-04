#include	"satori.h"
#include <algorithm>

#include "random.h"
#include "posix_utils.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

inline bool	is_empty_script(const wstring& script) {
	for ( const wchar_t* p = script.c_str() ; *p!=L'\0' ; ) {
		if ( p[0]==L'\\' ) {
			if ( p[1]==L'0' || p[1]==L'1' ) {
				p+=2;
				continue;
			}
			else if ( p[1]==L'p' && p[2]==L'[' ) {
				p+=3;
				while ( *p != L']' )
					if ( *p == L'\0' )
						return	false;
					else
						++p;
				++p;	// ]
				continue;
			}
		}
		return	false;
	}
	return	true;
}

// ホールド判定が読む mousedown_reference_array の要素数（Reference3, 4 を使う）
static const size_t	MOUSEDOWN_REFERENCE_COUNT = 5;

// イベントの処理が固定の添字で読むReferenceの数。
static size_t	required_reference_count(const wstring& iEvent)
{
	static const struct {
		const wchar_t*	event;
		size_t	count;
	} table[] = {
		{ L"OnSecondChange",	4 },
		{ L"OnMinuteChange",	4 },
		{ L"OnSurfaceChange",	2 },
		{ L"OnUpdateReady",		1 },
		{ L"OnMouseDown",		6 },
		{ L"OnMouseMove",		5 },
		{ L"OnMouseWheel",		5 },
		{ L"OnAnchorSelect",	1 },
		{ L"OnChoiceSelect",	1 },
		{ L"OnRecommendsiteChoice",	5 },
		{ L"OnCommunicate",		2 },
	};
	for ( size_t i=0 ; i<sizeof(table)/sizeof(table[0]) ; ++i ) {
		if ( iEvent == table[i].event ) {
			return	table[i].count;
		}
	}
	return	0;
}

int	Satori::EventOperation(wstring iEvent, std::map<wstring,wstring> &oResponse)
{
	// 以降の処理は固定の添字でReferenceを読む。リクエストに足りない分があっても範囲外を読まないよう、空で埋めておく。
	// （Referenceが揃っている正しいリクエストでは何も変わらない）
	{
		const size_t	need = required_reference_count(iEvent);
		if ( mReferences.size() < need ) {
			mReferences.resize(need);
		}
	}

	// トーク先頭に「→」があると設定される。無ければ""のまま。
	mCommunicateFor=L"";
	// スクリプト文字列
	wstring	script=L"";

	// 直前の OnChoiceSelectEx で実行したID。次のリクエストまでしか持ち越さない。
	wstring	last_choice_select_ex_id;
	last_choice_select_ex_id.swap(choice_select_ex_id);


	bool talking = false;
	if ( mIsStatusHeaderExist ) {
		strmap::const_iterator it = mRequestMap.find(L"Status");
		if ( it != mRequestMap.end() ) {
			if ( wcsstr(it->second.c_str(),L"talking") ) {
				talking = true;
			}
		}
	}
	

	// システムが欲しい情報を拾っておく
	if ( iEvent==L"OnSecondChange" || iEvent==L"OnMinuteChange" ) {

#ifndef POSIX
		if ( characters_hwnd.empty() && updateGhostsInfo() && !ghosts_info.empty() ) {
			GetSender().sender() << L"■FMOからhwndを取得しました。" << std::endl;

			strmap &ghost = ghosts_info[0];
			strmap::const_iterator it = ghost.find(L"hwnd");
			if ( it != ghost.end() ) {
				characters_hwnd[0] = (void*)(stoi_internal(it->second));
			}
			it = ghost.find(L"kerohwnd");
			if ( it != ghost.end() ) {
				characters_hwnd[1] = (void*)(stoi_internal(it->second));
			}
		}

		if ( is_single_monitor ) {
			//GetSender().sender() << "■シングルモニタです。見切れの独自判定を行いません。" <<std::endl;
		}
		else if ( ! mIsMateria ) {
			//GetSender().sender() << "■Materia以外では見切れの判定を処理系に任せます。" <<std::endl;
		}
		else if ( characters_hwnd.empty() ) {
			//GetSender().sender() << "■マルチモニタですが、hwndが取得できていないため、見切れの独自判定を行いません。" <<std::endl;
		}
		else {
		//	GetSender().sender() << "■見切れ判定処理" <<std::endl;
			RECT	rc;
			::GetWindowRect((HWND)characters_hwnd[0], &rc);
			int	center = (rc.left + rc.right)/2;
			mRequestMap[L"Reference1"] = mReferences[1] =
				( center >= max_screen_rect.left && center <= max_screen_rect.right ) ? L"0" : L"1";
			/*GetSender().sender() << "シェルの左端: " << rc.left <<std::endl;
			GetSender().sender() << "シェルの右端: " << rc.right <<std::endl;
			GetSender().sender() << "シェルの中央: " << center <<std::endl;
			GetSender().sender() << "デスクトップの左端: " << max_screen_rect.left <<std::endl;
			GetSender().sender() << "デスクトップの右端: " << max_screen_rect.right <<std::endl;
			GetSender().sender() << "■見切れ判定結果: " << mReferences[1] <<std::endl;*/
		}
#endif
		//ホールド
		if ( mousedown_reference_array.size() >= MOUSEDOWN_REFERENCE_COUNT && mousedown_exec_complete == false ) {
			if ( (posix_get_current_tick() - mousedown_time) > 1000 ) {
				wstring	str = mousedown_reference_array[3]+mousedown_reference_array[4]+L"ホールド";

				mousedown_exec_complete = true;
				if ( talks.is_exist(str) ) {
					script=GetSentence(str);
				}
			}
		}

		//ホールド終了
		if ( ! talking ) {
			if ( mousedown_secchange_delay_exec ) {
				if ( (posix_get_current_tick() - mousedown_secchange_delay_time) > 1000 ) {
					mousedown_secchange_delay_exec = false;
					mousedown_secchange_delay_time = 0;

					// 押し下げの後に別のクリックなどで配列が空になっていることがある
					if ( mousedown_reference_array.size() >= MOUSEDOWN_REFERENCE_COUNT ) {
						wstring	str = mousedown_reference_array[3]+mousedown_reference_array[4]+L"ホールド終了";
						if ( talks.is_exist(str) ) {
							script=GetSentence(str);
						}
					}
					mousedown_reference_array.clear();
					mousedown_time = 0;
				}
			}
		}

		mikire_flag = stoi_internal(mReferences[1])!=0;
		kasanari_flag = stoi_internal(mReferences[2])!=0;
		can_talk_flag = ( mReferences[3] != L"0" );
		if ( iEvent[2]==L'S' )
			++second_from_last_talk;
	}
	else if ( iEvent==L"OnSurfaceChange" ) {
		cur_surface[0]=stoi_internal(mReferences[0]);
		cur_surface[1]=stoi_internal(mReferences[1]);
	} else if ( iEvent==L"OnUpdateReady" ) {
		mReferences[0] = itos(stoi_internal(mReferences[0])+1);
	}

	bool hold_complete_exec = false;

	if ( (iEvent==L"OnBoot" || iEvent==L"OnGhostChanged") && !is_empty_script(on_loaded_script) ) {
		script = on_loaded_script;
		on_loaded_script = L"";
	}
	else if ( iEvent==L"OnClose" || iEvent==L"OnGhostChanging" ) {
		wstring	on_unloading_script = GetSentence(L"OnSatoriClose");
		diet_script(on_unloading_script);
		if ( !is_empty_script(on_unloading_script) )
			script = on_unloading_script;
	}
	else if ( iEvent==L"OnMouseDragStart" ) {
		if ( ! mousedown_exec_complete ) {
			mousedown_reference_array.clear();
			mousedown_time = 0;
		}
	}
	else if ( iEvent==L"OnMouseDragEnd" ) {
		if ( mousedown_exec_complete ) {
			hold_complete_exec = true;
		}
	}
	else if ( iEvent==L"OnMouseDown" ) {
		if ( _wtoi(mReferences[5].c_str()) == 0 ) {
			//ホールド計測開始
			mousedown_reference_array = mReferences;
			mousedown_time = posix_get_current_tick();
			mousedown_exec_complete = false;
		}
	}
	else if ( iEvent==L"OnMouseUp" ) {
		if ( mousedown_exec_complete ) {
			hold_complete_exec = true;
		}
		else {
			mousedown_reference_array.clear();
			mousedown_time = 0;
		}
	}

	if ( hold_complete_exec ) {
		if ( talking ) {
			mousedown_secchange_delay_exec = true;
			mousedown_secchange_delay_time = posix_get_current_tick();
		}
		else {
			if ( mousedown_reference_array.size() >= MOUSEDOWN_REFERENCE_COUNT ) {
				wstring	str = mousedown_reference_array[3]+mousedown_reference_array[4]+L"ホールド終了";
				if ( talks.is_exist(str) ) {
					script=GetSentence(str);
				}
			}
			mousedown_reference_array.clear();
			mousedown_time = 0;
		}
	}

	if ( !script.empty() ) {
	}
	else if ( iEvent==L"OnAnchorSelect" && std::find(anchors.begin(),anchors.end(),mReferences[0])!=anchors.end() ) {
		// OnAnchorSelectがきたとき、ref0がdicAnchorの文名の場合は
		// イベントが定義されている場合でもシステム側を優先する。
		script=GetSentence(mReferences[0]);
	}
	// ************** イベントコール *****************************************************
	else if ( FindEventTalk(iEvent) ) {	// この際、互換イベントへの置換も同時に行われる
		// 定義されているならそれを優先する。
		script=GetSentence(iEvent);

	}
	// ************** これより以下は、イベントが定義されていない場合のデフォルト処理 **************
	else if ( iEvent==L"OnMouseDoubleClick" )
	{
		static strvec v;
		if ( v.empty() )
		{
			v.push_back(L"＞（Ｒ３）（Ｒ４）つつかれ");
			v.push_back(L"（）");
		}
		script = SentenceToSakuraScriptExec(v);
	}
	else if ( iEvent==L"OnMouseMove" ) {
		nade_valid_time = nade_valid_time_initializer; // なでセッションの有効期限を更新
		int&	cur_nede_count = nade_count[ mReferences[4] ];
		if ( ++cur_nede_count >= nade_sensitivity ) {
#ifdef POSIX
		    int ret = 0;
#else
		    LRESULT	ret = 0;
#endif

			if ( mIsStatusHeaderExist ) {
				strmap::const_iterator it = mRequestMap.find(L"Status");
				if ( it != mRequestMap.end() && (!insert_nade_talk_at_other_talk) ) {
					if ( wcsstr(it->second.c_str(),L"talking") ) {
						ret = 1;
					}
					if ( wcsstr(it->second.c_str(),L"induction") ) {
						ret = 1;
					}
					if ( wcsstr(it->second.c_str(),L"passive") ) {
						ret = 1;
					}
					if ( wcsstr(it->second.c_str(),L"timecritical") ) {
						ret = 1;
					}
				}
			}
#ifndef POSIX
			else {
				if ( !insert_nade_talk_at_other_talk && updateGhostsInfo() ) {
					wstring	hwnd_str = (ghosts_info[0])[L"hwnd"];
					HWND	hwnd = (HWND)(stoi_internal(hwnd_str));
					if ( hwnd!=NULL ) {
						UINT	WM_SAKURAAPI = RegisterWindowMessage(L"Sakura");
						DWORD ret_dword = 0;
						if ( ::SendMessageTimeout(hwnd, WM_SAKURAAPI, 140, 0,SMTO_BLOCK|SMTO_ABORTIFHUNG,5000,&ret_dword) ) { //GETGHOSTSTATE
							ret = ret_dword;
						}
					}
				}
			}
#endif
			
			if ( ret == 0 ) {
				if (bool_of_action_when_ghost_is_stroked && talks.is_exist(L"なでられ時の反応")) {
					script = GetSentence(L"なでられ時の反応");
				}
				else {
					wstring	str = mReferences[3] + mReferences[4] + L"なでられ";
					if (talks.is_exist(str))
						script = GetSentence(str);
				}
				GetSender().sender() << L"Talk: " << script << std::endl;
			}
			nade_count.clear();
		}
	}
	else if ( iEvent==L"OnChoiceSelect" ) {
		// 直前の OnChoiceSelectEx で実行済みなら、同じ文を二度実行しない
		if ( last_choice_select_ex_id.empty() || last_choice_select_ex_id != mReferences[0] ) {
			script=GetSentence(mReferences[0]);
		}
	}
	else if ( iEvent==L"OnChoiceSelectEx" && mReferences.size() > 2 && !talks.is_exist(L"OnChoiceSelect") && talks.is_exist(mReferences[1]) ) {
		// 引数付きの選択肢。ＩＤの文を、Reference2以降を（Ａ０）～にして実行する
		strvec	args(mReferences.begin()+2, mReferences.end());
		mCallStack.push(args);
		script=GetSentence(mReferences[1]);
		mCallStack.pop();
		choice_select_ex_id = mReferences[1];
	}
	else if ( iEvent==L"OnWindowStateRestore"
		|| iEvent==L"OnShellChanged"
		|| iEvent==L"OnSurfaceRestore"
		|| iEvent==L"起動" ) {	// 「起動」は「OnBoot」「OnGhostChanged」からリダイレクトされる
		script=surface_restore_string();
	}
	else if ( iEvent==L"OnMouseWheel" ) {
		koro_valid_time = 3;
		koro_count[ mReferences[4] ] += 1;
		if ( koro_count[ mReferences[4] ] >= 2 ) {
			wstring	str = mReferences[3]+mReferences[4]+L"ころころ";
			if ( talks.is_exist(str) ) {
				script=GetSentence(str);
				koro_count.clear();
			}
		}
	}
	else if ( iEvent==L"OnRecommendsiteChoice" )
	{
		if ( mReferences[3] != L"" && mReferences[4] != L"" ) {
			if ( mReferences[3] == L"portal" ) {
				if ( ! GetRecommendsiteSentence(L"sakura.portalsites", script) ) {
					script = L"";
				}
			}
			else {
				bool found = false;
				if ( zen2int(mReferences[4]) == 0 ) {
					if ( GetRecommendsiteSentence(L"sakura.recommendsites", script) ) {
						found = true;
					}
				}
				else if ( zen2int(mReferences[4]) == 1 ) {
					if ( GetRecommendsiteSentence(L"kero.recommendsites", script) ) {
						found = true;
					}
				}

				if ( ! found ) {
					wstring req = wstring(L"char") + mReferences[4] + L".recommendsites";
					if ( ! GetRecommendsiteSentence(req.c_str(), script) ) {
						script = L"";
					}
				}
			}
		}
		else {
			if ( GetRecommendsiteSentence(L"sakura.recommendsites", script) ) {
				/*NOOP*/;
			}
			else if ( GetRecommendsiteSentence(L"kero.recommendsites", script) ) {
				/*NOOP*/;
			}
			else if ( GetRecommendsiteSentence(L"sakura.portalsites", script) ) {
				/*NOOP*/;
			}
			else {
				script=L"";
			}
		}
	}
	else if ( iEvent==L"OnCommunicate" ) {	// 話し掛けられた

		// ＄Sender	（if,（Ｒ０）==User,ユーザ,（Ｒ０））
		if ( mReferences[0]==L"user" )
			mReferences[0]=L"ユーザ";

		// スクリプトに話者名を付加
		if ( !TalkSearch(mReferences[0]+L"「"+mReferences[1]+L"」", script, false) )
			script=GetSentence(L"COMMUNICATE該当なし");

		if ( mCommunicateFor==L""  ) {	// 手動打ち切り
			GetSender().sender() << L"里々COMMUNICATE、辞書に続行指示が無いことによる打ち切り" <<std::endl;
			mCommunicateLog.clear();
		}
		else if ( mCommunicateLog.find(script) != mCommunicateLog.end() ) {
			GetSender().sender() << L"里々COMMUNICATE、自分側ループにより打ち切り" <<std::endl;
			script=L"";	// 何も言わない
			mCommunicateLog.clear();
			mCommunicateFor = L"";
		}
		else  if ( mCommunicateLog.find(mReferences[1]) != mCommunicateLog.end() ) {
			GetSender().sender() << L"里々COMMUNICATE、相手側ループにより打ち切り" <<std::endl;
			mCommunicateLog.clear();
			mCommunicateFor = L"";
		} 
		else {	// 続行
			GetSender().sender() << L"里々COMMUNICATE、続行" <<std::endl;
			mCommunicateLog.insert(mReferences[1]);
			mCommunicateLog.insert(script);
		}
	}

	if ( iEvent==L"OnSecondChange" ) {

		// 存在するタイマのディクリメント
		for (strintmap::iterator i=timer_sec.begin();i!=timer_sec.end();++i) {
			variables[i->first + L"タイマ"] = int2zen( --(i->second) );
		}

		// 自動セーブ
		if ( mAutoSaveInterval > 0 ) {
			if ( --mAutoSaveCurrentCount <= 0 ) {
				if ( is_dic_loaded ) {
					this->Save(false);
				}
				mAutoSaveCurrentCount = mAutoSaveInterval;
			}
		}
	}

	diet_script(script);

	bool is_rnd_talk = false;

	if ( is_empty_script(script) && can_talk_flag && iEvent==L"OnSecondChange" ) {

		// タイマ予約発話
		for (strintmap::const_iterator i=timer_sec.begin();i!=timer_sec.end();++i) {
			if ( i->second < 1 ) {
				//GetSentence実行後にtimerのイテレータは状態変化しているかもしれないので
				//いったん保存しておく
				wstring  timer_name = i->first;
				wstring	var_name = timer_name + L"タイマ";

				GetSender().sender() << var_name << L"が発動。" <<std::endl;

				reset_speaked_status();
				if ( !talks.is_exist(timer_name) && talks.is_exist(L"OnSatoriTimer") ) {
					// ＊名前 が無ければ ＊OnSatoriTimer に、（Ａ０）タイマの名前、（Ａ１）遅れた秒数 を渡す
					strvec	args;
					args.push_back(timer_name);
					args.push_back(int2zen(-(i->second)));
					mCallStack.push(args);
					script=GetSentence(L"OnSatoriTimer");
					mCallStack.pop();
				}
				else {
					script=GetSentence(timer_name);
				}
				
				strintmap::const_iterator tm = timer_sec.find(timer_name);
				if ( tm != timer_sec.end() ) { //まだタイマー変数が残っている
					if ( tm->second < 1 ) { //再設定されてない
						timer_sec.erase(timer_name);
						variables.erase(var_name);
					}
				}

				if ( !is_empty_script(script) ) {
					diet_script(script);
				}
				is_rnd_talk = true;
				break;
			}
		}

		// 自動発話
		if ( is_empty_script(script) && (mikire_flag==false || is_call_ontalk_at_mikire==true)) {
			if ( nade_valid_time>0 ) {
				if ( --nade_valid_time == 0 ) {
					nade_count.clear();
				}
			}

			if ( koro_valid_time>0 ) {
				if ( --koro_valid_time == 0 ) {
					koro_count.clear();
				}
			}

			if ( talk_interval>0 && --talk_interval_count<0 ) {
				wstring	iEvent=L"OnTalk";
				FindEventTalk(iEvent);

				reset_speaked_status();
				script=GetSentence(iEvent);

				diet_script(script);
				is_rnd_talk = true;
			}
		}

		//発話が発生してない場合、辞書が独自で実行する自動発話向けにOnSatoriSecondChangeを呼ぶ
		if (is_empty_script(script))
		{
			wstring iEvent = L"OnSatoriSecondChange";
			FindEventTalk(iEvent);

			reset_speaked_status();
			script = GetSentence(iEvent);

			diet_script(script);
			if (!is_empty_script(script))
			{
				is_rnd_talk = true;
			}
		}
	}

	if ( is_empty_script(script) ) {
		if ( mRequestID==L"OnClose" ) {
			oResponse[L"Value"] = L"\\-";
			return 200;
		}
		else {
			return 204;	// 喋らない
		}
	}

	// scriptへの付与処理
	if ( is_speaked_anybody() || is_rnd_talk ) {
		script = append_at_talk_start + surface_restore_string() + script + append_at_talk_end;

		// 喋りカウント初期化
		int	dist = int(talk_interval*(talk_interval_random/100.0));
		talk_interval_count = ( dist==0 ) ? talk_interval : 
			(talk_interval-dist)+(random(dist*2));
	}
	script += ( mRequestID==L"OnClose" ) ? L"\\-" : L"\\e";


	if ( !mCommunicateFor.empty() ) { // 話しかけの有無
		GetSender().sender() << L"里々COMMUNICATE、「" << mCommunicateFor << L"」へ話し掛け" <<std::endl;
		oResponse[L"Reference0"] = mCommunicateFor;
		oResponse[L"To"] = mCommunicateFor;
		if ( iEvent!=L"OnCommunicate" )
		{
			mCommunicateLog.clear(); // 初回のみ。続行時にはここでクリアはしない。
		}
		//mCommunicateLog.insert(script);
	}
	oResponse[L"Value"]=script;

	// １トーク中でのみ有効な重複回避をクリア
	words.handle_talk_end();
	talks.handle_talk_end();

	// バルーン位置が有効なら設定
	if ( validBalloonOffset[0] && validBalloonOffset[1] )
		oResponse[L"BalloonOffset"]=wstring()+BalloonOffset[0]+byte1_dlmt+BalloonOffset[1];

	return	200;
}



//		string	iEvent="OnTalk";
//		FindEventTalk(iEvent);
//		script = append_at_talk_start + surface_restore_string() + GetSentence(iEvent) + append_at_talk_end + "\\e";



// Communicate形式検索。該当ありならそのスクリプトを取得、該当なしならfalse。
bool	Satori::TalkSearch(const wstring& iSentence, wstring& oScript, bool iAndMode)
{
	const Talk* talk = talks.communicate_search(iSentence, iAndMode,type_of_communicate_search, *this);
	if ( talk == NULL )
	{
		return false;
	}

	oScript = SentenceToSakuraScriptExec(*talk);
	GetSender().sender() << oScript <<std::endl;
	return	true;
}



