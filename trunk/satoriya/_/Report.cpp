#include	"Report.h"
#include	<assert.h>
#include	<stdio.h>

// staticメンバ
int	Report::m_nInstance = 0;
HWND	Report::m_hMainWindow = NULL;

//------------------------------------------------------//
Report::Report(
	LPCSTR szProcess,	// 報告対象となる処理の名前
	HWND hWnd )		// 報告用メッセージボックスの引数
{
	// 報告対象となる処理を記憶
	assert( szProcess != NULL );
	::CopyMemory( m_szProcess, szProcess, LINE_MAX );

	// 生成管理
	m_nNest = m_nInstance++;

	// 報告開始
	TCHAR	buf[LINE_MAX];
	sprintf( buf, "%s : start", m_szProcess );
	Output(buf);
	m_nNest++;

	m_hWnd = hWnd;
	m_szStep[0] = '\0';
	m_szMessage[0] = '\0';
	m_iMessagePos = 0;
}

//------------------------------------------------------//
Report::~Report() 
{
	// メッセージがあれば出力
	if ( m_iMessagePos > 0 )
		Output(m_szMessage);

	// 正常に終了しなかった場合
	if ( m_szStep[0] != '\0' ) {

		TCHAR	buf[LINE_MAX];
		if ( m_szMessage[0] == '\0' ) {
			sprintf( buf,
				"process : %s\nstep : %s\n\n"
				"処理は正しく終了しませんでした。",
				m_szProcess, m_szStep );
		} 
		else 
		{
			sprintf( buf,
				"process : %s\nstep : %s\n\n"
				"%s",
				m_szProcess, m_szStep, m_szMessage );
		}

		// メッセージボックスを表示
		::MessageBox( m_hWnd, buf, "報告", MB_OK|MB_SYSTEMMODAL );
	}

	// 報告終了
	m_nNest--;
	TCHAR	buf[LINE_MAX];
	sprintf( buf, "%s : end", m_szProcess );
	Output(buf);

	// 消滅管理
	m_nInstance--;
}

//------------------------------------------------------//
void
Report::Output( LPCSTR str ) 
{
	assert( str != NULL );

	TCHAR	buf[LINE_MAX];
	::FillMemory( buf, m_nNest*2, ' ' );
	sprintf( buf + m_nNest*2, "%s\n", str );

	::OutputDebugString(buf);
}

//------------------------------------------------------//
Report&
Report::operator=( LPCSTR szStep ) {
	assert( szStep != NULL );

	::lstrcpy( m_szStep, szStep );
	if ( m_szStep[0] != '\0' )
		Output( m_szStep );

	return	*this;
}

//------------------------------------------------------//
Report&
Report::operator<<( LPCSTR str ) 
{
	assert( str != NULL );
	m_iMessagePos += sprintf( 
		m_szMessage + m_iMessagePos,
		"%s", str );
	return	*this;
}

//------------------------------------------------------//
Report&
Report::operator<<( int iValue )
{
	m_iMessagePos += sprintf( 
		m_szMessage + m_iMessagePos,
		"%d",
		iValue );
	return	*this;
}

//------------------------------------------------------//
Report&
Report::operator<<( DWORD dwValue )
{
	m_iMessagePos += sprintf( 
		m_szMessage + m_iMessagePos,
		"%u",
		dwValue );
	return	*this;
}



//------------------------------------------------------//
#include	<ddraw.h>
Report&
Report::DD( 
	HRESULT hr )	// DirectDraw関数の返り値
{
	TCHAR	buf[LINE_MAX];
#ifdef	_DEBUG
	switch( hr ) {
	case DD_OK: strcpy( buf, "DD_OK\n要求は成功し完了した。" ); break;
	case DDERR_ALREADYINITIALIZED: strcpy( buf, "DDERR_ALREADYINITIALIZED\nオブジェクトはすでに初期化されている。" ); break;
	case DDERR_BLTFASTCANTCLIP: strcpy( buf, "DDERR_BLTFASTCANTCLIP\nDirectDrawClipper オブジェクトがIdirectDrawSurface3::BltFastメソッドの呼び出しに渡される転送元サーフェスにアタッチされている。" ); break;
	case DDERR_CANNOTATTACHSURFACE: strcpy( buf, "DDERR_CANNOTATTACHSURFACE\n要求されたサーフェスにはサーフェスをアタッチできない。" ); break;
	case DDERR_CANNOTDETACHSURFACE: strcpy( buf, "DDERR_CANNOTDETACHSURFACE\n要求されたサーフェスからサーフェスをデタッチできない。" ); break;
	case DDERR_CANTCREATEDC: strcpy( buf, "DDERR_CANTCREATEDC\nWindowsはそれ以上のいかなるデバイスコンテキスト（ DC ）を作ることができない。" ); break;
	case DDERR_CANTDUPLICATE: strcpy( buf, "DDERR_CANTDUPLICATE\nプライマリサーフェス、3Dサーフェス、暗黙的に作られるサーフェスは複製できない。" ); break;
	case DDERR_CANTLOCKSURFACE: strcpy( buf, "DDERR_CANTLOCKSURFACE\nDCI サポートなしでプライマリサーフェスのロックをしようとしたため、このサーフェスへのアクセスが拒否された。" ); break;
	case DDERR_CANTPAGELOCK: strcpy( buf, "DDERR_CANTPAGELOCK\nサーフェスのページ ロックが失敗した。 ページ ロックはディスプレイ メモリ サーフェスあるいはエミュレートされたプライマリサーフェスの上では動作しない。" ); break;
	case DDERR_CANTPAGEUNLOCK: strcpy( buf, "DDERR_CANTPAGEUNLOCK\nサーフェスのページ ロック解除が失敗した。 ページ ロック解除はディスプレイ メモリ サーフェスあるいはエミュレートされたプライマリサーフェスの上では動作しない。" ); break;
	case DDERR_CLIPPERISUSINGHWND: strcpy( buf, "DDERR_CLIPPERISUSINGHWND\nウィンドウハンドルをすでにモニタしているDirectDrawClipper オブジェクトにクリッピングリストをセットしようとした。" ); break;
	case DDERR_COLORKEYNOTSET: strcpy( buf, "DDERR_COLORKEYNOTSET\n転送元カラーキー が指定されていない。" ); break;
	case DDERR_CURRENTLYNOTAVAIL: strcpy( buf, "DDERR_CURRENTLYNOTAVAIL\n現在サポートされていない。" ); break;
	case DDERR_DCALREADYCREATED: strcpy( buf, "DDERR_DCALREADYCREATED\nデバイスコンテキスト（DC）はすでにこのサーフェスに対し返されている。１サーフェスにつき1つのDCしか取得することはできない。" ); break;
	case DDERR_DEVICEDOESNTOWNSURFACE: strcpy( buf, "DDERR_DEVICEDOESNTOWNSURFACE\n1 つの DirectDraw デバイスによって作成されたサーフェスは、別の DirectDraw デバイスが直接使用することはできない。" ); break;
	case DDERR_DIRECTDRAWALREADYCREATED: strcpy( buf, "DDERR_DIRECTDRAWALREADYCREATED\nDirectDrawオブジェクトはすでに作成されている。" ); break;
	case DDERR_EXCEPTION: strcpy( buf, "DDERR_EXCEPTION\n要求された処理を行う間に例外が発生した。" ); break;
	case DDERR_EXCLUSIVEMODEALREADYSET: strcpy( buf, "DDERR_EXCLUSIVEMODEALREADYSET\nすでに排他的モードがセットされているにもかかわらず、協調レベルをセットしようとした。" ); break;
	case DDERR_GENERIC: strcpy( buf, "DDERR_GENERIC\n未定義のエラー状態である。" ); break;
	case DDERR_HEIGHTALIGN: strcpy( buf, "DDERR_HEIGHTALIGN\n与えられた矩形の高さは必要とされる整列の倍数ではない。" ); break;
	case DDERR_HWNDALREADYSET: strcpy( buf, "DDERR_HWNDALREADYSET\nDirectDraw協調レベル ウィンドウハンドルはすでに設定されている。ウィンドウハンドルはプロセスがサーフェスあるいは生成したパレットを保持している間はリセットできない。" ); break;
	case DDERR_HWNDSUBCLASSED: strcpy( buf, "DDERR_HWNDSUBCLASSED\nDirectDraw協調レベル ウィンドウハンドルが サブクラス化されているため、DirectDrawの状態のリストアが阻止された。" ); break;
	case DDERR_IMPLICITLYCREATED: strcpy( buf, "DDERR_IMPLICITLYCREATED\n暗黙的に作られたサーフェスをリストアしようとした。" ); break;
	case DDERR_INCOMPATIBLEPRIMARY: strcpy( buf, "DDERR_INCOMPATIBLEPRIMARY\nプライマリサーフェス 作成要求が既存のプライマリサーフェスと一致していない。" ); break;
	case DDERR_INVALIDCAPS: strcpy( buf, "DDERR_INVALIDCAPS\nコールバック関数に渡された1つ以上の能力ビットが不正である。" ); break;
	case DDERR_INVALIDCLIPLIST: strcpy( buf, "DDERR_INVALIDCLIPLIST\nDirectDraw は与えられたクリッピングリスト をサポートしない。" ); break;
	case DDERR_INVALIDDIRECTDRAWGUID: strcpy( buf, "DDERR_INVALIDDIRECTDRAWGUID\nDirectDrawCreate 関数に渡されるグローバルユニーク識別子（ GUID ）は有効な DirectDraw ドライバ識別子ではない。" ); break;
	case DDERR_INVALIDMODE: strcpy( buf, "DDERR_INVALIDMODE\nDirectDraw は要求されたモードをサポートしない。" ); break;
	case DDERR_INVALIDOBJECT: strcpy( buf, "DDERR_INVALIDOBJECT\nDirectDraw は無効なDirectDraw オブジェクトのポインタを受け取った。" ); break;
	case DDERR_INVALIDPARAMS: strcpy( buf, "DDERR_INVALIDPARAMS\nメソッドに渡された1つ以上のパラメータが正しくない。" ); break;
	case DDERR_INVALIDPIXELFORMAT: strcpy( buf, "DDERR_INVALIDPIXELFORMAT\n指定されたピクセルフォーマットは無効である。" ); break;
	case DDERR_INVALIDPOSITION: strcpy( buf, "DDERR_INVALIDPOSITION\n転送先上のオーバーレイの位置が不正である。" ); break;
	case DDERR_INVALIDRECT: strcpy( buf, "DDERR_INVALIDRECT\n与えられた矩形が無効である。" ); break;
	case DDERR_INVALIDSURFACETYPE: strcpy( buf, "DDERR_INVALIDSURFACETYPE\nサーフェスが間違ったタイプであったため、要求された処理を実行できなかった。" ); break;
	case DDERR_LOCKEDSURFACES: strcpy( buf, "DDERR_LOCKEDSURFACES\n1つ以上のサーフェスがロックされており、要求された処理に失敗した。" ); break;
	case DDERR_MOREDATA: strcpy( buf, "DDERR_MOREDATA\n指定したバッファサイズが保有できる以上のデータが存在する。" ); break;
	case DDERR_NO3D: strcpy( buf, "DDERR_NO3D\n3Dハードウェアあるいはエミュレーションが存在しない。" ); break;
	case DDERR_NOALPHAHW: strcpy( buf, "DDERR_NOALPHAHW\nアルファ アクセラレーション ハードウェアが存在しないかあるいは利用できなかったため、要求された処理に失敗した。" ); break;
	case DDERR_NOBLTHW: strcpy( buf, "DDERR_NOBLTHW\nブロック転送ハードウェアが存在しない。" ); break;
	case DDERR_NOCLIPLIST: strcpy( buf, "DDERR_NOCLIPLIST\nクリッピングリストが利用できない。" ); break;
	case DDERR_NOCLIPPERATTACHED: strcpy( buf, "DDERR_NOCLIPPERATTACHED\nこのサーフェス オブジェクトにはDirectDrawClipper オブジェクトをアタッチできない。" ); break;
	case DDERR_NOCOLORCONVHW: strcpy( buf, "DDERR_NOCOLORCONVHW\n色変換ハードウェアが存在しない、あるいは利用できないため、処理が実行できない。" ); break;
	case DDERR_NOCOLORKEY: strcpy( buf, "DDERR_NOCOLORKEY\nサーフェスは現在カラーキーを持っていない。" ); break;
	case DDERR_NOCOLORKEYHW: strcpy( buf, "DDERR_NOCOLORKEYHW\n転送先カラーキーに対するハードウェアサポートがないため、処理が実行できない。" ); break;
	case DDERR_NOCOOPERATIVELEVELSET: strcpy( buf, "DDERR_NOCOOPERATIVELEVELSET\nIdirectDraw2::SetCooperativeLevelメソッドを呼び出すことなく作成関数が呼び出された。" ); break;
	case DDERR_NODC: strcpy( buf, "DDERR_NODC\nDCは、これまでにこのサーフェス用に作られていなかった。" ); break;
	case DDERR_NODDROPSHW: strcpy( buf, "DDERR_NODDROPSHW\nDirectDraw ラスタ処理（ ROP ）ハードウェアが利用できない。" ); break;
	case DDERR_NODIRECTDRAWHW: strcpy( buf, "DDERR_NODIRECTDRAWHW\nハードウェア専用DirectDraw オブジェクトを作成できない。ドライバはハードウェアをサポートしない。" ); break;
	case DDERR_NODIRECTDRAWSUPPORT: strcpy( buf, "DDERR_NODIRECTDRAWSUPPORT\n現在のディスプレイ ドライバはDirectDrawをサポートしていない。" ); break;
	case DDERR_NOEMULATION: strcpy( buf, "DDERR_NOEMULATION\nソフトウェア エミュレーションが利用できない。" ); break;
	case DDERR_NOEXCLUSIVEMODE: strcpy( buf, "DDERR_NOEXCLUSIVEMODE\nアプリケーションに対し排他的モードが要求されたが、アプリケーションは排他的モードを持っていない。" ); break;
	case DDERR_NOFLIPHW: strcpy( buf, "DDERR_NOFLIPHW\n可視サーフェスはフリッピングできない。" ); break;
	case DDERR_NOGDI: strcpy( buf, "DDERR_NOGDI\nGDI が存在しない。" ); break;
	case DDERR_NOHWND: strcpy( buf, "DDERR_NOHWND\nクリッピング通知がウィンドウハンドルを必要とする、あるいはウィンドウハンドルが協調レベル ウィンドウハンドルとしてあらかじめセットされていない。" ); break;
	case DDERR_NOMIPMAPHW: strcpy( buf, "DDERR_NOMIPMAPHW\nミップマップ テクスチャ マッピングのハードウェアが存在していないか、あるいは利用できないので処理を実行できない。" ); break;
	case DDERR_NOMIRRORHW: strcpy( buf, "DDERR_NOMIRRORHW\nミラーリング ハードウェアが存在しないかあるいは利用できないため、処理が実行できない。" ); break;
	case DDERR_NONONLOCALVIDMEM: strcpy( buf, "DDERR_NONONLOCALVIDMEM\n非ローカルビデオメモリをサポートしないデバイスから、非ローカルビデオメモリを割り当てようとした。" ); break;
	case DDERR_NOOPTIMIZEHW: strcpy( buf, "DDERR_NOOPTIMIZEHW\nデバイスは、最適化されたサーフェイスをサポートしない。" ); break;
	case DDERR_NOOVERLAYDEST: strcpy( buf, "DDERR_NOOVERLAYDEST\n転送先確立のためのIdirectDrawSurface3::UpdateOverlayメソッド呼び出しをしていないオーバーレイに対し、IdirectDrawSurface3::GetOverlayPositionメソッドが呼び出された。" ); break;
	case DDERR_NOOVERLAYHW: strcpy( buf, "DDERR_NOOVERLAYHW\nオーバーレイ ハードウェアが存在していないかあるいは利用できないため、処理が実行できない。" ); break;
	case DDERR_NOPALETTEATTACHED: strcpy( buf, "DDERR_NOPALETTEATTACHED\nパレット オブジェクトがこのサーフェスにアタッチされていない。" ); break;
	case DDERR_NOPALETTEHW: strcpy( buf, "DDERR_NOPALETTEHW\n16色あるいは256色のパレットに対するハードウェアサポートがない。" ); break;
	case DDERR_NORASTEROPHW: strcpy( buf, "DDERR_NORASTEROPHW\n適切なラスタ処理ハードウェアが存在しないかあるいは利用できないため、処理が実行できない。" ); break;
	case DDERR_NOROTATIONHW: strcpy( buf, "DDERR_NOROTATIONHW\n回転ハードウェアが存在していないか、あるいは利用できないため処理が実行できない。" ); break;
	case DDERR_NOSTRETCHHW: strcpy( buf, "DDERR_NOSTRETCHHW\n拡大ハードウェアが存在していないかあるいは利用できないため、処理が実行できない。" ); break;
	case DDERR_NOT4BITCOLOR: strcpy( buf, "DDERR_NOT4BITCOLOR\nDirectDrawSurface オブジェクトは、要求された４ビット色パレットを使っていない。" ); break;
	case DDERR_NOT4BITCOLORINDEX: strcpy( buf, "DDERR_NOT4BITCOLORINDEX\nDirectDrawSurface オブジェクトは、要求された４ビットのカラーインデックスパレットを使っていない。" ); break;
	case DDERR_NOT8BITCOLOR: strcpy( buf, "DDERR_NOT8BITCOLOR\nDirectDrawSurface オブジェクトは、要求された８ビット色パレットを使っていない。" ); break;
	case DDERR_NOTAOVERLAYSURFACE: strcpy( buf, "DDERR_NOTAOVERLAYSURFACE\n非オーバーレイサーフェスに対し、オーバーレイ コンポーネントが呼び出された。" ); break;
	case DDERR_NOTEXTUREHW: strcpy( buf, "DDERR_NOTEXTUREHW\nテクスチャ マッピング ハードウェアが存在していないかあるいは利用できないため、処理が実行できない。" ); break;
	case DDERR_NOTFLIPPABLE: strcpy( buf, "DDERR_NOTFLIPPABLE\nフリッピングできないサーフェスをフリッピングしようとした。" ); break;
	case DDERR_NOTFOUND: strcpy( buf, "DDERR_NOTFOUND\n要求された項目は見つからなかった。" ); break;
	case DDERR_NOTINITIALIZED: strcpy( buf, "DDERR_NOTINITIALIZED\nオブジェクトが初期化される前に、CoCreateInstanceで作成された DirectDraw オブジェクトのインターフェイス メソッドを呼び出そうとした。" ); break;
	case DDERR_NOTLOADED: strcpy( buf, "DDERR_NOTLOADED\nサーフェイスは最適化されているが、まだメモリは割り当てられてない。" ); break;
	case DDERR_NOTLOCKED: strcpy( buf, "DDERR_NOTLOCKED\nロックされていないサーフェスのロック解除が行われた。" ); break;
	case DDERR_NOTPAGELOCKED: strcpy( buf, "DDERR_NOTPAGELOCKED\n未処理のページ ロックがないサーフェスのページ ロック解除が行われた。" ); break;
	case DDERR_NOTPALETTIZED: strcpy( buf, "DDERR_NOTPALETTIZED\n使用しているサーフェスはパレット ベースのサーフェスではない。" ); break;
	case DDERR_NOVSYNCHW: strcpy( buf, "DDERR_NOVSYNCHW\n垂直帰線同期処理に対するハードウェアサポートがないため、処理を実行できない。" ); break;
	case DDERR_NOZBUFFERHW: strcpy( buf, "DDERR_NOZBUFFERHW\nZバッファに対するハードウェアサポートがないため、ディスプレイ メモリでの Z バッファ作成あるいは Z バッファを使用したブロック転送が実行できない。" ); break;
	case DDERR_NOZOVERLAYHW: strcpy( buf, "DDERR_NOZOVERLAYHW\nオーバーレイサーフェスは、ハードウェアがオーバーレイ Zオーダーをサポートしないため、Zオーダーが無視された。" ); break;
	case DDERR_OUTOFCAPS: strcpy( buf, "DDERR_OUTOFCAPS\n要求された処理に必要なハードウェアがすでに割り当てられている。" ); break;
	case DDERR_OUTOFMEMORY: strcpy( buf, "DDERR_OUTOFMEMORY\nDirectDrawが処理を行うのに十分なメモリがない。" ); break;
	case DDERR_OUTOFVIDEOMEMORY: strcpy( buf, "DDERR_OUTOFVIDEOMEMORY\nDirectDrawが処理を行うのに十分なディスプレイ メモリがない。" ); break;
	case DDERR_OVERLAYCANTCLIP: strcpy( buf, "DDERR_OVERLAYCANTCLIP\nハードウェアはクリッピング オーバーレイをサポートしない。" ); break;
	case DDERR_OVERLAYCOLORKEYONLYONEACTIVE: strcpy( buf, "DDERR_OVERLAYCOLORKEYONLYONEACTIVE\nオーバーレイ上に1つ以上の有効なカラーキーを持とうとした。" ); break;
	case DDERR_OVERLAYNOTVISIBLE: strcpy( buf, "DDERR_OVERLAYNOTVISIBLE\nIdirectDrawSurface3::GetOverlayPositionメソッドが不可視オーバーレイ上で呼び出された。" ); break;
	case DDERR_PALETTEBUSY: strcpy( buf, "DDERR_PALETTEBUSY\nパレットがもう1つのスレッドによってロックされているため、アクセスが拒否された。" ); break;
	case DDERR_PRIMARYSURFACEALREADYEXISTS: strcpy( buf, "DDERR_PRIMARYSURFACEALREADYEXISTS\nこの処理はすでにプライマリサーフェスを作成している。" ); break;
	case DDERR_REGIONTOOSMALL: strcpy( buf, "DDERR_REGIONTOOSMALL\nIdirectDrawClipper::GetClipListメソッドに渡された領域が小さすぎる。" ); break;
	case DDERR_SURFACEALREADYATTACHED: strcpy( buf, "DDERR_SURFACEALREADYATTACHED\nすでにアタッチしているもう1つのサーフェスにサーフェスをアタッチしようとした。" ); break;
	case DDERR_SURFACEALREADYDEPENDENT: strcpy( buf, "DDERR_SURFACEALREADYDEPENDENT\nすでに他のサーフェスに依存しているサーフェスを依存させようとした。" ); break;
	case DDERR_SURFACEBUSY: strcpy( buf, "DDERR_SURFACEBUSY\nサーフェスがもう1つのスレッドによってロックされているため、サーフェスへのアクセスが拒否された。" ); break;
	case DDERR_SURFACEISOBSCURED: strcpy( buf, "DDERR_SURFACEISOBSCURED\nサーフェスへのアクセスが、サーフェスが隠蔽されているため拒否された。" ); break;
	case DDERR_SURFACELOST: strcpy( buf, "DDERR_SURFACELOST\nサーフェス メモリの不足のため、サーフェスへのアクセスが拒否された。 このサーフェスを表すDirectDrawSurface オブジェクトにはIdirectDrawSurface3::Restoreメソッドが呼び出されなければならない。" ); break;
	case DDERR_SURFACENOTATTACHED: strcpy( buf, "DDERR_SURFACENOTATTACHED\n要求されたサーフェスはアタッチできない。" ); break;
	case DDERR_TOOBIGHEIGHT: strcpy( buf, "DDERR_TOOBIGHEIGHT\nDirectDraw によって要求された高さが大きすぎる。" ); break;
	case DDERR_TOOBIGSIZE: strcpy( buf, "DDERR_TOOBIGSIZE\nDirectDraw によって要求された大きさが大きすぎる。しかし、個別の高さと幅は問題がない。" ); break;
	case DDERR_TOOBIGWIDTH: strcpy( buf, "DDERR_TOOBIGWIDTH\nDirectDraw によって要求された幅が大きすぎる。" ); break;
	case DDERR_UNSUPPORTED: strcpy( buf, "DDERR_UNSUPPORTED\nサポートされていない処理であった。" ); break;
	case DDERR_UNSUPPORTEDFORMAT: strcpy( buf, "DDERR_UNSUPPORTEDFORMAT\n要求された FourCC フォーマットはDirectDraw によってサポートされていない。" ); break;
	case DDERR_UNSUPPORTEDMASK: strcpy( buf, "DDERR_UNSUPPORTEDMASK\n要求されたピクセルフォーマットでの ビットマスク はDirectDraw によってサポートされていない。" ); break;
	case DDERR_UNSUPPORTEDMODE: strcpy( buf, "DDERR_UNSUPPORTEDMODE\nディスプレイは現在サポートされていないモードである。" ); break;
	case DDERR_VERTICALBLANKINPROGRESS: strcpy( buf, "DDERR_VERTICALBLANKINPROGRESS\n垂直帰線期間である。" ); break;
	case DDERR_VIDEONOTACTIVE: strcpy( buf, "DDERR_VIDEONOTACTIVE\nビデオポートは、非アクティブである。" ); break;
	case DDERR_WASSTILLDRAWING: strcpy( buf, "DDERR_WASSTILLDRAWING\nこのサーフェスから、またはサーフェスへの前回のブロック転送操作が完了していない。" ); break;
	case DDERR_WRONGMODE: strcpy( buf, "DDERR_WRONGMODE\n異なったモードで作られたため、このサーフェスはリストアすることができない。" ); break;
	case DDERR_XALIGN: strcpy( buf, "DDERR_XALIGN\n与えられた矩形を必要とされる範囲に水平一列に並べることができなかった。" ); break;

	default:
		strcpy( buf, "DDERR_???\n未定義のエラーです。");
	}
#else	// _DEBUG
	sprintf( buf, "DDERR_%d", hr );
#endif	// _DEBUG
	m_iMessagePos += sprintf( 
		m_szMessage + m_iMessagePos,
		"%s\n",
		buf );

	::MessageBox( m_hMainWindow, m_szMessage, "DirectDraw error", MB_OK|MB_SYSTEMMODAL );

	return	*this;
}


//------------------------------------------------------//
#include	<dinput.h>
Report&
Report::DI( 
	HRESULT hr )	// DirectInput関数の返り値
{
	TCHAR	buf[LINE_MAX] = "";

	switch( hr ) {
	case DI_BUFFEROVERFLOW: ::lstrcat( buf, "DI_BUFFEROVERFLOW / DI_NOEFFECT / DI_NOTATTACHED / DI_PROPNOEFFECT" ); break;
	case DI_DOWNLOADSKIPPED: ::lstrcat( buf, "DI_DOWNLOADSKIPPED" ); break;
	case DI_EFFECTRESTARTED: ::lstrcat( buf, "DI_EFFECTRESTARTED" ); break;
	case DI_OK: ::lstrcat( buf, "DI_OK" ); break;
	case DI_POLLEDDEVICE: ::lstrcat( buf, "DI_POLLEDDEVICE" ); break;
	case DI_TRUNCATED: ::lstrcat( buf, "DI_TRUNCATED" ); break;
	case DI_TRUNCATEDANDRESTARTED : ::lstrcat( buf, "DI_TRUNCATEDANDRESTARTED " ); break;
//	case DI_EFFECTRESTARTED: ::lstrcat( buf, "DI_EFFECTRESTARTED" ); break;
		
	case DIERR_ACQUIRED: ::lstrcat( buf, "DIERR_ACQUIRED" ); break;
	case DIERR_ALREADYINITIALIZED: ::lstrcat( buf, "DIERR_ALREADYINITIALIZED" ); break;
	case DIERR_BADDRIVERVER: ::lstrcat( buf, "DIERR_BADDRIVERVER" ); break;
	case DIERR_BETADIRECTINPUTVERSION: ::lstrcat( buf, "DIERR_BETADIRECTINPUTVERSION" ); break;
	case DIERR_DEVICEFULL: ::lstrcat( buf, "DIERR_DEVICEFULL" ); break;
	case DIERR_DEVICENOTREG: ::lstrcat( buf, "DIERR_DEVICENOTREG" ); break;
	case DIERR_EFFECTPLAYING: ::lstrcat( buf, "DIERR_EFFECTPLAYING" ); break;
	case DIERR_HASEFFECTS: ::lstrcat( buf, "DIERR_HASEFFECTS" ); break;
	case DIERR_GENERIC: ::lstrcat( buf, "DIERR_GENERIC" ); break;
	case DIERR_HANDLEEXISTS: ::lstrcat( buf, "DIERR_HANDLEEXISTS" ); break;
	case DIERR_INCOMPLETEEFFECT: ::lstrcat( buf, "DIERR_INCOMPLETEEFFECT" ); break;
	case DIERR_INPUTLOST: ::lstrcat( buf, "DIERR_INPUTLOST" ); break;
	case DIERR_INVALIDPARAM: ::lstrcat( buf, "DIERR_INVALIDPARAM" ); break;
	case DIERR_MOREDATA: ::lstrcat( buf, "DIERR_MOREDATA" ); break;
	case DIERR_NOAGGREGATION: ::lstrcat( buf, "DIERR_NOAGGREGATION" ); break;
	case DIERR_NOINTERFACE: ::lstrcat( buf, "DIERR_NOINTERFACE" ); break;
	case DIERR_NOTACQUIRED: ::lstrcat( buf, "DIERR_NOTACQUIRED" ); break;
	case DIERR_NOTBUFFERED: ::lstrcat( buf, "DIERR_NOTBUFFERED" ); break;
	case DIERR_NOTDOWNLOADED: ::lstrcat( buf, "DIERR_NOTDOWNLOADED" ); break;
	case DIERR_NOTEXCLUSIVEACQUIRED: ::lstrcat( buf, "DIERR_NOTEXCLUSIVEACQUIRED" ); break;
	case DIERR_NOTFOUND: ::lstrcat( buf, "DIERR_NOTFOUND" ); break;
	case DIERR_NOTINITIALIZED: ::lstrcat( buf, "DIERR_NOTINITIALIZED" ); break;
//	case DIERR_OBJECTNOTFOUND: ::lstrcat( buf, "DIERR_OBJECTNOTFOUND" ); break;
	case DIERR_OLDDIRECTINPUTVERSION: ::lstrcat( buf, "DIERR_OLDDIRECTINPUTVERSION" ); break;
	case DIERR_OUTOFMEMORY: ::lstrcat( buf, "DIERR_OUTOFMEMORY" ); break;
//	case DIERR_READONLY: ::lstrcat( buf, "DIERR_READONLY / DIERR_OTHERAPPHASPRIO" ); break;
	case DIERR_UNSUPPORTED: ::lstrcat( buf, "DIERR_UNSUPPORTED" ); break;
	case E_PENDING: ::lstrcat( buf, "E_PENDING" ); break;
	default:
		strcpy( buf, "DIERR_??? ");
	}

	m_iMessagePos += sprintf( 
		m_szMessage + m_iMessagePos,
		"%s\n",
		buf );
	return	*this;
}



//------------------------------------------------------//
Report&	
Report::API()
{
	DWORD	dwErrorCode = GetLastError();
	TCHAR	buf[LINE_MAX];

//#ifndef	DEBUG
#if	1

	sprintf( buf, "Win32API failed : %u", dwErrorCode );

#else

	sprintf( buf, "Win32API failed : \n" );

	switch ( dwErrorCode ) 
	{
	case -8: ::lstrcat( buf, "LZERROR_UNKNOWNALG " ); break;
	case -7: ::lstrcat( buf, "LZERROR_BADVALUE " ); break;
	case -6: ::lstrcat( buf, "LZERROR_GLOBLOCK " ); break;
	case -5: ::lstrcat( buf, "LZERROR_GLOBALLOC " ); break;
	case -4: ::lstrcat( buf, "LZERROR_WRITE " ); break;
	case -3: ::lstrcat( buf, "LZERROR_READ " ); break;
	case -2: ::lstrcat( buf, "LZERROR_BADOUTHANDLE " ); break;
	case -1: ::lstrcat( buf, "LZERROR_BADINHANDLE " ); break;

	case 0L: ::lstrcat( buf, "NO_ERROR or ERROR_SUCCESS" ); break;

	case 1L: ::lstrcat( buf, "ERROR_INVALID_FUNCTION " ); break;
	case 2L: ::lstrcat( buf, "ERROR_FILE_NOT_FOUND " ); break;
	case 3L: ::lstrcat( buf, "ERROR_PATH_NOT_FOUND " ); break;

	case 4L: ::lstrcat( buf, "ERROR_TOO_MANY_OPEN_FILES " ); break;
	case 5L: ::lstrcat( buf, "ERROR_ACCESS_DENIED " ); break;
	case 6L: ::lstrcat( buf, "ERROR_INVALID_HANDLE " ); break;
	case 7L: ::lstrcat( buf, "ERROR_ARENA_TRASHED " ); break;
	case 8L: ::lstrcat( buf, "ERROR_NOT_ENOUGH_MEMORY " ); break;
	case 9L: ::lstrcat( buf, "ERROR_INVALID_BLOCK " ); break;
	case 10L: ::lstrcat( buf, "ERROR_BAD_ENVIRONMENT " ); break;
	case 11L: ::lstrcat( buf, "ERROR_BAD_FORMAT " ); break;
	case 12L: ::lstrcat( buf, "ERROR_INVALID_ACCESS " ); break;
	case 13L: ::lstrcat( buf, "ERROR_INVALID_DATA " ); break;
	case 14L: ::lstrcat( buf, "ERROR_OUTOFMEMORY " ); break;
	case 15L: ::lstrcat( buf, "ERROR_INVALID_DRIVE " ); break;

	case 16L: ::lstrcat( buf, "ERROR_CURRENT_DIRECTORY " ); break;
	case 17L: ::lstrcat( buf, "ERROR_NOT_SAME_DEVICE " ); break;
	case 18L: ::lstrcat( buf, "ERROR_NO_MORE_FILES " ); break;
	case 19L: ::lstrcat( buf, "ERROR_WRITE_PROTECT " ); break;
	case 20L: ::lstrcat( buf, "ERROR_BAD_UNIT " ); break;
	case 21L: ::lstrcat( buf, "ERROR_NOT_READY " ); break;
	case 22L: ::lstrcat( buf, "ERROR_BAD_COMMAND " ); break;
	case 23L: ::lstrcat( buf, "ERROR_CRC " ); break;
	case 24L: ::lstrcat( buf, "ERROR_BAD_LENGTH " ); break;
	case 25L: ::lstrcat( buf, "ERROR_SEEK " ); break;
	case 26L: ::lstrcat( buf, "ERROR_NOT_DOS_DISK " ); break;
	case 27L: ::lstrcat( buf, "ERROR_SECTOR_NOT_FOUND " ); break;

	case 28L: ::lstrcat( buf, "ERROR_OUT_OF_PAPER " ); break;
	case 29L: ::lstrcat( buf, "ERROR_WRITE_FAULT " ); break;
	case 30L: ::lstrcat( buf, "ERROR_READ_FAULT " ); break;
	case 31L: ::lstrcat( buf, "ERROR_GEN_FAILURE " ); break;
	case 32L: ::lstrcat( buf, "ERROR_SHARING_VIOLATION " ); break;
	case 33L: ::lstrcat( buf, "ERROR_LOCK_VIOLATION " ); break;
	case 34L: ::lstrcat( buf, "ERROR_WRONG_DISK " ); break;
	case 36L: ::lstrcat( buf, "ERROR_SHARING_BUFFER_EXCEEDED " ); break;
	case 38L: ::lstrcat( buf, "ERROR_HANDLE_EOF " ); break;
	case 39L: ::lstrcat( buf, "ERROR_HANDLE_DISK_FULL " ); break;
	case 50L: ::lstrcat( buf, "ERROR_NOT_SUPPORTED " ); break;
	case 51L: ::lstrcat( buf, "ERROR_REM_NOT_LIST " ); break;

	case 52L: ::lstrcat( buf, "ERROR_DUP_NAME " ); break;
	case 53L: ::lstrcat( buf, "ERROR_BAD_NETPATH " ); break;
	case 54L: ::lstrcat( buf, "ERROR_NETWORK_BUSY " ); break;
	case 55L: ::lstrcat( buf, "ERROR_DEV_NOT_EXIST " ); break;
	case 56L: ::lstrcat( buf, "ERROR_TOO_MANY_CMDS " ); break;
	case 57L: ::lstrcat( buf, "ERROR_ADAP_HDW_ERR " ); break;
	case 58L: ::lstrcat( buf, "ERROR_BAD_NET_RESP " ); break;
	case 59L: ::lstrcat( buf, "ERROR_UNEXP_NET_ERR " ); break;
	case 60L: ::lstrcat( buf, "ERROR_BAD_REM_ADAP " ); break;
	case 61L: ::lstrcat( buf, "ERROR_PRINTQ_FULL " ); break;
	case 62L: ::lstrcat( buf, "ERROR_NO_SPOOL_SPACE " ); break;
	case 63L: ::lstrcat( buf, "ERROR_PRINT_CANCELLED " ); break;

	case 64L: ::lstrcat( buf, "ERROR_NETNAME_DELETED " ); break;
	case 65L: ::lstrcat( buf, "ERROR_NETWORK_ACCESS_DENIED " ); break;
	case 66L: ::lstrcat( buf, "ERROR_BAD_DEV_TYPE " ); break;
	case 67L: ::lstrcat( buf, "ERROR_BAD_NET_NAME " ); break;
	case 68L: ::lstrcat( buf, "ERROR_TOO_MANY_NAMES " ); break;
	case 69L: ::lstrcat( buf, "ERROR_TOO_MANY_SESS " ); break;
	case 70L: ::lstrcat( buf, "ERROR_SHARING_PAUSED " ); break;
	case 71L: ::lstrcat( buf, "ERROR_REQ_NOT_ACCEP " ); break;
	case 72L: ::lstrcat( buf, "ERROR_REDIR_PAUSED " ); break;
	case 80L: ::lstrcat( buf, "ERROR_FILE_EXISTS " ); break;
	case 82L: ::lstrcat( buf, "ERROR_CANNOT_MAKE " ); break;
	case 83L: ::lstrcat( buf, "ERROR_FAIL_I24 " ); break;

	case 84L: ::lstrcat( buf, "ERROR_OUT_OF_STRUCTURES " ); break;
	case 85L: ::lstrcat( buf, "ERROR_ALREADY_ASSIGNED " ); break;
	case 86L: ::lstrcat( buf, "ERROR_INVALID_PASSWORD " ); break;
	case 87L: ::lstrcat( buf, "ERROR_INVALID_PARAMETER " ); break;
	case 88L: ::lstrcat( buf, "ERROR_NET_WRITE_FAULT " ); break;
	case 89L: ::lstrcat( buf, "ERROR_NO_PROC_SLOTS " ); break;
	case 100L: ::lstrcat( buf, "ERROR_TOO_MANY_SEMAPHORES " ); break;
	case 101L: ::lstrcat( buf, "ERROR_EXCL_SEM_ALREADY_OWNED " ); break;
	case 102L: ::lstrcat( buf, "ERROR_SEM_IS_SET " ); break;
	case 103L: ::lstrcat( buf, "ERROR_TOO_MANY_SEM_REQUESTS " ); break;
	case 104L: ::lstrcat( buf, "ERROR_INVALID_AT_INTERRUPT_TIME " ); break;

	case 105L: ::lstrcat( buf, "ERROR_SEM_OWNER_DIED " ); break;
	case 106L: ::lstrcat( buf, "ERROR_SEM_USER_LIMIT " ); break;
	case 107L: ::lstrcat( buf, "ERROR_DISK_CHANGE " ); break;
	case 108L: ::lstrcat( buf, "ERROR_DRIVE_LOCKED " ); break;
	case 109L: ::lstrcat( buf, "ERROR_BROKEN_PIPE " ); break;
	case 110L: ::lstrcat( buf, "ERROR_OPEN_FAILED " ); break;
	case 111L: ::lstrcat( buf, "ERROR_BUFFER_OVERFLOW " ); break;
	case 112L: ::lstrcat( buf, "ERROR_DISK_FULL " ); break;
	case 113L: ::lstrcat( buf, "ERROR_NO_MORE_SEARCH_HANDLES " ); break;
	case 114L: ::lstrcat( buf, "ERROR_INVALID_TARGET_HANDLE " ); break;
	case 117L: ::lstrcat( buf, "ERROR_INVALID_CATEGORY " ); break;

	case 118L: ::lstrcat( buf, "ERROR_INVALID_VERIFY_SWITCH " ); break;
	case 119L: ::lstrcat( buf, "ERROR_BAD_DRIVER_LEVEL " ); break;
	case 120L: ::lstrcat( buf, "ERROR_CALL_NOT_IMPLEMENTED " ); break;
	case 121L: ::lstrcat( buf, "ERROR_SEM_TIMEOUT " ); break;
	case 122L: ::lstrcat( buf, "ERROR_INSUFFICIENT_BUFFER " ); break;
	case 123L: ::lstrcat( buf, "ERROR_INVALID_NAME " ); break;
	case 124L: ::lstrcat( buf, "ERROR_INVALID_LEVEL " ); break;
	case 125L: ::lstrcat( buf, "ERROR_NO_VOLUME_LABEL " ); break;
	case 126L: ::lstrcat( buf, "ERROR_MOD_NOT_FOUND " ); break;
	case 127L: ::lstrcat( buf, "ERROR_PROC_NOT_FOUND " ); break;
	case 128L: ::lstrcat( buf, "ERROR_WAIT_NO_CHILDREN " ); break;

	case 129L: ::lstrcat( buf, "ERROR_CHILD_NOT_COMPLETE " ); break;
	case 130L: ::lstrcat( buf, "ERROR_DIRECT_ACCESS_HANDLE " ); break;
	case 131L: ::lstrcat( buf, "ERROR_NEGATIVE_SEEK " ); break;
	case 132L: ::lstrcat( buf, "ERROR_SEEK_ON_DEVICE " ); break;
	case 133L: ::lstrcat( buf, "ERROR_IS_JOIN_TARGET " ); break;
	case 134L: ::lstrcat( buf, "ERROR_IS_JOINED " ); break;
	case 135L: ::lstrcat( buf, "ERROR_IS_SUBSTED " ); break;
	case 136L: ::lstrcat( buf, "ERROR_NOT_JOINED " ); break;
	case 137L: ::lstrcat( buf, "ERROR_NOT_SUBSTED " ); break;
	case 138L: ::lstrcat( buf, "ERROR_JOIN_TO_JOIN " ); break;
	case 139L: ::lstrcat( buf, "ERROR_SUBST_TO_SUBST " ); break;
	case 140L: ::lstrcat( buf, "ERROR_JOIN_TO_SUBST " ); break;

	case 141L: ::lstrcat( buf, "ERROR_SUBST_TO_JOIN " ); break;
	case 142L: ::lstrcat( buf, "ERROR_BUSY_DRIVE " ); break;
	case 143L: ::lstrcat( buf, "ERROR_SAME_DRIVE " ); break;
	case 144L: ::lstrcat( buf, "ERROR_DIR_NOT_ROOT " ); break;
	case 145L: ::lstrcat( buf, "ERROR_DIR_NOT_EMPTY " ); break;
	case 146L: ::lstrcat( buf, "ERROR_IS_SUBST_PATH " ); break;
	case 147L: ::lstrcat( buf, "ERROR_IS_JOIN_PATH " ); break;
	case 148L: ::lstrcat( buf, "ERROR_PATH_BUSY " ); break;
	case 149L: ::lstrcat( buf, "ERROR_IS_SUBST_TARGET " ); break;
	case 150L: ::lstrcat( buf, "ERROR_SYSTEM_TRACE " ); break;
	case 151L: ::lstrcat( buf, "ERROR_INVALID_EVENT_COUNT " ); break;
	case 152L: ::lstrcat( buf, "ERROR_TOO_MANY_MUXWAITERS " ); break;

	case 153L: ::lstrcat( buf, "ERROR_INVALID_LIST_FORMAT " ); break;
	case 154L: ::lstrcat( buf, "ERROR_LABEL_TOO_LONG " ); break;
	case 155L: ::lstrcat( buf, "ERROR_TOO_MANY_TCBS " ); break;
	case 156L: ::lstrcat( buf, "ERROR_SIGNAL_REFUSED " ); break;
	case 157L: ::lstrcat( buf, "ERROR_DISCARDED " ); break;
	case 158L: ::lstrcat( buf, "ERROR_NOT_LOCKED " ); break;
	case 159L: ::lstrcat( buf, "ERROR_BAD_THREADID_ADDR " ); break;
	case 160L: ::lstrcat( buf, "ERROR_BAD_ARGUMENTS " ); break;
	case 161L: ::lstrcat( buf, "ERROR_BAD_PATHNAME " ); break;
	case 162L: ::lstrcat( buf, "ERROR_SIGNAL_PENDING " ); break;
	case 164L: ::lstrcat( buf, "ERROR_MAX_THRDS_REACHED " ); break;

	case 167L: ::lstrcat( buf, "ERROR_LOCK_FAILED " ); break;
	case 170L: ::lstrcat( buf, "ERROR_BUSY " ); break;
	case 173L: ::lstrcat( buf, "ERROR_CANCEL_VIOLATION " ); break;
	case 174L: ::lstrcat( buf, "ERROR_ATOMIC_LOCKS_NOT_SUPPORTED " ); break;
	case 180L: ::lstrcat( buf, "ERROR_INVALID_SEGMENT_NUMBER " ); break;
	case 182L: ::lstrcat( buf, "ERROR_INVALID_ORDINAL " ); break;
	case 183L: ::lstrcat( buf, "ERROR_ALREADY_EXISTS " ); break;
	case 186L: ::lstrcat( buf, "ERROR_INVALID_FLAG_NUMBER " ); break;
	case 187L: ::lstrcat( buf, "ERROR_SEM_NOT_FOUND " ); break;
	case 188L: ::lstrcat( buf, "ERROR_INVALID_STARTING_CODESEG " ); break;
	case 189L: ::lstrcat( buf, "ERROR_INVALID_STACKSEG " ); break;

	case 190L: ::lstrcat( buf, "ERROR_INVALID_MODULETYPE " ); break;
	case 191L: ::lstrcat( buf, "ERROR_INVALID_EXE_SIGNATURE " ); break;
	case 192L: ::lstrcat( buf, "ERROR_EXE_MARKED_INVALID " ); break;
	case 193L: ::lstrcat( buf, "ERROR_BAD_EXE_FORMAT " ); break;
	case 194L: ::lstrcat( buf, "ERROR_ITERATED_DATA_EXCEEDS_64k " ); break;
	case 195L: ::lstrcat( buf, "ERROR_INVALID_MINALLOCSIZE " ); break;
	case 196L: ::lstrcat( buf, "ERROR_DYNLINK_FROM_INVALID_RING " ); break;
	case 197L: ::lstrcat( buf, "ERROR_IOPL_NOT_ENABLED " ); break;
	case 198L: ::lstrcat( buf, "ERROR_INVALID_SEGDPL " ); break;
	case 199L: ::lstrcat( buf, "ERROR_AUTODATASEG_EXCEEDS_64k " ); break;
	case 200L: ::lstrcat( buf, "ERROR_RING2SEG_MUST_BE_MOVABLE " ); break;

	case 201L: ::lstrcat( buf, "ERROR_RELOC_CHAIN_XEEDS_SEGLIM " ); break;
	case 202L: ::lstrcat( buf, "ERROR_INFLOOP_IN_RELOC_CHAIN " ); break;
	case 203L: ::lstrcat( buf, "ERROR_ENVVAR_NOT_FOUND " ); break;
	case 205L: ::lstrcat( buf, "ERROR_NO_SIGNAL_SENT " ); break;
	case 206L: ::lstrcat( buf, "ERROR_FILENAME_EXCED_RANGE " ); break;
	case 207L: ::lstrcat( buf, "ERROR_RING2_STACK_IN_USE " ); break;
	case 208L: ::lstrcat( buf, "ERROR_META_EXPANSION_TOO_LONG " ); break;
	case 209L: ::lstrcat( buf, "ERROR_INVALID_SIGNAL_NUMBER " ); break;
	case 210L: ::lstrcat( buf, "ERROR_THREAD_1_INACTIVE " ); break;
	case 212L: ::lstrcat( buf, "ERROR_LOCKED " ); break;

	case 214L: ::lstrcat( buf, "ERROR_TOO_MANY_MODULES " ); break;
	case 215L: ::lstrcat( buf, "ERROR_NESTING_NOT_ALLOWED " ); break;
	case 230L: ::lstrcat( buf, "ERROR_BAD_PIPE " ); break;
	case 231L: ::lstrcat( buf, "ERROR_PIPE_BUSY " ); break;
	case 232L: ::lstrcat( buf, "ERROR_NO_DATA " ); break;
	case 233L: ::lstrcat( buf, "ERROR_PIPE_NOT_CONNECTED " ); break;
	case 234L: ::lstrcat( buf, "ERROR_MORE_DATA " ); break;
	case 240L: ::lstrcat( buf, "ERROR_VC_DISCONNECTED " ); break;
	case 254L: ::lstrcat( buf, "ERROR_INVALID_EA_NAME " ); break;
	case 255L: ::lstrcat( buf, "ERROR_EA_LIST_INCONSISTENT " ); break;
	case 259L: ::lstrcat( buf, "ERROR_NO_MORE_ITEMS " ); break;

	case 266L: ::lstrcat( buf, "ERROR_CANNOT_COPY " ); break;
	case 267L: ::lstrcat( buf, "ERROR_DIRECTORY " ); break;
	case 275L: ::lstrcat( buf, "ERROR_EAS_DIDNT_FIT " ); break;
	case 276L: ::lstrcat( buf, "ERROR_EA_FILE_CORRUPT " ); break;
	case 277L: ::lstrcat( buf, "ERROR_EA_TABLE_FULL " ); break;
	case 278L: ::lstrcat( buf, "ERROR_INVALID_EA_HANDLE " ); break;
	case 282L: ::lstrcat( buf, "ERROR_EAS_NOT_SUPPORTED " ); break;
	case 288L: ::lstrcat( buf, "ERROR_NOT_OWNER " ); break;
	case 298L: ::lstrcat( buf, "ERROR_TOO_MANY_POSTS " ); break;
	case 317L: ::lstrcat( buf, "ERROR_MR_MID_NOT_FOUND " ); break;
	case 487L: ::lstrcat( buf, "ERROR_INVALID_ADDRESS " ); break;

	case 534L: ::lstrcat( buf, "ERROR_ARITHMETIC_OVERFLOW " ); break;
	case 535L: ::lstrcat( buf, "ERROR_PIPE_CONNECTED " ); break;
	case 536L: ::lstrcat( buf, "ERROR_PIPE_LISTENING " ); break;
	case 994L: ::lstrcat( buf, "ERROR_EA_ACCESS_DENIED " ); break;
	case 995L: ::lstrcat( buf, "ERROR_OPERATION_ABORTED " ); break;
	case 996L: ::lstrcat( buf, "ERROR_IO_INCOMPLETE " ); break;
	case 997L: ::lstrcat( buf, "ERROR_IO_PENDING " ); break;
	case 998L: ::lstrcat( buf, "ERROR_NOACCESS " ); break;
	case 999L: ::lstrcat( buf, "ERROR_SWAPERROR " ); break;
	case 1001L: ::lstrcat( buf, "ERROR_STACK_OVERFLOW " ); break;
	case 1002L: ::lstrcat( buf, "ERROR_INVALID_MESSAGE " ); break;

	case 1003L: ::lstrcat( buf, "ERROR_CAN_NOT_COMPLETE " ); break;
	case 1004L: ::lstrcat( buf, "ERROR_INVALID_FLAGS " ); break;
	case 1005L: ::lstrcat( buf, "ERROR_UNRECOGNIZED_VOLUME " ); break;
	case 1006L: ::lstrcat( buf, "ERROR_FILE_INVALID " ); break;
	case 1007L: ::lstrcat( buf, "ERROR_FULLSCREEN_MODE " ); break;
	case 1008L: ::lstrcat( buf, "ERROR_NO_TOKEN " ); break;
	case 1009L: ::lstrcat( buf, "ERROR_BADDB " ); break;
	case 1010L: ::lstrcat( buf, "ERROR_BADKEY " ); break;
	case 1011L: ::lstrcat( buf, "ERROR_CANTOPEN " ); break;
	case 1012L: ::lstrcat( buf, "ERROR_CANTREAD " ); break;
	case 1013L: ::lstrcat( buf, "ERROR_CANTWRITE " ); break;
	case 1014L: ::lstrcat( buf, "ERROR_REGISTRY_RECOVERED " ); break;

	case 1015L: ::lstrcat( buf, "ERROR_REGISTRY_CORRUPT " ); break;
	case 1016L: ::lstrcat( buf, "ERROR_REGISTRY_IO_FAILED " ); break;
	case 1017L: ::lstrcat( buf, "ERROR_NOT_REGISTRY_FILE " ); break;
	case 1018L: ::lstrcat( buf, "ERROR_KEY_DELETED " ); break;
	case 1019L: ::lstrcat( buf, "ERROR_NO_LOG_SPACE " ); break;
	case 1020L: ::lstrcat( buf, "ERROR_KEY_HAS_CHILDREN " ); break;
	case 1021L: ::lstrcat( buf, "ERROR_CHILD_MUST_BE_VOLATILE " ); break;
	case 1022L: ::lstrcat( buf, "ERROR_NOTIFY_ENUM_DIR " ); break;
	case 1051L: ::lstrcat( buf, "ERROR_DEPENDENT_SERVICES_RUNNING " ); break;
	case 1052L: ::lstrcat( buf, "ERROR_INVALID_SERVICE_CONTROL " ); break;

	case 1053L: ::lstrcat( buf, "ERROR_SERVICE_REQUEST_TIMEOUT " ); break;
	case 1054L: ::lstrcat( buf, "ERROR_SERVICE_NO_THREAD " ); break;
	case 1055L: ::lstrcat( buf, "ERROR_SERVICE_DATABASE_LOCKED " ); break;
	case 1056L: ::lstrcat( buf, "ERROR_SERVICE_ALREADY_RUNNING " ); break;
	case 1057L: ::lstrcat( buf, "ERROR_INVALID_SERVICE_ACCOUNT " ); break;
	case 1058L: ::lstrcat( buf, "ERROR_SERVICE_DISABLED " ); break;
	case 1059L: ::lstrcat( buf, "ERROR_CIRCULAR_DEPENDENCY " ); break;
	case 1060L: ::lstrcat( buf, "ERROR_SERVICE_DOES_NOT_EXIST " ); break;
	case 1061L: ::lstrcat( buf, "ERROR_SERVICE_CANNOT_ACCEPT_CTRL " ); break;
	case 1062L: ::lstrcat( buf, "ERROR_SERVICE_NOT_ACTIVE " ); break;

	case 1063L: ::lstrcat( buf, "ERROR_FAILED_SERVICE_CONTROLLER_CONNECT " ); break;
	case 1064L: ::lstrcat( buf, "ERROR_EXCEPTION_IN_SERVICE " ); break;
	case 1065L: ::lstrcat( buf, "ERROR_DATABASE_DOES_NOT_EXIST " ); break;
	case 1066L: ::lstrcat( buf, "ERROR_SERVICE_SPECIFIC_ERROR " ); break;
	case 1067L: ::lstrcat( buf, "ERROR_PROCESS_ABORTED " ); break;
	case 1068L: ::lstrcat( buf, "ERROR_SERVICE_DEPENDENCY_FAIL " ); break;
	case 1069L: ::lstrcat( buf, "ERROR_SERVICE_LOGON_FAILED " ); break;
	case 1070L: ::lstrcat( buf, "ERROR_SERVICE_START_HANG " ); break;
	case 1071L: ::lstrcat( buf, "ERROR_INVALID_SERVICE_LOCK " ); break;
	case 1072L: ::lstrcat( buf, "ERROR_SERVICE_MARKED_FOR_DELETE " ); break;

	case 1073L: ::lstrcat( buf, "ERROR_SERVICE_EXISTS " ); break;
	case 1074L: ::lstrcat( buf, "ERROR_ALREADY_RUNNING_LKG " ); break;
	case 1075L: ::lstrcat( buf, "ERROR_SERVICE_DEPENDENCY_DELETED" ); break;
	case 1076L: ::lstrcat( buf, "ERROR_BOOT_ALREADY_ACCEPTED " ); break;
	case 1077L: ::lstrcat( buf, "ERROR_SERVICE_NEVER_STARTED " ); break;
	case 1078L: ::lstrcat( buf, "ERROR_DUPLICATE_SERVICE_NAME " ); break;
	case 1100L: ::lstrcat( buf, "ERROR_END_OF_MEDIA " ); break;
	case 1101L: ::lstrcat( buf, "ERROR_FILEMARK_DETECTED " ); break;
	case 1102L: ::lstrcat( buf, "ERROR_BEGINNING_OF_MEDIA " ); break;
	case 1103L: ::lstrcat( buf, "ERROR_SETMARK_DETECTED " ); break;

	case 1104L: ::lstrcat( buf, "ERROR_NO_DATA_DETECTED " ); break;
	case 1105L: ::lstrcat( buf, "ERROR_PARTITION_FAILURE " ); break;
	case 1106L: ::lstrcat( buf, "ERROR_INVALID_BLOCK_LENGTH " ); break;
	case 1107L: ::lstrcat( buf, "ERROR_DEVICE_NOT_PARTITIONED " ); break;
	case 1108L: ::lstrcat( buf, "ERROR_UNABLE_TO_LOCK_MEDIA " ); break;
	case 1109L: ::lstrcat( buf, "ERROR_UNABLE_TO_UNLOAD_MEDIA " ); break;
	case 1110L: ::lstrcat( buf, "ERROR_MEDIA_CHANGED " ); break;
	case 1111L: ::lstrcat( buf, "ERROR_BUS_RESET " ); break;
	case 1112L: ::lstrcat( buf, "ERROR_NO_MEDIA_IN_DRIVE " ); break;
	case 1113L: ::lstrcat( buf, "ERROR_NO_UNICODE_TRANSLATION " ); break;

	case 1114L: ::lstrcat( buf, "ERROR_DLL_INIT_FAILED " ); break;
	case 1115L: ::lstrcat( buf, "ERROR_SHUTDOWN_IN_PROGRESS " ); break;
	case 1116L: ::lstrcat( buf, "ERROR_NO_SHUTDOWN_IN_PROGRESS " ); break;
	case 1117L: ::lstrcat( buf, "ERROR_IO_DEVICE " ); break;
	case 1118L: ::lstrcat( buf, "ERROR_SERIAL_NO_DEVICE " ); break;
	case 1119L: ::lstrcat( buf, "ERROR_IRQ_BUSY " ); break;
	case 1120L: ::lstrcat( buf, "ERROR_MORE_WRITES " ); break;
	case 1121L: ::lstrcat( buf, "ERROR_COUNTER_TIMEOUT " ); break;
	case 1122L: ::lstrcat( buf, "ERROR_FLOPPY_ID_MARK_NOT_FOUND " ); break;
	case 1123L: ::lstrcat( buf, "ERROR_FLOPPY_WRONG_CYLINDER " ); break;
	case 1124L: ::lstrcat( buf, "ERROR_FLOPPY_UNKNOWN_ERROR " ); break;

	case 1125L: ::lstrcat( buf, "ERROR_FLOPPY_BAD_REGISTERS " ); break;
	case 1126L: ::lstrcat( buf, "ERROR_DISK_RECALIBRATE_FAILED " ); break;
	case 1127L: ::lstrcat( buf, "ERROR_DISK_OPERATION_FAILED " ); break;
	case 1128L: ::lstrcat( buf, "ERROR_DISK_RESET_FAILED " ); break;
	case 1129L: ::lstrcat( buf, "ERROR_EOM_OVERFLOW " ); break;
	case 1130L: ::lstrcat( buf, "ERROR_NOT_ENOUGH_SERVER_MEMORY " ); break;
	case 1131L: ::lstrcat( buf, "ERROR_POSSIBLE_DEADLOCK " ); break;
	case 1200L: ::lstrcat( buf, "ERROR_BAD_DEVICE " ); break;
	case 1201L: ::lstrcat( buf, "ERROR_CONNECTION_UNAVAIL " ); break;
	case 1202L: ::lstrcat( buf, "ERROR_DEVICE_ALREADY_REMEMBERED " ); break;

	case 1203L: ::lstrcat( buf, "ERROR_NO_NET_OR_BAD_PATH " ); break;
	case 1204L: ::lstrcat( buf, "ERROR_BAD_PROVIDER " ); break;
	case 1205L: ::lstrcat( buf, "ERROR_CANNOT_OPEN_PROFILE " ); break;
	case 1206L: ::lstrcat( buf, "ERROR_BAD_PROFILE " ); break;
	case 1207L: ::lstrcat( buf, "ERROR_NOT_CONTAINER " ); break;
	case 1208L: ::lstrcat( buf, "ERROR_EXTENDED_ERROR " ); break;
	case 1209L: ::lstrcat( buf, "ERROR_INVALID_GROUPNAME " ); break;
	case 1210L: ::lstrcat( buf, "ERROR_INVALID_COMPUTERNAME " ); break;
	case 1211L: ::lstrcat( buf, "ERROR_INVALID_EVENTNAME " ); break;
	case 1212L: ::lstrcat( buf, "ERROR_INVALID_DOMAINNAME " ); break;
	case 1213L: ::lstrcat( buf, "ERROR_INVALID_SERVICENAME " ); break;

	case 1214L: ::lstrcat( buf, "ERROR_INVALID_NETNAME " ); break;
	case 1215L: ::lstrcat( buf, "ERROR_INVALID_SHARENAME " ); break;
	case 1216L: ::lstrcat( buf, "ERROR_INVALID_PASSWORDNAME " ); break;
	case 1217L: ::lstrcat( buf, "ERROR_INVALID_MESSAGENAME " ); break;
	case 1218L: ::lstrcat( buf, "ERROR_INVALID_MESSAGEDEST " ); break;
	case 1219L: ::lstrcat( buf, "ERROR_SESSION_CREDENTIAL_CONFLICT " ); break;
	case 1220L: ::lstrcat( buf, "ERROR_REMOTE_SESSION_LIMIT_EXCEEDED " ); break;
	case 1221L: ::lstrcat( buf, "ERROR_DUP_DOMAINNAME " ); break;
	case 1300L: ::lstrcat( buf, "ERROR_NOT_ALL_ASSIGNED " ); break;
	case 1301L: ::lstrcat( buf, "ERROR_SOME_NOT_MAPPED " ); break;

	case 1302L: ::lstrcat( buf, "ERROR_NO_QUOTAS_FOR_ACCOUNT " ); break;
	case 1303L: ::lstrcat( buf, "ERROR_LOCAL_USER_SESSION_KEY " ); break;
	case 1304L: ::lstrcat( buf, "ERROR_NULL_LM_PASSWORD " ); break;
	case 1305L: ::lstrcat( buf, "ERROR_UNKNOWN_REVISION " ); break;
	case 1306L: ::lstrcat( buf, "ERROR_REVISION_MISMATCH " ); break;
	case 1307L: ::lstrcat( buf, "ERROR_INVALID_OWNER " ); break;
	case 1308L: ::lstrcat( buf, "ERROR_INVALID_PRIMARY_GROUP " ); break;
	case 1309L: ::lstrcat( buf, "ERROR_NO_IMPERSONATION_TOKEN " ); break;
	case 1310L: ::lstrcat( buf, "ERROR_CANT_DISABLE_MANDATORY " ); break;
	case 1311L: ::lstrcat( buf, "ERROR_NO_LOGON_SERVERS " ); break;

	case 1312L: ::lstrcat( buf, "ERROR_NO_SUCH_LOGON_SESSION " ); break;
	case 1313L: ::lstrcat( buf, "ERROR_NO_SUCH_PRIVILEGE " ); break;
	case 1314L: ::lstrcat( buf, "ERROR_PRIVILEGE_NOT_HELD " ); break;
	case 1315L: ::lstrcat( buf, "ERROR_INVALID_ACCOUNT_NAME " ); break;
	case 1316L: ::lstrcat( buf, "ERROR_USER_EXISTS " ); break;
	case 1317L: ::lstrcat( buf, "ERROR_NO_SUCH_USER " ); break;
	case 1318L: ::lstrcat( buf, "ERROR_GROUP_EXISTS " ); break;
	case 1319L: ::lstrcat( buf, "ERROR_NO_SUCH_GROUP " ); break;
	case 1320L: ::lstrcat( buf, "ERROR_MEMBER_IN_GROUP " ); break;
	case 1321L: ::lstrcat( buf, "ERROR_MEMBER_NOT_IN_GROUP " ); break;
	case 1322L: ::lstrcat( buf, "ERROR_LAST_ADMIN " ); break;

	case 1323L: ::lstrcat( buf, "ERROR_WRONG_PASSWORD " ); break;
	case 1324L: ::lstrcat( buf, "ERROR_ILL_FORMED_PASSWORD " ); break;
	case 1325L: ::lstrcat( buf, "ERROR_PASSWORD_RESTRICTION " ); break;
	case 1326L: ::lstrcat( buf, "ERROR_LOGON_FAILURE " ); break;
	case 1327L: ::lstrcat( buf, "ERROR_ACCOUNT_RESTRICTION " ); break;
	case 1328L: ::lstrcat( buf, "ERROR_INVALID_LOGON_HOURS " ); break;
	case 1329L: ::lstrcat( buf, "ERROR_INVALID_WORKSTATION " ); break;
	case 1330L: ::lstrcat( buf, "ERROR_PASSWORD_EXPIRED " ); break;
	case 1331L: ::lstrcat( buf, "ERROR_ACCOUNT_DISABLED " ); break;
	case 1332L: ::lstrcat( buf, "ERROR_NONE_MAPPED " ); break;
	case 1333L: ::lstrcat( buf, "ERROR_TOO_MANY_LUIDS_REQUESTED " ); break;

	case 1334L: ::lstrcat( buf, "ERROR_LUIDS_EXHAUSTED " ); break;
	case 1335L: ::lstrcat( buf, "ERROR_INVALID_SUB_AUTHORITY " ); break;
	case 1336L: ::lstrcat( buf, "ERROR_INVALID_ACL " ); break;
	case 1337L: ::lstrcat( buf, "ERROR_INVALID_SID " ); break;
	case 1338L: ::lstrcat( buf, "ERROR_INVALID_SECURITY_DESCR " ); break;
	case 1340L: ::lstrcat( buf, "ERROR_BAD_INHERITANCE_ACL " ); break;
	case 1341L: ::lstrcat( buf, "ERROR_SERVER_DISABLED " ); break;
	case 1342L: ::lstrcat( buf, "ERROR_SERVER_NOT_DISABLED " ); break;
	case 1343L: ::lstrcat( buf, "ERROR_INVALID_ID_AUTHORITY " ); break;
	case 1344L: ::lstrcat( buf, "ERROR_ALLOTTED_SPACE_EXCEEDED " ); break;

	case 1345L: ::lstrcat( buf, "ERROR_INVALID_GROUP_ATTRIBUTES " ); break;
	case 1346L: ::lstrcat( buf, "ERROR_BAD_IMPERSONATION_LEVEL " ); break;
	case 1347L: ::lstrcat( buf, "ERROR_CANT_OPEN_ANONYMOUS " ); break;
	case 1348L: ::lstrcat( buf, "ERROR_BAD_VALIDATION_CLASS " ); break;
	case 1349L: ::lstrcat( buf, "ERROR_BAD_TOKEN_TYPE " ); break;
	case 1350L: ::lstrcat( buf, "ERROR_NO_SECURITY_ON_OBJECT " ); break;
	case 1351L: ::lstrcat( buf, "ERROR_CANT_ACCESS_DOMAIN_INFO " ); break;
	case 1352L: ::lstrcat( buf, "ERROR_INVALID_SERVER_STATE " ); break;
	case 1353L: ::lstrcat( buf, "ERROR_INVALID_DOMAIN_STATE " ); break;
	case 1354L: ::lstrcat( buf, "ERROR_INVALID_DOMAIN_ROLE " ); break;

	case 1355L: ::lstrcat( buf, "ERROR_NO_SUCH_DOMAIN " ); break;
	case 1356L: ::lstrcat( buf, "ERROR_DOMAIN_EXISTS " ); break;
	case 1357L: ::lstrcat( buf, "ERROR_DOMAIN_LIMIT_EXCEEDED " ); break;
	case 1358L: ::lstrcat( buf, "ERROR_INTERNAL_DB_CORRUPTION " ); break;
	case 1359L: ::lstrcat( buf, "ERROR_INTERNAL_ERROR " ); break;
	case 1360L: ::lstrcat( buf, "ERROR_GENERIC_NOT_MAPPED " ); break;
	case 1361L: ::lstrcat( buf, "ERROR_BAD_DESCRIPTOR_FORMAT " ); break;
	case 1362L: ::lstrcat( buf, "ERROR_NOT_LOGON_PROCESS " ); break;
	case 1363L: ::lstrcat( buf, "ERROR_LOGON_SESSION_EXISTS " ); break;
	case 1364L: ::lstrcat( buf, "ERROR_NO_SUCH_PACKAGE " ); break;

	case 1365L: ::lstrcat( buf, "ERROR_BAD_LOGON_SESSION_STATE " ); break;
	case 1366L: ::lstrcat( buf, "ERROR_LOGON_SESSION_COLLISION " ); break;
	case 1367L: ::lstrcat( buf, "ERROR_INVALID_LOGON_TYPE " ); break;
	case 1368L: ::lstrcat( buf, "ERROR_CANNOT_IMPERSONATE " ); break;
	case 1369L: ::lstrcat( buf, "ERROR_RXACT_INVALID_STATE " ); break;
	case 1370L: ::lstrcat( buf, "ERROR_RXACT_COMMIT_FAILURE " ); break;
	case 1371L: ::lstrcat( buf, "ERROR_SPECIAL_ACCOUNT " ); break;
	case 1372L: ::lstrcat( buf, "ERROR_SPECIAL_GROUP " ); break;
	case 1373L: ::lstrcat( buf, "ERROR_SPECIAL_USER " ); break;
	case 1374L: ::lstrcat( buf, "ERROR_MEMBERS_PRIMARY_GROUP " ); break;

	case 1375L: ::lstrcat( buf, "ERROR_TOKEN_ALREADY_IN_USE " ); break;
	case 1376L: ::lstrcat( buf, "ERROR_NO_SUCH_ALIAS " ); break;
	case 1377L: ::lstrcat( buf, "ERROR_MEMBER_NOT_IN_ALIAS " ); break;
	case 1378L: ::lstrcat( buf, "ERROR_MEMBER_IN_ALIAS " ); break;
	case 1379L: ::lstrcat( buf, "ERROR_ALIAS_EXISTS " ); break;
	case 1380L: ::lstrcat( buf, "ERROR_LOGON_NOT_GRANTED " ); break;
	case 1381L: ::lstrcat( buf, "ERROR_TOO_MANY_SECRETS " ); break;
	case 1382L: ::lstrcat( buf, "ERROR_SECRET_TOO_LONG " ); break;
	case 1383L: ::lstrcat( buf, "ERROR_INTERNAL_DB_ERROR " ); break;
	case 1384L: ::lstrcat( buf, "ERROR_TOO_MANY_CONTEXT_IDS " ); break;
	case 1385L: ::lstrcat( buf, "ERROR_LOGON_TYPE_NOT_GRANTED " ); break;

	case 1386L: ::lstrcat( buf, "ERROR_NT_CROSS_ENCRYPTION_REQUIRED " ); break;
	case 1387L: ::lstrcat( buf, "ERROR_NO_SUCH_MEMBER " ); break;
	case 1388L: ::lstrcat( buf, "ERROR_INVALID_MEMBER " ); break;
	case 1389L: ::lstrcat( buf, "ERROR_TOO_MANY_SIDS " ); break;
	case 1390L: ::lstrcat( buf, "ERROR_LM_CROSS_ENCRYPTION_REQUIRED " ); break;
	case 1391L: ::lstrcat( buf, "ERROR_NO_INHERITANCE " ); break;
	case 1392L: ::lstrcat( buf, "ERROR_FILE_CORRUPT " ); break;
	case 1393L: ::lstrcat( buf, "ERROR_DISK_CORRUPT " ); break;
	case 1394L: ::lstrcat( buf, "ERROR_NO_USER_SESSION_KEY " ); break;
	case 1400L: ::lstrcat( buf, "ERROR_INVALID_WINDOW_HANDLE " ); break;

	case 1401L: ::lstrcat( buf, "ERROR_INVALID_MENU_HANDLE " ); break;
	case 1402L: ::lstrcat( buf, "ERROR_INVALID_CURSOR_HANDLE " ); break;
	case 1403L: ::lstrcat( buf, "ERROR_INVALID_ACCEL_HANDLE " ); break;
	case 1404L: ::lstrcat( buf, "ERROR_INVALID_HOOK_HANDLE " ); break;
	case 1405L: ::lstrcat( buf, "ERROR_INVALID_DWP_HANDLE " ); break;
	case 1406L: ::lstrcat( buf, "ERROR_TLW_WITH_WSCHILD " ); break;
	case 1407L: ::lstrcat( buf, "ERROR_CANNOT_FIND_WND_CLASS " ); break;
	case 1408L: ::lstrcat( buf, "ERROR_WINDOW_OF_OTHER_THREAD " ); break;
	case 1409L: ::lstrcat( buf, "ERROR_HOTKEY_ALREADY_REGISTERED " ); break;
	case 1410L: ::lstrcat( buf, "ERROR_CLASS_ALREADY_EXISTS " ); break;

	case 1411L: ::lstrcat( buf, "ERROR_CLASS_DOES_NOT_EXIST " ); break;
	case 1412L: ::lstrcat( buf, "ERROR_CLASS_HAS_WINDOWS " ); break;
	case 1413L: ::lstrcat( buf, "ERROR_INVALID_INDEX " ); break;
	case 1414L: ::lstrcat( buf, "ERROR_INVALID_ICON_HANDLE " ); break;
	case 1415L: ::lstrcat( buf, "ERROR_PRIVATE_DIALOG_INDEX " ); break;
	case 1416L: ::lstrcat( buf, "ERROR_LISTBOX_ID_NOT_FOUND " ); break;
	case 1417L: ::lstrcat( buf, "ERROR_NO_WILDCARD_CHARACTERS " ); break;
	case 1418L: ::lstrcat( buf, "ERROR_CLIPBOARD_NOT_OPEN " ); break;
	case 1419L: ::lstrcat( buf, "ERROR_HOTKEY_NOT_REGISTERED " ); break;
	case 1420L: ::lstrcat( buf, "ERROR_WINDOW_NOT_DIALOG " ); break;

	case 1421L: ::lstrcat( buf, "ERROR_CONTROL_ID_NOT_FOUND " ); break;
	case 1422L: ::lstrcat( buf, "ERROR_INVALID_COMBOBOX_MESSAGE " ); break;
	case 1423L: ::lstrcat( buf, "ERROR_WINDOW_NOT_COMBOBOX " ); break;
	case 1424L: ::lstrcat( buf, "ERROR_INVALID_EDIT_HEIGHT " ); break;
	case 1425L: ::lstrcat( buf, "ERROR_DC_NOT_FOUND " ); break;
	case 1426L: ::lstrcat( buf, "ERROR_INVALID_HOOK_FILTER " ); break;
	case 1427L: ::lstrcat( buf, "ERROR_INVALID_FILTER_PROC " ); break;
	case 1428L: ::lstrcat( buf, "ERROR_HOOK_NEEDS_HMOD " ); break;
	case 1429L: ::lstrcat( buf, "ERROR_GLOBAL_ONLY_HOOK " ); break;
	case 1430L: ::lstrcat( buf, "ERROR_JOURNAL_HOOK_SET " ); break;

	case 1431L: ::lstrcat( buf, "ERROR_HOOK_NOT_INSTALLED " ); break;
	case 1432L: ::lstrcat( buf, "ERROR_INVALID_LB_MESSAGE " ); break;
	case 1433L: ::lstrcat( buf, "ERROR_SETCOUNT_ON_BAD_LB " ); break;
	case 1434L: ::lstrcat( buf, "ERROR_LB_WITHOUT_TABSTOPS " ); break;
	case 1435L: ::lstrcat( buf, "ERROR_DESTROY_OBJECT_OF_OTHER_THREAD " ); break;
	case 1436L: ::lstrcat( buf, "ERROR_CHILD_WINDOW_MENU " ); break;
	case 1437L: ::lstrcat( buf, "ERROR_NO_SYSTEM_MENU " ); break;
	case 1438L: ::lstrcat( buf, "ERROR_INVALID_MSGBOX_STYLE " ); break;
	case 1439L: ::lstrcat( buf, "ERROR_INVALID_SPI_VALUE " ); break;
	case 1440L: ::lstrcat( buf, "ERROR_SCREEN_ALREADY_LOCKED " ); break;

	case 1441L: ::lstrcat( buf, "ERROR_HWNDS_HAVE_DIFFERENT_PARENT " ); break;
	case 1442L: ::lstrcat( buf, "ERROR_NOT_CHILD_WINDOW " ); break;
	case 1443L: ::lstrcat( buf, "ERROR_INVALID_GW_COMMAND " ); break;
	case 1444L: ::lstrcat( buf, "ERROR_INVALID_THREAD_ID " ); break;
	case 1445L: ::lstrcat( buf, "ERROR_NON_MDICHILD_WINDOW " ); break;
	case 1446L: ::lstrcat( buf, "ERROR_POPUP_ALREADY_ACTIVE " ); break;
	case 1447L: ::lstrcat( buf, "ERROR_NO_SCROLLBARS " ); break;
	case 1448L: ::lstrcat( buf, "ERROR_INVALID_SCROLLBAR_RANGE " ); break;
	case 1449L: ::lstrcat( buf, "ERROR_INVALID_SHOWWIN_COMMAND " ); break;
	case 1500L: ::lstrcat( buf, "ERROR_EVENTLOG_FILE_CORRUPT " ); break;

	case 1501L: ::lstrcat( buf, "ERROR_EVENTLOG_CANT_START " ); break;
	case 1502L: ::lstrcat( buf, "ERROR_LOG_FILE_FULL " ); break;
	case 1503L: ::lstrcat( buf, "ERROR_EVENTLOG_FILE_CHANGED " ); break;
	case 1700L: ::lstrcat( buf, "RPC_S_INVALID_STRING_BINDING " ); break;
	case 1701L: ::lstrcat( buf, "RPC_S_WRONG_KIND_OF_BINDING " ); break;
	case 1702L: ::lstrcat( buf, "RPC_S_INVALID_BINDING " ); break;
	case 1703L: ::lstrcat( buf, "RPC_S_PROTSEQ_NOT_SUPPORTED " ); break;
	case 1704L: ::lstrcat( buf, "RPC_S_INVALID_RPC_PROTSEQ " ); break;
	case 1705L: ::lstrcat( buf, "RPC_S_INVALID_STRING_UUID " ); break;
	case 1706L: ::lstrcat( buf, "RPC_S_INVALID_ENDPOINT_FORMAT " ); break;

	case 1707L: ::lstrcat( buf, "RPC_S_INVALID_NET_ADDR " ); break;
	case 1708L: ::lstrcat( buf, "RPC_S_NO_ENDPOINT_FOUND " ); break;
	case 1709L: ::lstrcat( buf, "RPC_S_INVALID_TIMEOUT " ); break;
	case 1710L: ::lstrcat( buf, "RPC_S_OBJECT_NOT_FOUND " ); break;
	case 1711L: ::lstrcat( buf, "RPC_S_ALREADY_REGISTERED " ); break;
	case 1712L: ::lstrcat( buf, "RPC_S_TYPE_ALREADY_REGISTERED " ); break;
	case 1713L: ::lstrcat( buf, "RPC_S_ALREADY_LISTENING " ); break;
	case 1714L: ::lstrcat( buf, "RPC_S_NO_PROTSEQS_REGISTERED " ); break;
	case 1715L: ::lstrcat( buf, "RPC_S_NOT_LISTENING " ); break;
	case 1716L: ::lstrcat( buf, "RPC_S_UNKNOWN_MGR_TYPE " ); break;

	case 1717L: ::lstrcat( buf, "RPC_S_UNKNOWN_IF " ); break;
	case 1718L: ::lstrcat( buf, "RPC_S_NO_BINDINGS " ); break;
	case 1719L: ::lstrcat( buf, "RPC_S_NO_PROTSEQS " ); break;
	case 1720L: ::lstrcat( buf, "RPC_S_CANT_CREATE_ENDPOINT " ); break;
	case 1721L: ::lstrcat( buf, "RPC_S_OUT_OF_RESOURCES " ); break;
	case 1722L: ::lstrcat( buf, "RPC_S_SERVER_UNAVAILABLE " ); break;
	case 1723L: ::lstrcat( buf, "RPC_S_SERVER_TOO_BUSY " ); break;
	case 1724L: ::lstrcat( buf, "RPC_S_INVALID_NETWORK_OPTIONS " ); break;
	case 1725L: ::lstrcat( buf, "RPC_S_NO_CALL_ACTIVE " ); break;
	case 1726L: ::lstrcat( buf, "RPC_S_CALL_FAILED " ); break;
	case 1727L: ::lstrcat( buf, "RPC_S_CALL_FAILED_DNE " ); break;

	case 1728L: ::lstrcat( buf, "RPC_S_PROTOCOL_ERROR " ); break;
	case 1730L: ::lstrcat( buf, "RPC_S_UNSUPPORTED_TRANS_SYN " ); break;
	case 1731L: ::lstrcat( buf, "RPC_S_SERVER_OUT_OF_MEMORY " ); break;
	case 1732L: ::lstrcat( buf, "RPC_S_UNSUPPORTED_TYPE " ); break;
	case 1733L: ::lstrcat( buf, "RPC_S_INVALID_TAG " ); break;
	case 1734L: ::lstrcat( buf, "RPC_S_INVALID_BOUND " ); break;
	case 1735L: ::lstrcat( buf, "RPC_S_NO_ENTRY_NAME " ); break;
	case 1736L: ::lstrcat( buf, "RPC_S_INVALID_NAME_SYNTAX " ); break;
	case 1737L: ::lstrcat( buf, "RPC_S_UNSUPPORTED_NAME_SYNTAX " ); break;
	case 1739L: ::lstrcat( buf, "RPC_S_UUID_NO_ADDRESS " ); break;
	case 1740L: ::lstrcat( buf, "RPC_S_DUPLICATE_ENDPOINT " ); break;

	case 1741L: ::lstrcat( buf, "RPC_S_UNKNOWN_AUTHN_TYPE " ); break;
	case 1742L: ::lstrcat( buf, "RPC_S_MAX_CALLS_TOO_SMALL " ); break;
	case 1743L: ::lstrcat( buf, "RPC_S_STRING_TOO_LONG " ); break;
	case 1744L: ::lstrcat( buf, "RPC_S_PROTSEQ_NOT_FOUND " ); break;
	case 1745L: ::lstrcat( buf, "RPC_S_PROCNUM_OUT_OF_RANGE " ); break;
	case 1746L: ::lstrcat( buf, "RPC_S_BINDING_HAS_NO_AUTH " ); break;
	case 1747L: ::lstrcat( buf, "RPC_S_UNKNOWN_AUTHN_SERVICE " ); break;
	case 1748L: ::lstrcat( buf, "RPC_S_UNKNOWN_AUTHN_LEVEL " ); break;
	case 1749L: ::lstrcat( buf, "RPC_S_INVALID_AUTH_IDENTITY " ); break;
	case 1750L: ::lstrcat( buf, "RPC_S_UNKNOWN_AUTHZ_SERVICE " ); break;

	case 1751L: ::lstrcat( buf, "EPT_S_INVALID_ENTRY " ); break;
	case 1752L: ::lstrcat( buf, "EPT_S_CANT_PERFORM_OP " ); break;
	case 1753L: ::lstrcat( buf, "EPT_S_NOT_REGISTERED " ); break;
	case 1755L: ::lstrcat( buf, "RPC_S_INCOMPLETE_NAME " ); break;
	case 1756L: ::lstrcat( buf, "RPC_S_INVALID_VERS_OPTION " ); break;
	case 1757L: ::lstrcat( buf, "RPC_S_NO_MORE_MEMBERS " ); break;
	case 1759L: ::lstrcat( buf, "RPC_S_INTERFACE_NOT_FOUND " ); break;
	case 1760L: ::lstrcat( buf, "RPC_S_ENTRY_ALREADY_EXISTS " ); break;
	case 1761L: ::lstrcat( buf, "RPC_S_ENTRY_NOT_FOUND " ); break;
	case 1762L: ::lstrcat( buf, "RPC_S_NAME_SERVICE_UNAVAILABLE " ); break;

	case 1764L: ::lstrcat( buf, "RPC_S_CANNOT_SUPPORT " ); break;
	case 1765L: ::lstrcat( buf, "RPC_S_NO_CONTEXT_AVAILABLE " ); break;
	case 1766L: ::lstrcat( buf, "RPC_S_INTERNAL_ERROR " ); break;
	case 1767L: ::lstrcat( buf, "RPC_S_ZERO_DIVIDE " ); break;
	case 1768L: ::lstrcat( buf, "RPC_S_ADDRESS_ERROR " ); break;
	case 1769L: ::lstrcat( buf, "RPC_S_FP_DIV_ZERO " ); break;
	case 1770L: ::lstrcat( buf, "RPC_S_FP_UNDERFLOW " ); break;
	case 1771L: ::lstrcat( buf, "RPC_S_FP_OVERFLOW " ); break;
	case 1772L: ::lstrcat( buf, "RPC_X_NO_MORE_ENTRIES " ); break;
	case 1773L: ::lstrcat( buf, "RPC_X_SS_CHAR_TRANS_OPEN_FAIL " ); break;
	case 1774L: ::lstrcat( buf, "RPC_X_SS_CHAR_TRANS_SHORT_FILE " ); break;

	case 1775L: ::lstrcat( buf, "RPC_X_SS_IN_NULL_CONTEXT " ); break;
	case 1776L: ::lstrcat( buf, "RPC_X_SS_CONTEXT_MISMATCH " ); break;
	case 1777L: ::lstrcat( buf, "RPC_X_SS_CONTEXT_DAMAGED " ); break;
	case 1778L: ::lstrcat( buf, "RPC_X_SS_HANDLES_MISMATCH " ); break;
	case 1779L: ::lstrcat( buf, "RPC_X_SS_CANNOT_GET_CALL_HANDLE " ); break;
	case 1780L: ::lstrcat( buf, "RPC_X_NULL_REF_POINTER " ); break;
	case 1781L: ::lstrcat( buf, "RPC_X_ENUM_VALUE_OUT_OF_RANGE " ); break;
	case 1782L: ::lstrcat( buf, "RPC_X_BYTE_COUNT_TOO_SMALL " ); break;
	case 1783L: ::lstrcat( buf, "RPC_X_BAD_STUB_DATA " ); break;
	case 1784L: ::lstrcat( buf, "ERROR_INVALID_USER_BUFFER " ); break;

	case 1785L: ::lstrcat( buf, "ERROR_UNRECOGNIZED_MEDIA " ); break;
	case 1786L: ::lstrcat( buf, "ERROR_NO_TRUST_LSA_SECRET " ); break;
	case 1787L: ::lstrcat( buf, "ERROR_NO_TRUST_SAM_ACCOUNT " ); break;
	case 1788L: ::lstrcat( buf, "ERROR_TRUSTED_DOMAIN_FAILURE " ); break;
	case 1789L: ::lstrcat( buf, "ERROR_TRUSTED_RELATIONSHIP_FAILURE " ); break;
	case 1790L: ::lstrcat( buf, "ERROR_TRUST_FAILURE " ); break;
	case 1791L: ::lstrcat( buf, "RPC_S_CALL_IN_PROGRESS " ); break;
	case 1792L: ::lstrcat( buf, "ERROR_NETLOGON_NOT_STARTED " ); break;
	case 1793L: ::lstrcat( buf, "ERROR_ACCOUNT_EXPIRED " ); break;
	case 1794L: ::lstrcat( buf, "ERROR_REDIRECTOR_HAS_OPEN_HANDLES " ); break;

	case 1795L: ::lstrcat( buf, "ERROR_PRINTER_DRIVER_ALREADY_INSTALLED " ); break;
	case 1796L: ::lstrcat( buf, "ERROR_UNKNOWN_PORT " ); break;
	case 1797L: ::lstrcat( buf, "ERROR_UNKNOWN_PRINTER_DRIVER " ); break;
	case 1798L: ::lstrcat( buf, "ERROR_UNKNOWN_PRINTPROCESSOR " ); break;
	case 1799L: ::lstrcat( buf, "ERROR_INVALID_SEPARATOR_FILE " ); break;
	case 1800L: ::lstrcat( buf, "ERROR_INVALID_PRIORITY " ); break;
	case 1801L: ::lstrcat( buf, "ERROR_INVALID_PRINTER_NAME " ); break;
	case 1802L: ::lstrcat( buf, "ERROR_PRINTER_ALREADY_EXISTS " ); break;
	case 1803L: ::lstrcat( buf, "ERROR_INVALID_PRINTER_COMMAND " ); break;
	case 1804L: ::lstrcat( buf, "ERROR_INVALID_DATATYPE " ); break;

	case 1805L: ::lstrcat( buf, "ERROR_INVALID_ENVIRONMENT " ); break;
	case 1806L: ::lstrcat( buf, "RPC_S_NO_MORE_BINDINGS " ); break;
	case 1807L: ::lstrcat( buf, "ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT " ); break;
	case 1808L: ::lstrcat( buf, "ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT " ); break;
	case 1809L: ::lstrcat( buf, "ERROR_NOLOGON_SERVER_TRUST_ACCOUNT " ); break;
	case 1810L: ::lstrcat( buf, "ERROR_DOMAIN_TRUST_INCONSISTENT " ); break;
	case 1811L: ::lstrcat( buf, "ERROR_SERVER_HAS_OPEN_HANDLES " ); break;
	case 1812L: ::lstrcat( buf, "ERROR_RESOURCE_DATA_NOT_FOUND " ); break;
	case 1813L: ::lstrcat( buf, "ERROR_RESOURCE_TYPE_NOT_FOUND " ); break;

	case 1814L: ::lstrcat( buf, "ERROR_RESOURCE_NAME_NOT_FOUND " ); break;
	case 1815L: ::lstrcat( buf, "ERROR_RESOURCE_LANG_NOT_FOUND " ); break;
	case 1816L: ::lstrcat( buf, "ERROR_NOT_ENOUGH_QUOTA " ); break;
	case 1898L: ::lstrcat( buf, "RPC_S_GROUP_MEMBER_NOT_FOUND " ); break;
	case 1899L: ::lstrcat( buf, "EPT_S_CANT_CREATE " ); break;
	case 1900L: ::lstrcat( buf, "RPC_S_INVALID_OBJECT " ); break;
	case 2138L: ::lstrcat( buf, "ERROR_NO_NETWORK " ); break;
	case 2202L: ::lstrcat( buf, "ERROR_BAD_USERNAME " ); break;
	case 2250L: ::lstrcat( buf, "ERROR_NOT_CONNECTED " ); break;
	case 2401L: ::lstrcat( buf, "ERROR_OPEN_FILES " ); break;
	case 2404L: ::lstrcat( buf, "ERROR_DEVICE_IN_USE " ); break;

	case 6118L: ::lstrcat( buf, "ERROR_NO_BROWSER_SERVERS_FOUND" ); break;
	
	default: ::lstrcat( buf, "ERROR unknown code" ); break;
	}

#endif	// DEBUG

	m_iMessagePos += sprintf( 
		m_szMessage + m_iMessagePos,
		"%s\n", buf );
	return	*this;
}
