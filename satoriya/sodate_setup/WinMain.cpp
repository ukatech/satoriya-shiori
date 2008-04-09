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
#include	"resource.h"

string	base_folder;	// 作業フォルダ名
strmap	conf;

static const char* conf_filename = "sodate.dat";
static const char byte_value_1[2] = {1,0};
static const char byte_value_2[2] = {2,0};
static const byte xor_magic = 186;


class UserDialog : public Dialog {
	bool	Save();
public:
	virtual	BOOL	OnInitDialog( HWND hwndFocus, LONG lInitParam );
	virtual	BOOL	OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCrl );
};



BOOL	UserDialog::OnInitDialog( HWND hwndFocus, LONG lInitParam ) {
	if ( strmap_from_file(conf, base_folder+"\\"+conf_filename, byte_value_1) )
	{
		// 設定ファイルの読み込みに成功

		// \2区切りだったものを\r\nに戻す
		replace( conf["allow_files"], byte_value_2, "\r\n");
		replace( conf["deny_files"], byte_value_2, "\r\n");

		// パスワードをデコード
		string	str = decode(conf["password"]);
		int	len = str.size()/2;
		byte*	buf = new byte[len+1];
		buf[len]='\0';
		string_to_binary(str, buf);
		xor_filter(buf, len, xor_magic);
		conf["password"] = decode( (char*)buf );
	}
	else
	{
		// 設定ファイルの読み込みに失敗、confにデフォルト値を設定する

		conf["is_upload"]="1";
		conf["host"]="127.0.0.1";
		conf["id"]="test";
		conf["password"]="1234";
		conf["ghostroot"]="homepage/myghost";
		conf["timediff"]="0";
		conf["yeardiff"]="0";
		conf["is_create_archive"]="1";
		//conf["archive_filename"]="myGhost%year%month%day_%hour%minute.nar";
		conf["archive_filename"]="myghost%year%month%day.nar";
		conf["archive_local_folder"]=base_folder;
		conf["allow_files"]=
			"delete.txt\r\n"
			"install.txt\r\n"
			"readme.txt\r\n"
			"thumbnail.*\r\n"
			"ghost/master/*.dll\r\n"
			"ghost/master/saori/*.dll\r\n"
			"ghost/master/*.txt\r\n"
			"ghost/master/*.ico\r\n"
			"ghost/master/*.png\r\n"
			"ghost/master/*.sat\r\n"
			"ghost/master/*.dic\r\n"
			"ghost/master/*.ayc\r\n"
			"ghost/master/openkeeps/*.kis\r\n"
			"ghost/master/openkeeps/plugin/*.kis\r\n"
			"ghost/master/template/*.kis\r\n"
			"ghost/master/template/another/*.kis\r\n"
			"ghost/master/*.kis\r\n"
			"ghost/master/*.kaw\r\n"
			"shell/master/*.txt\r\n"
			"shell/master/*.dll\r\n"
			"shell/master/*.ico\r\n"
			"shell/master/*.png\r\n"
			"";
		conf["deny_files"]=
			"ghost/master/satori_savedata.txt\r\n"
			"ghost/master/satori_savebackup.txt\r\n"
			"ghost/master/satorite.txt\r\n"
			"ghost/master/nsconf.txt\r\n"
			"ghost/master/Dict-KEEPSs.txt\r\n"
			"ghost/master/Dict-Learned.txt\r\n"
			"ghost/master/misaka_vars.txt\r\n"
			"ghost/master/aya_variable.cfg\r\n"
			"ghost/master/yaya_variable.cfg\r\n"
			"ghost/master/ssp_shiori_log.txt\r\n"
			"ghost/master/れしば.txt\r\n"
			"";
		conf["morelog"] = "0";
		conf["yeardiff"] = "0";
		conf["dog"] = "1";
		conf["is_passive"] = "1";
	}

	// confの内容をダイアログに設定

	Check(IDC_IS_UPLOAD, conf["is_upload"]!="0");
	SetText(IDC_HOST, conf["host"]);
	SetText(IDC_ID, conf["id"]);
	SetText(IDC_PASSWORD, conf["password"]);
	SetText(IDC_GHOSTROOT, conf["ghostroot"]);
	SetText(IDC_TIMEDIFF, conf["timediff"]);
	Check(IDC_IS_PASSIVE, conf["is_passive"]=="1");
	Check(IDC_IS_CREATE_ARCHIVE, conf["is_create_archive"]!="0");
	SetText(IDC_ARCHIVE_FILENAME, conf["archive_filename"]);
	SetText(IDC_ARCHIVE_LOCAL_FOLDER, conf["archive_local_folder"]);
	SetText(IDC_ALLOW_FILES, conf["allow_files"]);
	SetText(IDC_DENY_FILES, conf["deny_files"]);


	BOOL	flag = isChecked(IDC_IS_UPLOAD) ? TRUE : FALSE;
	Enable(IDC_HOST, flag);
	Enable(IDC_ID, flag);
	Enable(IDC_PASSWORD, flag);
	Enable(IDC_GHOSTROOT, flag);
	Enable(IDC_TIMEDIFF, flag);
	Enable(IDC_IS_PASSIVE, flag);
	flag = isChecked(IDC_IS_CREATE_ARCHIVE) ? TRUE : FALSE;
	Enable(IDC_ARCHIVE_FILENAME, flag);
	Enable(IDC_ARCHIVE_LOCAL_FOLDER, flag);

	return	FALSE;
}

bool	UserDialog::Save()
{
	conf["is_upload"] = isChecked(IDC_IS_UPLOAD) ? "1" : "0";
	GetText(IDC_HOST, conf["host"]);
	GetText(IDC_ID, conf["id"]);
	GetText(IDC_PASSWORD, conf["password"]);
	GetText(IDC_GHOSTROOT, conf["ghostroot"]);
	GetText(IDC_TIMEDIFF, conf["timediff"]);
	conf["is_passive"] = isChecked(IDC_IS_PASSIVE) ? "1" : "0";
	conf["is_create_archive"] = isChecked(IDC_IS_CREATE_ARCHIVE) ? "1" : "0";
	GetText(IDC_ARCHIVE_FILENAME, conf["archive_filename"]);
	GetText(IDC_ARCHIVE_LOCAL_FOLDER, conf["archive_local_folder"]);
	GetText(IDC_ALLOW_FILES, conf["allow_files"]);
	GetText(IDC_DENY_FILES, conf["deny_files"]);

	// 保存用に\r\nを\2区切りに変換
	replace( conf["allow_files"], "\r\n", byte_value_2);
	replace( conf["deny_files"], "\r\n", byte_value_2);

	string	str = encode(conf["password"]);
	int	len = str.size();
	byte*	buf = new byte[len];
	memcpy(buf, str.c_str(), len);
	xor_filter(buf, len, xor_magic);
	conf["password"] = encode( binary_to_string(buf, len) );
	delete [] buf;

	// ファイルに保存
	if ( !strmap_to_file(conf, base_folder+"\\"+conf_filename, byte_value_1) ) 
	{
		::MessageBox(m_hDlg,
			(base_folder+"\\"+conf_filename+"\n\n設定ファイルが保存できませんでした。").c_str()
			,"error", MB_OK);
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

	case	IDC_IS_CREATE_ARCHIVE:
		{
			BOOL	flag = isChecked(IDC_IS_CREATE_ARCHIVE) ? TRUE : FALSE;
			Enable(IDC_ARCHIVE_FILENAME, flag);
			Enable(IDC_ARCHIVE_LOCAL_FOLDER, flag);
		}
		break;

	case	IDC_ARCHIVE_HELP:
		::MessageBox(m_hDlg,
			"ファイル名の設定には、以下のようなタグが使えます（現在の日時に置き換えられます）。\n"
			"\n"
			"%year\t年\n"
			"%year4\t年（４桁）\n"
			"%month\t月\n"
			"%day\t日\n"
			"%hour\t時\n"
			"%minute\t分\n"
			"%second\t秒\n"
			"\n"
			"拡張子はnarまたはzipを指定してください（この２つの形式は、名前が違うだけで中身は同じです）。\n"
			,"nar/zipアーカイブについて", MB_OK);
		break;

	case	IDC_FTP_HELP:
		::MessageBox(m_hDlg,
			"ホスト名、ID、パスワードは、普段使っているFTPクライアントと同じ設定にしてください。\n"
			"\n"
			"「ゴーストのルートディレクトリ」には、FTPサーバに接続した時のディレクトリから、ゴーストの更新データを置くディレクトリへの相対パスを指定してください。\n"
			"\n"
			"サーバによっては時差を指定するといいかも。たぶん。\n"
			"ファイアウォール内にいる場合などは「PASVモード」にチェックをいれるといいかも。たぶん。\n"
			,"FTPアップロードについて", MB_OK);
		break;

	case	IDC_DOG:
		conf["morelog"] = ( IDYES==::MessageBox(m_hDlg, "経過を細かく表示しますか？", "詳しい設定", 
			MB_YESNO|((conf["morelog"]!="0")?0:MB_DEFBUTTON2)) ) ? "1" : "0";
		conf["yeardiff"] = ( IDYES==::MessageBox(m_hDlg, "リモート日時が１年遅れてたりしますか？", "詳しい設定", 
			MB_YESNO|((conf["yeardiff"]!="0")?0:MB_DEFBUTTON2)) ) ? "1" : "0";
		conf["dog"] = ( IDYES==::MessageBox(m_hDlg, "犬は好きですか？", "詳しい設定", 
			MB_YESNO|((conf["dog"]!="0")?0:MB_DEFBUTTON2)) ) ? "1" : "0";
		break;


	case	IDC_TARGET_HELP:
		::MessageBox(m_hDlg,
			"ゴーストのルートフォルダからの相対パスを、改行で区切って指定してください。\n"
			"ワイルドカード（*.txt、*.*など）も使えます。\n"
			"\n"
			"上段で指定したファイルだけがアップロードされます。\n"
			"上段で指定したうちに含まれていても、下段で指定された場合は除外されます。\n"
			"\n"
			"なお、updates2.dauはここでは指定しないでください。\n"
			"sodate.exeが自動的に処理します。\n"
			,"対象ファイルについて", MB_OK);
		break;

	case IDC_ABOUT:
		if ( IDYES==::MessageBox(m_hDlg,
			"sodate\n"
			"\n"
			"作者\t櫛ヶ浜やぎ\n"
			"連絡先\thttp://www.geocities.jp/poskoma/\n"
			"\n"
			"\n"
			"ご意見等あればお寄せください。\n"
			"今、上記ページを開きますか？\n"
			,"about", MB_YESNO|MB_DEFBUTTON2 )) {

			::ShellExecute(NULL, NULL, "http://www.geocities.jp/poskoma/", NULL, NULL, SW_SHOW);
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
	char	theBaseFolder[MAX_PATH];	// 作業フォルダ
	::GetModuleFileName( NULL, theBaseFolder, MAX_PATH );
	char*	p = FindFinalChar(theBaseFolder, '\\');
	*(++p)='\0';	// ファイル名を削除
	base_folder = theBaseFolder;

	UserDialog*	dlg = new UserDialog;
	int	ret = dlg->Run(hInstance, IDD_DIALOG, NULL);
	delete dlg;
	dlg=NULL;
	return	ret;
}


