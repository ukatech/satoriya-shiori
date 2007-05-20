/*------------------------------------------------------------
	Report	エラー報告クラス

	使い方

	void	function() 
	{
		Report	rp("process name");	

		rp="process step 1";
		if ( ～～ )
			return;		// エラー報告されます。

		rp="process step 2";
		if ( ～～ )
			return;		// エラー報告されます。

		rp="";	// 正常終了。エラー報告されません。
	}



------------------------------------------------------------*/
#ifndef	REPORT_H
#define	REPORT_H

#define		WIN32_LEAN_AND_MEAN
#include	<windows.h>

#ifndef	LINE_MAX
#define	LINE_MAX	1024
#endif

typedef long HRESULT;	//	for return value of 'DirectX'

class	Report {

	// コピー・代入の禁止
	Report(Report&);
	Report&	operator=(Report&);

	// メッセージボックスの親ウィンドウ
	HWND	m_hWnd;

	// メッセージバッファ
	TCHAR	m_szProcess[LINE_MAX];
	TCHAR	m_szStep[LINE_MAX];
	TCHAR	m_szMessage[LINE_MAX];
	int		m_iMessagePos;	// szMessage中の書き込み位置

	// Reportクラスのオブジェクト存在個数	
	static	int	m_nInstance;
	// メッセージボックス表示時の親ウィンドウ
	static	HWND	m_hMainWindow;
	// 生成時のnInstance値。階層深度を示す。
			int	m_nNest;

	// m_nNest*2の空白を置いて、各デバイスに出力
	void	Output( LPCSTR );

public:

	// 報告する処理名/メッセージボックスの親ウィンドウ
	Report( LPCSTR szProcess, HWND hWnd=NULL );

	// この時点で m_szStep に内容があればエラー処理。
	~Report();

	// 処理の段階
	Report&	operator=( LPCSTR szStep );
	void	finish() { (*this)=""; }

	// エラー詳細の記述、またはログ。
	Report&	operator<<( LPCSTR str );
	Report&	operator<<( int iValue );
	Report&	operator<<( DWORD dwValue );

	// エラー内容を m_szMessage に格納。
	Report&	DD( HRESULT hr );
	Report&	DI( HRESULT hr );
	Report&	API();	// use GetLastError()
};

// ソースファイル中の位置を出力するマクロ
// usage : rp << here;
#define	here	__FILE__ << " (" << __LINE__ << ")\n"

#endif	//	REPORT_H
