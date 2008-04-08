	/*

		files.clear();	// 元のリストをクリア


		list<string>	dir_names;
		for (map<string, set<string> >::iterator j=files_on_dir.begin(); j!=files_on_dir.end() ; ++j)
			dir_names.push_back(j->first);
		dir_names.sort();
		dir_names.erase(base_folder);	// ルートは



		for (j=files_on_dir.begin(); j!=files_on_dir.end() ; ++j) {

			set<string>&	a_folder_files = j->second();
			list<string>	temp;

			for (set<string>::iterator k=a_folder_files.begin() ; k!=a_folder_files.end() ; ++k) {


			}

				// 再格納
		}
		*/



// 指定フォルダ以下の全ファイルを取得する
void	do_folder(string iBaseFolder, string iSubFolders, list<string>& oList) {

	vector<WIN32_FIND_DATA>	datas;

	FileLister	lister;
	if ( !lister.Open((iBaseFolder+"\\"+iSubFolders+"*.*").c_str()) )
		return;
	while ( lister.Next() )
		datas.push_back( *lister.Data() );
	lister.Close();

	for (vector<WIN32_FIND_DATA>::iterator i=datas.begin(); i!=datas.end() ; ++i ) {
		const char* theFileName = i->cFileName;
		if ( strcmp(theFileName, ".")==0 )
			continue;
		if ( strcmp(theFileName, "..")==0 )
			continue;
		if ( deny_filename_set.find( iSubFolders+theFileName ) != deny_filename_set.end() )
			continue;

		if ( i->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY )
			do_folder(iBaseFolder, iSubFolders + theFileName + "\\", oList);
		else
			oList.push_back(iSubFolders+theFileName);
	}
}



	// 除外ファイルsetの作成
	strvec	deny;
	if ( strvec_from_file(deny, base_folder+"\\network_updater_deny.txt") ) {
		for (strvec::iterator i=deny.begin(); i!=deny.end() ; ++i ) {
			replace(*i, "/", "\\");
			deny_filename_set.insert(*i);
		}
	}


	// 旧いのは削除しとく（あとで足す。filesでの重複を避ける目的）
	::DeleteFile((base_folder+"\\updates2.dau").c_str());	

	// 除外しつつファイル一覧を作成
	do_folder(base_folder, "", files);

	// 足す
	files.push_front("updates2.dau");



