#include	"common.h"

#include	<iomanip>



// file-scope
static HINTERNET hInternet;


static bool	upload_a_dir(const wstring& base_folder, const wstring& dir_name, set<wstring>& dir_files) {

	wstring	local_folder = base_folder+L"\\"+dir_name;
	wstring	remote_folder = conf[L"ghostroot"];
	replace(remote_folder, L"/", L"\\");
	if ( remote_folder.empty() || remote_folder[ remote_folder.size()-1 ] != L'\\' )
		remote_folder += L"\\";
	remote_folder += dir_name;

	// ftp接続
	FTP	ftp(hInternet);
	if ( !ftp.open(conf[L"host"], conf[L"id"], conf[L"password"], conf[L"is_passive"]!=L"0") ) {
		error(L"FTPサーバへの接続が確立できませんでした。設定を見直してください。");
		return	false;
	}

	// フォルダ名分割
	strvec	remote_folders;
	split(remote_folder, L"\\", remote_folders);

	// 目標のフォルダへ降りつつフォルダ作成
	if(1){	
		for ( strvec::iterator i=remote_folders.begin() ; i!=remote_folders.end() ; ++i ) {
			if ( !ftp.cd(*i) ) {
				if ( !ftp.mkdir(*i) ) {
					error(L"FTPサーバ内でフォルダ作成に失敗しました。");
					return	false;
				}
				else if ( !ftp.cd(*i) ) {
					error(L"FTPサーバ内でフォルダ移動に失敗？？");
					return	false;
				}
			}
		}
	}

	
	// リモートのファイル情報を取得
	wstring pwdtxt;
	ftp.pwd(pwdtxt);
	GetSender().sender() << pwdtxt << L"内のファイル情報を取得中...";
	map<wstring, WIN32_FIND_DATAW>	remoteFilesData;
	ftp.ls(remoteFilesData);
	GetSender().sender() << L"完了" << endl << local_folder << L"内のファイルとの更新時間照合を開始。" << endl;

	// アップロード
	for (set<wstring>::iterator i=dir_files.begin(); i!=dir_files.end() ; ++i) {

		wstring	local_file_path = local_folder+L"\\"+*i;
		bool	isUpload;

		if ( remoteFilesData.find(*i)==remoteFilesData.end() ) {
			isUpload = true;	// サーバ上に存在しない
		}
		else {
			SYSTEMTIME	stLocal, stRemote;
			GetLastWriteTime(local_file_path.c_str(), stLocal);
			WIN32_FIND_DATAW	fd = remoteFilesData[*i];
			FILETIME	ft=fd.ftLastWriteTime, ftLocal;
			//FileTimeToLocalFileTime(&ft, &ftLocal);
			ftLocal = ft;
			FileTimeToSystemTime( &ftLocal, &stRemote);

			if ( conf.find(L"yeardiff")!=conf.end() )
				stRemote.wYear += stoi_internal( conf[L"yeardiff"] );
			if ( conf.find(L"timediff")!=conf.end() ) {
				int	hour = stRemote.wHour;
				hour += stoi_internal( conf[L"timediff"] );
				if ( hour>=24 ) {
					stRemote.wDay++;
					hour -= 24;
				}
				else if ( hour<0 ) {
					stRemote.wDay--;
					hour += 24;
				}
				stRemote.wHour = hour;
			}

			if ( conf[L"morelog"]!=L"0" ) {
				
				GetSender().sender() << L"Local  " 
					<< stLocal.wYear << L"/"
					 << setfill(L'0') << setw(2)<< stLocal.wMonth << L"/"
					  << setfill(L'0') << setw(2)<< stLocal.wDay << L" "
					   << setfill(L'0') << setw(2)<< stLocal.wHour << L":"
						<< setfill(L'0') << setw(2)<< stLocal.wMinute << L":"
						 << setfill(L'0') << setw(2)<< stLocal.wSecond << endl;
				GetSender().sender() << L"Remote " 
					<< stRemote.wYear << L"/"
					 << setfill(L'0') << setw(2)<< stRemote.wMonth << L"/"
					  << setfill(L'0') << setw(2)<< stRemote.wDay << L" "
					   << setfill(L'0') << setw(2)<< stRemote.wHour << L":"
						<< setfill(L'0') << setw(2)<< stRemote.wMinute << L":"
						 << setfill(L'0') << setw(2)<< stRemote.wSecond << endl;
			}


			isUpload = CompareTime(stLocal, stRemote)>0;	// Localが新しければ。
		}

		if ( isUpload ) {
			GetSender().sender() << L"　" << *i << L"をアップロード中...";
			if ( !ftp.put(local_file_path, *i) ) {
				GetSender().sender() << L"失敗" << endl;
				error(local_file_path+L"のアップロードに失敗");
				return	false;
			}
			GetSender().sender() << L"完了" << endl;
		}
		else {
			GetSender().sender() << L"　" << *i << L"は更新されていません。" << endl;
		}
	}

	/*GetSender().sender() << "ftp-dir: " << ftp.getCurrentDirectry() << endl;
	GetSender().sender() << local_folder << dir_name << endl;
	GetSender().sender() << remote_folder << dir_name << endl;*/

	return	true;
}


bool	uploadFiles(const wstring& base_folder, list<wstring>& files) {

	/* WININETの初期化 */
	hInternet = ::InternetOpen(
		L"network_updater",
		INTERNET_OPEN_TYPE_PRECONFIG,//INTERNET_OPEN_TYPE_DIRECT,
		NULL,
		NULL,
		0);
	if ( hInternet==NULL ) {
		error(L"インターネットへの接続できませんでした");
		return	false;
	}


	// フォルダ単位でアップロードを実行
	for (map<wstring, set<wstring> >::iterator j=files_on_dir.begin(); j!=files_on_dir.end() ; ++j) {
		if ( !upload_a_dir(base_folder, j->first, j->second) ) {
			::InternetCloseHandle(hInternet);
			return	false;
		}
	}

	
	::InternetCloseHandle(hInternet);
	return	true;
}


