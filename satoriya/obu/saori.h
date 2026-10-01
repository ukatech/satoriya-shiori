
#include	"../_/stltool.h"
#include	"../satori/SaoriHost.h"

class	obu : public SaoriHost {
public:
	virtual bool	load(const wstring& iBaseFolder);
	virtual bool	unload();
	virtual SRV		request(std::deque<wstring>& iArguments, std::deque<wstring>& oValues);
};
