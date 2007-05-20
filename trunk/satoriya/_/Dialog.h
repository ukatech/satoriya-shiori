#ifndef	DIALOG_H
#define	DIALOG_H

#include	<windows.h>
#include	<commctrl.h>
#include	<stdarg.h>
#include	<stdio.h>

// ダイアログボックスの基底クラス。
// 使い方は下の方に。
class	Dialog {

private:
	static	Dialog*	sm_pDialog;
	static	BOOL WINAPI DialogProcedure( HWND, UINT, WPARAM, LPARAM );

	// イベントハンドラ
	virtual	BOOL	OnInitDialog( HWND hwndFocus, LONG lInitParam ) { return TRUE; }
	virtual	BOOL	OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCrl ) { return FALSE; }
	virtual	BOOL	OnSysCommand( UINT uCmdType, WORD xPos, WORD yPos );
	virtual	BOOL	OnHScroll( int nScrollCode, int nPos, HWND hwndScrollBar ) { return FALSE; }
	virtual	BOOL	OnVScroll( int nScrollCode, int nPos, HWND hwndScrollBar ) { return FALSE; }
	virtual	BOOL	OnKeyDown( int nVirtKey, long lKeyData  ) { return FALSE; }
	virtual	BOOL	OnTimer( WPARAM wTimerID, TIMERPROC* tmprc ) { return FALSE; }
	virtual BOOL	OnJoy1ButtonDown( WPARAM button, int x, int y ) { return FALSE; }
	virtual	BOOL	OnCopyData( HWND hwndSender, PCOPYDATASTRUCT pcds) { return FALSE; }
	virtual	BOOL	OnClose() { return FALSE; }
	virtual	BOOL	OnDestroy() { return FALSE; }

protected:
	HWND		m_hOwner;
	HWND		m_hDlg;
	HINSTANCE	m_hInstance;
	BOOL		m_fModeless;

public:
	// コントロールアクセスヘルパー
	
	HWND	toHWND( int id ) { return ::GetDlgItem( m_hDlg, id ); }
	int		toID( HWND hWnd ) { return ::GetDlgCtrlID( hWnd ); }
	int		toID( LPARAM lParam ) { return ::GetDlgCtrlID( (HWND)lParam ); }

	void	Enable( int id, BOOL f=TRUE ) { ::EnableWindow( toHWND(id), f ); }
	void	Disable( int id ) { ::EnableWindow( toHWND(id), FALSE ); }
	BOOL	isEnabled( int id ) { return ::IsWindowEnabled( toHWND(id) ); }

	void	Check( int id, BOOL f=TRUE ) { ::SendMessage( toHWND(id), BM_SETCHECK, (WPARAM)f, 0 ); }
	void	Uncheck( int id ) {  ::SendMessage( toHWND(id), BM_SETCHECK, (WPARAM)FALSE, 0 ); }
	BOOL	isChecked( int id ) { return ::SendMessage( toHWND(id), BM_GETCHECK, 0, 0 ); }
	BOOL	isChecked( LPARAM lParam ) { return ::SendMessage( (HWND)lParam, BM_GETCHECK, 0, 0 ); }

	void	SetRange( int id, int min, int max ) {
		::SendMessage( toHWND(id), TBM_SETRANGEMIN, FALSE, min );
		::SendMessage( toHWND(id), TBM_SETRANGEMAX, TRUE, max );
	}
	int		GetPos( int id ) { return ::SendMessage( toHWND(id), TBM_GETPOS, 0 ,0 ); }
	int		GetPos( LPARAM lParam ) { return ::SendMessage( (HWND)lParam, TBM_GETPOS, 0 ,0 ); }
	void	SetPos( int id, int pos ) { ::SendMessage( toHWND(id), TBM_SETPOS, (WPARAM)TRUE , (LPARAM)pos ); }

	int		GetTextLength(int id) { return ::SendMessage(toHWND(id), WM_GETTEXTLENGTH, 0, 0); }
	void	GetText( int id, LPSTR str, int max ) { ::GetDlgItemText( m_hDlg, id, str, max ); }
	void	SetText( int id, const char* format, ... );
	void	SetSel( int id, int nStart, int nEnd ) { ::SendDlgItemMessage( m_hDlg, id, EM_SETSEL, (WPARAM)nStart, (LPARAM)nEnd ); }
	void	SelAll( int id ) { SetSel( id, 0, -1 ); }

#ifdef	_STRING_
	void	GetText(int id, string& str) {
		int	len = GetTextLength(id);
		char*	buf = new char[len+1];
		GetText(id, buf, len+1); 
		str=buf;
		delete [] buf;
	}
	string	GetText(int id) { string str; GetText(id, str); return str; }
	void	SetText(int id, const string& str) { ::SetDlgItemText(m_hDlg, id, str.c_str()); }
#endif

	void	End( int nResult ) { ::EndDialog( m_hDlg, nResult ); }

	void	LB_ResetContent( int id ) { ::SendDlgItemMessage( m_hDlg, id, LB_RESETCONTENT, 0, 0 ); }
	LONG	LB_AddString( int id, LPCSTR str ) { return ::SendDlgItemMessage( m_hDlg, id, LB_ADDSTRING, 0, (LPARAM)str ); }
	void	LB_SetCurSel( int id, int index ) { ::SendDlgItemMessage( m_hDlg, id, LB_SETCURSEL, (WPARAM)index, 0 ); }
	LONG	LB_GetCurSel( int id ) { return ::SendDlgItemMessage( m_hDlg, id, LB_GETCURSEL, 0, 0 ); }
	LONG	LB_GetSelCount( int id ) { ::SendDlgItemMessage( m_hDlg, id, LB_GETSELCOUNT, 0, 0 ); }
	LONG	LB_GetText( int id, int index, LPSTR buf ) { return ::SendDlgItemMessage( m_hDlg, id, LB_GETTEXT, (WPARAM)index, (LPARAM)buf ); }
	LONG	LB_SetTopIndex( int id, int index ) { return ::SendDlgItemMessage( m_hDlg, id, LB_SETTOPINDEX, (WPARAM)index, 0 ); }
	LONG	LB_GetCount(int id) { return ::SendDlgItemMessage( m_hDlg, id, LB_GETCOUNT, 0, 0 ); }

	bool	LB_SelectText(int iID, LPCSTR iBuf) {
		TCHAR	theBuf[1024];
		int	max=LB_GetCount(iID);
		for ( int i=0 ; i<max ; i++ ) {
			LB_GetText(iID, i, theBuf);
			if ( lstrcmp(iBuf, theBuf)==0 ) {
				LB_SetCurSel(iID, i);
				return	true;
			}
		}
		return	false; 
	}

	void	CB_ResetContent( int id ) { ::SendDlgItemMessage( m_hDlg, id, CB_RESETCONTENT, 0, 0 ); }
	LONG	CB_AddString( int id, LPCSTR str ) { return ::SendDlgItemMessage( m_hDlg, id, CB_ADDSTRING, 0, (LPARAM)str ); }
	void	CB_SetCurSel( int id, int index ) { ::SendDlgItemMessage( m_hDlg, id, CB_SETCURSEL, (WPARAM)index, 0 ); }
	LONG	CB_GetCurSel( int id ) { return ::SendDlgItemMessage( m_hDlg, id, CB_GETCURSEL, 0, 0 ); }
	LONG	CB_SetTopIndex( int id, int index ) { return ::SendDlgItemMessage( m_hDlg, id, CB_SETTOPINDEX, (WPARAM)index, 0 ); }

	// ダイアログボックスを開始。終了するまで処理を戻さない。
	virtual 
	int		// EndDialogに渡された値。
	Run( 
		HINSTANCE hInstance,	// インスタンスハンドル
		WORD idDialog, 			// リソースID
		HWND hwndOwner=NULL );	// 親ウィンドウのハンドル

	// ダイアログボックスを開始。開始したら処理を戻す。
	virtual 
	HWND	// 作成したダイアログのハンドル
	RunAsync( 
		HINSTANCE hInstance, WORD idDialog, HWND hwndOwner=NULL );
};

/*-- usage ---------------------------------------------------------

class UserDialog : public Dialog {
public:
	// override event-handler
	virtual	BOOL	OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCrl ) {
		...
	}
}

void	foo() {
	UserDialog	dlg;
	int	ret = dlg.Run( hInst, IDD_DIALOG, hWnd );
}

------------------------------------------------------------------*/

// 一般的によく使うダイアログのテンプレート

// ２ボタン、３ボタン、文字列の入力、整数値の入力、複数選択、など。

BOOL	// 変更の有無
GetStringFromDialog(
	LPCSTR	szTitle,	// ダイアログタイトル
	LPCSTR	szMessage,	// STATIC_TEXT値
	LPCSTR	szDefault,	// 入力エリアの初期値
	LPSTR	szBuffer,	// 入力された文字列
	int		iBufSize	// szBufferのサイズ
);




//------------------------------------------------------------------
/*class TabSheetDialog : public Dialog
{
protected:
	void add_sheet(int i_id, const char* i_name)
	{
		if ( m_sheets.empty() )
		{
		}

		m_sheets.push_back(

	}


private:
	vector<int> m_sheets;
};

*/






#endif	//	DIALOG_H
