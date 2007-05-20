
// ファイル更新通知
class ChangeNotification
{
	HANDLE m_handle;
public:
	ChangeNotification(
		string i_folder,
		bool i_is_watch_subtree = false,
		unsigned long i_flags = FILE_NOTIFY_CHANGE_LAST_WRITE|FILE_NOTIFY_CHANGE_FILE_NAME)
	{
		m_handle = ::FindFirstChangeNotification(
			i_folder.c_str(),
			i_is_watch_subtree ? TRUE : FALSE,
			i_flags);
	}
	~ChangeNotification()
	{
		if ( m_handle != NULL )
		{
			::FindCloseChangeNotification(m_handle);
			m_handle = NULL;
		}
	}

	HANDLE data()
	{
		return m_handle;
	}

	DWORD wait(DWORD i_wait_time=INFINITE)
	{
		return WaitForSingleObject(this->data(), INFINITE);
	}

	bool next()
	{
		return ::FindNextChangeNotification(m_handle)!=FALSE;
	}
};
/*
	ChangeNotification cn(".");
	if ( cn.data() != NULL )
	{
		do
		{
			cn.wait();
			
			//operation
			
		} while ( cn.next() );
	}
*/

// WIN32_FIND_DATAの比較（set, map, list::sortなど）
inline bool operator<(const WIN32_FIND_DATA& lhs, const WIN32_FIND_DATA& rhs)
{
	return strcmp(lhs.cFileName, rhs.cFileName) < 0;
}

