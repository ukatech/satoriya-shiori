
extern "C" int zipmain(int,char**);


template<class T>
bool	makeZip(const string& iZipFileName, const T iFiles) {
	int	argc = iFiles.size()+3;
	char**	argv = new char*[argc];

	argv[0] = new char[MAX_PATH];
	::GetModuleFileName(NULL, argv[0], MAX_PATH);
	argv[1] = new char[3];
	strcpy(argv[1], "-r");
	argv[2] = new char[iZipFileName.size()+1];
	strcpy(argv[2], iZipFileName.c_str());
	T::const_iterator it=iFiles.begin();
	for (int i=3 ; i<argc ; ++i, ++it) {
		argv[i] = new char[ it->size()+1 ]; 
		strcpy(argv[i], it->c_str());
	}

	int	ret = zipmain(argc, argv);
	
	for (int i=argc-1 ; i>=0 ; --i)
		delete [] argv[i];
	delete [] argv;
	
	return ( ret==0 );
}


