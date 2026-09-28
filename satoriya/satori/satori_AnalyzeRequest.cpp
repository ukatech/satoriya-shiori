#include	"satori.h"

#ifdef POSIX
/* POSIXではstricmpは定義されていない。代わりにstrcasecmpが使える。 */
#  define _wcsicmp _wcsicmp
#  include <string.h>
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

int	Satori::request(
	const wstring& i_protocol,
	const wstring& i_protocol_version,
	const wstring& i_command,
	const strpairvec& i_data,
	
	wstring& o_protocol,
	wstring& o_protocol_version,
	strpairvec& o_data)
{
	SenderEnableBuffering seb(GetSender());

	//-------------------------------------------------
	// リクエスト単位のクラスメンバを初期化

	mRequestMap.clear();
	mRequestID = L"";
	mReferences.clear();
	mReferences.reserve(8); // 最小値

	// 引数をクラスメンバに設定
	mRequestCommand = i_command;
	mRequestType = i_protocol;
	mRequestVersion = i_protocol_version;
	mStatusLine = i_command + L" " + i_protocol + L"/" + i_protocol_version;

	// 返すプロトコルのデフォルト値
	o_protocol = L"SHIORI";
	o_protocol_version = L"3.0";

	// 喋るごとに初期化する変数
	return_empty = false;

	surface_restore_at_talk_onetime = SR_INVALID;
	auto_anchor_enable_onetime = auto_anchor_enable;
	auto_newline_enable_onetime = auto_newline_enable;

	is_quick_section = false;

	// スクリプトヘッダ
	header_script = L"";

	// プロトコルを判別
	if ( i_protocol==L"SAORI" && i_protocol_version[0]>=L'1' )
	{
		o_protocol = i_protocol;
		o_protocol_version = L"1.0";
		mRequestMode = SAORI;
	}
	else if ( i_protocol==L"SHIORI" && i_protocol_version[0]>=L'3' )
	{
		mRequestMode = SHIORI3;
	}
	else if ( i_protocol==L"SHIORI" && i_protocol_version[0]==L'2' )
	{
		mRequestMode = SHIORI2;
			// 2.xにもバージョンとしては3.0を返す
	}
	else if ( i_protocol==L"MAKOTO" && i_protocol_version[0]>=L'2' )
	{
		o_protocol = i_protocol;
		o_protocol_version = L"2.0";
		mRequestMode = MAKOTO2;
	}
	else if ( i_protocol==L"UNKNOWN" )
	{
		mRequestMode = UNKNOWN;
	}
	else
	{
		// 未対応のプロトコルだった。
		return	400;
	}

	// データ部をmRequestMapに格納。
	// SHOIRI/3.0以外のプロトコルの場合、SHOIRI/3.0に変換を行う。
	for ( strpairvec::const_iterator it = i_data.begin() ; it != i_data.end() ; ++it )
	{
		wstring key = it->first;
		const wstring& value = it->second;

		switch ( mRequestMode ) {
		case SAORI:
			if ( compare_head(key, L"Argument") ) {
				int	n = stoi_internal(key.c_str()+8);
				if ( n==0 )
					key = L"ID";
				else
					key = L"Reference" + itos(n-1);
			}
			break;
		case SHIORI2:	// こっちはてきとー
			if ( key==L"Event" )
				key=L"ID";
			break;
		case MAKOTO2:
			if ( key==L"String" )
				key=L"Reference0";
			break;
		default:
			break;
		}

		mRequestMap[key] = value;
		if ( compare_head(key, L"Reference") ) {
			int	n = stoi_internal(key.c_str()+9);
			if ( n>=0 && n<65536 ) {
				if ( n>=mReferences.size() )
					mReferences.resize(n+1);
				mReferences[n]=value;
			}
		}
	}

	if ( mRequestMode == MAKOTO2 )
	{
		mRequestMap[L"ID"] = L"OnMakoto";
	}

	mRequestID = mRequestMap[L"ID"];
	mIsMateria = ( mRequestMap[L"Sender"] == L"embryo" );
	mIsStatusHeaderExist = ( mRequestMap[L"Sender"] == L"SSP" );

	//-------------------------------------------------
	// リクエストを解釈

	if ( mRequestCommand==L"GET Version" )
	{
		if ( mRequestMode == SHIORI2 )
		{
			o_data.push_back( strpair(L"ID", gSatoriName) );
			o_data.push_back( strpair(L"Craftman", gSatoriCraftman) );
			o_data.push_back( strpair(L"Version", gSatoriVersion) );
		}
		return 200;
	}


	// 選択分岐記録を変数に代入。ref0を元に戻す。
	if ( compare_head(mRequestID, L"OnChoice") ) // OnChoiceSelect/OnChoiceEnterの両方
	{
		strvec	vec;
		int	ref_no = ( mRequestID==L"OnChoiceEnter" || mRequestID==L"OnChoiceSelectEx" )?1:0;
		wstring&	info = mRequestMap[wstring(L"Reference")+itos(ref_no)];
		if ( split(info, byte1_dlmt, vec)==3 ) // \1区切りの３文字列であるならば
		{
			info=mReferences[ref_no]=variables[L"選択ＩＤ"]=vec[0];
			variables[L"選択ラベル"]=vec[1];
			variables[L"選択番号"]=vec[2];
		}
	}

	// ログについて色々
	bool log_disable_soft = ( mRequestID==L"OnSurfaceChange" || mRequestID==L"OnSecondChange" || mRequestID==L"OnMinuteChange"
		|| mRequestID==L"OnMouseMove" || mRequestID==L"OnTranslate");

	bool log_disable_hard = ( /*compare_tail(mRequestID, "caption") || */compare_tail(mRequestID, L"visible")
		|| compare_head(mRequestID, L"menu.") || mRequestID.find(L".color.")!=wstring::npos );

	GetSender().next_event();

	if(fRequestLog)
	{
		GetSender().sender() << L"--- Request ---" << std::endl << mStatusLine <<std::endl; // << iRequest <<std::endl;
		GetSender().sender() << L"ID: " << mRequestID << std::endl;
		for (size_t i = 0; i < mReferences.size(); i++) {
			GetSender().sender() << L"Reference" << i << L": " << mReferences[i] << std::endl;
		}
	}

	// せきゅあ？
	strmap::const_iterator it = mRequestMap.find(L"SecurityLevel");
	secure_flag = ( it!=mRequestMap.end() && _wcsicmp(it->second.c_str(), L"local")==0 );

	// 予め指定したイベントプレフィックスはexternalでも実行可能に
	bool is_external = (it != mRequestMap.end() && _wcsicmp(it->second.c_str(), L"external") == 0);
	if (is_external) {
		for (strvec::const_iterator prefix = allow_external_event_prefixes.begin() ;
		prefix != allow_external_event_prefixes.end() ; ++prefix ) {
			if ( *prefix == L"全部" || ( _wcsnicmp(mRequestID.c_str(), prefix->c_str(), prefix->length()) == 0 ) ) {
				secure_flag = true;
				break;
			}
		}
	}

	// メイン処理
	GetSender().sender() << L"--- Operation ---" << std::endl;

	int status_code = 500;
	if ( mRequestID==L"enable_log" || mRequestID==L"enable_debug" ) {
		if ( secure_flag ) {
			bool flag = false;
			if ( mReferences.size() > 0 ) {
				flag = _wtoi(mReferences[0].c_str()) != 0;
			}

			if ( mRequestID==L"enable_debug" ) {
				fDebugMode = flag;
			}
			else {
				GetSender().validate(flag);
			}
			status_code = 200;
		}
		else {
			GetSender().sender() << L"local/Localでないので蹴りました: ShioriEcho" <<std::endl;
			status_code = 403;
		}
	}
	else if ( mRequestID==L"ShioriEcho" ) {
		// ShioriEcho実装
		if ( fDebugMode && secure_flag ) {
			wstring result = SentenceToSakuraScriptExec_with_PreProcess(mReferences);
			if ( result.length() ) {
				static const wchar_t* const dangerous_tag[] = {L"\\![updatebymyself]",
					L"\\![vanishbymyself]",
					L"\\![enter,passivemode]",
					L"\\![enter,inductionmode]",
					L"\\![leave,passivemode]",
					L"\\![leave,inductionmode]",
					L"\\![lock,repaint]",
					L"\\![unlock,repaint]",
					L"\\![biff]",
					L"\\![open,browser",
					L"\\![open,mailer",
					L"\\![raise",
					L"\\j["};

				std::wstring replace_to;
				for ( int i = 0 ; i < (sizeof(dangerous_tag)/sizeof(dangerous_tag[0])) ; ++i ) {
					replace_to = L"￥［";
					replace_to += dangerous_tag[i]+2; //\をヌキ
					replace(result,dangerous_tag[i],replace_to);
				}

				//Translate(result); - Translateは後でかかる
				mResponseMap[L"Value"] = result;
				status_code = 200;
			}
			else {
				status_code = 204;
			}
		}
		else {
			if ( fDebugMode ) {
				GetSender().sender() << L"local/Localでないので蹴りました: ShioriEcho" <<std::endl;
				status_code = 403;
			}
			else {
				static const std::wstring dbgmsg = L"デバッグモードが無効です。使用するためには＄デバッグ＝有効にしてください。: ShioriEcho";
				GetSender().sender() << dbgmsg <<std::endl;

				mResponseMap[L"Value"] = L"\\0" + dbgmsg + L"\\e";
				status_code = 200;
			}
		}
	}
	else if (mRequestID == L"SatolistEcho"){
#ifndef POSIX
		// さとりすとデバッガ実装
		if (fDebugMode && secure_flag) {

			//R0は除去される
			strvec customRef;
			for (int i = 1; i < mReferences.size(); i++){
				customRef.push_back(mReferences[i]);
			}

			wstring result = SentenceToSakuraScriptExec_with_PreProcess(customRef);
			if (result.length()) {
				result = wstring(L"SSTP 200 OK\r\nCharset: UTF-8\r\nResult: ") + result + L"\r\n\r\n";
			}
			else{
				//情報なし
				result = wstring(L"SSTP 204 No Content\r\nCharset: UTF-8\r\n\r\n");
			}

			const std::string copyData = WtoUTF8(result);

			COPYDATASTRUCT cds;
			cds.dwData = 0;
			cds.cbData = copyData.length() + 1;
			cds.lpData = const_cast<char*>(copyData.c_str());
			DWORD ret;

			SendMessageTimeout((HWND)stoi_internal(mReferences[0]), WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds, 0, 1000, &ret);

			//ここで204を返すと非対応時のエラーを出すので200で通知メッセージを表示する
			status_code = 200;
			mResponseMap[L"Value"] = L"\\0\\_q■情報を送信しました。\\e";
		}
		else {
			if (fDebugMode) {
				GetSender().sender() << L"local/Localでないので蹴りました: SatolistEcho" <<std::endl;
				status_code = 403;
			}
			else {
				static const std::wstring dbgmsg = L"デバッグモードが無効です。使用するためには＄デバッグ＝有効にしてください。: SatolistEcho";
				GetSender().sender() << dbgmsg <<std::endl;

				mResponseMap[L"Value"] = L"\\0" + dbgmsg + L"\\e";
				status_code = 200;
			}
		}
#endif // POSIX
	}
	else {
		status_code = CreateResponse(mResponseMap);
	}

	// 後処理１
	default_surface = next_default_surface;

	//--------------------------------------------------------------------

	// Valueに対する最終処理
	if ( status_code==200 ) {	// && compare_head(mRequestID, "On")
		strmap::iterator i = mResponseMap.find(L"Value");
		if ( i!=mResponseMap.end() ) {
			if ( return_empty ) {
				status_code = 204;
				mResponseMap.erase(i);
			}
			else {
				if ( !Translate(i->second) ) {
					status_code = 204;
					mResponseMap.erase(i);
				} 
				else {
					second_from_last_talk = 0;
					if ( compare_head(mRequestID, L"On") ) {
						mResponseHistory.push_front(i->second);
						if ( mResponseHistory.size() >= RESPONSE_HISTORY_SIZE )
							mResponseHistory.pop_back();
					}
				}
			}
		}
	}

	GetSender().sender() << L"status code : " << itos(status_code) <<std::endl;

	//--------------------------------------------------------------------

	for(strmap::const_iterator i=mResponseMap.begin() ; i!=mResponseMap.end() ; ++i)
	{
		wstring	key=i->first, value=i->second;

		switch ( mRequestMode ) {
		case SAORI:
			if ( key==L"Value" ) {
				key = L"Result";
				value = header_script + value;
			}
			else if ( compare_head(key, L"Reference") ) {
				key = wstring() + L"Value" + (key.c_str()+9);
			}
			break;
		case SHIORI2:
			if ( key==L"Value" ) {
				key = L"Sentence";
				value = header_script + value;
			}
			break;
		case MAKOTO2:
			if ( key==L"Value" ) {
				key = L"String";
				value = header_script + value;
			}
			break;
		default:
			if ( key==L"Value" ) {
				value = header_script + value;
			}
			break;
		}
		o_data.push_back( strpair(key, value) );
	}

	if ( GetSender().errsender().get_log_mode() ) {
		const std::vector<wstring> &errlog = GetSender().errsender().get_log();

		std::wstring errmsg;
		std::wstring errlevel;

		for ( std::vector<wstring>::const_iterator itr = errlog.begin() ; itr != errlog.end(); ++itr ) {
			errmsg += L"SATORI : ";
			errmsg += *itr;
			errmsg += L"\1";
			errlevel += L"critical\1";
		}

		if ( errmsg.length() ) {
			errmsg.erase(errmsg.end()-1,errmsg.end());
			errlevel.erase(errlevel.end()-1,errlevel.end());

			o_data.push_back( strpair(L"ErrorLevel",errlevel) );
			o_data.push_back( strpair(L"ErrorDescription",errmsg) );
		}
		GetSender().errsender().clear_log();
	}

	GetSender().validate();
	if(fResponseLog)
	{
		GetSender().sender() << L"--- Response ---" <<std::endl << mResponseMap <<std::endl;
	}
	mResponseMap.clear();

	if ( log_disable_hard ) {
		GetSender().delete_last_request();
	}
	else if ( log_disable_soft ) {
		if ( status_code != 200 ) {
			GetSender().delete_last_request();
		}
	}

	GetSender().flush();

	//--------------------------------------------------------------------

	// リロード処理
	if ( reload_flag )
	{
		reload_flag = false;
		wstring	tmp = mBaseFolder;
		GetSender().sender() << L"■■reloading." <<std::endl;
		unload();
		load(tmp);
		GetSender().sender() << L"■■reloaded." <<std::endl;

		GetSender().flush();
	}

	return	status_code;
}
