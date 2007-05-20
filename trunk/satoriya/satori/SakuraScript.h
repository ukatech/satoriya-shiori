#include "../_/stltool.h"

struct sakura_script_part
{
	string command; // "\p"など。トークの時は""
	string value;
	sakura_script_part(string i_command, string i_value) :
		command(i_command), value(i_value) {}
};

struct sakura_script_parts : public deque<sakura_script_part>
{
public:
	sakura_script_parts() {}
	sakura_script_parts(string i_script) { unserialize(i_script); }
	string serialize() const;
	void unserialize(string i_script);
};


