#pragma comment(lib, "comctl32.lib")
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h> // for コモンコントロール初期化 InitCommonControls()

// タブに貼り付けた子ダイアログのコールバック関数
LRESULT CALLBACK TabSheetProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_INITDIALOG:
		return TRUE;	// TRUEにするとことがミソ
	}
	return FALSE;	// あとは全部デフォルト任せ
}




#include	"../_/utilities.h"
#include	"../_/stltool.h"
#include	"../_/Dialog.h"
#include	"../sodate/password.h"
#include	"resource.h"

wstring	base_folder;	// 作業フォルダ名
strmap	conf;

static const wchar_t* conf_filename = L"sodate.dat";
static const wchar_t byte_value_1[2] = {1,0};
static const wchar_t byte_value_2[2] = {2,0};


class UserDialog : public Dialog {
	bool	Save();
public:
	virtual	BOOL	OnInitDialog( HWND hwndFocus, LONG lInitParam );
	virtual	BOOL	OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCrl );
};



BOOL	UserDialog::OnInitDialog( HWND hwndFocus, LONG lInitParam ) {
	if ( strmap_from_file(conf, base_folder+conf_filename, byte_value_1) )
	{
		// 設定ファイルの読み込みに成功

		// \2区切りだったものを\r\nに戻す
		replace( conf[L"allow_files"], byte_value_2, L"\r\n");
		replace( conf[L"deny_files"], byte_value_2, L"\r\n");

		// パスワードをデコード
		conf[L"password"] = decode_password(conf[L"password"]);
	}
	else
	{
		// 設定ファイルの読み込みに失敗、confにデフォルト値を設定する

		conf[L"is_upload"]=L"1";
		conf[L"host"]=L"127.0.0.1";
		conf[L"id"]=L"test";
		conf[L"password"]=L"1234";
		conf[L"ghostroot"]=L"homepage/myghost";
		conf[L"timediff"]=L"0";
		conf[L"yeardiff"]=L"0";
		conf[L"is_create_archive"]=L"0";
		//conf[L"archive_filename"]=L"myGhost%year%month%day_%hour%minute.nar";
		conf[L"archive_filename"]=L"myghost%year%month%day.nar";
		conf[L"archive_local_folder"]=base_folder;
		conf[L"allow_files"]=
			L"delete.txt\r\n"
			L"install.txt\r\n"
			L"readme.txt\r\n"
			L"thumbnail.*\r\n"
			L"ghost/master/*.dll\r\n"
			L"ghost/master/saori/*.dll\r\n"
			L"ghost/master/*.txt\r\n"
			L"ghost/master/*.ico\r\n"
			L"ghost/master/*.png\r\n"
			L"ghost/master/*.sat\r\n"
			L"ghost/master/*.dic\r\n"
			L"ghost/master/*.ayc\r\n"
			L"ghost/master/openkeeps/*.kis\r\n"
			L"ghost/master/openkeeps/plugin/*.kis\r\n"
			L"ghost/master/template/*.kis\r\n"
			L"ghost/master/template/another/*.kis\r\n"
			L"ghost/master/*.kis\r\n"
			L"ghost/master/*.kaw\r\n"
			L"shell/master/*.txt\r\n"
			L"shell/master/*.dll\r\n"
			L"shell/master/*.ico\r\n"
			L"shell/master/*.png\r\n"
			L"";
		conf[L"deny_files"]=
			L"ghost/master/satori_savedata.txt\r\n"
			L"ghost/master/satori_savebackup.txt\r\n"
			L"ghost/master/satorite.txt\r\n"
			L"ghost/master/nsconf.txt\r\n"
			L"ghost/master/Dict-KEEPSs.txt\r\n"
			L"ghost/master/Dict-Learned.txt\r\n"
			L"ghost/master/misaka_vars.txt\r\n"
			L"ghost/master/aya_variable.cfg\r\n"
			L"ghost/master/yaya_variable.cfg\r\n"
			L"ghost/master/ssp_shiori_log.txt\r\n"
			L"ghost/master/れしば.txt\r\n"
			L"";
		conf[L"morelog"] = L"0";
		conf[L"yeardiff"] = L"0";
		conf[L"dog"] = L"1";
		conf[L"is_passive"] = L"1";
	}

	// confの内容をダイアログに設定

	Check(IDC_IS_UPLOAD, conf[L"is_upload"]!=L"0");
	SetText(IDC_HOST, conf[L"host"]);
	SetText(IDC_ID, conf[L"id"]);
	SetText(IDC_PASSWORD, conf[L"password"]);
	SetText(IDC_GHOSTROOT, conf[L"ghostroot"]);
	SetText(IDC_TIMEDIFF, conf[L"timediff"]);
	Check(IDC_IS_PASSIVE, conf[L"is_passive"]==L"1");
	// zip/narアーカイブの作成には対応しなくなったので、関係する項目は無効にしておく
	conf[L"is_create_archive"] = L"0";
	Check(IDC_IS_CREATE_ARCHIVE, FALSE);
	Enable(IDC_IS_CREATE_ARCHIVE, FALSE);
	SetText(IDC_ARCHIVE_FILENAME, conf[L"archive_filename"]);
	SetText(IDC_ARCHIVE_LOCAL_FOLDER, conf[L"archive_local_folder"]);
	SetText(IDC_ALLOW_FILES, conf[L"allow_files"]);
	SetText(IDC_DENY_FILES, conf[L"deny_files"]);


	BOOL	flag = isChecked(IDC_IS_UPLOAD) ? TRUE : FALSE;
	Enable(IDC_HOST, flag);
	Enable(IDC_ID, flag);
	Enable(IDC_PASSWORD, flag);
	Enable(IDC_GHOSTROOT, flag);
	Enable(IDC_TIMEDIFF, flag);
	Enable(IDC_IS_PASSIVE, flag);
	Enable(IDC_ARCHIVE_FILENAME, FALSE);
	Enable(IDC_ARCHIVE_LOCAL_FOLDER, FALSE);

	return	FALSE;
}

bool	UserDialog::Save()
{
	conf[L"is_upload"] = isChecked(IDC_IS_UPLOAD) ? L"1" : L"0";
	GetText(IDC_HOST, conf[L"host"]);
	GetText(IDC_ID, conf[L"id"]);
	GetText(IDC_PASSWORD, conf[L"password"]);
	GetText(IDC_GHOSTROOT, conf[L"ghostroot"]);
	GetText(IDC_TIMEDIFF, conf[L"timediff"]);
	conf[L"is_passive"] = isChecked(IDC_IS_PASSIVE) ? L"1" : L"0";
	conf[L"is_create_archive"] = L"0";
	GetText(IDC_ARCHIVE_FILENAME, conf[L"archive_filename"]);
	GetText(IDC_ARCHIVE_LOCAL_FOLDER, conf[L"archive_local_folder"]);
	GetText(IDC_ALLOW_FILES, conf[L"allow_files"]);
	GetText(IDC_DENY_FILES, conf[L"deny_files"]);

	// 保存用に\r\nを\2区切りに変換
	replace( conf[L"allow_files"], L"\r\n", byte_value_2);
	replace( conf[L"deny_files"], L"\r\n", byte_value_2);

	// パスワードをエンコード（画面の値は変えずに保存用の写しを作る）
	strmap	save_conf = conf;
	save_conf[L"password"] = encode_password(conf[L"password"]);

	// ファイルに保存
	if ( !strmap_to_file(save_conf, base_folder+conf_filename, byte_value_1) ) 
	{
		::MessageBox(m_hDlg,
			(base_folder+conf_filename+L"\n\n設定ファイルが保存できませんでした。").c_str()
			,L"error", MB_OK);
		return	false;
	}
	return	true;
}

BOOL	UserDialog::OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCrl ) {
	switch (wID) {

	case	IDC_IS_UPLOAD:
		{
			BOOL	flag = isChecked(IDC_IS_UPLOAD) ? TRUE : FALSE;
			Enable(IDC_HOST, flag);
			Enable(IDC_ID, flag);
			Enable(IDC_PASSWORD, flag);
			Enable(IDC_GHOSTROOT, flag);
			Enable(IDC_TIMEDIFF, flag);
			Enable(IDC_IS_PASSIVE, flag);
		}
		break;

	case	IDC_ARCHIVE_HELP:
		::MessageBox(m_hDlg,
			L"このバージョンのsodateは、zip/narアーカイブの作成に対応していません。\n"
			L"アーカイブはSSPの開発者機能などで作成してください。\n"
			,L"nar/zipアーカイブについて", MB_OK);
		break;

	case	IDC_FTP_HELP:
		::MessageBox(m_hDlg,
			L"ホスト名、ID、パスワードは、普段使っているFTPクライアントと同じ設定にしてください。\n"
			L"\n"
			L"「ゴーストのルートディレクトリ」には、FTPサーバに接続した時のディレクトリから、ゴーストの更新データを置くディレクトリへの相対パスを指定してください。\n"
			L"\n"
			L"サーバによっては時差を指定するといいかも。たぶん。\n"
			L"ファイアウォール内にいる場合などは「PASVモード」にチェックをいれるといいかも。たぶん。\n"
			,L"FTPアップロードについて", MB_OK);
		break;

	case	IDC_DOG:
		conf[L"morelog"] = ( IDYES==::MessageBox(m_hDlg, L"経過を細かく表示しますか？", L"詳しい設定", 
			MB_YESNO|((conf[L"morelog"]!=L"0")?0:MB_DEFBUTTON2)) ) ? L"1" : L"0";
		conf[L"yeardiff"] = ( IDYES==::MessageBox(m_hDlg, L"リモート日時が１年遅れてたりしますか？", L"詳しい設定", 
			MB_YESNO|((conf[L"yeardiff"]!=L"0")?0:MB_DEFBUTTON2)) ) ? L"1" : L"0";
		conf[L"dog"] = ( IDYES==::MessageBox(m_hDlg, L"犬は好きですか？", L"詳しい設定", 
			MB_YESNO|((conf[L"dog"]!=L"0")?0:MB_DEFBUTTON2)) ) ? L"1" : L"0";
		break;


	case	IDC_TARGET_HELP:
		::MessageBox(m_hDlg,
			L"ゴーストのルートフォルダからの相対パスを、改行で区切って指定してください。\n"
			L"ワイルドカード（*.txt、*.*など）も使えます。\n"
			L"\n"
			L"上段で指定したファイルだけがアップロードされます。\n"
			L"上段で指定したうちに含まれていても、下段で指定された場合は除外されます。\n"
			L"\n"
			L"なお、updates2.dauはここでは指定しないでください。\n"
			L"sodate.exeが自動的に処理します。\n"
			,L"対象ファイルについて", MB_OK);
		break;

	case IDC_ABOUT:
		if ( IDYES==::MessageBox(m_hDlg,
			L"sodate\n"
			L"\n"
			L"作者\t櫛ヶ浜やぎ\n"
			L"連絡先\thttp://www.geocities.jp/poskoma/\n"
			L"\n"
			L"\n"
			L"ご意見等あればお寄せください。\n"
			L"今、上記ページを開きますか？\n"
			,L"about", MB_YESNO|MB_DEFBUTTON2 )) {

			::ShellExecute(NULL, NULL, L"http://www.geocities.jp/poskoma/", NULL, NULL, SW_SHOW);
		}
		break;

	case IDOK:
		if ( Save() )
			End(0);
		break;

	case IDCANCEL:
		End(0);
		break;

	default:
		return	FALSE;
	}
	return	TRUE;
}





int WINAPI
WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInst,
	LPSTR lpszCmdParam,
	int iShowCmd) 
{
	// 作業フォルダ名を作成
	wchar_t	theBaseFolder[MAX_PATH];	// 作業フォルダ
	::GetModuleFileName( NULL, theBaseFolder, MAX_PATH );
	wchar_t*	p = FindFinalChar(theBaseFolder, L'\\');
	*(++p)=L'\0';	// ファイル名を削除
	base_folder = theBaseFolder;

	UserDialog*	dlg = new UserDialog;
	int	ret = dlg->Run(hInstance, IDD_DIALOG, NULL);
	delete dlg;
	dlg=NULL;
	return	ret;
}


