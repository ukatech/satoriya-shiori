#include	"ftp.h"

FTP::FTP(HINTERNET iInternet) : mInternet(iInternet), mFtpSession(NULL) {}

FTP::~FTP() { close(); }

bool	FTP::open(const string& host, const string& id, const string& password, bool is_passive)
{
	close();

	mFtpSession = ::InternetConnect(
		mInternet,
		host.c_str(),
		INTERNET_DEFAULT_FTP_PORT,
		id.c_str(),
		password.c_str(),
		INTERNET_SERVICE_FTP,
		is_passive ? INTERNET_FLAG_PASSIVE : 0,
		0);

	return	(mFtpSession!=NULL);
}

void	FTP::close()
{
	if ( mFtpSession != NULL )
	{
		::InternetCloseHandle(mFtpSession);
		mFtpSession = NULL;
	}
}


bool	FTP::cd(const string& subdir)
{
	assert(mFtpSession != NULL);
	return	::FtpSetCurrentDirectory(mFtpSession, subdir.c_str()) != FALSE;
}

bool	FTP::pwd(string& o_dir)
{
	//assert(mFtpSession != NULL);
	char	curdir[2048]="";
	DWORD	size=2048;
	if ( FALSE == ::FtpGetCurrentDirectory(mFtpSession, curdir, &size) )
	{
		return false;
	}
	o_dir = curdir;
	return	true;
}

bool	FTP::mkdir(const string& subdir)
{
	assert(mFtpSession != NULL);
	return	::FtpCreateDirectory(mFtpSession, subdir.c_str()) != FALSE;
}

bool	FTP::put(const string& local_filepath, const string& upload_filename, bool isBinary)
{
	assert(mFtpSession != NULL);
	return	::FtpPutFile(
		mFtpSession,
		local_filepath.c_str(),
		upload_filename.c_str(),
		isBinary ? INTERNET_FLAG_TRANSFER_BINARY : FTP_TRANSFER_TYPE_ASCII,
		0) != FALSE;
}

void	FTP::ls(map<string, WIN32_FIND_DATA>& oFileList)
{
	assert(mFtpSession != NULL);

	WIN32_FIND_DATA	fd;
	HINTERNET	hFind=::FtpFindFirstFile(mFtpSession, NULL, &fd, 0, 0);
	if(hFind == NULL)
		return;
	do {

		SYSTEMTIME	st;
		FileTimeToSystemTime( &(fd.ftLastWriteTime), &st);

		oFileList[fd.cFileName]=fd;

	} while (::InternetFindNextFile(hFind, &fd));

	::InternetCloseHandle(hFind);
}


