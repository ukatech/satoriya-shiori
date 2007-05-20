#include <string>

// タスクトレイアイコン通知メッセージ
static const UINT WM_NOTIFYICON = WM_USER;

// タスクトレイアイコンの管理
class TrayIconManager
{
	NOTIFYICONDATA m_nid;
public:
	TrayIconManager()
	{
		::memset(&m_nid, 0, sizeof(NOTIFYICONDATA));
		m_nid.cbSize = sizeof(NOTIFYICONDATA);
		m_nid.uCallbackMessage = WM_NOTIFYICON;
	}
	~TrayIconManager()
	{
		remove();
	}

	bool add(HWND i_hwnd, UINT i_id, HICON i_icon, std::string i_tip)
	{
		m_nid.hWnd = i_hwnd;
		m_nid.uID = i_id;
		m_nid.hIcon = i_icon;
		m_nid.uFlags = NIF_MESSAGE | NIF_ICON | (i_tip.empty() ? 0 : NIF_TIP);
		strncpy(m_nid.szTip, i_tip.c_str(), 64);
		return ::Shell_NotifyIcon(NIM_ADD, &m_nid)!=FALSE;
	}

	bool re_add()
	{
		return ::Shell_NotifyIcon(NIM_ADD, &m_nid)!=FALSE;
	}

	bool modify(HICON i_icon, std::string i_tip)
	{
		m_nid.hIcon = i_icon;
		m_nid.uFlags = NIF_MESSAGE | NIF_ICON | (i_tip.empty() ? 0 : NIF_TIP);
		strncpy(m_nid.szTip, i_tip.c_str(), 64);
		return ::Shell_NotifyIcon(NIM_MODIFY, &m_nid)!=FALSE;
	}

	bool remove()
	{
		return ::Shell_NotifyIcon(NIM_DELETE, &m_nid)!=FALSE;
	}
};
