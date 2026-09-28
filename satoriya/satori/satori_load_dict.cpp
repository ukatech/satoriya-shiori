#include	"satori.h"

#include	<fstream>
#include	<cassert>
#include <algorithm>

#include	"../_/Utilities.h"
#include	"../_/charset.h"
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
	wstring typemark;
	wstring name;
	wstring condition;
	strvec body;
};
#ifdef _DEBUG
std::wostream& operator<<(std::wostream& o, const satori_unit& su)
{
	o << su.typemark << L"/" << su.name << L"/" << su.condition << std::endl;
	o << su.body;
	return o;
}
#endif

static void lines_to_units(
	const std::vector<wstring>& i_lines,
	const std::vector<wstring>& i_typemarks,
	const wstring& i_name_cond_delimiter,
	std::vector<satori_unit>& o_units)
{
	std::vector<wstring>::const_iterator line_it = i_lines.begin();
	for ( ; line_it != i_lines.end() ; ++line_it)
	{
		//cout << *line_it << std::endl;

		// 行頭にtypemarksのいずれかが出現しているか探す
		std::vector<wstring>::const_iterator mark_it = i_typemarks.begin();
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
			
			const wchar_t* name = line_it->c_str() + mark_it->size();
			const wchar_t* delimiter = strstr_hz(name, i_name_cond_delimiter.c_str());
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
	wstring	accumulater=L"";	// 行あきゅむれーた
	int	line_number=1;
	for ( strvec::const_iterator fi=in.begin() ; fi!=in.end() ; ++fi, ++line_number )
	{
		const wchar_t* p=fi->c_str();
		bool	escape = false;

		// カッコ内の場合、行頭のタブは無視する。
		if ( kakko_nest_count>0 )
			while ( *p==L'\t' )
				++p;

		// 一行（物理行）に対する処理
		while ( *p!=L'\0' )
		{
			wstring	c=get_a_chr(p);	// 全角半角問わず一文字取得。

			if ( escape ) 
			{
				accumulater += (c==L"φ") ? c : io_escaper.insert(c);
				escape = false;
			}
			else 
			{
				if ( c==L"φ" ) 
				{
					escape = true;
					continue;
				}
				if ( c==L"＃" )
					break;	// 行終了

				if ( c==L"（" )
					++kakko_nest_count;
				else if (  c==L"）" && kakko_nest_count>0 )
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
			accumulater=L"";
		}
		else if ( line_number == in.size() ) 
		{
			// エラー
			return false;
		}
	}
	return true;
}

wstring	Satori::SentenceToSakuraScriptExec_with_PreProcess(const strvec& i_vec)
{
	strvec vec;
	pre_process(i_vec, vec, m_escaper, replace_before_dic);
	return SentenceToSakuraScriptExec(vec);
}

// .txtと.satの両方がくるので、新しい方だけを読み込む。
bool Satori::select_dict_and_load_to_vector(const wstring& iFileName, strvec& oFileBody, bool warnFileName, CharactorSet cs)
{
	wstring txtfile = set_extention(iFileName, dic_load_ext);
	wstring satfile = set_extention(iFileName, L"sat");

	wstring realext = get_extention(iFileName);

	bool FileExist(const wstring& f);
	bool decodeMe = false;
	wstring file;

	//SAT / TXT
	if ( realext == L"sat" ) {
		if ( FileExist(satfile.c_str()) ) {
			file = satfile;
			decodeMe = true;
		}
		else {
			if ( warnFileName ) {
				GetSender().sender() << L"  " << satfile << L"is not exist." << std::endl;
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
				GetSender().sender() << L"  " << txtfile << L"is not exist." << std::endl;
			}
			file = satfile;
			decodeMe = true;
		}
	}

	GetSender().sender() << L"  loading " << get_file_name(file);
	std::string bytes;
	if ( !bytes_from_file(bytes, file) )
	{
		GetSender().sender() << L"... failed.";
		return	false;
	}
	GetSender().sender() << std::endl;

	std::vector<std::string> lines;
	split_lines(bytes, lines);

	if ( decodeMe ) {
		// 暗号化を解除（バイト列の並べ替えなので文字コード変換の前に行う）
		for ( std::vector<std::string>::iterator it=lines.begin() ; it!=lines.end() ; ++it )
		{
			*it = decode( decode(*it) );
		}
	}

	// 文字コードを変換。csがCS_NULLなら判定する。
	lines_to_strvec(lines, oFileBody, cs);

	return true;
}

static bool satori_anchor_compare(const wstring &lhs,const wstring &rhs)
{
	return lhs.size() > rhs.size();
}

// 辞書を読み込む。
bool Satori::LoadDictionary(const wstring& iFileName,bool warnFileName,bool isUTF8) 
{
	// ファイルからvectorへ読み込む。
	// その際、同ファイル名で拡張子が.txt(または指定拡張子)と.satのファイルの日付を比較し、新しい方だけを採用する。
	strvec	file_vec;
	// isUTF8が真ならUTF-8、偽なら文字コードを判定して読む
	if ( !select_dict_and_load_to_vector(iFileName, file_vec, warnFileName, isUTF8 ? CS_UTF8 : CS_NULL) )
	{
		return false;
	}

	bool	is_for_anchor = compare_head(get_file_name(iFileName), dic_load_prefix + L"Anchor");

	strvec preprocessed_vec;
	if ( false == pre_process(file_vec, preprocessed_vec, m_escaper, replace_before_dic) )
	{
#ifdef POSIX
	     GetSender().errsender() <<
		    L"syntax error - SATORI : " << iFileName << std::endl <<
		    std::endl <<
		    L"There are some mismatched parenthesis." << std::endl <<
		    L"The dictionary is not loaded correctly." << std::endl <<
		    std::endl <<
		    L"If you want to display parenthesis independently," << std::endl <<
		    L"use \"phi\" symbol to escape it." << satori::endl;
#else
		GetSender().errsender() << iFileName + L"\n\n"
			L"\n"
			L"カッコの対応関係が正しくない部分があります。" L"\n"
			L"辞書は正しく読み込まれていません。" L"\n"
			L"\n"
			L"カッコを単独で表示する場合は　φ（　と記述してください。" << satori::endl;
#endif
	}

	static std::vector<wstring> typemarks;
	if ( typemarks.empty() )
	{
		typemarks.push_back(L"＊");
		typemarks.push_back(L"＠");
	}

	std::vector<satori_unit> units;
	lines_to_units(preprocessed_vec, typemarks, L"\t", units); // 単語群名/トーク名と採用条件式の区切り


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
		
		if ( i->typemark == L"＊" )
		{
			// トークの場合
			if ( is_for_anchor ) {
				if ( i->name.size() > 0 ) {
					anchors.push_back(i->name);
				}
			}
			talks.add_element(i->name, i->body, i->condition);

#ifdef _DEBUG
			GetSender().sender() << L"＊" << i->name << L" " << i->condition << std::endl;
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
			GetSender().sender() << L"＠" << i->name << L" " << i->condition << std::endl;
#endif
		}

	}

	if ( is_for_anchor ) {
		std::sort(anchors.begin(),anchors.end(),satori_anchor_compare);
	}

	//GetSender().sender() << "　　　talk:" << talks.count_all() << ", word:" << words.count_all() << std::endl;
	GetSender().sender() << L"... ok." << std::endl;
	return	true;
}

#ifdef POSIX
#  include <sys/types.h>
#  include <dirent.h>
#endif

void list_files(wstring i_path, std::vector<wstring>& o_files)
{
	unify_dir_char(i_path); // \\と/を環境に応じて適切な方に統一
#ifdef POSIX

	DIR* dh = opendir(WtoUTF8(i_path).c_str());
	if (dh == NULL)
	{
	    GetSender().sender() << L"file not found." << std::endl;
	    return;
	}
	while (1) {
	    struct dirent* ent = readdir(dh);
	    if (ent == NULL) {
		break;
	    }
	    wstring fname = UTF8toW(ent->d_name);
		o_files.push_back(fname);
	}
	closedir(dh);
#else /* POSIX */
	HANDLE			hFIND;	// 検索ハンドル
	WIN32_FIND_DATA	fdFOUND;// 見つかったファイルの情報
	hFIND = ::FindFirstFile((i_path+L"*.*").c_str(), &fdFOUND);
	if ( hFIND == INVALID_HANDLE_VALUE )
	{
		GetSender().sender() << L"file not found." << std::endl;
		return;
	}

	do
	{
		o_files.push_back(fdFOUND.cFileName);
	} while ( ::FindNextFile(hFIND,&fdFOUND) );
	::FindClose(hFIND);
#endif /* POSIX */

}




int Satori::LoadDicFolder(const wstring& i_base_folder)
{
	GetSender().sender() << L"LoadDicFolder(" << i_base_folder << L")" << std::endl;
	std::vector<wstring> files;
	list_files(i_base_folder, files);

	int count = 0;
	
	wstring ext = L".";
	ext += dic_load_ext;
	
	for (std::vector<wstring>::const_iterator it=files.begin() ; it!=files.end() ; ++it)
	{
		const int len = it->size();
		if ( len < dic_load_prefix.length() + (ext.length() < 4 ? ext.length() : 4)  ) { continue; } // 最短ファイル名は辞書接頭辞 + min(辞書拡張子, ".sat")
		if ( it->compare(0,dic_load_prefix.length(),dic_load_prefix.c_str()) != 0 ) { continue; }
		if ( it->compare(len-ext.length(),ext.length(),ext.c_str()) != 0 && it->compare(len-4,4,L".sat") != 0 ) { continue; }

		if ( LoadDictionary(i_base_folder + *it,true,is_utf8_dic) ) {
			++count;
		}
	}

	GetSender().sender() << L"ok." << std::endl;
	return count;
}
