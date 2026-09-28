#include	"satori.h"
#include	"../_/Utilities.h"
#include "posix_utils.h"
#include	<time.h>
#ifndef POSIX
#include	<tlhelp32.h>
#else
#include <climits>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

struct shm_t {
	uint32_t size;
	sem_t sem;
	char buf[PATH_MAX];
};

const int BUFFER_SIZE = 1024;
#endif // !POSIX
#include	<sstream>

#include "random.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

void	add_characters(const wchar_t* p, int& chars_spoken) {
	// さくらスクリプトとそれ以外を分割して処理を加える
	while (*p) {
		if (*p==L'\\'||*p==L'%') {
			++p;
			if (*p==L'\\'||*p==L'%')	// エスケープされた\, %
				continue;
			while ( *p < 0x80 && (iswalpha(*p)||iswdigit(*p)||*p==L'!'||*p==L'*'||*p==L'&'||*p==L'?'||*p==L'_') )
				++p;
			if (*p==L'[') {
				p += 1;
				while ( *p && *p!=L']' ) {
					if (p[0]==L'\\' && p[1]==L']') {	// エスケープされた]
						++p;
					}
					else {
						get_a_chr(p);
					}
				}
			}
		}
		else {
			get_a_chr(p);
			chars_spoken += 1;
		}
	}
}

/*#ifndef POSIX
static	SYSTEMTIME	DwordToSystemTime(DWORD dw) {
	SYSTEMTIME	st = { 0, 0, 0, 0, 0, 0, 0, 0 };
	st.wMilliseconds=WORD(dw%1000); dw/=1000;
	st.wSecond=WORD(dw%60); dw/=60;
	st.wMinute=WORD(dw%60); dw/=60;
	st.wHour=WORD(dw);
	return	st;
}
#endif*/

//get_property関数用のハンドラと結果格納
#ifdef POSIX

static std::string SendDataUsingUnixSocket(const std::string &path, const std::string &request, bool has_header) {
	sockaddr_un addr = {};
	if (path.length() >= sizeof(addr.sun_path)) {
		return "";
	}
	int soc = socket(AF_UNIX, SOCK_STREAM, 0);
	if (soc == -1) {
		return "";
	}
	addr.sun_family = AF_UNIX;
	// null-terminatedも書き込ませる
	strncpy(addr.sun_path, path.c_str(), path.length() + 1);
	if (connect(soc, reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) == -1) {
		return "";
	}
	if (send(soc, request.data(), request.size(), 0) != request.size()) {
		close(soc);
		return "";
	}
	shutdown(soc, SHUT_WR);
	char buffer[BUFFER_SIZE] = {};
	std::string data;
	uint32_t remain = 0;
	if (has_header) {
		if (read(soc, buffer, sizeof(uint32_t)) != sizeof(uint32_t)) {
			close(soc);
			return "";
		}
		remain = *reinterpret_cast<uint32_t *>(buffer);
		data.reserve(remain);
	}
	while (true) {
		int ret = read(soc, buffer, BUFFER_SIZE);
		if (ret == -1) {
			close(soc);
			return "";
		}
		if (ret == 0) {
			close(soc);
			break;
		}
		if (!has_header || remain > ret) {
			data.append(buffer, ret);
		}
		else {
			data.append(buffer, remain);
		}
		remain -= ret;
	}
	return data;
}

static bool SendDirectSSTP(const void* targetHWnd, const std::wstring &sendText, std::wstring &result)
{
    result = L"";
    shm_t *shm;
    int fd = shm_open("/ninix", O_RDWR, 0);
    if (fd == -1) {
        return false;
    }
    shm = static_cast<shm_t *>(mmap(NULL, sizeof(shm_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    if (shm == MAP_FAILED) {
        return false;
    }
    if (sem_wait(&shm->sem) == -1) {
        return false;
    }
    std::string path(shm->buf, shm->size);
    if (sem_post(&shm->sem) == -1) {
        return false;
    }
    std::string data = SendDataUsingUnixSocket(path + "ninix", "GetFMO\r\n", true);
    if (data.empty()) {
        return false;
    }
    int target = reinterpret_cast<long>(targetHWnd);
    std::istringstream iss(data);
    std::string uuid;
    while (true) {
        if (!iss) {
            return false;
        }
        std::string tmp;
        int hwnd;
        std::getline(iss, tmp);
        std::istringstream line(tmp);
        std::getline(iss, uuid, '.');
        std::getline(iss, tmp, '\x01');
        if (tmp != "hwnd") {
            continue;
        }
        std::getline(iss, tmp);
        if (target == atoi(tmp.c_str())) {
            break;
        }
    }
    std::string request = "EXECUTE SSTP/1.1\r\nCharset: UTF-8\r\n" + WtoUTF8(sendText) + "Sender: Satori\r\n\r\n";
    data = SendDataUsingUnixSocket(path + uuid, request, false);
    std::wstring response = MBtoW(data, CharsetFromName(UTF8toW(find_charset_header(data))));
    std::wstring header = cut_token(response, CRLF);
    cut_token(header, L" ");
    if (header == L"200 OK")
    {
        //1行文読み捨て
        cut_token(response, CRLF);
        result = cut_token(response, CRLF);
        return true;
    }
    else {
        return false;
    }
}

#else

std::wstring execute_result;
bool execute_succeeded;

static LRESULT CALLBACK GetPropertyHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	if (message == WM_COPYDATA)
	{
		const COPYDATASTRUCT* cds = (const COPYDATASTRUCT*)lparam;
		std::string recv_bytes((const char*)cds->lpData, cds->cbData);
		wstring recv_str = MBtoW(recv_bytes, CharsetFromName(UTF8toW(find_charset_header(recv_bytes))));
		
		wstring header = cut_token(recv_str, CRLF);
		cut_token(header, L" ");
		if (header == L"200 OK")
		{
			//1行文読み捨て
			cut_token(recv_str, CRLF);
			execute_result = cut_token(recv_str, CRLF);
			execute_succeeded = true;
		}
	}
	return CallWindowProc(DefWindowProc, hwnd, message, wparam, lparam);
}

static bool SendDirectSSTP(const void* targetHWnd, std::wstring sendText, std::wstring &result)
{
	execute_result = L"";
	execute_succeeded = false;

	//結果受信用ウインドウ作成: リソースの仕様を局所化してみたけどオーバーヘッドがでかい場合はSHIORIの初期化周辺に絡めるといいのかも
	const wchar_t* windowname = L"satori_get_property";
	
	WNDCLASSEX windowClass;
	ZeroMemory(&windowClass,sizeof(windowClass));

	windowClass.cbSize = sizeof(windowClass);
	windowClass.hInstance = GetModuleHandle(NULL);
	windowClass.lpszClassName = windowname;
	windowClass.lpfnWndProc = ::GetPropertyHandler;
	
	::RegisterClassEx(&windowClass);

	HWND propertyWindow = ::CreateWindow(windowname, windowname, 0, 0, 0, 100, 100, NULL, NULL, windowClass.hInstance, NULL);

	std::string sendData = "EXECUTE SSTP/1.1\r\nCharset: UTF-8\r\n" + WtoUTF8(sendText) + "Sender: Satori\r\n\r\n";

	//メッセージ転送
	COPYDATASTRUCT cds;
	cds.dwData = 9801;
	cds.cbData = sendData.size();
	cds.lpData = malloc(cds.cbData);
	memcpy(cds.lpData, sendData.c_str(), cds.cbData);

	/*LRESULT res =*/ ::SendMessage((HWND)targetHWnd, WM_COPYDATA, (WPARAM)propertyWindow, (LPARAM)&cds);
	
	//リソースの開放
	free(cds.lpData);
	
	::DestroyWindow(propertyWindow);
	::UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);

	result = execute_result;
	return execute_succeeded;
}
#endif

wstring	Satori::inc_call(
	const wstring& iCallName, 
	const strvec& iArgv, 
	strvec& oResults, 
	bool iIsSecure) 
{

	if ( iCallName == L"バイト値" ) {
		if ( iArgv.size() ) {
			// Unicodeのコードポイントとして扱う
			unsigned long cp = zen2ul(iArgv[0]);
			wstring r;
			if ( cp >= 0xD800 && cp <= 0xDFFF ) {
				return L"";	// サロゲート単体は文字ではない
			}
			if ( sizeof(wchar_t) == 2 && cp >= 0x10000 && cp <= 0x10FFFF ) {
				cp -= 0x10000;
				r += (wchar_t)(0xD800 + (cp >> 10));
				r += (wchar_t)(0xDC00 + (cp & 0x3FF));
			}
			else {
				r += (wchar_t)cp;
			}
			return r;
		}
		else {
			GetSender().sender() << L"error: 'バイト値' : 引数が不正です。" << std::endl;
			return L"";
		}
	}

	if ( iCallName==L"nop" ) {
		return L"";
	}

	if ( iCallName == L"合成単語群" ) {
		if ( iArgv.size() ) {
			std::vector<const Word*> vt;
			for ( strvec::const_iterator it = iArgv.begin() ; it != iArgv.end() ; ++it ) {
				words.select_all(*it,*this,vt);
			}
			if ( vt.size() ) {
				return *(vt[random(vt.size())]);
			}
			else {
				return L"";
			}
		}
		else {
			GetSender().sender() << L"error: '合成単語群' : 引数が不正です。" << std::endl;
			return L"";
		}
	}

	if ( !iIsSecure ) {
		GetSender().sender() << L"local/Localでないので蹴りました: " << iCallName << std::endl;
		return	L"";
	}

	if ( iCallName==L"set" ) {
		if ( iArgv.size()==2 ) {
			wstring	result, key=iArgv[0], value=iArgv[1];

			SubstVariable(key,value,result,false);

			return	result;
		}
		return	L"";
	}
	
	if ( iCallName==L"loop" ) {
		int	init=1, max=0, step=1, arg_size=iArgv.size();
		if ( arg_size==2 ) {
			max=zen2int(iArgv[1]);
		}
		else if ( arg_size==3 ) {
			init=zen2int(iArgv[1]);
			max=zen2int(iArgv[2]);
		}
		else if ( arg_size==4 ) {
			init=zen2int(iArgv[1]);
			max=zen2int(iArgv[2]);
			step=zen2int(iArgv[3]);
		}
		else
			return	L"";
		wstring	name=iArgv[0];
		wstring	ret,temp;

		if ( step==0 )
			return	L"";
		else if ( step>0 ) {
			if ( init>max )
				return	L"";
			for (int i=init ; i<=max ; i+=step ) {
				variables[name+L"カウンタ"] = itos(i);
				if ( !Call(name, temp) )
					return	L"";
				ret += temp;
			}
		}
		else {
			if ( init<max )
				return	L"";
			for (int i=init ; i>=max ; i+=step ) {
				variables[name+L"カウンタ"] = itos(i);
				if ( !Call(name, temp) )
					return	L"";
				ret += temp;
			}
		}
		variables.erase(name+L"カウンタ");
		return	ret;
	}
	
	if ( iCallName==L"sync" ) {
		wstring	str = L"\\![raise,OnDirectSaoriCall";
		if ( !iArgv.empty() ) {
			wstring	arg;
			combine(arg, iArgv, L",");
			str += L",";
			str += arg;
		}
		str += L"]";
		return	str;
	}
	
	if ( iCallName==L"remember" ) {
		if ( iArgv.size() == 1 ) {
			int	n = zen2int(iArgv[0]);
			if ( mResponseHistory.size() > n ) {
				return	mResponseHistory[n];
			}
		}
		return	L"";
	}
	
	if ( iCallName==L"call" ) {
		if ( iArgv.size() >= 1 ) {
			mCallStack.push( strvec() );
			strvec&	v = mCallStack.top();
			for ( int i=1 ; i<iArgv.size() ; ++i )
				v.push_back( iArgv[i] );
			wstring	r;
			Call(iArgv[0],r,false,false,true);
			mCallStack.pop();
			return	r;
		}
		return	L"";
	}

	if (iCallName == L"vncall")
	{
		if (iArgv.size() >= 1) {
			
			strvec	v;// = mCallStack.top();
			wstring	r;

			for (int i = 1; i < iArgv.size(); ++i){
				Call(iArgv[i], r);
				v.push_back(r);
			}
//				v.push_back(*GetValue(iArgv[i], ex));

			mCallStack.push(v);		//ここでpushしてはだめ、呼び出し先でA0取るとすると、pushのタイミングはcallの直前。
			Call(iArgv[0], r, false, false, true);
			mCallStack.pop();
			return	r;
		}
		return	L"";
	}

	if (iCallName == L"get_property")
	{
		if (iArgv.size() >= 1)
		{
			const void* targetHWnd = characters_hwnd[0];
			std::wostringstream ost;
			ost << L"Command: GetProperty\r\nReference0: " << iArgv[0] << L"\r\n";
			std::wstring sendData = ost.str();

			std::wstring result;
			if (SendDirectSSTP(targetHWnd, sendData, result)) {
				return result;
			}
			return (iArgv.size() >= 2) ? iArgv[1] : L"";
		}
	}

	if (iCallName == L"set_property")
	{
		if (iArgv.size() >= 2)
		{
			const void* targetHWnd = characters_hwnd[0];
			std::wostringstream ost;
			ost << L"Command: SetProperty\r\nReference0: " << iArgv[0] << L"\r\nReference1: " << iArgv[1] << L"\r\n";
			std::wstring sendData = ost.str();

			std::wstring result;
			SendDirectSSTP(targetHWnd, sendData, result);
			return result;
		}
	}


	if (iCallName == L"load_saori")
	{
		if (iArgv.size() >= 2)
		{
			wstring load_line = iArgv[0];
			for (int i = 1; i < iArgv.size(); ++i)
				load_line += L"," + iArgv[i];

			mShioriPlugins->load_a_plugin(load_line);
		}
	}

	if (iCallName == L"equal") {
		if (iArgv.size() == 2) {
			const wstring &lhs = iArgv[0], &rhs = iArgv[1];
			return itos(lhs == rhs);
		}
		return	L"";
	}
	
	if ( iCallName == L"単語の追加" ) {

		if ( iArgv.size() == 2 )
		{
			Family<Word>* f = words.get_family(iArgv[0]);
			if ( f == NULL || false == f->is_exist_element(iArgv[1]) )
			{
				mAppendedWords[ iArgv[0] ].push_back( words.add_element(iArgv[0],iArgv[1],Condition()) );
				GetSender().sender() << L"単語群「" << iArgv[0] << L"」に単語「" << iArgv[1] << L"」が追加されました。" << std::endl;
			}
			else
			{
				GetSender().sender() << L"単語群「" << iArgv[0] << L"」に単語「" << iArgv[1] << L"」は既に存在します。" << std::endl;
			}
		}
		else {
			GetSender().sender() << L"error: '単語の追加' : 引数が不正です。" << std::endl;
		}
		return	L"";
	}

	if ( iCallName == L"追加単語の削除" ) {
		if ( iArgv.size() == 2 )
		{
			Family<Word>* f = words.get_family(iArgv[0]);
			if ( f && f->is_exist_element(iArgv[1]) ) { //すでに存在し…
				std::map<wstring, std::vector<Word> >::iterator it = mAppendedWords.find(iArgv[0]);
				if ( it != mAppendedWords.end() ) { //しかも「単語の追加」で追加したもので…
					std::vector<Word> &setword = it->second;
					
					std::vector<Word>::iterator itrm = std::remove(setword.begin(),setword.end(),iArgv[1]);
					if ( itrm != setword.end() ) {
						setword.erase(itrm,setword.end());
						f->delete_element(iArgv[1]);

						if ( setword.empty() ) {
							mAppendedWords.erase(it);
						}
						GetSender().sender() << L"単語群「" << iArgv[0] << L"」の単語「" << iArgv[1] << L"」が削除されました。" << std::endl;
						if ( f->empty() ) {
							words.erase(iArgv[0]);
						}
					}
				}
			}
		}
		else {
			GetSender().sender() << L"error: '追加単語の削除' : 引数が不正です。" << std::endl;
		}
		return	L"";
	}
	
	if ( iCallName == L"追加単語の全削除" ) {
		if ( iArgv.size() == 1 )
		{
			Family<Word>* f = words.get_family(iArgv[0]);
			if ( f ) { //すでに存在し…
				std::map<wstring, std::vector<Word> >::iterator it = mAppendedWords.find(iArgv[0]);
				if ( it != mAppendedWords.end() ) { //しかも「単語の追加」で追加したもので…
					std::vector<Word> &setword = it->second;
					for ( std::vector<Word>::const_iterator its = setword.begin(); its != setword.end() ; ++its ) {
						f->delete_element(*its);
					}
					mAppendedWords.erase(it);

					GetSender().sender() << L"単語群「" << iArgv[0] << L"」に追加された単語は全て削除されました。" << std::endl;
				}
				if ( f->empty() ) {
					words.erase(iArgv[0]);
				}
			}
		}
		else {
			GetSender().sender() << L"error: '単語の削除' : 引数が不正です。" << std::endl;
		}
		return	L"";
	}
	return	L"";
}

// R・H・A・S・Cを判定
bool Satori::IsArrayValue(const wstring &iName,int &ref,wchar_t &firstChar)
{
	if ( ((iName[0]==L'R' || iName[0]==L'H' || iName[0]==L'A' || iName[0]==L'S' || iName[0]==L'C') && iName.size() >= 2) ||
		((iName[0]==L'Ｒ' || iName[0]==L'Ｈ' || iName[0]==L'Ａ' || iName[0]==L'Ｓ' || iName[0]==L'Ｃ') && iName.size() >= 2) ) {

		wstring hankaku=zen2han(iName);

		if ( aredigits(hankaku.c_str()+1) ) {
			firstChar = hankaku[0];
			ref = stoi_internal(hankaku.c_str()+1);
			return true;
		}
	}
	return false;
}


// 変数取得
wstring* Satori::GetValue(const wstring &iName,bool &oIsSysValue,bool iIsExpand,bool *oIsExpanded,const wchar_t *pDefault)
{
	if ( oIsExpanded ) { *oIsExpanded = false; }
	oIsSysValue = false;

	int ref;
	wchar_t firstChar;

	if ( IsArrayValue(iName,ref,firstChar) ) {
		if ( firstChar==L'R' ) {
			oIsSysValue = true;

			// Event通知時の引数取得
			if (ref>=0 && ref<mReferences.size()) {
				return &(mReferences[ref]);
			}
			else {
				if ( iIsExpand && ref >= 0 ) {
					mReferences.resize(ref+1);
					if ( oIsExpanded ) { *oIsExpanded = true; }
					return &(mReferences[ref]);
				}
				return NULL;
			}
		}
		else if ( firstChar==L'H' ) {
			oIsSysValue = true;

			// 過去の置き換え履歴を参照
			if ( kakko_replace_history.empty() ) { return NULL; }

			strvec&	khr = kakko_replace_history.top();

			ref -= 1; //いっこまえでないと履歴にならん！

			if ( ref>=0 && ref < khr.size() ) {
				return &(khr[ref]);
			}
			else {
				return NULL;
			}
		}
		else if ( firstChar==L'A' ) {
			oIsSysValue = true;

			if ( mCallStack.empty() ) { return NULL; }

			// callによる呼び出しの引数を参照S
			strvec&	v = mCallStack.top();
			if ( ref >= 0 && ref < v.size() ) {
				return &(v[ref]);
			}
			else {
				return NULL;
			}
		}
		else if ( firstChar==L'S' ) {
			oIsSysValue = true;

			// SAORIなどコール時の結果処理
			if (ref>=0 && ref<mKakkoCallResults.size()) {
				return &(mKakkoCallResults[ref]);
			}
			else {
				if ( iIsExpand && ref >= 0 ) {
					mKakkoCallResults.resize(ref+1);
					if ( oIsExpanded ) { *oIsExpanded = true; }
					return &(mKakkoCallResults[ref]);
				}
				return NULL;
			}
		}
		else if ( firstChar==L'C' ) {
			oIsSysValue = true;

			if ( 0 <= ref && ref < mLoopCounters.size() ) {
				return &(mLoopCounters.from_top(ref));
			}
			else {
				return NULL;
			}
		}
	}

	if ( variables.find(iName) != variables.end() ) {
		// 変数名であれば変数の内容を返す
		return &(variables[iName]);
	}

	if ( iIsExpand ) {
		variables[iName] = wstring(pDefault);
		if ( oIsExpanded ) { *oIsExpanded = true; }
		return &(variables[iName]);
	}
	return NULL;
}


// 引数に渡されたものを何かの名前であるとし、置き換え対象があれば置き換える。
bool	Satori::Call(const wstring& iName, wstring& oResult, bool for_calc, bool for_non_talk, bool use_arg_callstack)
{
	if ( for_calc ) {
		for_non_talk = true;
	}

	++m_nest_count;

	if ( m_nest_limit > 0 && m_nest_count > m_nest_limit ) {
		GetSender().sender() << L"呼び出し回数超過：" << iName << std::endl;
		oResult = L"（" + iName + L"）";
		--m_nest_count;
		return false;
	}

	bool r = CallReal(iName,oResult,for_calc,for_non_talk, use_arg_callstack);
	--m_nest_count;

	if ( r && oResult.empty() ) {
		if ( for_calc ) {
			oResult = L"０";
		}
	}
	return r;
}

bool	Satori::CallReal(const wstring& iName, wstring& oResult, bool for_calc, bool for_non_talk, bool use_arg_callstack)
{
	simple_stack<strvec>::size_type stack_size_before_call = kakko_replace_history.size();

	bool	_pre_called_=false;

	// SAORI対応, 内蔵関数呼び出しもここで
	{
		wstring	thePluginName=L"";
		std::set<wstring>::const_iterator theDelimiter = mDelimiters.end();

		const wchar_t* p = NULL;
		enum { NO_CALL, SAORI_CALL, INC_CALL, SPECIAL_CALL } state = NO_CALL;

		if ( mShioriPlugins->find(iName) ) {
			thePluginName=iName;
			state = SAORI_CALL;
		} else {

			static std::set<wstring> inner_commands;
			if ( inner_commands.empty() ) {
				// 本当はstd::map<name, function>だなー　むー
				inner_commands.insert(L"set");
				inner_commands.insert(L"get_property");
				inner_commands.insert(L"set_property");
				inner_commands.insert(L"nop");
				inner_commands.insert(L"sync");
				inner_commands.insert(L"loop");
				inner_commands.insert(L"remember");
				inner_commands.insert(L"call");
				inner_commands.insert(L"vncall");
				inner_commands.insert(L"equal");
				inner_commands.insert(L"バイト値");
				inner_commands.insert(L"文の数");
				inner_commands.insert(L"単語の追加");
				inner_commands.insert(L"合成単語群");
				inner_commands.insert(L"追加単語の削除");
				inner_commands.insert(L"追加単語の全削除");
			}

			if (use_arg_callstack)
			{
				//コールスタックを引数として使う場合、区切り文字が無いので
				if (mShioriPlugins->find(iName.c_str())) {	// 存在確認
					thePluginName = iName.c_str();
					state = SAORI_CALL;
				}
				else if (special_commands.find(iName.c_str()) != special_commands.end()){
					thePluginName = iName.c_str();
					state = SPECIAL_CALL;
				}
				else if (inner_commands.find(iName.c_str()) != inner_commands.end()) {
					thePluginName = iName.c_str();
					state = INC_CALL;
				}
			}
			else
			{
				for (std::set<wstring>::const_iterator i = mDelimiters.begin(); i != mDelimiters.end(); ++i) {
					p = strstr_hz(iName.c_str(), i->c_str());
					if (p == NULL)
						continue;
					wstring	str(iName.c_str(), p - iName.c_str());
					if (mShioriPlugins->find(str)) {	// 存在確認
						thePluginName = str;
						theDelimiter = i;
						state = SAORI_CALL;
						break;
					}
					else if (special_commands.find(str) != special_commands.end()){
						thePluginName = str;
						theDelimiter = i;
						state = SPECIAL_CALL;
						break;
					}
					else if (inner_commands.find(str) != inner_commands.end()) {
						thePluginName = str;
						theDelimiter = i;
						state = INC_CALL;
						break;
					}
				}
			}
		}

		if ( state==NO_CALL ) {
			_pre_called_=false;
		}
		else
		{
			_pre_called_=true;
			strvec	theArguments;

			if (use_arg_callstack && !mCallStack.empty())
			{
				//call / vncall用の処理。
				//SPECIAL CALL の動作は保証しません
				theArguments = mCallStack.top();
			}
			else
			{
				if (p != NULL)// 引数があるなら
				{
					assert(theDelimiter != mDelimiters.end());

					if (state == SPECIAL_CALL) {
						int level = 0;
						get_a_chr(p);
						const wchar_t *p_start = p;
						while (true){
							if (*p == L'\0'){
								theArguments.push_back(wstring(p_start, p - p_start));
								break;
							}
							wstring c = get_a_chr(p);
							if (c == L"（") {
								level++;
							}
							if (c == L"）") {
								level--;
							}
							if (level < 0) {
								theArguments.push_back(wstring(p_start, p - p_start - c.size()));
								break;
							}
							if (level == 0) {
								if (c == *theDelimiter) {
									theArguments.push_back(wstring(p_start, p - p_start - c.size()));
									p_start = (wchar_t *)p;
								}
							}
						}
					}
					else{
						wstring argstr = UnKakko(p, false, true);
						while (true)
						{
							p += theDelimiter->size();
							const wchar_t* pdlmt = strstr_hz(p, theDelimiter->c_str());
							if (pdlmt == NULL) {
								theArguments.push_back(p);
								break;
							}
							theArguments.push_back(wstring(p, pdlmt - p));
							p = pdlmt;
						}

						if (mSaoriArgumentCalcMode != SACM_OFF) {
							for (strvec::iterator i = theArguments.begin(); i != theArguments.end(); ++i) {
								if (i->size() == 0)
									continue;
								if (mSaoriArgumentCalcMode == SACM_AUTO) {
									int	c = zen2han(*i).at(0);
									if (c != L'+' && c != L'-' && !(c >= L'0' && c <= L'9'))
										continue;
								}

								wstring	exp = *i;
								if (calc(exp, true)) {
									if (state == SAORI_CALL && aredigits(zen2han(exp))) {
										*i = zen2han(exp);
									}
									else {
										*i = exp;
									}
								}
							}
						}

					}
				}
			}

			

			// 引数渡して返値を取得、と。
			if ( state==SAORI_CALL ) {
				for ( strvec::iterator i=theArguments.begin() ; i!=theArguments.end() ; ++i ) {
					m_escaper.unescape(*i);
				}
				oResult = mShioriPlugins->request(thePluginName, theArguments, mKakkoCallResults, secure_flag ? L"Local" : L"External" );
			}
			else if ( state==SPECIAL_CALL ) {
				oResult = special_call(thePluginName, theArguments, false, true, secure_flag);
			}
			else {
				oResult = inc_call(thePluginName, theArguments, mKakkoCallResults, secure_flag);
			}
			oResult = UnKakko(oResult.c_str());	// 返値を再度カッコ展開
		}
	}

	const Word* w;
	wstring hankaku=zen2han(iName);
	bool isSysValue;
	wstring *pstr = GetValue(iName,isSysValue);

	if ( _pre_called_ ) {
		// 前段階ですでに対応カッコ展開済み
	}
	else if ( (w = words.select(iName, *this)) != NULL )
	{
		// 単語を選択した
		GetSender().sender() << L"＠" << iName << std::endl;

		if ( talks.is_exist(iName) ) {
			GetSender().sender() << L"同じ名前「" << iName << L"」の単語群と文があります。トラブルの元なので避けましょう。" << std::endl;
		}

		oResult = UnKakko( w->c_str() );
		//括弧展開後にチェックするようになったのでここは無効化
		//if ( ! for_non_talk ) {
		//	if ( oResult.size() ) {
		//		speaked_speaker.insert(speaker);
		//		add_characters(oResult.c_str(), chars_spoken);
		//	}
		//}
	}
	else if ( talks.is_exist(iName) ) {
		// ＊に定義があれば文を取得
		oResult = GetSentence(iName);
	}
	else if ( pstr || isSysValue ) {
		// 変数名であれば変数の内容を返す
		if ( pstr ) {
			oResult = *pstr;
		}
		else {
			oResult = L"";
		}
	}
	else if ( aredigits(hankaku) || (hankaku[0]==L'-' && aredigits(hankaku.c_str()+1)) ) {
		// サーフェス切り替え
		int	s = stoi_internal(hankaku);
		oResult = wstring(INTERNAL_MARK_STR) + L"\x02" + itos(s) + INTERNAL_MARK_STR; //内部特殊表現に一旦変換して、後でサーフェス加算処理をする
		/* 展開後に処理される
		if ( !is_speaked(speaker) ) {
			if ( surface_changed_before_speak.find(speaker) == surface_changed_before_speak.end() ) {
				surface_changed_before_speak.insert(std::map<int,bool>::value_type(speaker,is_speaked_anybody()) );
			}
		}*/
	}
	else if ( hankaku==L"Aの数" ) {
		if ( ! mCallStack.empty() ) {
			oResult = itos(mCallStack.top().size());
		}
		else {
			oResult = L"0";
		}
	}
	else if ( hankaku==L"Rの数" ) {
		oResult = itos(mReferences.size());
	}
	else if ( hankaku==L"Sの数" ) {
		oResult = itos(mKakkoCallResults.size());
	}
	else if ( compare_head(iName, L"乱数") && iName.size()>const_strlen(L"乱数")+1 ) {
		strvec	vec;
		// 区切りは FULLWIDTH TILDE(U+FF5E, CP932の0x8160) と WAVE DASH(U+301C) の両方を受け付ける
		if ( split( iName.c_str()+const_strlen(L"乱数"), L"\xFF5E\x301C", vec ) != 2 ) {
			oResult = L"※　乱数の指定が変です　※";
		}
		else {
			wstring vec0 = zen2han(vec[0]);
			int	bottom = stoi_internal(vec0);
			int	top = zen2int(vec[1]);
			if ( bottom > top )
				Swap(&bottom, &top);

			if ( vec0 != vec[0] ) {
				if ( bottom == top )
					oResult = int2zen(top);
				else 
					oResult = int2zen( random(top-bottom+1) + bottom );
			}
			else {
				if ( bottom == top )
					oResult = itos(top);
				else 
					oResult = itos( random(top-bottom+1) + bottom );
			}
		}
	}
	else if ( iName == L"里々のバージョン" ) {
		oResult = gSatoriVersion;
	}
	else if ( iName == L"里々のライセンス" ) {
		oResult = gSatoriLicense;
		replace(oResult, L"\n", L"\\n");
	}
	else if ( iName == L"現在年" ) {
#ifdef POSIX
	        time_t st = time(NULL);
	        oResult = int2zen(localtime(&st)->tm_year + 1900);
#else
		SYSTEMTIME st; ::GetLocalTime(&st); oResult=int2zen(st.wYear);
#endif
	}
	else if ( iName == L"現在曜日" ) {
#ifdef POSIX
	        time_t st = time(NULL);
		struct tm* st_tm = localtime(&st);
		static const wchar_t* const ary[7]={L"日",L"月",L"火",L"水",L"木",L"金",L"土"};
		oResult = (st_tm->tm_wday >= 0 && st_tm->tm_wday < 7) ? ary[st_tm->tm_wday] : L"？";
#else
		SYSTEMTIME st; ::GetLocalTime(&st);
		static const wchar_t* const ary[7]={L"日",L"月",L"火",L"水",L"木",L"金",L"土"};
		oResult = ( st.wDayOfWeek >= 0 && st.wDayOfWeek < 7 ) ? ary[st.wDayOfWeek] : L"？";
#endif
	}
#ifdef POSIX
	else if ( iName == L"現在月" ) { time_t st = time(NULL); oResult = int2zen(localtime(&st)->tm_mon + 1); }
	else if ( iName == L"現在日" ) { time_t st = time(NULL); oResult = int2zen(localtime(&st)->tm_mday); }
	else if ( iName == L"現在時" ) { time_t st = time(NULL); oResult = int2zen(localtime(&st)->tm_hour); }
	else if ( iName == L"現在分" ) { time_t st = time(NULL); oResult = int2zen(localtime(&st)->tm_min); }
	else if ( iName == L"現在秒" ) { time_t st = time(NULL); oResult = int2zen(localtime(&st)->tm_sec); }
#else
	else if ( iName == L"現在月" ) { SYSTEMTIME st; ::GetLocalTime(&st); oResult=int2zen(st.wMonth); }
	else if ( iName == L"現在日" ) { SYSTEMTIME st; ::GetLocalTime(&st); oResult=int2zen(st.wDay); }
	else if ( iName == L"現在時" ) { SYSTEMTIME st; ::GetLocalTime(&st); oResult=int2zen(st.wHour); }
	else if ( iName == L"現在分" ) { SYSTEMTIME st; ::GetLocalTime(&st); oResult=int2zen(st.wMinute); }
	else if ( iName == L"現在秒" ) { SYSTEMTIME st; ::GetLocalTime(&st); oResult=int2zen(st.wSecond); }
#endif
	//起動
	else if (iName == L"起動時") {
	    time_t sec = posix_get_current_sec() - sec_count_at_load;
	    time_t hour = sec / 60 / 60;
	    oResult = int2zen(hour);
	}
	else if (iName == L"起動分") {
	    time_t sec = posix_get_current_sec() - sec_count_at_load;
	    time_t hour = sec / 60 / 60;
	    sec -= hour * 60 * 60;
	    time_t minute = sec / 60;
	    oResult = int2zen(minute);
	}
	else if (iName == L"起動秒" ) {
	    time_t sec = posix_get_current_sec() - sec_count_at_load;
	    time_t hour = sec / 60 / 60;
	    sec -= hour * 60 * 60;
	    time_t minute = sec / 60;
	    sec -= minute * 60;
	    oResult = int2zen(sec);
	}
	else if (iName == L"単純起動秒" ) {
	    time_t sec = posix_get_current_sec() - sec_count_at_load;
	    oResult = int2zen(sec);
	}
	else if (iName == L"単純起動分") {
	    time_t sec = posix_get_current_sec() - sec_count_at_load;
	    oResult = int2zen(sec / 60);
	}
	else if (iName == L"単純起動時") {
	    time_t sec = posix_get_current_sec() - sec_count_at_load;
	    oResult = int2zen(sec / 60 / 60);
	}
	//OS起動
	else if (iName == L"OS起動時" || iName == L"ＯＳ起動時") {
	    time_t sec = posix_get_current_sec();
	    time_t hour = sec / 60 / 60;
	    oResult = int2zen(hour);
	}
	else if (iName == L"OS起動分" || iName == L"ＯＳ起動分" ) {
	    time_t sec = posix_get_current_sec();
	    time_t hour = sec / 60 / 60;
	    sec -= hour * 60 * 60;
	    time_t minute = sec / 60;
	    oResult = int2zen(minute);
	}
	else if (iName == L"OS起動秒" || iName == L"ＯＳ起動秒") {
	    time_t sec = posix_get_current_sec();
	    time_t hour = sec / 60 / 60;
	    sec -= hour * 60 * 60;
	    time_t minute = sec / 60;
	    sec -= minute * 60;
	    oResult = int2zen(sec);
	}
	else if (iName == L"単純OS起動秒" || iName == L"単純ＯＳ起動秒") {
	    time_t sec = posix_get_current_sec();
	    oResult = int2zen(sec);
	}
	else if (iName == L"単純OS起動分" || iName == L"単純ＯＳ起動分") {
	    time_t sec = posix_get_current_sec();
	    oResult = int2zen(sec / 60);
	}
	else if (iName == L"単純OS起動時" || iName == L"単純ＯＳ起動時") {
	    time_t sec = posix_get_current_sec();
	    oResult = int2zen(sec / 60 / 60);
	}
	//累計
	else if (iName == L"累計時") {
	    unsigned long sec = posix_get_current_sec() - sec_count_at_load + sec_count_total;
		unsigned long hour = sec / 60 / 60;
	    oResult = ul2zen(hour);
	}
	else if (iName == L"累計分" ) {
		unsigned long  sec = posix_get_current_sec() - sec_count_at_load + sec_count_total;
		unsigned long  hour = sec / 60 / 60;
	    sec -= hour * 60 * 60;
		unsigned long  minute = sec / 60;
		oResult = ul2zen(minute);
	}
	else if (iName == L"累計秒") {
		unsigned long  sec = posix_get_current_sec() - sec_count_at_load + sec_count_total;
		unsigned long  hour = sec / 60 / 60;
	    sec -= hour * 60 * 60;
		unsigned long  minute = sec / 60;
	    sec -= minute * 60;
		oResult = ul2zen(sec);
	}
	else if (iName == L"単純累計秒") {
		unsigned long  sec = posix_get_current_sec() - sec_count_at_load + sec_count_total;
		oResult = ul2zen(sec);
	}
	else if (iName == L"単純累計分") {
		unsigned long  sec = posix_get_current_sec() - sec_count_at_load + sec_count_total;
		oResult = ul2zen(sec / 60);
	}
	else if (iName == L"単純累計時") {
		unsigned long  sec = posix_get_current_sec() - sec_count_at_load + sec_count_total;
		oResult = ul2zen(sec / 60 / 60);
	}
	else if ( hankaku == L"time_t" ) { time_t tm; time(&tm); oResult=int2zen(tm); }
	else if ( iName == L"最終トークからの経過秒" ) { oResult=int2zen(second_from_last_talk); }

	else if ( compare_head(iName, L"サーフェス") && aredigits(iName.c_str()+const_strlen(L"サーフェス")) ) {
		oResult=itos(cur_surface[ zen2int(iName.c_str()+const_strlen(L"サーフェス")) ]);
	}
	else if ( compare_head(iName, L"前回終了時サーフェス") && iName.length() > const_strlen(L"前回終了時サーフェス") ) {
		oResult=itos(last_talk_exiting_surface[ zen2int(iName.c_str()+const_strlen(L"前回終了時サーフェス")) ]);
	}

	else if ( compare_head(iName, L"ウィンドウハンドル") && iName.length() > const_strlen(L"ウィンドウハンドル") ) {
		int character = zen2int(iName.c_str()+const_strlen(L"ウィンドウハンドル"));
		std::map<int,void*>::const_iterator found = characters_hwnd.find(character);
		if ( found != characters_hwnd.end() ) {
            // NOTE: sizeof(void *) == sizeof(long)
#ifdef POSIX
			oResult = uitos((unsigned long)found->second);
#else
			oResult = uitos((unsigned int)found->second);
#endif // POSIX
		}
	}

	else if ( iName == L"隣で起動しているゴースト" ) { 
		oResult = ( otherghostname.size()>=1 ) ? *otherghostname.begin() : L""; //自分自身はotherghostnameには含まない
	}
	else if ( iName == L"起動しているゴースト数" ) { 
		oResult = int2zen(otherghostname.size()+1); //自分自身はotherghostnameには含まないので +1 
	}
	else if ( compare_head(iName, L"isempty") && iName.size()>=8 ) {
		const wchar_t* p = iName.c_str()+7;
		get_a_chr(p);
		oResult = (*p==L'\0') ? L"1" : L"0";
	}

	else if ( compare_head(iName, L"文「") ) {
		if ( compare_tail(iName, L"」の存在") ) {
			wstring	str = strip_head_tail(iName, L"文「", L"」の存在");
			oResult = talks.is_exist(str) ? L"1" : L"0";
		}
		else if ( compare_tail(iName, L"」の数") ) {
			wstring	str = strip_head_tail(iName, L"文「", L"」の数");

			Family<Talk>* f = talks.get_family(str);

			if ( f ) {
				oResult = itos(f->size_of_element());
			}
			else {
				oResult = L"0";
			}
		}
	}
	else if ( compare_head(iName, L"単語群「") ) {
		if ( compare_tail(iName, L"」の存在") ) {
			wstring	str = strip_head_tail(iName, L"単語群「", L"」の存在");
			oResult = words.is_exist(str) ? L"1" : L"0";
		}
		else if ( compare_tail(iName, L"」の数") ) {
			wstring	str = strip_head_tail(iName, L"単語群「", L"」の数");

			int count = 0;

			Family<Word>* f = words.get_family(str);
			if ( f ) {
				count += f->size_of_element();
			}

			oResult = int2zen(count);
		}
		else if (compare_tail(iName, L"」の重複回避枯渇")) {
			wstring str = strip_head_tail(iName, L"単語群「", L"」の重複回避枯渇");
			Family<Word>* f = words.get_family(str);
			bool is_used_all = false;
			if (f) {
				if (f->is_OC_used_all(*this)) {
					is_used_all = true;
				}
			}
			if (is_used_all){
				oResult = L"1";
			}
			else {
				oResult = L"0";
			}
		}
	}


	else if ( compare_head(iName, L"変数「") && compare_tail(iName, L"」の存在") ) {
		wstring	str = strip_head_tail(iName, L"変数「", L"」の存在");
		bool isSysValue;
		wstring *v = GetValue(str,isSysValue); //こっちはシステム変数かどうかどっちでもいい
		oResult = v ? L"1" : L"0";
	}
	else if ( compare_head(iName, L"変数「") && compare_tail(iName, L"」か０") ) {
		wstring	str = strip_head_tail(iName, L"変数「", L"」か０");
		bool isSysValue;
		wstring *v = GetValue(str,isSysValue); //こっちはシステム変数かどうかどっちでもいい
		oResult = v ? *v : L"０";
	}
	else if ( compare_head(iName, L"変数「") && compare_tail(iName, L"」か空文字列") ) {
		wstring	str = strip_head_tail(iName, L"変数「", L"」か空文字列");
		bool isSysValue;
		wstring *v = GetValue(str,isSysValue); //こっちはシステム変数かどうかどっちでもいい
		oResult = v ? *v : L"";
	}


	else if (compare_head(iName, L"導入済みゴースト「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"導入済みゴースト「", L"」の存在");
		oResult = installed_ghost_name.count(str) ? L"1" : L"0";
	}
	else if (compare_head(iName, L"導入済みシェル「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"導入済みシェル「", L"」の存在");
		oResult = installed_shell_name.count(str) ? L"1" : L"0";
	}
	else if (compare_head(iName, L"導入済みバルーン「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"導入済みバルーン「", L"」の存在");
		oResult = installed_balloon_name.count(str) ? L"1" : L"0";
	}
	else if (compare_head(iName, L"導入済みヘッドライセンサ「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"導入済みヘッドライセンサ「", L"」の存在");
		oResult = installed_headline_name.count(str) ? L"1" : L"0";
	}
	else if (compare_head(iName, L"導入済みフォント「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"導入済みフォント「", L"」の存在");
		oResult = installed_font_name.count(str) ? L"1" : L"0";
	}
	else if (compare_head(iName, L"導入済みプラグイン「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"導入済みプラグイン「", L"」の存在");
		oResult = installed_plugin.count(str) ? L"1" : L"0";
	}
	else if (compare_head(iName, L"導入済みプラグイン「") && compare_tail(iName, L"」のID")){
		wstring	str = strip_head_tail(iName, L"導入済みプラグイン「", L"」のID");
		if (installed_plugin.count(str)){
			oResult = installed_plugin[str].plugin_id;
		}
	}
	else if (compare_head(iName, L"使ってるぞグラフ「") && compare_tail(iName, L"」の本体側の名前")){
		wstring	str = strip_head_tail(iName, L"使ってるぞグラフ「", L"」の本体側の名前");
		if (rate_of_use_graph.count(str)){
			oResult = rate_of_use_graph[str].sakura_name;
		}
	}
	else if (compare_head(iName, L"使ってるぞグラフ「") && compare_tail(iName, L"」の相方側の名前")){
		wstring	str = strip_head_tail(iName, L"使ってるぞグラフ「", L"」の相方側の名前");
		if (rate_of_use_graph.count(str)){
			oResult = rate_of_use_graph[str].kero_name;
		}
	}
	else if (compare_head(iName, L"使ってるぞグラフ「") && compare_tail(iName, L"」の起動回数")){
		wstring	str = strip_head_tail(iName, L"使ってるぞグラフ「", L"」の起動回数");
		if (rate_of_use_graph.count(str)){
			oResult = rate_of_use_graph[str].boot_count;
		}
	}
	else if (compare_head(iName, L"使ってるぞグラフ「") && compare_tail(iName, L"」の単純累計分")){
		wstring	str = strip_head_tail(iName, L"使ってるぞグラフ「", L"」の単純累計分");
		if (rate_of_use_graph.count(str)){
			oResult = rate_of_use_graph[str].boot_minutes;
		}
	}
	else if (compare_head(iName, L"使ってるぞグラフ「") && compare_tail(iName, L"」の起動割合")){
		wstring	str = strip_head_tail(iName, L"使ってるぞグラフ「", L"」の起動割合");
		if (rate_of_use_graph.count(str)){
			oResult = rate_of_use_graph[str].boot_percent;
		}
	}
	else if (compare_head(iName, L"使ってるぞグラフ「") && compare_tail(iName, L"」の状態")){
		wstring	str = strip_head_tail(iName, L"使ってるぞグラフ「", L"」の状態");
		if (rate_of_use_graph.count(str)){
			oResult = rate_of_use_graph[str].status;
		}
	}
	else if (compare_head(iName, L"ウインドウ「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"ウインドウ「", L"」の存在");

		oResult = ul2zen(FindTopLevelWindow(str.c_str(),false));
	}
	else if (compare_head(iName, L"「") && compare_tail(iName, L"」を含むウインドウの存在")){
		wstring	str = strip_head_tail(iName, L"「", L"」を含むウインドウの存在");

		oResult = ul2zen(FindTopLevelWindow(str.c_str(),true));
	}
	else if (compare_head(iName, L"プロセス「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"プロセス「", L"」の存在");

		oResult = ul2zen(FindProcessName(str.c_str(),false));
	}
	else if (compare_head(iName, L"「") && compare_tail(iName, L"」を含むプロセスの存在")){
		wstring	str = strip_head_tail(iName, L"「", L"」を含むプロセスの存在");

		oResult = ul2zen(FindProcessName(str.c_str(),true));
	}

	else if (compare_head(iName, L"起動中ゴースト「") && compare_tail(iName, L"」の存在")){
		wstring	str = strip_head_tail(iName, L"起動中ゴースト「", L"」の存在");
		oResult = otherghostname.count(str) ? L"1" : L"0"; //自分自身はotherghostnameには含まない
	}

	else if ( compare_tail(iName, L"の存在") ) {
		updateGhostsInfo();	// ゴースト情報を更新
		std::vector<strmap>::iterator i=ghosts_info.begin();
		for ( ; i!=ghosts_info.end() ; ++i )
			if ( compare_head(iName, (*i)[L"name"]) )
				break;
			else if ( compare_head(iName, (*i)[L"keroname"]) )
				break;
		oResult = ( i==ghosts_info.end() ) ? L"0" : L"1";
	}
	else if ( compare_tail(iName, L"のサーフェス") ) {
		updateGhostsInfo();	// ゴースト情報を更新
		std::vector<strmap>::iterator i=ghosts_info.begin();
		for ( ; i!=ghosts_info.end() ; ++i )
			if ( compare_head(iName, (*i)[L"name"]) ) {
				oResult = (*i)[L"sakura.surface"];
				break;
			} else if ( compare_head(iName, (*i)[L"keroname"]) ) {
				oResult = (*i)[L"kero.surface"];
				break;
			}

		if ( i==ghosts_info.end() ) {
			oResult = L"-1";
		}
	}
	else if ( compare_head(hankaku, L"FMO") && hankaku.size()>4 ) { // FMO?head
		updateGhostsInfo();	// ゴースト情報を更新
		unsigned int digit = 3;
		while ( hankaku[digit] >= L'0' && hankaku[digit] <= L'9' ) { ++digit; }

		if ( digit > 3 ) {
			unsigned int index = wcstoul(hankaku.c_str()+3,NULL,10);
			if ( index < ghosts_info.size() ) {
				strmap&	m=ghosts_info[index];
				wstring	value(hankaku.c_str()+digit);
				if ( m.find(value) != m.end() ) {
					oResult = m[value];
				}
			}
		}
	}
	else if ( compare_head(hankaku, L"count") )
	{
		wstring	name(hankaku.c_str()+5);
		int r = count_func(name);
		if ( r >= 0 ) {
			oResult = int2zen(r);
		}
	}
	else if ( iName==L"セーブデータ読み込み" ) {
		oResult = load_savedata_status;
	}
	else if ( iName==L"次のトーク" ) {
		std::map<int,wstring>::const_iterator it = reserved_talk.find(1);
		if ( it != reserved_talk.end() ) 
			oResult = it->second;
	}
	else if ( compare_head(iName,L"次から") && compare_tail(iName,L"回目のトーク") ) {
		int	count = zen2int( strip_head_tail(iName, L"次から", L"回目のトーク") );
		std::map<int,wstring>::const_iterator it = reserved_talk.find(count);
		if ( it != reserved_talk.end() ) {
			oResult = it->second;
		}
	}
	else if ( compare_head(iName, L"トーク「") && compare_tail(iName, L"」の予約有無") ) { // 「約」には\が含まれる。
		wstring	str = strip_head_tail(iName, L"トーク「", L"」の予約有無");
		oResult = L"0";
		for (std::map<int, wstring>::const_iterator it=reserved_talk.begin(); it!=reserved_talk.end() ; ++it) {
			if ( str == it->second ) {
				oResult = L"1";
				break;
			}
		}
	}
	else if ( iName == L"予約トーク数" ) { // 「約」には\が含まれる。
		oResult = int2zen( reserved_talk.size() );
	}
	else if ( iName == L"イベント名" ) { oResult=mRequestID; }
	else if ( iName == L"直前の選択肢名" ) { oResult=last_choice_name; }
	else if ( hankaku == L"pwd" ) { oResult=mBaseFolder; }
	else if ( iName == L"本体の所在" ) { oResult=mExeFolder; }
	else if ( mRequestMap.find(iName) != mRequestMap.end() ) {
		oResult = mRequestMap[iName];
	}
	else if (iName == L"全変数列挙"){
		if (fDebugMode && secure_flag) {
			oResult = L"";
			for (strmap::iterator i = variables.begin(); i != variables.end(); i++){
				oResult += wstring(L"＄") + i->first + L"\t" + i->second + L"\\n";
			}
		}
	}
	else {
		// 見つからなかった。通常喋り？
		//括弧展開後にチェックするようになったのでここは無効化
		//speaked_speaker.insert(speaker);
		//chars_spoken += oResult.size();
		GetSender().sender() << L"（" << iName << L"） not found." << std::endl;
		return	false;
	}

	if ( stack_size_before_call != 0 && stack_size_before_call <= kakko_replace_history.size() ) {
		kakko_replace_history[stack_size_before_call-1].push_back(oResult);
	}
	GetSender().sender() << L"（" << iName << L"）→" << oResult << L"" << std::endl;
	return	true;
}

//countコール・getaistate互換用
int Satori::count_func(const wstring &name)
{
	if ( name==L"Words" ) { return words.size_of_family(); }
	else if ( name==L"Variable" ) { return variables.size(); }
	else if ( name==L"Anchor" ) { return anchors.size(); }
	else if ( name==L"Talk" ) { return talks.size_of_element(); }
	else if ( name==L"Word" ) { return words.size_of_element(); }
	else if ( name==L"NoNameTalk" )
	{
		Family<Talk>* f = talks.get_family(L"");
		return ( f==0 ) ? 0 : f->size_of_element();
	}
	else if ( name==L"EventTalk" )
	{
		int	n=0;
		for ( std::map< wstring, Family<Talk> >::const_iterator it = talks.compatible().begin() ; it != talks.compatible().end() ; ++it ) {
			if ( compare_head(it->first, L"On") ) {
				n += it->second.size_of_element();
			}
		}
		return n;
	}
	else if ( name==L"OtherTalk" )
	{
		int	n=0;
		for ( std::map< wstring, Family<Talk> >::const_iterator it = talks.compatible().begin() ; it != talks.compatible().end() ; ++it ) {
			if ( !compare_head(it->first, L"On") && !it->first.empty() ) {
				n += it->second.size_of_element();
			}
		}
		return n;
	}
	else if ( name==L"Line" )
	{
		int	n=0;
		for ( std::map< wstring, Family<Talk> >::const_iterator it = talks.compatible().begin() ; it != talks.compatible().end() ; ++it )
		{
			std::vector<const Talk*> v;
			it->second.get_elements_pointers(v);
			for ( std::vector<const Talk*>::const_iterator el_it = v.begin() ; el_it != v.end() ; ++el_it )
			{
				n += (*el_it)->size();
			}
		}
		for ( std::map< wstring, Family<Word> >::const_iterator it = words.compatible().begin() ; it != words.compatible().end() ; ++it )
		{
			n += it->second.size_of_element();
		}
		return n;
	}
	else if ( name==L"Parenthesis" )
	{
		int	n=0;
		for ( std::map< wstring, Family<Talk> >::const_iterator it = talks.compatible().begin() ; it != talks.compatible().end() ; ++it )
		{
			std::vector<const Talk*> v;
			it->second.get_elements_pointers(v);
			for ( std::vector<const Talk*>::const_iterator el_it = v.begin() ; el_it != v.end() ; ++el_it )
			{
				for ( Talk::const_iterator tk_it = (*el_it)->begin() ; tk_it != (*el_it)->end() ; ++tk_it )
				{
					n += count(*tk_it, L"（");
				}
			}
		}
		for ( std::map< wstring, Family<Word> >::const_iterator it = words.compatible().begin() ; it != words.compatible().end() ; ++it )
		{
			std::vector<const Word*> v;
			it->second.get_elements_pointers(v);
			for ( std::vector<const Word*>::const_iterator el_it = v.begin() ; el_it != v.end() ; ++el_it )
			{
				n += count(**el_it, L"（");
			}
		}
		return n;
	}
	return -1;
}

//ウインドウ列挙
#ifndef POSIX
typedef struct EnumWindowsInfo
{
	const wchar_t* txt;
	wchar_t title[1024];
	bool isPartial;
	HWND hWnd;
} EnumWindowsInfo;

BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	EnumWindowsInfo &inf = *reinterpret_cast<EnumWindowsInfo*>(lParam);

	::GetWindowText(hwnd,inf.title,sizeof(inf.title)/sizeof(inf.title[0])-1);

	if ( inf.title[0] ) {
		if ( inf.isPartial ) {
			if ( strstri_hz(inf.title,inf.txt) ) {
				inf.hWnd = hwnd;
				return FALSE;
			}
		}
		else {
			if ( _wcsicmp(inf.title,inf.txt) == 0 ) {
				inf.hWnd = hwnd;
				return FALSE;
			}
		}
	}
	
	return TRUE;
}
#endif

//ウインドウ探索　ウインドウ存在判定で使う
unsigned long Satori::FindTopLevelWindow(const wchar_t* txt,bool isPartial)
{
#ifdef POSIX
	return 0;
#else

	if ( ! txt || ! *txt ) { return 0; }

	EnumWindowsInfo inf;
	inf.txt = txt;
	inf.isPartial = isPartial;
	inf.hWnd = NULL;

	::EnumWindows(EnumWindowsProc,(LPARAM)&inf);

	return (unsigned long)inf.hWnd;
#endif
}

//プロセス探索　ウインドウ存在判定で使う
unsigned long Satori::FindProcessName(const wchar_t* txt,bool isPartial)
{
#ifdef POSIX
	return 0;
#else

	if ( ! txt || ! *txt ) { return 0; }

	HANDLE hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

	PROCESSENTRY32 pinfo = {0};
	pinfo.dwSize = sizeof(pinfo);

	DWORD pid = 0;

	if ( ::Process32First(hSnap,&pinfo) ) {
		do {
			const wchar_t *pName = wcsrchr(pinfo.szExeFile,L'\\');
			if ( pName ) {
				pName += 1;
			}
			else {
				pName = pinfo.szExeFile;
			}

			if ( isPartial ) {
				if ( strstri_hz(pName,txt) ) {
					pid = pinfo.th32ProcessID;
					break;
				}
			}
			else {
				if ( _wcsicmp(pName,txt) == 0 ) {
					pid = pinfo.th32ProcessID;
					break;
				}
			}
		} while ( ::Process32Next(hSnap,&pinfo) );
	}

	::CloseHandle(hSnap);

	return pid;

#endif
}
