/*---------------------------------------------------------------------------

	wXiƂ�x

---------------------------------------------------------------------------*/
#ifndef SATORI_H
#define SATORI_H


//---------------------------------------------------------------------------
// ėp̃c[�
#include	"../_/stltool.h"
#include	"../_/simple_stack.h"

// ꂵ΂ւ̑M
#include	"../_/Sender.h"

// SAORI֘A
#include	"shiori_plugin.h"

// SakuraDLLƂČĂяo�߂̍\
#include	"SakuraDLLHost.h"

//---------------------------------------------------------------------------
// �[�

typedef string Word;
typedef strvec Talk;

#include "Families.h"
class AllWords : public Families<Word> {};
class AllTalks : public Families<Talk> {};

//---------------------------------------------------------------------------
#ifdef	_DEBUG
	#define	DBG(a)	a
#else
	#define	DBG(a)	NULL
#endif	//	_DEBUG

//---------------------------------------------------------------------------
// 萔I�

static const char	ret_dlmt[3] = { 13, 10, 0 };
static const char	byte1_dlmt[2] = { 1, 0 };

// o[W�
extern const char* gSatoriName;
extern const char* gSatoriNameW;
extern const char* gSatoriCraftman;
extern const char* gSatoriCraftmanW;
extern const char* gSatoriVersion;
extern const char* gShioriVersion;
extern const char* gSaoriVersion;

static const int RESPONSE_HISTORY_SIZE=64;

//---------------------------------------------------------------------------

class escaper
{
	// GXP[v㕶B
	// u@\ȂJbŔA��id+p���ɒu�
	// R[h�sjis̈ꕶƂĈ�邪AY镶͖B_ȈāB
	static const char sm_escape_sjis_code[3];

	//map<string, int> m_str2id;
	vector<string> m_id2str;

public:
	// �󂯎�AoɊi[AuGXP[vꂽ��B
	string insert(const string& i_str);
	// Ώە񒆂Ɋ܂܂�uGXP[vꂽ�쳂ɖ߂B
	void unescape(string& io_str);
	// o�A
	void clear();
};

//---------------------------------------------------------------------------
class Satori : public Evalcator, public SakuraDLLHost
{

private:
	string	mBaseFolder;	// satori.dll݂̑�H_
	string	mExeFolder;		// {�exȇ݂�H_

	// NGXg�
	string	mStatusLine;	// NGXg̈��
	strmap	mRequestMap;	// : Zp[g�key:value
	string	mRequestID;		// SHIORI/3.0 ID
	string	mRequestCommand;	// GET, NOTIFY, ...
	string	mRequestType;		// SHIORI / SAORI / MAKOTO
	string	mRequestVersion;	// 1.0, 2.x, 3.0
	bool	mIsMateria;	// ܂Ă肠͓Nꏈv�
	strvec	mReferences;
	strvec  mKakkoCallResults;
	enum { SAORI, SHIORI2, SHIORI3, MAKOTO2, UNKNOWN } mRequestMode;
	// i[ꂽo�esponse�BԒl̓Xe[^XR[hB
	int		CreateResponse(strmap& oResponse);
	// SHIORI/3.0 IDOnŎn܂BĂꍇACreateResponse�΂�
	int		EventOperation(string iEvent, map<string,string> &oResponse);

	// ߂�map
	strmap mResponseMap;

	// u
	strmap	replace_before_dic;
	strmap	replace_after_dic;

	inimap	mCharacters;	// characters.ini̓�

	// �GXP[vƏ߂
	escaper m_escaper;

	// SAORI/Ք߂�яoۂ̈��Ȃ镶�
	set<string>	mDelimiters;

	// SẮg[N
	AllTalks	talks;
	// � ...Wꂽ̒P�
	AllWords	words;


	// ϐ
	strmap	variables;
	// AJ[
	vector<string>	anchors;

	// ϐ̏KBƖ���
	void	erase_var(const string& key);

	// Iɓo^ꂽP�wordsɂ�^�satori_savedata.txtɕۑ�ړIB
	map<string, vector<const Word*> >	mAppendedWords; // setłȂvectorȂ̂́ud�F/~vɔ������āB

	// ߂g[N̗�
	deque<string>	mResponseHistory;

	// callstack ͍̂ċAB
	simple_stack<strvec>	mCallStack;

	// P�FC
	//map<string, set<string> >	mRelationalWord;	// <P� <> > 
	//set<string>	mUsedRelation;	// <gpꂽ> Pg[NŃZbgBZbg�B

	// I𕪊�L^    map<ID, pair<NUMBER, LABEL> >@
	// 悭l�I퀖ڂƂ邾Ȃ�svBB܂S̎擾łĂȫ͖�
	map<string, pair<int, string> >	question_record;

	int	second_from_last_talk;	// Ō�BĂ�oߎ�

	// Ȃł�񐔁Bkey͂Ȃł�ӏAvalue͉񐔁B
	strintmap	nade_count;
	// Ȃł�L�
	int	nade_valid_time;
		// SecondChangeɌARb�oveȂ�ade_count�NAB
		// nade_countnade_sensitivity��̃CxgAnade_count�NAB
	bool	insert_nade_talk_at_other_talk;	// BĂ�̂ȂŔL
	int		nade_valid_time_initializer;	// Ȃł�biȂŃZbV̊j
	int		nade_sensitivity;				// Ȃł�䖝񐔁i܂ł̉񐔁j


	strintmap	koro_count;	// 낱�
	int	koro_valid_time;	// 낱��

	// �uibjBOȂ��ϐ
	int	talk_interval;
	// �u덷iԊuɑ΂道jϐ
	int	talk_interval_random;
	// ����ł̎ԁB�ZbgBϐ
	int	talk_interval_count;
	// }EFCg̔{Bȗ100B
	int	rate_of_auto_insert_wait;
	// ؂�Ăҝ�OnTalk�яojǂtO
	bool is_call_ontalk_at_mikire;

	// tU�
	string	append_at_scope_change;
	string	append_at_scope_change_with_sakura_script;
	string	append_at_talk_start;
	string	append_at_talk_end;
	string	append_at_choice_start;
	string	append_at_choice_end;

	// ��BSentenceToSakuraScript̖ċAĂяoɏB
	int		speaker;		// b�
	set<int>	speaked_speaker;		// łҝBH
	bool	is_speaked(int n) { return speaked_speaker.find(n) != speaked_speaker.end(); }
	bool	is_speaked_anybody() { return speaked_speaker.size()>0; }
	int		characters;
	int		question_num;
	set<int>	surface_changed_before_speak;	// �OɃT[tFX؂�wBH


	// ߋ̃JbRu�BigHjŎgp
	// SentenceToSakuraScript̍ċAɓ�
	simple_stack<strvec>	kakko_replace_history;	

	// �T[tFX߂Eϐ
	bool	surface_restore_at_talk;
	map<int, int>	default_surface;
	map<int, int>	surface_add_value;
	map<int, int>	next_default_surface; // r�ef_surface�芷Ă��equestł͎g�
	string	surface_restore_string();

	// Ԃ�}~@\
	bool return_empty;

	// XNvgwb_@\
	string header_script;

	// Ăяo}~@\
	int nest_limit;

	// ΂��u
	map<int, bool>	validBalloonOffset;	// 1��肳ꂽ� [ЕƈӖBBނB
	map<int, string>	BalloonOffset;

	// Ԍn�擾p
#ifdef POSIX
	unsigned long tick_count_at_load, tick_count_total;
#else
	DWORD	tick_count_at_load, tick_count_total;
#endif
	
	// T[tFX
	map<int, int>	cur_surface;
	map<int, int>	last_talk_exiting_surface;
	// bXV�u�v
	bool	mikire_flag,kasanari_flag,can_talk_flag;

	// COMMUNICATE�
#ifdef POSIX
	bool updateGhostsInfo() { return true; } // µȂB
#else
	bool	updateGhostsInfo();	// FMO��
#endif
	vector<strmap>	ghosts_info;	// FMO̓�̂܂܁B0͎gA1`͏Ԃǂ�
	strmap*	find_ghost_info(string name);	// ghosts_info쟍�R�S[Xg̏�𓾂�

	string	mCommunicateFor;	// bΏۃS[XgBŐݒ肳�esponse�o��
	set<string>	mCommunicateLog;	// �OBJ�Bꍇ͉�ł؂�

	// eZNṼOfLB
	bool	fRequestLog, fOperationLog, fResponseLog;

	// ϐɂ�[hwꂽ�NB
	bool	reload_flag;
	// tH_
	strvec	dic_folder;

	// TEACH�ϐ
	string	teach_genre;

	// ^C}Fb܂ł̕b
	strintmap	timer;

	// g[N̗\�
	map<int, string>	reserved_talk;

	// uƎCxgɂ�materiaCxg̒uv̂߂̃XNvg�
	string	on_loaded_script;
	string	on_unloading_script;

	// xvOC
	ShioriPlugins	mShioriPlugins;
	string	inc_call(const string&, const strvec&, strvec&, bool is_secure);

	// SH
	bool	secure_flag;

	// ܂Ƃ�ǂݍ݂łǂ
	bool	is_dic_loaded;

	// O̕\I�
	string	last_choice_name;

	// Z[uf[^ۑ̈ÍL
	bool	fEncodeSavedata;
	// ��Z邩
	enum { SACM_ON, SACM_OFF, SACM_AUTO } mSaoriArgumentCalcMode;

	// Z[uԊu
	int	mAutoSaveInterval;
	int	mAutoSaveCurrentCount;

	// �
	int	numWord, numParentheres, numSurfaceChange,
		numDictionary, numDictionarySize;

	// VXe�n ----------

	enum { UNDEFINED, WIN95, WIN98, WINME, WINNT, WIN2K, WINXP } mOSType;

#ifndef POSIX
	// }`j^
	bool	is_single_monitor;	// ŏ�BꂪfalseȂ�ȉ�BĂ͂Ȃ
	RECT	desktop_rect;
	RECT	max_screen_rect;
	map<int, HWND>	characters_hwnd;
#endif

	// o֐

	void	InitMembers();

	int	 LoadDicFolder(const string& path);
	bool LoadDictionary(const string& filename,bool warnFileName = true);

	string	GetWord(const string& name);

	string	KakkoSection(const char*& p,bool for_calc = false);
	string	UnKakko(const char* p,bool for_calc = false);

	bool	GetURLList(const string& name, string& result);
	bool	GetRecommendsiteSentence(const string& name, string& result);

	// w�xgꍇACxg̒uµ�oevent�B�ֳȂ�alse
	bool	FindEventTalk(string& ioevent);

	// Communicate`�YȂȂ�alseBand_modetrueȂ�P�vȊO͖Ƃ�
	bool	TalkSearch(const string& iSentence, string& oScript, bool iAndMode);

	// VXeϐݒ莞̓�
	bool	system_variable_operation(string key, string value, string* result=NULL);

	// ՔBԒl͑s̗LBs�entenceName�etSentenceB
	bool	GetSentence(string& ioSentenceName, string& oResultScript);

	// �Aʂ̐^Ul�
	bool evalcate_to_bool(const Condition& i_cond);

	// ɓnꂽ�齂̖Oł�AuΏۂ�u�
	bool	CallReal(const string& word, string& result);

public:

	Satori() {}
	~Satori() {}

	// SHIORI/3.0C^tF[X
	virtual bool load(const string& i_base_folder);
	virtual bool unload();
	virtual int	request(
		const string& i_protocol,
		const string& i_protocol_version,
		const string& i_command,
		const strpairvec& i_data,
		
		string& o_protocol,
		string& o_protocol_version,
		strpairvec& o_data);

	// ϐ̃f[^�@Cɕۑ
	bool	Save(bool isOnUnload=false);	

	// strvec炳�Nvg𐶐�
	string	SentenceToSakuraScript(const strvec& vec);
	// strvecɃvvZX���Nvg𐶐�Ƃ�p
	string	SentenceToSakuraScript_with_PreProcess(const strvec& vec);
	// w肳ꂽÓ��
	string	GetSentence(const string& name);
	// ɓnꂽ�齂̖Oł�AuΏۂ�u�
	bool	Call(const string& word, string& result);
	// Xxł̌vZ�B߂�͐ہB
	bool calculate(const string& iExpression, string& oResult);

	// ŏIuBu�XNvggi
sĂµȂĂ�jƔf�alse�B
	bool	Translate(string& script);

};

//---------------------------------------------------------------------------
string	int2zen(int i);
string	zen2han(string str);
bool	calc(string&);
void	diet_script(string&);




//---------------------------------------------------------------------------
#endif


