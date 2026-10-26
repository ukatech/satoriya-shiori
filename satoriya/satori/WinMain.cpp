#include	"../_/utilities.h"
#include	"../_/FMO.h"
#include	"../satori/satori.h"
#include	"../_/Sender.h"
#include	"../_/stltool.h"
#include	"../_/Font.h"
#include	"resource.h"

#include	"../_/Dialog.h"
#include	"../_/dsstp.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


wstring	SATORITE_WINDOW_CAPTION()
{
	static wstring wcaption(wstring() + L"さとりて phase 9 / " + gSatoriName + L" " + gSatoriVersion); 
	return wcaption;
}


#include	"../_/Window.h"

class UserWindow : public Window
{
	HWND m_next_window;

public:
	UserWindow() : m_next_window(NULL) { }
	virtual ~UserWindow() { }

	virtual	bool	OnCreate(LPCREATESTRUCT iCreateStruct)
	{
		m_next_window = ::SetClipboardViewer(mWnd);
		return true;
	}

	virtual	bool OnDrawClipboard()
	{
		HWND	hwnd=::FindWindow(L"#32770", SATORITE_WINDOW_CAPTION().c_str());
		if ( hwnd!=NULL )
		{
			::PostMessage(hwnd, WM_COMMAND, ID_READCLIPBOARD_SYNC, 0);
		}
		// クリップボードビューアのチェーンの次へ渡す
		if ( m_next_window != NULL )
		{
			::SendMessage(m_next_window, WM_DRAWCLIPBOARD, 0, 0);
		}
		return true;
	}

	virtual	bool OnChangeCBChain(HWND hWndRemove, HWND hWndNext)
	{
		if( hWndRemove == m_next_window )
		{
			m_next_window = hWndNext;
		}
		else if ( m_next_window != NULL )
		{
			::SendMessage(m_next_window, WM_CHANGECBCHAIN, (WPARAM)hWndRemove, (LPARAM)hWndNext);
		}
		return true;
	}

	virtual	bool	OnDestroy()
	{
		::ChangeClipboardChain(mWnd, m_next_window);
		::PostQuitMessage(0);
		return true;
	}
};



#include	"../_/Thread.h"

class	UserThread : public Thread
{
	UserWindow w;

public:
	UserThread() { }
	virtual ~UserThread() { }

	// ウィンドウを閉じてメッセージループを抜けさせ、スレッドの終了を待つ
	void	quit()
	{
		HWND h = w;
		if ( h != NULL )
		{
			::PostMessage(h, WM_CLOSE, 0, 0);
			waitEnd(3000);
		}
		close();
	}

	virtual	DWORD	ThreadMain()
	{
		return w.run(
			0,
			(HINSTANCE)mParam,
			::LoadIcon(NULL, IDI_APPLICATION),
			::LoadIcon(NULL, IDI_APPLICATION),
			::LoadCursor(NULL, IDC_ARROW),
			NULL,
			NULL,
			L"Satorite_Clipboard_Viewer",
			0,
			L"Satorite_Clipboard_Viewer",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL,
			NULL,
			SW_HIDE,
			NULL);
	}
};





#include	<strstream>
using std::wstring;
#include	"SSTPClient.h"
struct	SHARED_DATA {
	HWND	hwndDialog;
	HWND	hwndEditBox;
};


Satori*	pSatori=NULL;
wchar_t	base_folder[MAX_PATH];	// 作業フォルダパス
HINSTANCE	gInstance;
static const int BUFFER_SIZE = 1024*1024-1; // 1MB
TCHAR	gBuffer[BUFFER_SIZE+1];	// 入力されたベタテキスト
wchar_t* CONFIG_FILENAME = L"satorite.dat";
strmap gConf;

wstring SATORITE_TALK_NAME = L"OnPlaySatoriteScript";
//"the_satorite_sentence"

wstring	Do(const wstring& str, bool like_dict, bool satori, HWND hwnd)
{
	strvec	vec;	// 受け渡しvector

	if ( like_dict )
	{
		vec.push_back(wstring()+L"＊"+SATORITE_TALK_NAME);
	}

	// 入力をvector化（改行で分割、CRは捨てる）
	{
		wstring	line;
		for ( wstring::const_iterator it = str.begin() ; it != str.end() ; ++it ) {
			if ( *it == L'\n' ) {
				vec.push_back(line);
				line.erase();
			}
			else if ( *it != L'\r' ) {
				line += *it;
			}
		}
		if ( !line.empty() ) {
			vec.push_back(line);
		}
	}

	DBG(GetSender().sender() << L"dialog input strlen(gBuffer): " << wcslen(gBuffer) << std::endl);
	DBG(GetSender().sender() << L"dialog input vec.size(): " << vec.size() << std::endl);

	wstring	script;
	if ( !satori )
	{
		for ( strvec::const_iterator i=vec.begin() ; i!=vec.end() ; ++i ) {
			script += *i;
		}
		pSatori->Translate(script);
	}
	else if ( like_dict ) 
	{
		// 辞書として保存
		wstring	fname = wstring(base_folder)+L"dic_maked_by_satorite.txt";
		strvec_to_file(vec, fname);
		// リロード
		pSatori->unload();
		pSatori->load(base_folder);
		// 辞書削除
		::DeleteFile(fname.c_str());
		// 文を取得
#if 0
		script = pSatori->GetSentence(SATORITE_TALK_NAME);
#endif
		strpairvec data, r_data;
		wstring protocol, protcol_version;
		data.push_back( strpair(L"ID", SATORITE_TALK_NAME) );
		data.push_back( strpair(L"Sender", L"Satorite") );
		data.push_back( strpair(L"SecurityLevel", L"local") );
		pSatori->request(L"SHIORI", L"3.0", L"GET", data, protocol, protcol_version, r_data); /**/
		/*data.push_back( strpair("Argument0", SATORITE_TALK_NAME) );
		data.push_back( strpair("Sender", "Satorite") );
		data.push_back( strpair("SecurityLevel", "Local") );
		pSatori->request("SAORI", "1.0", "EXECUTE", data, protocol, protcol_version, r_data); /**/
		GetSender().sender() << r_data << std::endl;
		for ( strpairvec::const_iterator it = r_data.begin() ; it != r_data.end() ; ++it )
		{
			if ( it->first == L"Value" || it->first == L"Result" )
			{
				script = it->second;
				break;
			}
		}
	} else {
		// さくらスクリプト変換
		script = pSatori->SentenceToSakuraScriptExec_with_PreProcess(vec) + L"\\e";
		pSatori->Translate(script);
	}

	direct_sstp(script, L"Satorite", hwnd);
	return	script;
}



// ShioriLint を実行し、見つけた問題を1件1要素で oLines に入れる。最後に件数の行を足す。
// 辞書は読み込み済みとして扱う（編集後に調べ直すときは、呼ぶ前に unload / load する）。
// error と critical が1件でもあれば true を返す。
bool	RunLint(strvec& oLines)
{
	strpairvec data, r_data;
	wstring protocol, protocol_version;
	data.push_back( strpair(L"ID", L"ShioriLint") );
	data.push_back( strpair(L"Sender", L"Satorite") );
	data.push_back( strpair(L"SecurityLevel", L"local") );
	pSatori->request(L"SHIORI", L"3.0", L"GET", data, protocol, protocol_version, r_data);

	// ErrorLevel / ErrorDescription は、要素をバイト値1で区切って並べてある（要素数は同じ）
	strvec levels, descriptions;
	for ( strpairvec::const_iterator it = r_data.begin() ; it != r_data.end() ; ++it )
	{
		if ( it->first == L"ErrorLevel" )
			split(it->second, L"\1", levels);
		else if ( it->first == L"ErrorDescription" )
			split(it->second, L"\1", descriptions);
	}

	int errors = 0, warnings = 0;
	for ( size_t i = 0 ; i < descriptions.size() ; ++i )
	{
		const wstring& level = ( i < levels.size() ) ? levels[i] : wstring();
		if ( level == L"error" || level == L"critical" )
			++errors;
		else if ( level == L"warning" )
			++warnings;
		oLines.push_back(descriptions[i]);
	}

	wchar_t	summary[128];
	swprintf(summary, L"検査が終わりました。エラー %d 件、警告 %d 件", errors, warnings);
	oLines.push_back(summary);
	return	errors > 0;
}


// コンソールか、リダイレクト先に出力する。どちらも無ければ（ショートカットなどからの起動）メッセージボックスに出す。
void	WriteLintResult(const strvec& lines)
{
	wstring	text;
	for ( strvec::const_iterator it = lines.begin() ; it != lines.end() ; ++it )
	{
		text += *it;
		text += L"\r\n";
	}

	// GUI サブシステムなので、コンソールから起動されていれば親のコンソールにつなぐ
	typedef	BOOL (WINAPI *ATTACHCONSOLE)(DWORD);
	ATTACHCONSOLE	pAttachConsole = (ATTACHCONSOLE)::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "AttachConsole");

	HANDLE	h = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if ( h == NULL || h == INVALID_HANDLE_VALUE )
	{
		if ( pAttachConsole != NULL && pAttachConsole((DWORD)-1) )
		{
			h = ::CreateFile(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
		}
	}

	DWORD	written = 0;
	DWORD	mode = 0;
	if ( h == NULL || h == INVALID_HANDLE_VALUE )
	{
		if ( text.size() > 3000 )
			text = text.substr(0, 3000) + L"…\r\n";
		::MessageBox(NULL, text.c_str(), L"さとりて ShioriLint", MB_OK);
	}
	else if ( ::GetConsoleMode(h, &mode) )
	{
		::WriteConsole(h, text.c_str(), (DWORD)text.size(), &written, NULL);
	}
	else
	{
		// リダイレクト先には、親のコンソールの出力コードページで書く（PowerShell などが同じ設定で読むため）。
		// 親にコンソールが無いときは、システムの ANSI コードページ（日本語環境なら Shift_JIS）にする。
		if ( pAttachConsole != NULL )
			pAttachConsole((DWORD)-1);
		UINT	cp = ::GetConsoleOutputCP();
		if ( cp == 0 )
			cp = CP_ACP;
		int	len = ::WideCharToMultiByte(cp, 0, text.c_str(), (int)text.size(), NULL, 0, NULL, NULL);
		std::string	bytes(len, '\0');
		if ( len > 0 )
			::WideCharToMultiByte(cp, 0, text.c_str(), (int)text.size(), &bytes[0], len, NULL, NULL);
		::WriteFile(h, bytes.c_str(), (DWORD)bytes.size(), &written, NULL);
	}
}


// コマンドラインの先頭（実行ファイル名）より後ろに、単独の引数 iOption があるか
bool	HasCommandLineOption(const wchar_t* iOption)
{
	const wchar_t*	p = ::GetCommandLineW();
	if ( *p == L'"' )
	{
		++p;
		while ( *p != L'\0' && *p != L'"' )
			++p;
		if ( *p == L'"' )
			++p;
	}
	else
	{
		while ( *p != L'\0' && *p != L' ' && *p != L'\t' )
			++p;
	}

	const size_t	len = wcslen(iOption);
	while ( *p != L'\0' )
	{
		while ( *p == L' ' || *p == L'\t' )
			++p;
		const wchar_t*	head = p;
		while ( *p != L'\0' && *p != L' ' && *p != L'\t' )
			++p;
		if ( size_t(p - head) == len && _wcsnicmp(head, iOption, len) == 0 )
			return	true;
	}
	return	false;
}


class UserDialog : public Dialog {

	FMO	fmo;
	UserThread	ut;
	Font m_font;

	// ウィンドウの大きさを変えたときの、コントロールの追従のしかた
	enum {
		MOVE_X = 1,	// 幅が増えた分だけ右へ動く
		MOVE_Y = 2,	// 高さが増えた分だけ下へ動く
		GROW_W = 4,	// 幅が増えた分だけ広がる
		GROW_H = 8	// 高さが増えた分だけ伸びる
	};
	struct LayoutItem {
		int	id;
		int	flags;
		RECT	rc;	// 初期状態のクライアント座標
	};
	enum { LAYOUT_COUNT = 7 };	// VC6 はクラス内の static const int の初期化に対応していない
	LayoutItem	m_layout[LAYOUT_COUNT];
	bool	m_layout_ready;
	SIZE	m_initial_client;	// 初期状態のクライアント領域の大きさ
	SIZE	m_min_window;	// これより小さくはしない（初期状態のウィンドウの大きさ）

	void	InitLayout()
	{
		static const int ids[LAYOUT_COUNT] = {
			IDC_EDIT1, IDC_EDIT2, IDOK,
			IDC_LIKE_DICT, IDC_ERASE_AT_ESC, IDC_SATORI, IDC_CHECK_CLIPBOARD,
		};
		static const int flags[LAYOUT_COUNT] = {
			GROW_W, GROW_W | GROW_H, MOVE_X,
			MOVE_Y, MOVE_Y, MOVE_Y, MOVE_X | MOVE_Y,
		};
		for ( int i = 0 ; i < LAYOUT_COUNT ; ++i )
		{
			m_layout[i].id = ids[i];
			m_layout[i].flags = flags[i];
			::GetWindowRect(toHWND(ids[i]), &m_layout[i].rc);
			::ScreenToClient(m_hDlg, (POINT*)&m_layout[i].rc.left);
			::ScreenToClient(m_hDlg, (POINT*)&m_layout[i].rc.right);
		}

		RECT	client, window;
		::GetClientRect(m_hDlg, &client);
		::GetWindowRect(m_hDlg, &window);
		m_initial_client.cx = client.right;
		m_initial_client.cy = client.bottom;
		m_min_window.cx = window.right - window.left;
		m_min_window.cy = window.bottom - window.top;
		m_layout_ready = true;
	}

	// 保存してあった位置と大きさを、画面内に収まる場合だけ反映する
	void	RestoreWindowRect()
	{
		if ( gConf.find(L"WINDOW_W") == gConf.end() || gConf.find(L"WINDOW_H") == gConf.end() )
			return;
		int	w = _wtoi(gConf[L"WINDOW_W"].c_str());
		int	h = _wtoi(gConf[L"WINDOW_H"].c_str());
		if ( w < m_min_window.cx )
			w = m_min_window.cx;
		if ( h < m_min_window.cy )
			h = m_min_window.cy;

		UINT	flags = SWP_NOZORDER | SWP_NOACTIVATE;
		int	x = 0, y = 0;
		if ( gConf.find(L"WINDOW_X") != gConf.end() && gConf.find(L"WINDOW_Y") != gConf.end() )
		{
			x = _wtoi(gConf[L"WINDOW_X"].c_str());
			y = _wtoi(gConf[L"WINDOW_Y"].c_str());
			// 別のモニタが外されていたときのため、タイトルバーの左端あたりが画面内にあるときだけ動かす
			int	vx = ::GetSystemMetrics(SM_XVIRTUALSCREEN);
			int	vy = ::GetSystemMetrics(SM_YVIRTUALSCREEN);
			int	vw = ::GetSystemMetrics(SM_CXVIRTUALSCREEN);
			int	vh = ::GetSystemMetrics(SM_CYVIRTUALSCREEN);
			if ( vw <= 0 || vh <= 0 )
			{
				vx = 0; vy = 0;
				vw = ::GetSystemMetrics(SM_CXSCREEN);
				vh = ::GetSystemMetrics(SM_CYSCREEN);
			}
			if ( x + 32 < vx || x + 32 > vx + vw || y < vy || y + 16 > vy + vh )
				flags |= SWP_NOMOVE;
		}
		else
		{
			flags |= SWP_NOMOVE;
		}
		::SetWindowPos(m_hDlg, NULL, x, y, w, h, flags);
	}

	void	SaveWindowRect()
	{
		if ( ::IsIconic(m_hDlg) )
			return;	// 最小化中の座標は使えないので、前回の値を残す
		RECT	rc;
		::GetWindowRect(m_hDlg, &rc);
		wchar_t	buf[32];
		swprintf(buf, L"%d", (int)rc.left);
		gConf[L"WINDOW_X"] = buf;
		swprintf(buf, L"%d", (int)rc.top);
		gConf[L"WINDOW_Y"] = buf;
		swprintf(buf, L"%d", (int)(rc.right - rc.left));
		gConf[L"WINDOW_W"] = buf;
		swprintf(buf, L"%d", (int)(rc.bottom - rc.top));
		gConf[L"WINDOW_H"] = buf;
	}

public:
	UserDialog() : m_layout_ready(false) { }
	virtual ~UserDialog() { }

	// イベントハンドラ
	virtual	BOOL	OnSize( UINT nType, int cx, int cy ) {
		if ( !m_layout_ready || nType == SIZE_MINIMIZED )
			return FALSE;
		int	dx = cx - m_initial_client.cx;
		int	dy = cy - m_initial_client.cy;
		if ( dx < 0 )
			dx = 0;
		if ( dy < 0 )
			dy = 0;
		for ( int i = 0 ; i < LAYOUT_COUNT ; ++i )
		{
			const LayoutItem&	item = m_layout[i];
			int	x = item.rc.left + ((item.flags & MOVE_X) ? dx : 0);
			int	y = item.rc.top + ((item.flags & MOVE_Y) ? dy : 0);
			int	w = (item.rc.right - item.rc.left) + ((item.flags & GROW_W) ? dx : 0);
			int	h = (item.rc.bottom - item.rc.top) + ((item.flags & GROW_H) ? dy : 0);
			::MoveWindow(toHWND(item.id), x, y, w, h, TRUE);
		}
		return TRUE;
	}

	virtual	BOOL	OnGetMinMaxInfo( MINMAXINFO* pInfo ) {
		if ( !m_layout_ready )
			return FALSE;
		pInfo->ptMinTrackSize.x = m_min_window.cx;
		pInfo->ptMinTrackSize.y = m_min_window.cy;
		return TRUE;
	}

	virtual	BOOL	OnInitDialog( HWND hwndFocus, LONG lInitParam ) {
		::SetWindowText(m_hDlg, SATORITE_WINDOW_CAPTION().c_str());

		// ダイアログは既定ではアイコンを持たないので、タイトルバーとタスクバー用に設定する
		::SendMessage(m_hDlg, WM_SETICON, ICON_BIG,
			(LPARAM)::LoadImage(gInstance, MAKEINTRESOURCE(IDI_SATORITE), IMAGE_ICON,
				::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_SHARED));
		::SendMessage(m_hDlg, WM_SETICON, ICON_SMALL,
			(LPARAM)::LoadImage(gInstance, MAKEINTRESOURCE(IDI_SATORITE), IMAGE_ICON,
				::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_SHARED));

		InitLayout();
		/*fmo.Create(PAGE_READWRITE, sizeof(SHARED_DATA), "Satorite");
		SHARED_DATA*	p = (SHARED_DATA*)(fmo.Map());
		p->hwndDialog = m_hDlg;
		p->hwndEditBox = toHWND(IDC_EDIT1);
		fmo.Unmap(p);*/

		ut.create(gInstance);

		if ( strmap_from_file(gConf, CONFIG_FILENAME, L",") )
		{
			wstring s = gConf[L"IDC_EDIT1"];
			replace(s, L"\\\\", L"\\");
			replace(s, L"\\r\\n", L"\r\n");
			SetText(IDC_EDIT1, s);

			Check(IDC_ERASE_AT_ESC, (gConf[L"IDC_ERASE_AT_ESC"]!=L"0"));
			Check(IDC_SATORI, (gConf[L"IDC_SATORI"]!=L"0"));
			Check(IDC_LIKE_DICT, (gConf[L"IDC_LIKE_DICT"]!=L"0"));
			Check(IDC_CHECK_CLIPBOARD, (gConf[L"IDC_CHECK_CLIPBOARD"]!=L"0"));

			RestoreWindowRect();
		}
		else
		{
			Check(IDC_SATORI);
		}
		
		// フォントを固定長のものにする
		m_font.Size(12);
		m_font.update();
		::SendMessage(toHWND(IDC_EDIT1), WM_SETFONT, (WPARAM)(m_font.getFont()), 1);
		::SendMessage(toHWND(IDC_EDIT2), WM_SETFONT, (WPARAM)(m_font.getFont()), 1);

		SetFocus( toHWND(IDC_EDIT1) );
		return TRUE;
	}

	virtual	BOOL	OnDestroy()
	{
		{
			wstring& s = gConf[L"IDC_EDIT1"];
			s = GetText(IDC_EDIT1);
			replace(s, L"\\", L"\\\\");
			replace(s, L"\r\n", L"\\r\\n");

			gConf[L"IDC_ERASE_AT_ESC"] = isChecked(IDC_ERASE_AT_ESC) ? L"1" : L"0";
			gConf[L"IDC_SATORI"] = isChecked(IDC_SATORI) ? L"1" : L"0";
			gConf[L"IDC_LIKE_DICT"] = isChecked(IDC_LIKE_DICT) ? L"1" : L"0";
			gConf[L"IDC_CHECK_CLIPBOARD"] = isChecked(IDC_CHECK_CLIPBOARD) ? L"1" : L"0";

			SaveWindowRect();

			strmap_to_file(gConf, CONFIG_FILENAME, L",");
		}
		//fmo.Close();
		ut.quit();
		return FALSE;
	}

	virtual	BOOL	OnSysCommand( UINT uCmdType, WORD xPos, WORD yPos ) {
		if ( uCmdType==SC_CLOSE ) {
			End(0);
			return TRUE;
		}
		return FALSE;
	}

	// override event-handler
	virtual	BOOL	OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCrl ) {
		switch (wID) {

		case IDC_SATORI:
			if ( isChecked(IDC_SATORI)!=0 ) {
				Enable(IDC_LIKE_DICT);
			} else {
				Check(IDC_LIKE_DICT, FALSE);
				Disable(IDC_LIKE_DICT);
			}
			break;

		case ID_READCLIPBOARD_SYNC:
			if ( isChecked(IDC_CHECK_CLIPBOARD)==0 )
				break;
			/* FALL_THRU */
		case ID_READCLIPBOARD:
			{
				wstring	str=L"";
				::OpenClipboard(NULL);
				HANDLE	hText = ::GetClipboardData(CF_UNICODETEXT);
				if(hText != NULL) {
					const wchar_t* pText = (const wchar_t*)(::GlobalLock(hText));
					if ( pText != NULL ) {
						str = pText;
						::GlobalUnlock(hText);
					}
				}
				::CloseClipboard();
				if ( str != L"" )
					SetText(IDC_EDIT1, str);	// 書式として解釈させない（% を含むスクリプトで落ちる）
			}
			/* FALL_THRU */
		case IDOK:
			GetText(IDC_EDIT1, gBuffer, BUFFER_SIZE);
			// 書式経由だと 4095 文字で切れて終端も付かないので、文字列のまま渡す
			SetText(IDC_EDIT2, Do(gBuffer, isChecked(IDC_LIKE_DICT)!=0, isChecked(IDC_SATORI)!=0, m_hDlg));
			break;

		case IDC_LINT:
			{
				// 辞書の編集を反映するため、読み直してから検査する
				pSatori->unload();
				pSatori->load(base_folder);
				strvec	lines;
				RunLint(lines);
				wstring	text;
				for ( strvec::const_iterator it = lines.begin() ; it != lines.end() ; ++it )
				{
					text += *it;
					text += L"\r\n";
				}
				SetText(IDC_EDIT2, text);
			}
			break;

		case IDEXIT:
			End(0);
			break;

		case IDCANCEL:
			if ( isChecked(IDC_ERASE_AT_ESC)!=0 ) {
				SetText(IDC_EDIT1, L"");
				SetText(IDC_EDIT2, L"");
			} else {
				GetText(IDC_EDIT1, gBuffer, BUFFER_SIZE);
				SetText(IDC_EDIT2, Do(gBuffer, isChecked(IDC_LIKE_DICT)!=0, isChecked(IDC_SATORI)!=0, m_hDlg));
			}
			break;

		default:
			return	FALSE;
		}
		return	TRUE;
	}
};






int WINAPI
WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInst,
	LPSTR lpszCmdParam,
	int iShowCmd) 
{
	setlocale(LC_ALL, "Japanese");

	gInstance = hInstance;

	// --lint：ダイアログを出さずに ShioriLint だけ実行して終了する（単一起動の確認より前に分岐する）。
	// 終了コードは、エラーがあれば1、なければ0。
	if ( HasCommandLineOption(L"--lint") )
	{
		::GetModuleFileName( NULL, base_folder, MAX_PATH );
		wchar_t*	lp = FindFinalChar(base_folder, DIR_CHAR);
		*(++lp)=L'\0';

		pSatori = new Satori;
		pSatori->load(base_folder);
		strvec	lines;
		bool	has_error = RunLint(lines);
		WriteLintResult(lines);
		pSatori->unload();
		delete pSatori;
		pSatori = NULL;
		return	has_error ? 1 : 0;
	}

	/*HANDLE hMutex = */::CreateMutex( NULL, FALSE, L"SATORITE_MUTEX_OBJECT" );
	if ( ::GetLastError() == ERROR_ALREADY_EXISTS )
	{
		/*FMO	fmo;
		if ( str!="" && fmo.Open(FILE_MAP_READ, FALSE, "Satorite")) {
			SHARED_DATA*	p = (SHARED_DATA*)(fmo.Map());
			::SetWindowText(p->hwndEditBox, str.c_str());
			::PostMessage(p->hwndDialog, WM_COMMAND, IDOK, 0);
			fmo.Unmap(p);
		}
		if ( str!="" && hwnd!=NULL ) {
			COPYDATASTRUCT	cds = {0,  str.size()+1, (void*)str.c_str()};
			::PostMessage(hwnd, WM_COPYDATA, NULL, LPARAM(&cds));
			::PostMessage(hwnd, WM_COMMAND, IDOK, 0);
		}*/
		HWND	hwnd=::FindWindow(L"#32770", SATORITE_WINDOW_CAPTION().c_str());
		if ( hwnd!=NULL )
			::PostMessage(hwnd, WM_COMMAND, ID_READCLIPBOARD, 0);

		return	FALSE;
	}

	GetSender().validate();

	// 作業フォルダ名を作成
	::GetModuleFileName( NULL, base_folder, MAX_PATH );
	wchar_t*	p = FindFinalChar(base_folder, DIR_CHAR);
	*(++p)=L'\0';	// ファイル名を削除

	std::vector<HWND> hwndVec;
	CheckSakuraFileMapping(NULL,hwndVec);

	pSatori = new Satori;
	pSatori->load(base_folder);

	if ( hwndVec.size() > 0 ) {
		pSatori->SetCharacterHWnd((void*)hwndVec[0]);
	}

	UserDialog*	dlg = new UserDialog;
	int	ret = dlg->Run(hInstance, IDD_DIALOG, NULL);
	delete dlg;
	dlg=NULL;

	pSatori->unload();
	delete pSatori;
	pSatori = NULL;

	return	ret;
}



