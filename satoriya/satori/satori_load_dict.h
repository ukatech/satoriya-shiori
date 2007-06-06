#ifdef POSIX
#  include "posix_utils.h"
#endif

void	Satori::InitMembers() {

	mRequestMap.clear();
	mRequestID="";

	replace_before_dic.clear();
	replace_after_dic.clear();

	mCharacters.clear();

	m_escaper.clear();

	mDelimiters.clear();
	mDelimiters.insert(",");
	mDelimiters.insert("、");
	mDelimiters.insert("､");
	mDelimiters.insert("，");
	mDelimiters.insert(byte1_dlmt);

	talks.clear();
	words.clear();
	anchors.clear();
	variables.clear();

	mAppendedWords.clear();
	mResponseHistory.clear();

	question_record.clear();

	nade_count.clear();
	nade_valid_time=0;
	insert_nade_talk_at_other_talk=false;
	nade_valid_time_initializer=2;
	nade_sensitivity=60;

	koro_count.clear();
	koro_valid_time=0;

	talk_interval=0;
	talk_interval_random=10;
	talk_interval_count=0;
	rate_of_auto_insert_wait=100;

	append_at_scope_change = "\\n[half]";
	append_at_scope_change_with_sakura_script = "";
	append_at_talk_start = "";
	append_at_talk_end = "";
	append_at_choice_start = "";
	append_at_choice_end = "\\n";

	assert(kakko_replace_history.size()==0);

	surface_restore_at_talk=SR_NORMAL;
	surface_add_value.clear();
	last_talk_exiting_surface.clear();
	next_default_surface.clear();
	cur_surface.clear();
	default_surface.clear();
	last_talk_exiting_surface[1]=next_default_surface[1]=cur_surface[1]=default_surface[1]=10;

	BalloonOffset.clear();
	BalloonOffset[0] = BalloonOffset[1] = "0,0";
	validBalloonOffset.clear();

#ifdef POSIX
	// GetTickCount()はWindows起動後の経過ミリ秒…らしい。
	// それを取得することは出来ないので、エポックミリ秒で代用！
	tick_count_at_load = posix_get_current_millis();
#else
	tick_count_at_load = ::GetTickCount();
#endif
	tick_count_total = 0;

	ghosts_info.clear();
	mCommunicateFor="";
	mCommunicateLog.clear();

	fRequestLog = true;
	fOperationLog = true;
	fResponseLog = true;

	reload_flag = false;
	dic_folder.clear();

	teach_genre="";

	timer.clear();

	on_loaded_script = "";
	on_unloading_script = "";

	//@ mShioriPlugins.clear(); // 特殊。

	secure_flag = false;

	last_choice_name="";

	fEncodeSavedata = false;
	mSaoriArgumentCalcMode = SACM_AUTO;

	reserved_talk.clear();

	second_from_last_talk=0;

	return_empty = false;
	is_dic_loaded = false;
	is_call_ontalk_at_mikire = false;
	nest_limit = 200; //200回も呼ぶな！
	header_script = "";

	mReferences.clear();
	mKakkoCallResults.clear();
	
#ifndef POSIX
	is_single_monitor = true;
	::ZeroMemory(&desktop_rect, sizeof(RECT));
	::ZeroMemory(&max_screen_rect, sizeof(RECT));
	//初期化しない
	//characters_hwnd.clear();
#endif
}
