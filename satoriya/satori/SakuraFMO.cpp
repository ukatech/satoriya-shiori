#include "SakuraFMO.h"
#include "../_/FMO.h"
#include "../_/Sender.h"

//////////DEBUG/////////////////////////
#include "warning.h"
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////


// FMOの内容をバイト列で読む。先頭4バイトはサイズ（自身を含む）。
static bool	read_fmo(const wchar_t* name, std::string& o)
{
	FMO	fmo;
	if ( !fmo.open(FILE_MAP_READ, FALSE, name) ) {
		return	false;
	}

	LPVOID	p = fmo.map();
	if ( p==NULL ) {
		GetSender().sender() << L"FMO can't mapping." << std::endl;
		return	false;
	}

	long size = *((long*)p);
	// 共有メモリの内容は他のプロセスが書いたものなので、マップされた範囲を越えないようにする
	MEMORY_BASIC_INFORMATION	mbi;
	if ( ::VirtualQuery(p, &mbi, sizeof(mbi)) == 0 || size > static_cast<long>(mbi.RegionSize) ) {
		size = 0;
	}
	if ( size > 4 ) {
		o.assign(static_cast<const char*>(p) + 4, size - 4);
		std::string::size_type nul = o.find('\0');
		if ( nul != std::string::npos ) {
			o.erase(nul);
		}
	}
	fmo.unmap(p);
	fmo.close();
	return	true;
}

bool SakuraFMO::update()
{
	// SakuraUnicode（UTF-8固定、SSP 2.5.26以降）を優先し、無ければ Sakura（OS依存の文字コード）を読む
	std::string bytes;
	wstring text;
	if ( read_fmo(L"SakuraUnicode", bytes) ) {
		text = UTF8toW(bytes);
	}
	else if ( read_fmo(L"Sakura", bytes) ) {
		text = ACPtoW(bytes);
	}
	else {
		GetSender().sender() << L"FMO can't open." << std::endl;
		return	false;
	}

	strvec	lines;
	split(text, CRLF, lines);
	for( strvec::iterator i=lines.begin() ; i!=lines.end() ; ++i ) {
		strvec	MD5andDATA;
		if ( split(*i, L".", MD5andDATA, 2) != 2 )
		{
			continue;
		}

		strvec	ENTRYandVALUE;
		static const wchar_t BYTE1[2] = {1,0};
		if ( split(MD5andDATA[1], BYTE1, ENTRYandVALUE) != 2 )
		{
			continue;
		}

		((*this)[ MD5andDATA[0] ])[ ENTRYandVALUE[0] ] = ENTRYandVALUE[1];
	}

	// log
	for( const_iterator i=begin() ; i!=end() ; ++i )
	{
		const strmap&	m = i->second;
		GetSender().sender() << i->first << std::endl;
		for( strmap::const_iterator j=m.begin() ; j!=m.end() ; ++j )
		{
			GetSender().sender() << L"　" << j->first << L":" << j->second << std::endl;
		}
	}
	
	return	true;
}



