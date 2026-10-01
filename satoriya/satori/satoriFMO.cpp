#include "SakuraFMO.h"
#include	"satori.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


bool	Satori::updateGhostsInfo() {

	if ( mExeFolder==L"" )
	{
		return	false;
	}

	SakuraFMO theSakuraFMO;
	if ( !theSakuraFMO.update() )
	{
		return	false;
	}

	ghosts_info.clear();
	ghosts_info.push_back( strmap() );
	for( std::map<wstring, strmap>::iterator i=theSakuraFMO.begin() ; i!=theSakuraFMO.end() ; ++i ) {
		//ダミーを蹴る
		if ( i->first.find(L"ssp_fmo_header_dummyentry") != wstring::npos ) { continue; }
		if ( i->first.find(L"SSTPVIEWER-") != wstring::npos ) { continue; }
		if ( i->first.find(L"SSSB") != wstring::npos ) { continue; }

		strmap&	m = i->second;
		bool	isSelfData = false;// 自分自身のデータか？

		if ( m.find(L"ghostpath") != m.end() ) {
			if ( _wcsicmp((m[L"ghostpath"]+L"ghost\\master\\").c_str(), mBaseFolder.c_str())==0 )
				isSelfData = true;
		}
		else {
			if ( _wcsicmp(m[L"path"].c_str(), mExeFolder.c_str())==0 )
				isSelfData = true;
		}

		if ( isSelfData )
			ghosts_info[0]=m;	
		else
			ghosts_info.push_back(m);	// 他は順次末尾に
	}

	return	true;
}
