#include	"satori.h"

#include	<fstream>
#include	<cassert>
#include <algorithm>

#include	"../_/Utilities.h"
#include	"satori_load_dict.h"

#ifdef POSIX
#  include <iostream>
#  include "stltool.h"
#endif

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

struct satori_unit
{
	string typemark;
	string name;
	string condition;
	strvec body;
};
#ifdef _DEBUG
std::ostream& operator<<(std::ostream& o, const satori_unit& su)
{
	o << su.typemark << "/" << su.name << "/" << su.condition << std::endl;
	o << su.body;
	return o;
}
#endif

static void lines_to_units(
	const std::vector<string>& i_lines,
	const std::vector<string>& i_typemarks,
	const string& i_name_cond_delimiter,
	std::vector<satori_unit>& o_units)
{
	std::vector<string>::const_iterator line_it = i_lines.begin();
	for ( ; line_it != i_lines.end() ; ++line_it)
	{
		//cout << *line_it << std::endl;

		// 行頭にtypemarksのいずれかが出現しているか探す
		std::vector<string>::const_iterator mark_it = i_typemarks.begin();
		for ( ; mark_it != i_typemarks.end() ; ++mark_it) 
		{
			if ( line_it->compare(0, mark_it->size(), mark_it->c_str()) ==0 )
			{
				break;
			}
		}
		
		// 出現してたらヘッダ、さもなくばボディ
		if ( mark_it != i_typemarks.end() )
		{
			satori_unit unit;
			unit.typemark = *mark_it;
			
			const char* name = line_it->c_str() + mark_it->size();
			const char* delimiter = strstr_hz(name, i_name_cond_delimiter.c_str());
			if ( delimiter==NULL )
			{
				unit.name.assign(name);
			}
			else
			{
				unit.name.assign(name, delimiter);
				unit.condition.assign(delimiter + i_name_cond_delimiter.size());
			}

			o_units.push_back(unit);
		}
		else
		{
			if ( o_units.empty() ) 
			{
				// o_unitsが空のとき（最初のtypemarks出現前）はコメントと見做し、何もしない。
			}
			else
			{
				// 最後のo_unitsにミを追加
				o_units.back().body.push_back(*line_it);
			}
		}
	}

	// 各unit末尾の空行を削る
	for ( std::vector<satori_unit>::iterator i = o_units.begin() ; i != o_units.end() ; ++i )
	{
		while (true)
		{
			strvec::iterator j = i->body.end();
			if ( j == i->body.begin() || (--j)->length()>0 )
			{
				break;
			}
			i->body.pop_back();
		}

		//GetSender().sender() << *i << std::endl;
	}
}


// 極めて高確率でUTF-8な文字列を検出
static bool is_utf8_dic(const strvec& in)
{
	unsigned int possible_utf8_count = 0;
	static char possible_3byte_table[] = "\x83\x84\x86\x88\x89\x8a\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9a\x9e\xa0\xbf"; //＃＄＆（）＊０～９：＞＠＿

	for ( strvec::const_iterator fi=in.begin() ; fi!=in.end() ; ++fi )
	{
		const char* p = fi->c_str();

		const char* ps = strstr(p,"\xef\xbc");

		while ( ps ) {
			char c = ps[2]; //3バイト目

			if ( strchr(possible_3byte_table,c) ) {
				possible_utf8_count += 1;
				if ( possible_utf8_count >= 10 ) {
					return true;
				}
			}

			ps = strstr(ps+3,"\xef\xbc");
		}
	}

	return false;
}

// プリプロセス的な処理。
// 改行キャンセル適用、コメント削除、before_replaceの適用
static bool pre_process(
	const strvec& in,
	strvec& out,

	escaper& io_escaper,
	strmap& io_replace_dic
	
	)
{
	int	kakko_nest_count=0;	// "（" のネスト数。1以上の場合は改行を無効化する。
	string	accumulater="";	// 行あきゅむれーた
	int	line_number=1;
	for ( strvec::const_iterator fi=in.begin() ; fi!=in.end() ; ++fi, ++line_number )
	{
		const char* p=fi->c_str();
		bool	escape = false;

		// カッコ内の場合、行頭のタブは無視する。
		if ( kakko_nest_count>0 )
			while ( *p=='\t' )
				++p;

		// 一行（物理行）に対する処理
		while ( *p!='\0' )
		{
			string	c=get_a_chr(p);	// 全角半角問わず一文字取得。

			if ( escape ) 
			{
				accumulater += (c=="φ") ? c : io_escaper.insert(c);
				escape = false;
			}
			else 
			{
				if ( c=="φ" ) 
				{
					escape = true;
					continue;
				}
				if ( c=="＃" )
					break;	// 行終了

				if ( c=="（" )
					++kakko_nest_count;
				else if (  c=="）" && kakko_nest_count>0 )
					--kakko_nest_count;

				accumulater += c;
			}
		}

		if ( escape )
		{
			continue;
		}

		if ( kakko_nest_count==0 )
		{
			// 置き換え辞書適用
			for ( strmap::iterator di=io_replace_dic.begin() ; di!=io_replace_dic.end() ; ++di )
			{
				replace(accumulater, di->first, di->second);
			}

			// 一行追加
			out.push_back(accumulater);
			//GetSender().sender() << line_number << " [" << accumulater << "]" << std::endl;
			accumulater="";
		}
		else if ( line_number == in.size() ) 
		{
			// エラー
			return false;
		}
	}
	return true;
}

string	Satori::SentenceToSakuraScriptExec_with_PreProcess(const strvec& i_vec)
{
	strvec vec;
	pre_process(i_vec, vec, m_escaper, replace_before_dic);
	return SentenceToSakuraScriptExec(vec);
}

// .txtと.satの両方がくるので、新しい方だけを読み込む。
bool Satori::select_dict_and_load_to_vector(const string& iFileName, strvec& oFileBody, bool warnFileName)
{
	string txtfile = set_extention(iFileName, dic_load_ext);
	string satfile = set_extention(iFileName, "sat");

	string realext = get_extention(iFileName);

	bool FileExist(const string& f);
	bool decodeMe = false;
	string file;

	//SAT / TXT
	if ( realext == "sat" ) {
		if ( FileExist(satfile.c_str()) ) {
			file = satfile;
			decodeMe = true;
		}
		else {
			if ( warnFileName ) {
				GetSender().sender() << "  " << satfile << "is not exist." << std::endl;
			}
			file = txtfile;
		}
	}
	else {
		if ( FileExist(txtfile.c_str()) ) {
			file = txtfile;
		}
		else {
			if ( warnFileName ) {
				GetSender().sender() << "  " << txtfile << "is not exist." << std::endl;
			}
			file = satfile;
			decodeMe = true;
		}
	}

	GetSender().sender() << "  loading " << get_file_name(file);
	if ( !strvec_from_file(oFileBody, file) )
	{
		GetSender().sender() << "... failed.";
		return	false;
	}
	GetSender().sender() << std::endl;

	if ( decodeMe ) {
		// 暗号化を解除
		for ( strvec::iterator it=oFileBody.begin() ; it!=oFileBody.end() ; ++it )
		{
			*it = decode( decode(*it) );
		}
	}

	return true;
}

static bool satori_anchor_compare(const string &lhs,const string &rhs)
{
	return lhs.size() > rhs.size();
}

// 辞書を読み込む。
bool Satori::LoadDictionary(const string& iFileName,bool warnFileName) 
{
	// ファイルからvectorへ読み込む。
	// その際、同ファイル名で拡張子が.txt(または指定拡張子)と.satのファイルの日付を比較し、新しい方だけを採用する。
	strvec	file_vec;
	if ( !select_dict_and_load_to_vector(iFileName, file_vec, warnFileName) )
	{
		return false;
	}

	if ( is_utf8_dic(file_vec) ) {
#ifdef POSIX
	     GetSender().errsender() <<
		    "syntax error - SATORI : " << iFileName << std::endl <<
		    std::endl <<
		    "It is highly possible that you tried to read a dictionary whose character code is UTF-8." << std::endl <<
		    "The dictionary is not loaded correctly." << std::endl <<
		    std::endl <<
		    "Please use Shift-JIS character code." << std::endl;
#else
		GetSender().errsender() << iFileName + "\n\n"
			"\n"
			"文字コードがUTF-8の辞書を読み込もうとした可能性が高いです。" "\n"
			"辞書は正しく読み込まれていません。" "\n"
			"\n"
			"保存の際に、Shift-JISを指定してくだしあ" << std::endl;
#endif
		return false;
	}

	bool	is_for_anchor = compare_head(get_file_name(iFileName), dic_load_prefix + "Anchor");

	strvec preprocessed_vec;
	if ( false == pre_process(file_vec, preprocessed_vec, m_escaper, replace_before_dic) )
	{
#ifdef POSIX
	     GetSender().errsender() <<
		    "syntax error - SATORI : " << iFileName << std::endl <<
		    std::endl <<
		    "There are some mismatched parenthesis." << std::endl <<
		    "The dictionary is not loaded correctly." << std::endl <<
		    std::endl <<
		    "If you want to display parenthesis independently," << std::endl <<
		    "use \"phi\" symbol to escape it." << std::endl;
#else
		GetSender().errsender() << iFileName + "\n\n"
			"\n"
			"カッコの対応関係が正しくない部分があります。" "\n"
			"辞書は正しく読み込まれていません。" "\n"
			"\n"
			"カッコを単独で表示する場合は　φ（　と記述してください。" << std::endl;
#endif
	}

	static std::vector<string> typemarks;
	if ( typemarks.empty() )
	{
		typemarks.push_back("＊");
		typemarks.push_back("＠");
	}

	std::vector<satori_unit> units;
	lines_to_units(preprocessed_vec, typemarks, "\t", units); // 単語群名/トーク名と採用条件式の区切り


	for ( std::vector<satori_unit>::iterator i=units.begin() ; i!=units.end() ; ++i)
	{
		// 末尾の空行を削除
		//while ( i->body.size()>0 && i->body.size()==0 )
		//{
		//	i->body.pop_back();
		//}
	        while (i->body.size() > 0 && i->body[i->body.size()-1].length() == 0) {
		        i->body.pop_back();
		}

		m_escaper.unescape(i->name);
		
		if ( i->typemark == "＊" )
		{
			// トークの場合
			if ( is_for_anchor ) {
				if ( i->name.size() > 0 ) {
					anchors.push_back(i->name);
				}
			}
			talks.add_element(i->name, i->body, i->condition);

#ifdef _DEBUG
			GetSender().sender() << "＊" << i->name << " " << i->condition << std::endl;
#endif
		}
		else
		{
			// 単語群の場合
			const strvec& v = i->body;
			for ( strvec::const_iterator j=v.begin() ; j!=v.end() ; ++j)
			{
				words.add_element(i->name, *j, i->condition);
			}

#ifdef _DEBUG
			GetSender().sender() << "＠" << i->name << " " << i->condition << std::endl;
#endif
		}

	}

	if ( is_for_anchor ) {
		std::sort(anchors.begin(),anchors.end(),satori_anchor_compare);
	}

	//GetSender().sender() << "　　　talk:" << talks.count_all() << ", word:" << words.count_all() << std::endl;
	GetSender().sender() << "... ok." << std::endl;
	return	true;
}

#ifdef POSIX
#  include <sys/types.h>
#  include <dirent.h>
#endif

void list_files(string i_path, std::vector<string>& o_files)
{
	unify_dir_char(i_path); // \\と/を環境に応じて適切な方に統一
#ifdef POSIX

	DIR* dh = opendir(i_path.c_str());
	if (dh == NULL)
	{
	    GetSender().sender() << "file not found." << std::endl;
	}
	while (1) {
	    struct dirent* ent = readdir(dh);
	    if (ent == NULL) {
		break;
	    }
#if defined(__WINDOWS__) || defined(__CYGWIN__)
	    string fname(ent->d_name);
#else
//	    string fname(ent->d_name, ent->d_namlen);
	    string fname(ent->d_name);
#endif
		o_files.push_back(fname);
	}
	closedir(dh);
#else /* POSIX */
	HANDLE			hFIND;	// 検索ハンドル
	WIN32_FIND_DATA	fdFOUND;// 見つかったファイルの情報
	hFIND = ::FindFirstFile((i_path+"*.*").c_str(), &fdFOUND);
	if ( hFIND == INVALID_HANDLE_VALUE )
	{
		GetSender().sender() << "file not found." << std::endl;
	}

	do
	{
		o_files.push_back(fdFOUND.cFileName);
	} while ( ::FindNextFile(hFIND,&fdFOUND) );
	::FindClose(hFIND);
#endif /* POSIX */

}




int Satori::LoadDicFolder(const string& i_base_folder)
{
	GetSender().sender() << "LoadDicFolder(" << i_base_folder << ")" << std::endl;
	std::vector<string> files;
	list_files(i_base_folder, files);

	int count = 0;
	
	string ext = ".";
	ext += dic_load_ext;
	
	for (std::vector<string>::const_iterator it=files.begin() ; it!=files.end() ; ++it)
	{
		const int len = it->size();
		if ( len < 3 ) { continue; } // 最短ファイル名3文字以上
		if ( it->compare(0,3,dic_load_prefix.c_str()) != 0 ) { continue; }
		if ( it->compare(len-4,4,ext.c_str()) != 0 && it->compare(len-4,4,".sat") != 0 ) { continue; }

		if ( LoadDictionary(i_base_folder + *it) ) {
			++count;
		}
	}

	GetSender().sender() << "ok." << std::endl;
	return count;
}
