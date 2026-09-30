/*---------------------------------------------------------------------------

	『里々（さとり）』

---------------------------------------------------------------------------*/
#ifndef SATORI_H
#define SATORI_H

//---------------------------------------------------------------------------
// 汎用のツール類
#include	"../_/stltool.h"
#include	"../_/simple_stack.h"

// れしばへの送信
#include	"../_/Sender.h"

// SAORI関連
#include	"shiori_plugin.h"

// SakuraDLLとして呼び出されるための構造
#include	"SakuraDLLHost.h"

//---------------------------------------------------------------------------
// 辞書を格納する構造

typedef wstring Word;
typedef strvec Talk;

#include "Families.h"

typedef Families<Word> AllWords;
typedef Families<Talk> AllTalks;
//class AllWords : public Families<Word> {};
//class AllTalks : public Families<Talk> {};

//---------------------------------------------------------------------------
#ifdef	_DEBUG
	#define	DBG(a)	a
#else
	#define	DBG(a)	NULL
#endif	//	_DEBUG

//---------------------------------------------------------------------------
// 定数的な

static const wchar_t	ret_dlmt[3] = { 13, 10, 0 };
static const wchar_t	byte1_dlmt[2] = { 1, 0 };

// 内部特殊表現の開始・終了を表す文字（私用領域）。SJIS時代の 0xff の代わり。
// 使い方： INTERNAL_MARK 種別(INTERNAL_MARK_SCOPE か INTERNAL_MARK_SURFACE) 数値 INTERNAL_MARK
// 種別はSJIS時代は \x01 / \x02 だったが、（バイト値、１）などの引数区切りと衝突するので私用領域の文字にした。
static const wchar_t	INTERNAL_MARK = 0xE0FF;
static const wchar_t	INTERNAL_MARK_STR[2] = { INTERNAL_MARK, 0 };
static const wchar_t	INTERNAL_MARK_SCOPE_STR[2] = { 0xE0FD, 0 };		// スコープ切り替え
static const wchar_t	INTERNAL_MARK_SURFACE_STR[2] = { 0xE0FC, 0 };	// サーフェス加算のための遅延評価

// バージョン文字列とか
extern const wchar_t* gSatoriName;
extern const wchar_t* gSatoriNameW;
extern const wchar_t* gSatoriCraftman;
extern const wchar_t* gSatoriCraftmanW;
extern const wchar_t* gSatoriVersion;
extern const wchar_t* gSatoriLicense;
extern const wchar_t* gShioriVersion;
extern const wchar_t* gSaoriVersion;

static const int RESPONSE_HISTORY_SIZE=64;

// ＄Ｒ／＄Ｓの添字の上限（これ以上は配列を広げない）。Referenceの最大数と同じ。
static const int MAX_ARRAY_INDEX=65536;

enum SurfaceRestoreMode {
	SR_INVALID = -1,
	SR_NONE = 0,
	SR_NORMAL,
	SR_FORCE,
};

//---------------------------------------------------------------------------

class escaper
{
	// エスケープ後文字。
	// 置き換え機能を持たないカッコは、一旦これ+id+半角空白に置き換える。
	// 私用領域の文字を使っている。
	static const wchar_t sm_escape_code[2];

	//map<string, int> m_str2id;
	std::vector<wstring> m_id2str;

public:
	// 引数文字列を受け取り、メンバに格納し、「エスケープされた文字列」を返す。
	wstring insert(const wstring& i_str);
	// 対象文字列中に含まれる「エスケープされた文字列」を元に戻す。
	void unescape(wstring& io_str);
	void unescape_for_dic(wstring& io_str);
	// メンバをクリア
	void clear();
};

//---------------------------------------------------------------------------

//プラグイン。（ShioriPluginじゃなくて、ベースウェアに関連付けられるPLUGIN）
struct PluginInfo
{
	wstring plugin_name;
	wstring plugin_id;
};

//使ってるぞグラフの情報。
struct RateOfUseGraph
{
	wstring ghost_name;
	wstring sakura_name;
	wstring kero_name;
	wstring boot_count;
	wstring boot_minutes;
	wstring boot_percent;
	wstring status;
};

//---------------------------------------------------------------------------

// 内蔵関数の表（satori_builtin.cpp）
struct SatoriFunction;
struct SatoriFunctionTable;

class Satori : public Evalcator, public SakuraDLLHost
{

private:
	wstring	mBaseFolder;	// satori.dllの存在するフォルダ
	wstring	mExeFolder;		// 本体.exeの存在するフォルダ

	// リクエスト内容
	wstring	mStatusLine;	// リクエストの一行目
	strmap	mRequestMap;	// : セパレートされてたkey:value
	wstring	mRequestID;		// SHIORI/3.0 ID
	wstring	mRequestCommand;	// GET, NOTIFY, ...
	wstring	mRequestType;		// SHIORI / SAORI / MAKOTO
	wstring	mRequestVersion;	// 1.0, 2.x, 3.0
	bool	mIsMateria;	// まてりあは特殊処理が要る
	bool    mIsStatusHeaderExist; //ステータスヘッダ対応してるかどうか
	strvec	mReferences;
	// n番目のReference。足りないときは空文字列（リクエストにReferenceが無くても範囲外を読まないように）
	const wstring&	reference_or_empty(size_t n) const {
		static const wstring	empty;
		return	( n < mReferences.size() ) ? mReferences[n] : empty;
	}
	strvec  mKakkoCallResults;
	enum { SAORI, SHIORI2, SHIORI3, MAKOTO2, UNKNOWN } mRequestMode;
	// 格納されたメンバからResponseを作成。返値はステータスコード。
	int		CreateResponse(strmap& oResponse);
	// SHIORI/3.0 IDがOnで始まってた場合、CreateResponseから呼ばれる
	int		EventOperation(wstring iEvent, std::map<wstring,wstring> &oResponse);

	// 戻り値map
	strmap mResponseMap;

	// 置き換え辞書
	strmap	replace_before_dic;
	strmap	replace_after_dic;

	inimap	mCharacters;	// characters.iniの内容

	// 文字列のエスケープと書き戻し
	escaper m_escaper;

	// SAORI/内部命令を呼び出す際の引数区切りとなる文字列群
	std::set<wstring>	mDelimiters;

	// 全ての＊トーク
	AllTalks	talks;
	// ＠定義 ...ジャンル分けされた複数の単語。
	AllWords	words;


	// ＄変数
	strmap	variables;
	// 自動アンカー
	std::vector<wstring>	anchors;

	// 変数の消去。何かと問題があるらしいよ？
	void	erase_var(const wstring& key);

	// 動的に登録された単語。wordsにも収録する。satori_savedata.txtに保存するのが目的。
	std::map<wstring, std::vector<Word> >	mAppendedWords;

	// 戻したトークの履歴
	std::deque<wstring>	mResponseHistory;

	// call引数stack 実装自体は再帰。
	simple_stack<strvec>	mCallStack;

	// 単語チェイン
	//map<string, set<string> >	mRelationalWord;	// <単語, <属性> > 
	//set<string>	mUsedRelation;	// <使用された属性> １トークでリセット。明示リセットも可。

	// 選択分岐の記録    map<ID, pair<NUMBER, LABEL> >　
	// よく考えたら、選択項目とるだけなら全く不要だった。まぁ全体取得できても悪くは無いけど
	std::map<wstring, std::pair<int, wstring> >	question_record;

	int	second_from_last_talk;	// 最後に喋ってからの経過時間

	// なでられ回数。keyはなでられ箇所、valueは回数。
	strintmap	nade_count;
	// なでられ有効期間
	int	nade_valid_time;
		// SecondChange毎に減少、３秒間moveがこないとnade_countを全クリア。
		// nade_countがnade_sensitivityを越えるとそのイベントが発生、nade_countを全クリア。
	bool	insert_nade_talk_at_other_talk;	// 喋ってる最中のなで反応有無
	int		nade_valid_time_initializer;	// なでられ持続秒数（なでセッションの期限）
	int		nade_sensitivity;				// なでられ我慢回数（発動までの回数）
	bool	bool_of_action_when_ghost_is_stroked; //ゴーストがなでられた際の実行イベントの切り替え用

	unsigned int mousedown_time;
	strvec mousedown_reference_array;
	bool mousedown_exec_complete;

	bool mousedown_secchange_delay_exec;
	unsigned int mousedown_secchange_delay_time;

	strintmap	koro_count;	// ころころ回数
	int	koro_valid_time;	// ころころ有効期間

	// 喋り間隔（秒）。０なら黙る。＄変数
	int	talk_interval;
	// 喋り間隔誤差（間隔に対する％）＄変数
	int	talk_interval_random;
	// 次回自発喋りまでの時間。喋るとリセット。＄変数
	int	talk_interval_count;
	// 自動挿入ウェイトの倍率。省略時100。
	int	rate_of_auto_insert_wait;
	// 自動挿入処理のタイプ。省略時従来処理＝1。0で無効、2で一般的な処理。
	int type_of_auto_insert_wait;
	// 見切れてても喋る（OnTalkを呼び出す）かどうかフラグ
	bool is_call_ontalk_at_mikire;
	// コミュニケート検索のタイプ
	FamilyComSearchType type_of_communicate_search;

	// 付加文字列
	wstring	append_at_scope_change;
	wstring	append_at_scope_change_with_sakura_script;
	wstring	append_at_talk_start;
	wstring	append_at_talk_end;
	wstring	append_at_choice_start;
	wstring	append_at_choice_end;

	// しゃべり管理。SentenceToSakuraScriptExecの未再帰呼び出し時に初期化。
	int		speaker;		// 話者
	std::set<int>	speaked_speaker;		// 少しでも喋った？
	bool	is_speaked(int n) { return speaked_speaker.find(n) != speaked_speaker.end(); }
	bool	is_speaked_anybody() { return !speaked_speaker.empty(); }
	void    reset_speaked_status() {
		speaked_speaker.clear(); // 少しでも喋ったかどうか
		surface_changed_before_speak.clear(); // 会話前にサーフェス切り換え指示があったか
	}

	int		chars_spoken;
	int		next_wait_value;
	//int		question_num;

	std::map<int,bool> surface_changed_before_speak;	// 会話前にサーフェスが切り替え指示があった？


	// 過去のカッコ置き換えを記憶。反復（Ｈ？）で使用
	simple_stack<strvec>	kakko_replace_history;	

	// 会話時サーフェス戻し・＄変数
	enum SurfaceRestoreMode	surface_restore_at_talk;
	enum SurfaceRestoreMode	surface_restore_at_talk_onetime;

	bool auto_anchor_enable;
	bool auto_anchor_enable_onetime;

	bool auto_newline_enable;
	bool auto_newline_enable_onetime;

	wstring load_savedata_status;

	std::map<int, int>	default_surface;
	std::map<int, int>	surface_add_value;
	std::map<int, int>	next_default_surface; // 途中でdef_surfaceを切り換えても、そのrequestでは使わない
	wstring	surface_restore_string();

	// 返り値抑止
	bool return_empty;

	//クイックセクションかどうか
	bool is_quick_section;

	// スクリプトヘッダ
	wstring header_script;

	// 無限呼び出し抑止
	int m_nest_limit;
	int m_jump_limit;
	int m_kakko_size_limit;

	int m_nest_count;

	// 括弧（（）の入れ子の深さ。深さの上限を越えたら展開せずに打ち切る（スタックを使い果たさないため）
	int m_kakko_depth;

	// 呼び出し総数の制限。深さではなく総数で、分岐する再帰や、本体が空の長い繰り返しを止める。
	// （名前）の呼び出しと繰り返しの1周を1回と数える。0なら無制限。リクエスト・読み込みごとに数え直す。
	int m_total_call_limit;
	int m_total_call_count;
	bool m_total_call_reported;
	// 呼び出し総数の予算を1つ使う。使い切っていたらtrueを返す（呼び出し側は処理を打ち切ること）。
	bool use_call_budget();
	void reset_call_budget() { m_total_call_count = 0; m_total_call_reported = false; }

	// ばるーん位置
	std::map<int, bool>	validBalloonOffset;	// 1回でも設定されたら有効 つーか片方だけだと意味無かった。むぅ。
	std::map<int, wstring>	BalloonOffset;

	// 時間系情報取得用
	unsigned long sec_count_at_load, sec_count_total;

	// 辞書拡張子・接頭辞
	wstring dic_load_ext, dic_load_prefix;

	// 外部から実行可能なイベントの接頭辞
	strvec allow_external_event_prefixes;

	// サーフェス
	std::map<int, int>	cur_surface;
	std::map<int, int>	last_talk_exiting_surface;
	// 毎秒更新される「状態」
	bool	mikire_flag,kasanari_flag,can_talk_flag;

	// COMMUNICATE周り
#ifdef POSIX
	bool updateGhostsInfo() { return true; } // 何もしない。
#else
	bool	updateGhostsInfo();	// FMOから情報取得
#endif
	std::vector<strmap>	ghosts_info;	// FMOの内容そのまま。0は自分自身、1～は順番どおり 自分自身はghosts_infoには *含まれる*
	std::set<wstring>    otherghostname; // NOTIFY otherghostnameから拾ったsakura名 自分自身はotherghostnameには *含まない*
	strmap*	find_ghost_info(wstring name);	// ghosts_infoを検索、特定のゴーストの情報を得る

	wstring	mCommunicateFor;	// 話しかけ対象ゴースト。→で設定されresponseにToをつける
	std::set<wstring>	mCommunicateLog;	// 会話ログ。繰り返しがあった場合は会話打ち切り

	// でばぐもーど
	bool    fDebugMode;

	// 各セクションのログ吐き有無。
	bool	fRequestLog, fOperationLog, fResponseLog;

	// 変数によりリロードが指示されたらON。
	bool	reload_flag;

	// 辞書フォルダ
	strvec	dic_folder;

	// TEACHされる変数名
	wstring	teach_genre;

	// タイマ名：発話までの秒数
	strintmap	timer_sec;

	// トークの予約
	std::map<int, wstring>	reserved_talk;

	// 「独自イベントによるmateriaイベントの置き換え」のためのスクリプト入れ
	wstring	on_loaded_script;
	wstring	on_unloading_script;

	// 栞プラグイン
	ShioriPlugins	*mShioriPlugins;
	bool calc_argument(const wstring &iExpression, int &oResult, bool for_non_talk);

	// 安全？
	bool	secure_flag;

	// まともに辞書読み込みできたかどうか
	bool	is_dic_loaded;

	// 直前の表示選択肢
	wstring	last_choice_name;

	// セーブデータ保存時の暗号化有無
	bool	fEncodeSavedata;
	// さおり引数を計算するか
	enum { SACM_ON, SACM_OFF, SACM_AUTO } mSaoriArgumentCalcMode;
	// タイマ変数はセーブしない
	bool	fDontSaveTimerValue;

	// 自動セーブ間隔
	int	mAutoSaveInterval;
	int	mAutoSaveCurrentCount;

	// 辞書情報
	int	numWord, numParentheres, numSurfaceChange,
		numDictionary, numDictionarySize;

	//Notifyの保存
	std::set<wstring> installed_ghost_name;
	std::set<wstring> installed_shell_name;
	std::set<wstring> installed_balloon_name;
	std::set<wstring> installed_headline_name;
	std::set<wstring> installed_font_name;
	std::map<wstring, PluginInfo> installed_plugin;
	std::map<wstring, RateOfUseGraph> rate_of_use_graph;

	//Notifyの収拾
	bool is_save_notify;

	//satori_config.txtから読んだやつ
	bool is_utf8_dic;
	bool is_utf8_replace;
	bool is_utf8_savedata;
	bool is_utf8_charactersini;

	// システム情報系 ----------

//	enum { UNDEFINED, WIN95, WIN98, WINME, WINNT, WIN2K, WINXP } mOSType;
	enum { SATORI_OS_UNDEFINED, SATORI_OS_WIN95, SATORI_OS_WIN98, SATORI_OS_WINME, SATORI_OS_WINNT, SATORI_OS_WIN2K, SATORI_OS_WINXP } mOSType;

#ifndef POSIX
	// マルチモニタ
	bool	is_single_monitor;	// 最上位。これがfalseならば以下を使ってはいけない
	RECT	desktop_rect;
	RECT	max_screen_rect;
#endif

	//HWNDテーブル(POSIXにはHWNDはないのでvoid*とする)
	std::map<int, void*>	characters_hwnd;

	// ループ時のカウンタ参照用
	simple_stack<wstring> mLoopCounters; //バグを誘発させそうなのでstring

	// メンバ関数

	void	InitMembers();

	int	 LoadDicFolder(const wstring& path);
	bool LoadDictionary(const wstring& filename,bool warnFileName,bool isUTF8);

	wstring	GetWord(const wstring& name);

	wstring	KakkoSection(const wchar_t*& p,bool for_calc = false,bool for_non_talk = false);
	wstring	UnKakko(const wchar_t* p,bool for_calc = false,bool for_non_talk = false);

	bool	GetURLList(const wstring& name, wstring& result);
	bool	GetRecommendsiteSentence(const wstring& name, wstring& result);

	// 指定イベント無い場合、イベント名の置き換えもしてioeventを返す。それでも無いならfalse
	bool	FindEventTalk(wstring& ioevent);

	// Communicate形式検索。該当なしならfalse。and_modeがtrueなら全単語一致以外は無効とする
	bool	TalkSearch(const wstring& iSentence, wstring& oScript, bool iAndMode);

	// システム変数設定時の動作（satori_builtin.cpp）
	// return = SYSVAR_NONE(処理なし) / SYSVAR_SET(処理した) / SYSVAR_NOSET(処理したけど変数設定してはだめ)
	enum { SYSVAR_NONE = 0, SYSVAR_SET = 1, SYSVAR_NOSET = -1 };
	int	system_variable_operation(wstring key, wstring value, wstring* result=NULL);
	int	system_variable_operation_real(wstring key, wstring value, wstring* result); //内部処理用(直接使わないで)

	// 内部。返値は続行の有無。続行時はSentenceNameをGetSentence。
	const Talk* GetSentenceInternal(wstring& ioSentenceName);

	// 式を評価し、結果の真偽値を返す
	bool evalcate_to_bool(const Condition& i_cond);

	// 引数に渡されたものを何かの名前であるとし、置き換え対象があれば置き換える。
	bool	CallReal(const wstring& word, wstring& result, bool for_calc, bool for_non_talk, bool use_arg_callstack);

	wstring* GetValue(const wstring &key,bool &oIsSysValue,bool iIsExpand = false,bool *oIsExpanded = NULL,const wchar_t *pDefault = L"");
	bool IsArrayValue(const wstring &iName,int &ref,wchar_t &firstChar);

	void surface_restore_string_addfunc(wstring &str, std::map<int, int>::const_iterator &i);

	// 辞書読み込み内部実装用関数
	bool select_dict_and_load_to_vector(const wstring& iFileName, strvec& oFileBody, bool warnFileName, CharactorSet cs);

	// SentenceToSakuraScriptExecの実体。
	int SentenceToSakuraScriptInternal(const Talk &vec,wstring &result,wstring &jumpto,std::ptrdiff_t &ip);

	// 変数代入
	bool SubstVariable(const wstring &key,wstring &value,wstring &result,bool do_calc);

	// ウインドウ探索
	unsigned long FindTopLevelWindow(const wchar_t* txt,bool isPartial);

	// プロセス探索
	unsigned long FindProcessName(const wchar_t* txt,bool isPartial);

	// count
	int count_func(const wstring &name);

	//---------------------------------------------------------------------------
	// 内蔵関数・内蔵変数・システム変数（satori_builtin.cpp）
	// 名前と処理の対応はsatori_builtin.cppの表にある。足すときは処理をここに宣言し、表に1行加える。

	// iNameがSAORIか内蔵関数の呼び出しなら呼んでoResultに結果を入れ、trueを返す。
	bool	CallFunction(const wstring& iName, wstring& oResult, bool use_arg_callstack);
	// 文章中の（の直後pが「特殊形式の名前＋区切り」なら呼び出してtrueを返す。
	bool	CallSpecialFunction(const wchar_t*& p, wstring& oResult, bool for_calc, bool for_non_talk);
	// iNameが内蔵変数なら値をoResultに入れ、trueを返す。
	bool	GetBuiltinValue(const wstring& iName, wstring& oResult);

	static const SatoriFunctionTable&	function_table();
	static const SatoriFunction*	find_function(const wstring& iName);

	// 繰り返しの結果が大きすぎたら警告してtrueを返す
	bool	loop_result_too_large(const wstring& iResult);

	// 内蔵関数：（名前、引数…）
	wstring	func_set(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_loop(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_call(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_vncall(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_equal(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_nop(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_sync(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_remember(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_erase_variables(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_copy_variables(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_list_variables(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_split_to(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_byte_value(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_synthesized_words(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_talk_count(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_add_word(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_remove_added_word(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_remove_all_added_words(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_get_property(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_set_property(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_load_saori(const strvec& iArgv, bool for_calc, bool for_non_talk);
	// 特殊形式：引数を展開せずに受け取る
	wstring	func_when(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_whenlist(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_times(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_while(const strvec& iArgv, bool for_calc, bool for_non_talk);
	wstring	func_for(const strvec& iArgv, bool for_calc, bool for_non_talk);

	// 内蔵変数：（名前）。iNameは名前全体、iArgは名前から前後の決まった部分を除いたもの、iParamは表で指定した値。
	enum { TIME_YEAR, TIME_MONTH, TIME_DAY, TIME_HOUR, TIME_MINUTE, TIME_SECOND, TIME_WEEKDAY };
	enum {
		UPTIME_HOUR, UPTIME_MINUTE, UPTIME_SECOND, UPTIME_TOTAL_HOURS, UPTIME_TOTAL_MINUTES, UPTIME_TOTAL_SECONDS,
		UPTIME_KIND_MASK = 0x0F,
		UPTIME_FROM_LOAD = 0x00, UPTIME_FROM_OS = 0x10, UPTIME_TOTAL = 0x20,
		UPTIME_FROM_MASK = 0xF0
	};
	enum { VARIABLE_EXISTS, VARIABLE_OR_ZERO, VARIABLE_OR_EMPTY };
	enum { INSTALLED_GHOST, INSTALLED_SHELL, INSTALLED_BALLOON, INSTALLED_HEADLINE, INSTALLED_FONT, INSTALLED_PLUGIN };
	enum { GRAPH_SAKURA_NAME, GRAPH_KERO_NAME, GRAPH_BOOT_COUNT, GRAPH_BOOT_MINUTES, GRAPH_BOOT_PERCENT, GRAPH_STATUS };
	bool	var_version(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_license(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_current_time(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_uptime(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_time_t(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_seconds_from_last_talk(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_random(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_isempty(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_argument_count(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_surface(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_last_exiting_surface(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_window_handle(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_neighbor_ghost(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_ghost_count(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_running_ghost_exists(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_ghost_exists(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_ghost_surface(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_fmo(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_talk_exists(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_talk_count(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_word_exists(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_word_count(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_word_used_all(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_variable(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_count(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_savedata_status(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_next_talk(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_reserved_talk(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_is_talk_reserved(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_reserved_talk_count(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_event_name(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_last_choice_name(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_base_folder(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_exe_folder(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_installed(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_plugin_id(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_rate_of_use_graph(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_window_exists(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);
	bool	var_process_exists(const wstring& iName, const wstring& iArg, int iParam, wstring& oResult);

	// システム変数：＄名前＝値。iKeyは変数名全体、iArgは変数名から前後の決まった部分を除いたもの。
	int	sysvar_talk_interval(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_talk_interval_random(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_header_script(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_nest_limit(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_kakko_size_limit(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_jump_limit(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_total_call_limit(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_surface_restore(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_surface_restore_onetime(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_auto_anchor(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_auto_newline(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_auto_insert_wait_rate(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_auto_insert_wait_type(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_stroked_event(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_nade_valid_time(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_nade_sensitivity(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_log(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_sender(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_delay_save_count(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_communicate_search(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_dic_folder(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_reload(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_savedata_status(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_save(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_auto_save_interval(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_save_notify(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_word_overlap(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_talk_overlap(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_teach(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_next_talk(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_reserve_talk(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_cancel_reserved_talk(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_clear_timers(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_timer(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_surface_add_value(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_default_surface(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_balloon_offset(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_saori_argument_calc(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_add_delimiter(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_remove_delimiter(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_response_value(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_response_header(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);
	int	sysvar_external_event_prefixes(const wstring& iKey, const wstring& iArg, const wstring& iValue, wstring* oResult);

public:

	Satori();
	virtual ~Satori();

	// SHIORI/3.0インタフェース
	virtual bool load(const wstring& i_base_folder);
	virtual bool unload();
	// リクエスト処理中に例外で中断されたとき、途中だった呼び出しの状態を初期化する
	virtual void on_request_exception();
	virtual wstring getversionlist(const wstring& i_base_folder);
	virtual int	request(
		const wstring& i_protocol,
		const wstring& i_protocol_version,
		const wstring& i_command,
		const strpairvec& i_data,
		
		wstring& o_protocol,
		wstring& o_protocol_version,
		strpairvec& o_data);

	// 変数等のデータをファイルに保存
	bool	Save(bool isOnUnload=false);	

	// strvecからさくらスクリプトを生成する
	wstring	SentenceToSakuraScriptExec(const strvec& vec);
	// strvecにプリプロセスを掛けた後、さくらスクリプトを生成する。さとりて用
	wstring	SentenceToSakuraScriptExec_with_PreProcess(const strvec& vec);
	// 指定された名前の＊文を取得する
	wstring	GetSentence(const wstring& name);
	// 引数に渡されたものを何かの名前であるとし、置き換え対象があれば置き換える。
	bool	Call(const wstring& word, wstring& result, bool for_calc = false, bool for_non_talk = false, bool use_arg_callstack = false);
	// 里々レベルでの計算を行う。戻り値は成否。
	bool calculate(const wstring& iExpression, wstring& oResult);

	// 最終置き換え処理。置換後のスクリプトが中身が無い（実行してもしなくても一緒）と判断したらfalseを返す。
	bool	Translate(wstring& script);

	void SetCharacterHWnd(void* hWnd)
	{
		std::map<int,void*>::const_iterator found = characters_hwnd.find(0);
		if ( found == characters_hwnd.end() ) {
			characters_hwnd[0] = hWnd;
		}
	}
};

//---------------------------------------------------------------------------

bool	calc(wstring&,bool isStrict = false);
void	diet_script(wstring&);
// 文・さくらスクリプト変換の再帰の深さ（静的な変数）を0に戻す
void	reset_nest_counters();

//---------------------------------------------------------------------------
#endif




