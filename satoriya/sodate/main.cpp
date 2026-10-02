#include	"common.h"
#include	"password.h"



int main( int argc, char *argv[ ], char *envp[ ] )
{
	// 実行ファイルのあるフォルダ名を取得
	wchar_t	szPath[MAX_PATH]=L"";
	::GetModuleFileName(NULL, szPath, MAX_PATH);
	wchar_t*	p = FindFinalChar(szPath, L'\\');
	if ( p == NULL )
		return	1;
	*p=L'\0';
	base_folder = szPath;
	GetSender().sender() << L"[sodate]" << endl;
	GetSender().sender() << L"ディスク上の対象フォルダは " << base_folder << L"です。" << endl;

	::SetCurrentDirectory(base_folder.c_str());

	// 設置場所の確認
	if ( !isExistFolder( (base_folder+L"\\ghost\\master").c_str() ) ) {
		error(L"ghost/masterが見つかりません。設置位置を確認してください。");
		return	false;
	}
	if ( !isExistFolder( (base_folder+L"\\shell\\master").c_str() ) ) {
		error(L"shell/masterが見つかりません。設置位置を確認してください。");
		return	false;
	}

	// 設定ファイルの読み込み
	GetSender().sender() << L"設定ファイル sodate.dat を読み込みます。" << endl;
	if ( !strmap_from_file(conf, base_folder+L"\\sodate.dat", byte_value_1) ) {
		error(L"先に sodate_setup.exe を実行してください。");
		return	false;
	}

	// 暗号保存されたパスワードをデコード
	conf[L"password"] = decode_password(conf[L"password"]);

	// 古いupdates2.dauを削除
	::DeleteFile( (base_folder+L"\\updates2.dau").c_str() );
	::DeleteFile( (base_folder+L"\\ghost\\master\\updates2.dau").c_str() );

	// 対象ファイル選定
	console(L"対象ファイルを選定中\n");
	strvec	allow_files_vec, deny_files_vec;
	stringset	deny_files_set;
	split(conf[L"allow_files"], byte_value_2, allow_files_vec);
	split(conf[L"deny_files"], byte_value_2, deny_files_vec);

	// 除外ファイルが（存在すれば）set化
	for (strvec::iterator i=deny_files_vec.begin() ; i!=deny_files_vec.end() ; ++i) {
		wstring	full_path = base_folder+L"\\"+*i;
		replace(full_path, L"/", L"\\");
		wstring	folder_name = get_folder_name(full_path);

		WIN32_FIND_DATA	fd;
		HANDLE	h = ::FindFirstFile(full_path.c_str(), &fd);
		if ( h == INVALID_HANDLE_VALUE )
			continue;
		
		do {
			if ( fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY )
				continue;
			deny_files_set.insert(folder_name+L"\\"+fd.cFileName);
		} while ( ::FindNextFile(h, &fd) );
		::FindClose(h);
	}

	// 対象ファイルを、除外対象に一致しないことを確認しつつfiles化
	for (strvec::iterator i=allow_files_vec.begin() ; i!=allow_files_vec.end() ; ++i) {

		wstring	full_path = base_folder+L"\\"+*i;
		replace(full_path, L"/", L"\\");
		wstring	folder_name = get_folder_name(full_path);

		WIN32_FIND_DATA	fd;
		HANDLE	h = ::FindFirstFile(full_path.c_str(), &fd);
		if ( h == INVALID_HANDLE_VALUE ) {
			GetSender().sender() << *i << L"に該当するファイルがありません。" << endl;
			continue;
		}
		
		int	n=0;
		do {
			if ( fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY )
				continue;
			wstring	filename=folder_name+L"\\"+fd.cFileName;
			if ( deny_files_set.find(filename) == deny_files_set.end() ) {
				files.push_back(filename);
				++n;
			}
		} while ( ::FindNextFile(h, &fd) );
		::FindClose(h);

		if ( n==0 ) {
			GetSender().sender() << *i << L"に該当するファイルがありません。" << endl;
			continue;
		}
	}

	if ( files.size()==0 ) {
		error(L"対象となるファイルが存在しません。");
		return	1;
	}
	else {

		// 本家のupdates2.dauに構造を合わせるためのリストのソート

		list<wstring>	OLD = files;	// 複製を作る
		files.clear();
		list<wstring>&	NEW = files;	// 空っぽのほう

		// まずは単なるアルファベットソート
		OLD.sort();		

		// まずはshellフォルダの中身を移行
		list<wstring>::iterator	i;;
		for (i=OLD.begin(); i!=OLD.end() ;) {
			if ( compare_head(get_folder_name(*i), base_folder+L"\\shell\\") ) {
				NEW.push_back(*i);
				i=OLD.erase(i);
			} else
				++i;
		}

		// 続いてghostフォルダの中身を移行
		for (i=OLD.begin(); i!=OLD.end() ;) {
			if ( compare_head(get_folder_name(*i), base_folder+L"\\ghost\\") ) {
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



	GetSender().sender() << L"updates2.dauを作成中" << endl;
	bool	makeUpdates2(const wstring& base_folder, const list<wstring>& files);
	if ( !makeUpdates2(base_folder, files) ) {
		error(L"updates2.dauが作成できませんでした。");
		return	1;
	}
	GetSender().sender() << L"updates2.dauを作成完了" << endl;
	// 同じものをコピー
	::CopyFile( (base_folder+L"\\updates2.dau").c_str(), (base_folder+L"\\ghost\\master\\updates2.dau").c_str(), FALSE );


	// zip/narアーカイブの作成機能は、使っていた Info-ZIP のソースがリポジトリに無いため削除した。
	if ( conf[L"is_create_archive"]!=L"0") {
		GetSender().sender() << L"このバージョンのsodateはzip/narアーカイブの作成に対応していません。" << endl;
	}

	files.push_back(base_folder+L"\\updates2.dau");
	// フォルダ別に分離格納 as global
	//		map<wstring, set<wstring> >	files_on_dir;	// dirname : filenames
	{
		for (list<wstring>::iterator i=files.begin(); i!=files.end() ; ++i) {
			wstring	rel_path = (i->c_str()+base_folder.size()+1);
			files_on_dir[ get_folder_name(rel_path) ].insert( get_file_name(rel_path) );
		}
	}

	bool	uploadFiles(const wstring&, list<wstring>&);
	if ( conf[L"is_upload"]!=L"0")
		uploadFiles(base_folder, files);

	// ルートのupdates2.dauはアップロードした後に削除。
	::DeleteFile( (base_folder+L"\\updates2.dau").c_str() );

	return	0;
}


// filesには そのままオープン可能なフルパスが入っていること。
// updates2.dauにはbase_folderを差し引いた相対パスが格納される。
// 文字コードは旧版と同じくShift_JIS。
bool	makeUpdates2(const wstring& base_folder, const list<wstring>& files) {
	std::string	out;

	for (list<wstring>::const_iterator i=files.begin(); i!=files.end() ; ++i ) {
		
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

		wstring	filename(*i);
		replace(filename, L"\\", L"/");
		const wstring	rel_path = filename.c_str()+base_folder.size()+1;
		const char	sep[2] = { 1, 0 };
		out += WtoSJIS(rel_path);
		out += sep;
		out += md5;
		out += sep;
		out += "\r\n";
		if ( conf[L"morelog"]!=L"0" )
			GetSender().sender() << rel_path << L" " << ascii_to_w(md5) << endl;
	}

	return	bytes_to_file(out, base_folder+L"\\updates2.dau");
}




