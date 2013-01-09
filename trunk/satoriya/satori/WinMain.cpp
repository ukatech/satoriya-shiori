#include	"../_/utilities.h"
#include	"../_/FMO.h"
#include	"../satori/satori.h"
#include	"../_/Sender.h"
#include	"../_/stltool.h"
#include	"../_/Font.h"
#include	"resource.h"

#include	"../_/Dialog.h"

string	SATORITE_WINDOW_CAPTION()
{
	static string wcaption(string() + "さとりて phase 9 / " + gSatoriName + " " + gSatoriVersion); 
	return wcaption;
}


#include	"../_/Window.h"

class UserWindow : public Window
{
	HWND m_next_window;

	virtual	bool	OnCreate(LPCREATESTRUCT iCreateStruct)
	{
		m_next_window = ::SetClipboardViewer(mWnd);
		return true;
	}

	virtual	bool OnDrawClipboard()
	{
		HWND	hwnd=::FindWindow("#32770", SATORITE_WINDOW_CAPTION().c_str());
		if ( hwnd!=NULL )
		{
			::PostMessage(hwnd, WM_COMMAND, ID_READCLIPBOARD_SYNC, 0);
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
			"Satorite_Clipboard_Viewer",
			0,
			"Satorite_Clipboard_Viewer",
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			NULL,
			NULL,
			SW_HIDE,
			NULL);
	}
};





#include	<strstream>
using namespace std;
#include	"SSTPClient.h"
struct	SHARED_DATA {
	HWND	hwndDialog;
	HWND	hwndEditBox;
};


Satori*	pSatori=NULL;
char	base_folder[MAX_PATH];	// 作業フォルダパス
HINSTANCE	gInstance;
static const int BUFFER_SIZE = 1024*1024-1; // 1MB
TCHAR	gBuffer[BUFFER_SIZE+1];	// 入力されたベタテキスト
char* CONFIG_FILENAME = "satorite.dat";
strmap gConf;

string SATORITE_TALK_NAME = "OnPlaySatoriteScript";
//"the_satorite_sentence"

string	Do(const string& str, bool like_dict, bool satori, HWND hwnd)
{
	strvec	vec;	// 受け渡しvector

	if ( like_dict )
	{
		vec.push_back(string()+"＊"+SATORITE_TALK_NAME);
	}

	// 入力をvector化
	istrstream	in(gBuffer);
	while ( in.peek() != EOF ) {
		// １行読み込み
		strstream	line;
		int	c;
		while ( (c=in.get()) != '\n' && c!=EOF)
			if ( c!=13 )
				line.put(c);
		line.put('\0');

		// 行ストリームを固定、各行に対し処理
		char* p=line.str();
		vec.push_back(p);
		// 行ストリームの固定を解除
		line.rdbuf()->freeze(0);
	}
	in.clear();

	DBG(sender << "dialog input strlen(gBuffer): " << strlen(gBuffer) << endl);
	DBG(sender << "dialog input vec.size(): " << vec.size() << endl);

	string	script;
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
		string	fname = string(base_folder)+"dic_maked_by_satorite.txt";
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
		string protocol, protcol_version;
		data.push_back( strpair("ID", SATORITE_TALK_NAME) );
		data.push_back( strpair("Sender", "Satorite") );
		data.push_back( strpair("SecurityLevel", "local") );
		pSatori->request("SHIORI", "3.0", "GET", data, protocol, protcol_version, r_data); /**/
		/*data.push_back( strpair("Argument0", SATORITE_TALK_NAME) );
		data.push_back( strpair("Sender", "Satorite") );
		data.push_back( strpair("SecurityLevel", "Local") );
		pSatori->request("SAORI", "1.0", "EXECUTE", data, protocol, protcol_version, r_data); /**/
		sender << r_data << endl;
		for ( strpairvec::const_iterator it = r_data.begin() ; it != r_data.end() ; ++it )
		{
			if ( it->first == "Value" || it->first == "Result" )
			{
				script = it->second;
				break;
			}
		}
	} else {
		// さくらスクリプト変換
		script = pSatori->SentenceToSakuraScriptExec_with_PreProcess(vec) + "\\e";
		pSatori->Translate(script);
	}

	direct_sstp(script, "Satorite", hwnd);
	return	script;
}



class UserDialog : public Dialog {

	FMO	fmo;
	UserThread	ut;
	Font m_font;
public:
	// イベントハンドラ
	virtual	BOOL	OnInitDialog( HWND hwndFocus, LONG lInitParam ) {
		::SetWindowText(m_hDlg, SATORITE_WINDOW_CAPTION().c_str());
		/*fmo.Create(PAGE_READWRITE, sizeof(SHARED_DATA), "Satorite");
		SHARED_DATA*	p = (SHARED_DATA*)(fmo.Map());
		p->hwndDialog = m_hDlg;
		p->hwndEditBox = toHWND(IDC_EDIT1);
		fmo.Unmap(p);*/

		ut.create(gInstance);

		if ( strmap_from_file(gConf, CONFIG_FILENAME, ",") )
		{
			string s = gConf["IDC_EDIT1"];
			replace(s, "\\\\", "\\");
			replace(s, "\\r\\n", "\r\n");
			SetText(IDC_EDIT1, s);

			Check(IDC_ERASE_AT_ESC, (gConf["IDC_ERASE_AT_ESC"]!="0"));
			Check(IDC_SATORI, (gConf["IDC_SATORI"]!="0"));
			Check(IDC_LIKE_DICT, (gConf["IDC_LIKE_DICT"]!="0"));
			Check(IDC_CHECK_CLIPBOARD, (gConf["IDC_CHECK_CLIPBOARD"]!="0"));
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
			string& s = gConf["IDC_EDIT1"];
			s = GetText(IDC_EDIT1);
			replace(s, "\\", "\\\\");
			replace(s, "\r\n", "\\r\\n");

			gConf["IDC_ERASE_AT_ESC"] = isChecked(IDC_ERASE_AT_ESC) ? "1" : "0";
			gConf["IDC_SATORI"] = isChecked(IDC_SATORI) ? "1" : "0";
			gConf["IDC_LIKE_DICT"] = isChecked(IDC_LIKE_DICT) ? "1" : "0";
			gConf["IDC_CHECK_CLIPBOARD"] = isChecked(IDC_CHECK_CLIPBOARD) ? "1" : "0";

			strmap_to_file(gConf, CONFIG_FILENAME, ",");
		}
		//fmo.Close();
		ut.close();
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
				string	str="";
				::OpenClipboard(NULL);
				HANDLE	hText = ::GetClipboardData(CF_TEXT);
				if(hText != NULL) {
					str = (char*)(::GlobalLock(hText));
					::GlobalUnlock(hText);
				}
				::CloseClipboard();
				if ( str != "" )
					SetText(IDC_EDIT1, str.c_str());
			}
			/* FALL_THRU */
		case IDOK:
			GetText(IDC_EDIT1, gBuffer, BUFFER_SIZE);
			SetText(IDC_EDIT2, "%s", Do(gBuffer, isChecked(IDC_LIKE_DICT)!=0, isChecked(IDC_SATORI)!=0, m_hDlg).c_str());
			break;

		case IDEXIT:
			End(0);
			break;

		case IDCANCEL:
			if ( isChecked(IDC_ERASE_AT_ESC)!=0 ) {
				SetText(IDC_EDIT1, "");
				SetText(IDC_EDIT2, "");
			} else {
				GetText(IDC_EDIT1, gBuffer, BUFFER_SIZE);
				SetText(IDC_EDIT2, "%s", Do(gBuffer, isChecked(IDC_LIKE_DICT)!=0, isChecked(IDC_SATORI)!=0, m_hDlg).c_str());
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
	_setmbcp(_MB_CP_LOCALE);

	gInstance = hInstance;

	HANDLE	hMutex = ::CreateMutex( NULL, FALSE, "SATORITE_MUTEX_OBJECT" );
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
		HWND	hwnd=::FindWindow("#32770", SATORITE_WINDOW_CAPTION().c_str());
		if ( hwnd!=NULL )
			::PostMessage(hwnd, WM_COMMAND, ID_READCLIPBOARD, 0);

		return	FALSE;
	}

	Sender::validate();

	// 作業フォルダ名を作成
	::GetModuleFileName( NULL, base_folder, MAX_PATH );
	char*	p = FindFinalChar(base_folder, DIR_CHAR);
	*(++p)='\0';	// ファイル名を削除

	pSatori = new Satori;
	pSatori->load(base_folder);

	UserDialog*	dlg = new UserDialog;
	int	ret = dlg->Run(hInstance, IDD_DIALOG, NULL);
	delete dlg;
	dlg=NULL;

	pSatori->unload();
	delete pSatori;
	pSatori = NULL;

	return	ret;
}



