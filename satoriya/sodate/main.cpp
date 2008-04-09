#include	"common.h"
#include	"makeZip.h"



int main( int argc, char *argv[ ], char *envp[ ] )
{
	// 実行ファイルのあるフォルダ名を取得
	TCHAR	szPath[MAX_PATH]="";
	::GetModuleFileName(NULL, szPath, MAX_PATH);
	char*	p = FindFinalChar(szPath, '\\');
	if ( *p == NULL )
		return	1;
	*p='\0';
	base_folder = szPath;
	sender << "[sodate]" << endl;
	sender << "ディスク上の対象フォルダは " << base_folder << "です。" << endl;

	::SetCurrentDirectory(base_folder.c_str());

	// 設置場所の確認
	if ( !isExistFolder( (base_folder+"\\ghost\\master").c_str() ) ) {
		error("ghost/masterが見つかりません。設置位置を確認してください。");
		return	false;
	}
	if ( !isExistFolder( (base_folder+"\\shell\\master").c_str() ) ) {
		error("shell/masterが見つかりません。設置位置を確認してください。");
		return	false;
	}

	// 設定ファイルの読み込み
	sender << "設定ファイル sodate.dat を読み込みます。" << endl;
	if ( !strmap_from_file(conf, base_folder+"\\sodate.dat", byte_value_1) ) {
		error("先に sodate_setup.exe を実行してください。");
		return	false;
	}

	// 暗号保存されたパスワードをデコード
	string	str = decode(conf["password"]);
	int	len = str.size()/2;
	byte*	buf = new byte[len+1];
	buf[len]='\0';
	string_to_binary(str, buf);
	xor_filter(buf, len, 186);
	conf["password"] = decode( (char*)buf );

	// 古いupdates2.dauを削除
	::DeleteFile( (base_folder+"\\updates2.dau").c_str() );
	::DeleteFile( (base_folder+"\\ghost\\master\\updates2.dau").c_str() );

	// 対象ファイル選定
	cout << "対象ファイルを選定中" << endl;
	strvec	allow_files_vec, deny_files_vec;
	stringset	deny_files_set;
	split(conf["allow_files"], byte_value_2, allow_files_vec);
	split(conf["deny_files"], byte_value_2, deny_files_vec);

	// 除外ファイルが（存在すれば）set化
	for (strvec::iterator i=deny_files_vec.begin() ; i!=deny_files_vec.end() ; ++i) {
		string	full_path = base_folder+"\\"+*i;
		replace(full_path, "/", "\\");
		string	folder_name = get_folder_name(full_path);

		WIN32_FIND_DATA	fd;
		HANDLE	h = ::FindFirstFile(full_path.c_str(), &fd);
		if ( h == INVALID_HANDLE_VALUE )
			continue;
		
		do {
			if ( fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY )
				continue;
			deny_files_set.insert(folder_name+"\\"+fd.cFileName);
		} while ( ::FindNextFile(h, &fd) );
		::FindClose(h);
	}

	// 対象ファイルを、除外対象に一致しないことを確認しつつfiles化
	for (strvec::iterator i=allow_files_vec.begin() ; i!=allow_files_vec.end() ; ++i) {

		string	full_path = base_folder+"\\"+*i;
		replace(full_path, "/", "\\");
		string	folder_name = get_folder_name(full_path);

		WIN32_FIND_DATA	fd;
		HANDLE	h = ::FindFirstFile(full_path.c_str(), &fd);
		if ( h == INVALID_HANDLE_VALUE ) {
			sender << *i << "に該当するファイルがありません。" << endl;
			continue;
		}
		
		int	n=0;
		do {
			if ( fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY )
				continue;
			string	filename=folder_name+"\\"+fd.cFileName;
			if ( deny_files_set.find(filename) == deny_files_set.end() ) {
				files.push_back(filename);
				++n;
			}
		} while ( ::FindNextFile(h, &fd) );
		::FindClose(h);

		if ( n==0 ) {
			sender << *i << "に該当するファイルがありません。" << endl;
			continue;
		}
	}

	if ( files.size()==0 ) {
		error("対象となるファイルが存在しません。");
		return	1;
	}
	else {

		// 本家のupdates2.dauに構造を合わせるためのリストのソート

		list<string>	OLD = files;	// 複製を作る
		files.clear();
		list<string>&	NEW = files;	// 空っぽのほう

		// まずは単なるアルファベットソート
		files.sort();		

		// まずはshellフォルダの中身を移行
		list<string>::iterator	i;;
		for (i=OLD.begin(); i!=OLD.end() ;) {
			if ( compare_head(get_folder_name(*i), base_folder+"\\shell\\") ) {
				NEW.push_back(*i);
				i=OLD.erase(i);
			} else
				++i;
		}

		// 続いてghostフォルダの中身を移行
		for (i=OLD.begin(); i!=OLD.end() ;) {
			if ( compare_head(get_folder_name(*i), base_folder+"\\ghost\\") ) {
				NEW.insert(NEW.begin(), *i);	// 反転しないように
				i=OLD.erase(i);
			} else
				++i;
		}

		// ghost, shell,ルート以外のフォルダの中身を移行（あるのか？）
		for (i=OLD.begin(); i!=OLD.end() ;) {
			if ( !compare_head(get_folder_name(*i), base_folder) ) {
				NEW.push_back(*i);
				i=OLD.erase(i);
			} else
				++i;
		}

		// 残り。ルートフォルダの中身を移行
		NEW.insert(NEW.end(), OLD.begin(), OLD.end());
		OLD.clear();
	}



	sender << "updates2.dauを作成中" << endl;
	bool	makeUpdates2(string base_folder, const list<string>& files);
	if ( !makeUpdates2(szPath, files) ) {
		error("updates2.dauが作成できませんでした。");
		return	1;
	}
	sender << "updates2.dauを作成完了" << endl;
	// 同じものをコピー
	::CopyFile( (base_folder+"\\updates2.dau").c_str(), (base_folder+"\\ghost\\master\\updates2.dau").c_str(), FALSE );


	if ( conf["is_create_archive"]!="0") {
		sender << "zip/narアーカイブを作成中" << endl;

		if ( !is_exist_file(base_folder+"\\install.txt") ) {
			error("zip/narアーカイブを作成するためには、ゴーストのルートフォルダにinstall.txtが必要です。");
			return	false;
		}

		SYSTEMTIME	st;
		char	buf[10];
		::GetLocalTime(&st);
		sprintf(buf, "%02d", st.wYear%100);
		replace(conf["archive_filename"], "%year", buf);
		sprintf(buf, "%04d", st.wYear);
		replace(conf["archive_filename"], "%year4", buf);
		sprintf(buf, "%02d", st.wMonth);
		replace(conf["archive_filename"], "%month", buf);
		sprintf(buf, "%02d", st.wDay);
		replace(conf["archive_filename"], "%day", buf);
		sprintf(buf, "%02d", st.wHour);
		replace(conf["archive_filename"], "%hour", buf);
		sprintf(buf, "%02d", st.wMinute);
		replace(conf["archive_filename"], "%minute", buf);
		sprintf(buf, "%02d", st.wSecond);
		replace(conf["archive_filename"], "%second", buf);

		if ( (conf["archive_local_folder"])[ conf["archive_local_folder"].size()-1 ] != '\\' )
			conf["archive_local_folder"] += "\\";

		string	archive_filename = conf["archive_local_folder"]+conf["archive_filename"];

		// 既にあるなら削除しておく
		::DeleteFile(archive_filename.c_str());

		/*
		string	cmd = "zip.exe";
		string	opt = "-r " + archive_filename;
		for (list<string>::const_iterator i=files.begin(); i!=files.end() ; ++i ) {
			string	filename(*i);
			opt += string(" ") + (filename.c_str()+base_folder.size()+1);
		}
		opt += " ghost\\master\\updates2.dau";
		if ( opt.find(" install.txt")==string::npos )
			opt += " install.txt";	// nar時には必須なので。
		opt += " > nul";
		*/
		//system((cmd+" "+opt).c_str());
		//::ShellExecute(NULL, NULL, (cmd+" "+opt).c_str(), NULL, NULL, SW_SHOW);
		//if ( !is_exist_file(archive_filename) ) {

		list<string>	zip_files;
		bool	exist_install_txt=false;
		for (list<string>::const_iterator i=files.begin(); i!=files.end() ; ++i ) {
			zip_files.push_back(i->c_str()+base_folder.size()+1);
			if ( zip_files.back()=="install.txt" )
				exist_install_txt = true;
		}
		zip_files.push_back("ghost\\master\\updates2.dau");
		if ( !exist_install_txt )
			zip_files.push_back("install.txt");

		if ( !makeZip(archive_filename, zip_files) ) {
			error("アーカイブファイル'"+archive_filename+"'が作成できませんでした。");
		}
		else {
			sender << "zip/narアーカイブ '" << archive_filename << "' を作成完了" << endl;
		}
	}

	files.push_back(base_folder+"\\updates2.dau");
	// フォルダ別に分離格納 as global
	//		map<string, set<string> >	files_on_dir;	// dirname : filenames
	{
		for (list<string>::iterator i=files.begin(); i!=files.end() ; ++i) {
			string	rel_path = (i->c_str()+base_folder.size()+1);
			files_on_dir[ get_folder_name(rel_path) ].insert( get_file_name(rel_path) );
		}
	}

	bool	uploadFiles(const string&, list<string>&);
	if ( conf["is_upload"]!="0")
		uploadFiles(szPath, files);

	// ルートのupdates2.dauはアップロードした後に削除。
	::DeleteFile( (base_folder+"\\updates2.dau").c_str() );

	return	0;
}


// filesには そのままオープン可能なフルパスが入っていること。
// updates2.dauにはbase_folderを差し引いた相対パスが格納される。
bool	makeUpdates2(string base_folder, const list<string>& files) {
	ofstream	o((base_folder+"\\updates2.dau").c_str());
	if ( !o.is_open() )
		return	false;

	for (list<string>::const_iterator i=files.begin(); i!=files.end() ; ++i ) {
		
		File	file;
		if ( !file.Open( i->c_str(), File::READ) )
			continue;
		DWORD	size;
		if ( !file.GetSize(&size) )
			continue;
		char*	p = new char[size];
		if ( p==NULL )
			continue;
		if ( !file.Read(p, size) ) {
			delete [] p;
			continue;
		}

		MyMD5	md5calc;
		char	md5[33];
		md5calc.MD5_String(p, size, md5);
		delete [] p;
		file.Close();

		string	filename(*i);
		replace(filename, "\\", "/");
		char	sep[2] = { 1, 0 };
		o << (filename.c_str()+base_folder.size()+1) << sep << md5 << sep << endl;
		if ( conf["morelog"]!="0" )
			sender << (filename.c_str()+base_folder.size()+1) << " " << md5 << endl;
	}
	o.close();
	
	return	true;
}




