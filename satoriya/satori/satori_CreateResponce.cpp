#include	"satori.h"	

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

void CreateStringSet(const strvec& vec, std::set<string>& strset);
void CreateStringSetFromFirstElement(const strvec& vec, std::set<string>& strset);

int		Satori::CreateResponse(strmap& oResponse)
{
	// NOTIFYであれば値を保存
	/*strmap	mNotifiedMap;
	if ( mRequestCommand=="NOTIFY" )
		mNotifiedMap[mRequestID] = mRequestMap["Reference0"];*/
	if ( mRequestCommand=="NOTIFY" ) {
		if ( mRequestID=="hwnd" ) {
#ifndef POSIX
			strvec	vec;
			const int max = split(mReferences[0], byte1_dlmt, vec);
			if ( max > 0 ) {
				characters_hwnd.clear();
			}
			for (int n=0 ; n<max ; ++n) {
				characters_hwnd[n] = (HWND)(stoi_internal(vec[n]));
				GetSender().sender() << "里々は id:" << n << " のhWndを取得しました。" << std::endl;
			}
#endif
		}
		else if ( mRequestID=="capability" ) {
			bool isErrorHeader = false;
			for ( strvec::const_iterator it = mReferences.begin() ; it != mReferences.end(); ++it ) {
				if ( *it == "response.errorlevel" ) {
					isErrorHeader = true;
				}
			}
			GetSender().errsender().set_log_mode(isErrorHeader);
		}
	}

	string	result;

	//喋り変換部初期化
	reset_speaked_status();
	
	//otherghostnameは常に取得する
	if (mRequestID == "otherghostname"){
		CreateStringSetFromFirstElement(mReferences, otherghostname);
	}

	//notifyの保存しておく
	if (is_save_notify){
		
		if (mRequestID == "installedghostname"){
			//インストール済み情報
			CreateStringSet(mReferences, installed_ghost_name);
		}
		else if (mRequestID == "installedshellname"){
			CreateStringSet(mReferences, installed_shell_name);
		}
		else if (mRequestID == "installedballoonname"){
			CreateStringSet(mReferences, installed_balloon_name);
		}
		else if (mRequestID == "installedheadlinename"){
			CreateStringSet(mReferences, installed_headline_name);
		}
		else if (mRequestID == "OnNotifyFontInfo"){
			CreateStringSet(mReferences, installed_font_name);
		}
		else if (mRequestID == "installedplugin"){
			installed_plugin.clear();
			const char dm[2] = { 1, 0 };//区切り文字
			for (int i = 0; i < mReferences.size(); i++){
				std::vector<string> plugins;
				split(mReferences[i], dm, plugins);
				if (plugins.size() == 2){
					PluginInfo info;
					info.plugin_name = plugins[0];
					info.plugin_id = plugins[1];
					installed_plugin.insert(std::map<string, PluginInfo>::value_type(info.plugin_name, info));
				}
			}
		}
		else if (mRequestID == "rateofusegraph")
		{
			rate_of_use_graph.clear();
			const char dm[2] = { 1, 0 };//区切り文字
			for (int i = 0; i < mReferences.size(); i++){
				std::vector<string> rate;
				split(mReferences[i], dm, rate);
				if (rate.size() == 7){
					RateOfUseGraph info;
					info.ghost_name = rate[0];
					info.sakura_name = rate[1];
					info.kero_name = rate[2];
					info.boot_count = rate[3];
					info.boot_minutes = rate[4];
					info.boot_percent = rate[5];
					info.status = rate[6];
					rate_of_use_graph.insert(std::map<string, RateOfUseGraph>::value_type(info.ghost_name, info));
				}
			}
		}
	}

	//実際の呼び出し開始
	if ( mRequestID == "OnDirectSaoriCall" ) {
		string	str;
		int	n=0;
		bool	isfirst = true;
		for (strvec::const_iterator i=mReferences.begin() ; i!=mReferences.end() ; ++i, ++n)
			if ( i->empty() && mRequestMap.find( string("Reference")+itos(n) )==mRequestMap.end() )
				break;
			else
				if ( isfirst ) {
					str = *i;
					isfirst = false;
				}
				else
					str += "," + *i;
		//while ( compare_tail(str, ",") )
		//	str.assign( str.substr(0, str.size()-2)+"]" );
		string	temp;
		if ( secure_flag ) {
			GetSender().sender() << "[DirectCall]" << std::endl;
			Call(str, temp);
			return	204;
		}
		else {
			GetSender().sender() << "local/Localでないので蹴りました: " << str << std::endl;
			return	204;
		}
	}
	else if ( compare_head(mRequestID, "On") ) {
		if ( words.is_exist(mRequestID) )
			return	Call(mRequestID, oResponse["Value"]) ? 200 : 204;
		return	EventOperation(mRequestID, oResponse);
	}
	else if ( mRequestID == "version" )
		result = gSatoriVersion;
	else if ( mRequestID == "craftman" )
		result = gSatoriCraftman;
	else if ( mRequestID == "craftmanw" )
		result = gSatoriCraftmanW;
	else if ( mRequestID == "name" )
		result = gSatoriName;
	else if ( mRequestID.find(".recommendsites") != string::npos || mRequestID=="sakura.portalsites" ) {
		if ( !GetURLList(mRequestID, result) )	// URLリストの取得
			return	204;
	} 
	else if ( !Call(mRequestID, result) ) {

		if ( mRequestID=="getaistate" ) {
			static const string names[] = {"NoNameTalk", "EventTalk", "OtherTalk", "Word", "Anchor", "Variable"};
			static const int names_count = sizeof(names) / sizeof(names[0]);

			string res;
			int cnt;

			//表示値
			res += itos( count_func(names[0]) );
			for ( cnt = 1 ; cnt < names_count ; ++cnt ) {
				res += ",";
				res += itos( count_func(names[cnt]) );
			}

			res += byte1_dlmt;

			//ラベル
			res += names[0];
			for ( cnt = 1 ; cnt < names_count ; ++cnt ) {
				res += ",";
				res += names[cnt];
			}

			oResponse["Value"] = res;
			return 200;
		}

		return	204;
	}
	else {
	}

	if ( result.empty() )
		return	204;

	oResponse["Value"] = result;
	return	200;
}

void CreateStringSet(const strvec& vec, std::set<string>& strset)
{
	strset.clear();
	for (int i = 0; i < vec.size(); i++){
		strset.insert(vec.at(i));
	}
}

void CreateStringSetFromFirstElement(const strvec& vec, std::set<string>& strset)
{
	strset.clear();

	strvec byte1splitter;
	string vecelem;

	for (int i = 0; i < vec.size(); i++){
		vecelem = vec.at(i);
		byte1splitter.clear();
		split(vecelem,"\1",byte1splitter);
		strset.insert(byte1splitter[0]);
	}
}