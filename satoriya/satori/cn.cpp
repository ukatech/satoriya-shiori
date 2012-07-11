#include	"SaoriHost.h"
#include	<map>

class cn : public SaoriHost {
public:
	virtual SRV		request(deque<string>& iArguments, deque<string>& oValues);
};
SakuraDLLHost* SakuraDLLHost::m_dll = new cn;


static SRV	call_cn(string iCommand, deque<string>& iArguments, deque<string>& oValues)
{
	// 名前と命令を関連付けたmap
	typedef SRV (*Command)(deque<string>&, deque<string>&);
	static map<string, Command>	theMap;
	if ( theMap.empty() )
	{ 
		// 初回準備
		#define	d(iName)	\
			SRV	_##iName(deque<string>&, deque<string>&); \
			theMap[ #iName ] = _##iName
		// 命令一覧の宣言と関連付け。
		d(at);
		#undef	d
	}

	// 命令の存在を確認
	map<string, Command>::const_iterator i = theMap.find(iCommand);
	if ( i==theMap.end() ) {
		return SRV(400, string()+"Error: '"+iCommand+"'という名前の命令は定義されていません。");
	}

	// 実際に呼ぶ
	return	i->second(iArguments, oValues);
}

SRV	cn::request(deque<string>& iArguments, deque<string>& oValues) {
	if ( iArguments.size()<1 )
		return	SRV(400, "命令が指定されていません");

	// 最初の引数は命令名として扱う
	string	theCommand = iArguments.front();
	iArguments.pop_front();
	return	call_cn(theCommand, iArguments, oValues);
}


// ここから実装

#ifdef POSIX
#  include      "../_/Utilities.h"
#else
#  include	<windows.h>
#  include	<mbctype.h>
#endif
#include	"../_/stltool.h"

SRV _at(deque<string>& iArguments, deque<string>& oValues) {

	if ( iArguments.size()==2 ) {
	}
	else
		return	SRV(400, "引数が正しくありません。");

	return 200;
}
