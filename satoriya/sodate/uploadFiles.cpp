#include	"common.h"

#include	<iomanip>



// file-scope
static HINTERNET hInternet;


static bool	upload_a_dir(const string& base_folder, const string& dir_name, set<string>& dir_files) {

	string	local_folder = base_folder+"\\"+dir_name;
	string	remote_folder = conf["ghostroot"];
	replace(remote_folder, "/", "\\");
	if ( remote_folder[ remote_folder.size()-1 ] != '\\' )
		remote_folder += "\\";
	remote_folder += dir_name;

	// ftp接続
	FTP	ftp(hInternet);
	if ( !ftp.open(conf["host"], conf["id"], conf["password"], conf["is_passive"]!="0") ) {
		error("FTPサーバへの接続が確立できませんでした。設定を見直してください。");
		return	false;
	}

	// フォルダ名分割
	strvec	remote_folders;
	split(remote_folder, "\\", remote_folders);

	// 目標のフォルダへ降りつつフォルダ作成
	if(1){	
		for ( strvec::iterator i=remote_folders.begin() ; i!=remote_folders.end() ; ++i ) {
			if ( !ftp.cd(*i) ) {
				if ( !ftp.mkdir(*i) ) {
					error("FTPサーバ内でフォルダ作成に失敗しました。");
					return	false;
				}
				else if ( !ftp.cd(*i) ) {
					error("FTPサーバ内でフォルダ移動に失敗？？");
					return	false;
				}
			}
		}
	}

	
	// リモートのファイル情報を取得
	string pwdtxt;
	ftp.pwd(pwdtxt);
	sender << pwdtxt << "内のファイル情報を取得中...";
	map<string, WIN32_FIND_DATA>	remoteFilesData;
	ftp.ls(remoteFilesData);
	sender << "完了" << endl << local_folder << "内のファイルとの更新時間照合を開始。" << endl;

	// アップロード
	for (set<string>::iterator i=dir_files.begin(); i!=dir_files.end() ; ++i) {

		string	local_file_path = local_folder+"\\"+*i;
		bool	isUpload;

		if ( remoteFilesData.find(*i)==remoteFilesData.end() ) {
			isUpload = true;	// サーバ上に存在しない
		}
		else {
			SYSTEMTIME	stLocal, stRemote;
			GetLastWriteTime(local_file_path.c_str(), stLocal);
			WIN32_FIND_DATA	fd = remoteFilesData[*i];
			FILETIME	ft=fd.ftLastWriteTime, ftLocal;
			//FileTimeToLocalFileTime(&ft, &ftLocal);
			ftLocal = ft;
			FileTimeToSystemTime( &ftLocal, &stRemote);

			if ( conf.find("yeardiff")!=conf.end() )
				stRemote.wYear += stoi( conf["yeardiff"] );
			if ( conf.find("timediff")!=conf.end() ) {
				int	hour = stRemote.wHour;
				hour += stoi( conf["timediff"] );
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

			if ( conf["morelog"]!="0" ) {
				
				sender << "Local  " 
					<< stLocal.wYear << "/"
					 << setfill('0') << setw(2)<< stLocal.wMonth << "/"
					  << setfill('0') << setw(2)<< stLocal.wDay << " "
					   << setfill('0') << setw(2)<< stLocal.wHour << ":"
						<< setfill('0') << setw(2)<< stLocal.wMinute << ":"
						 << setfill('0') << setw(2)<< stLocal.wSecond << endl;
				sender << "Remote " 
					<< stRemote.wYear << "/"
					 << setfill('0') << setw(2)<< stRemote.wMonth << "/"
					  << setfill('0') << setw(2)<< stRemote.wDay << " "
					   << setfill('0') << setw(2)<< stRemote.wHour << ":"
						<< setfill('0') << setw(2)<< stRemote.wMinute << ":"
						 << setfill('0') << setw(2)<< stRemote.wSecond << endl;
			}


			isUpload = CompareTime(stLocal, stRemote)>0;	// Localが新しければ。
		}

		if ( isUpload ) {
			sender << "　" << *i << "をアップロード中...";
			if ( !ftp.put(local_file_path, *i) ) {
				sender << "失敗" << endl;
				error(local_file_path+"のアップロードに失敗");
				return	false;
			}
			sender << "完了" << endl;
		}
		else {
			sender << "　" << *i << "は更新されていません。" << endl;
		}
	}

	/*sender << "ftp-dir: " << ftp.getCurrentDirectry() << endl;
	sender << local_folder << dir_name << endl;
	sender << remote_folder << dir_name << endl;*/

	return	true;
}


bool	uploadFiles(const string& base_folder, list<string>& files) {

	/* WININETの初期化 */
	hInternet = ::InternetOpen(
		"network_updater",
		INTERNET_OPEN_TYPE_PRECONFIG,//INTERNET_OPEN_TYPE_DIRECT,
		NULL,
		NULL,
		0);
	if ( hInternet==NULL ) {
		error("インターネットへの接続できませんでした");
		return	false;
	}


	// フォルダ単位でアップロードを実行
	for (map<string, set<string> >::iterator j=files_on_dir.begin(); j!=files_on_dir.end() ; ++j) {
		if ( !upload_a_dir(base_folder, j->first, j->second) ) {
			::InternetCloseHandle(hInternet);
			return	false;
		}
	}

	
	::InternetCloseHandle(hInternet);
	return	true;
}


