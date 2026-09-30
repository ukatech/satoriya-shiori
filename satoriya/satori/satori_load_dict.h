#include "posix_utils.h"

void	Satori::InitMembers() {

	mRequestMap.clear();
	mRequestID=L"";

	// コンストラクタで初期化していないメンバ。リクエストのたびに設定されるか、グローバル実体の
	// ゼロ初期化に頼っていたもの（POSIXのnewでは不定になる）
	mIsMateria = false;
	mIsStatusHeaderExist = false;
	mRequestMode = UNKNOWN;
	mOSType = SATORI_OS_UNDEFINED;
	mikire_flag = false;
	kasanari_flag = false;
	can_talk_flag = false;
	mAutoSaveInterval = 0;
	mAutoSaveCurrentCount = 0;
	speaker = 1;
	chars_spoken = 0;
	next_wait_value = 0;

	replace_before_dic.clear();
	replace_after_dic.clear();

	mCharacters.clear();

	m_escaper.clear();

	mDelimiters.clear();
	mDelimiters.insert(L",");
	mDelimiters.insert(L"、");
	mDelimiters.insert(L"､");
	mDelimiters.insert(L"，");
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
	bool_of_action_when_ghost_is_stroked=false;

	koro_count.clear();
	koro_valid_time=0;

	talk_interval=0;
	talk_interval_random=10;
	talk_interval_count=0;
	rate_of_auto_insert_wait=100;
	type_of_auto_insert_wait = 1;

	type_of_communicate_search = COMSEARCH_DEFAULT;

	append_at_scope_change = L"\\n[half]";
	append_at_scope_change_with_sakura_script = L"";
	append_at_talk_start = L"";
	append_at_talk_end = L"";
	append_at_choice_start = L"";
	append_at_choice_end = L"\\n";

	assert(kakko_replace_history.size()==0);

	surface_restore_at_talk=SR_NORMAL;
	surface_restore_at_talk_onetime=SR_INVALID;

	auto_anchor_enable = true;
	auto_anchor_enable_onetime = true;

	auto_newline_enable = true;
	auto_newline_enable_onetime = true;

	load_savedata_status = L"正常";

	surface_add_value.clear();
	last_talk_exiting_surface.clear();
	next_default_surface.clear();
	cur_surface.clear();
	default_surface.clear();
	last_talk_exiting_surface[1]=next_default_surface[1]=cur_surface[1]=default_surface[1]=10;

	BalloonOffset.clear();
	BalloonOffset[0] = BalloonOffset[1] = L"0,0";
	validBalloonOffset.clear();

	sec_count_at_load = posix_get_current_sec();
	sec_count_total = 0;

	dic_load_ext = L"txt";
	dic_load_prefix = L"dic";

	ghosts_info.clear();
	otherghostname.clear();
	mCommunicateFor=L"";
	mCommunicateLog.clear();

	fRequestLog = true;
	fOperationLog = true;
	fResponseLog = true;

	fDebugMode = false;

	reload_flag = false;
	dic_folder.clear();

	teach_genre=L"";

	timer_sec.clear();

	on_loaded_script = L"";
	on_unloading_script = L"";

	//@ mShioriPlugins->clear(); // 特殊。

	secure_flag = false;

	last_choice_name=L"";

	fEncodeSavedata = false;
	mSaoriArgumentCalcMode = SACM_AUTO;

	fDontSaveTimerValue = false;

	reserved_talk.clear();

	second_from_last_talk=0;

	mousedown_time = 0;
	mousedown_exec_complete = false;
	mousedown_secchange_delay_exec = false;
	mousedown_secchange_delay_time = 0;

	is_save_notify = false;

	//喋るごとに初期化する変数
	return_empty = false;
	is_quick_section = false;

	surface_restore_at_talk_onetime = SR_INVALID;
	auto_anchor_enable_onetime = auto_anchor_enable;
	auto_newline_enable_onetime = auto_newline_enable;

	is_dic_loaded = false;
	is_call_ontalk_at_mikire = false;

	m_nest_limit = 200; //200回も呼ぶな！
	m_jump_limit = 20000; //こっちはかなり許容
	m_kakko_size_limit = 256*1024; //1括弧で256KBもあったら大事故だろう

	m_nest_count = 0;
	m_kakko_depth = 0;

	m_total_call_limit = 100000; //1リクエストで10万回も呼ぶな！
	reset_call_budget();

	header_script = L"";

	mReferences.clear();
	mKakkoCallResults.clear();

	mLoopCounters.clear();

#ifndef POSIX
	is_single_monitor = true;
	::ZeroMemory(&desktop_rect, sizeof(RECT));
	::ZeroMemory(&max_screen_rect, sizeof(RECT));
	//初期化しない
	//characters_hwnd.clear();
#endif
}
