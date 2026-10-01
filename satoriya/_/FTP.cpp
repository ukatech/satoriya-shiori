#include	"ftp.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

FTP::FTP(HINTERNET iInternet) : mInternet(iInternet), mFtpSession(NULL) {}

FTP::~FTP() { close(); }

bool	FTP::open(const std::wstring& host, const std::wstring& id, const std::wstring& password, bool is_passive)
{
	close();

	mFtpSession = ::InternetConnectW(
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


bool	FTP::cd(const std::wstring& subdir)
{
	assert(mFtpSession != NULL);
	return	::FtpSetCurrentDirectoryW(mFtpSession, subdir.c_str()) != FALSE;
}

bool	FTP::pwd(std::wstring& o_dir)
{
	//assert(mFtpSession != NULL);
	wchar_t	curdir[2048]=L"";
	DWORD	size=2048;
	if ( FALSE == ::FtpGetCurrentDirectoryW(mFtpSession, curdir, &size) )
	{
		return false;
	}
	o_dir = curdir;
	return	true;
}

bool	FTP::mkdir(const std::wstring& subdir)
{
	assert(mFtpSession != NULL);
	return	::FtpCreateDirectoryW(mFtpSession, subdir.c_str()) != FALSE;
}

bool	FTP::put(const std::wstring& local_filepath, const std::wstring& upload_filename, bool isBinary)
{
	assert(mFtpSession != NULL);
	return	::FtpPutFileW(
		mFtpSession,
		local_filepath.c_str(),
		upload_filename.c_str(),
		isBinary ? INTERNET_FLAG_TRANSFER_BINARY : FTP_TRANSFER_TYPE_ASCII,
		0) != FALSE;
}

void	FTP::ls(std::map<std::wstring, WIN32_FIND_DATAW>& oFileList)
{
	assert(mFtpSession != NULL);

	WIN32_FIND_DATAW	fd;
	HINTERNET	hFind=::FtpFindFirstFileW(mFtpSession, NULL, &fd, 0, 0);
	if(hFind == NULL)
		return;
	do {

		SYSTEMTIME	st;
		FileTimeToSystemTime( &(fd.ftLastWriteTime), &st);

		oFileList[fd.cFileName]=fd;

	} while (::InternetFindNextFileW(hFind, &fd));

	::InternetCloseHandle(hFind);
}


