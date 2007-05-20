
#include <string>
using namespace std;


class Menu
{
	HMENU m_menu;
public:
	Menu()
	{
		m_menu = ::CreateMenu();
	}
	~Menu()
	{
		::DestroyMenu(m_menu);
	}
	
	HMENU get()
	{
		return m_menu;
	}
	
	void add_separator()
	{
		MENUITEMINFO mii;
		::ZeroMemory(&mii, sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_TYPE;
		mii.fType = MFT_SEPARATOR;

		::InsertMenuItem(m_menu, 0, FALSE, &mii);
	}

	void add_item(int i_id, string i_str)
	{
		MENUITEMINFO mii;
		::ZeroMemory(&mii, sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_TYPE;
		mii.fType = MFT_STRING;

		 //(char*)i_str.c_str() 
		mii.dwTypeData = TEXT("TEST");
		mii.cch = i_str.size() + 1;
		//BOOL r = ::InsertMenuItem(m_menu, i_id, TRUE, &mii);
		BOOL r = ::InsertMenuItem(m_menu, 0, TRUE, &mii);
		int a = 1;


	}
};

