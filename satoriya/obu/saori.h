
#include	"../_/stltool.h"

class	Saori {
public:
	bool	load(const string& iBaseFolder);
	bool	unload();
	int		request(std::deque<string>& iArguments, string& oResult, std::deque<string>& oValues);
private:

	
};
