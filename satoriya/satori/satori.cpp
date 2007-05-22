#include "satori.h"

//////////DEBUG/////////////////////////
#ifdef _WINDOWS
#ifdef _DEBUG
#include <crtdbg.h>
#define new new( _NORMAL_BLOCK, __FILE__, __LINE__)
#endif
#endif
////////////////////////////////////////

const char* gSatoriName = "Satori";
const char* gSatoriNameW = "X";
const char* gSatoriCraftman = "Yagi Kushigahama/The Maintenance Shop";
const char* gSatoriCraftmanW = "l€/�";
const char* gSatoriVersion = "phase Mc132-3";
const char* gShioriVersion = "3.0";
const char* gSaoriVersion = "1.0";



#ifdef SATORI_DLL
	// Satori̗B��
	Satori gSatori;
	SakuraDLLHost* SakuraDLLHost::m_dll = &gSatori;
#else
	SakuraDLLHost* SakuraDLLHost::m_dll = NULL;
#endif // SATORI_DLL


// GXP[v�
const char escaper::sm_escape_sjis_code[3]={(char)0x9e,(char)0xff,0x00};

// �󂯎�AoɊi[AuGXP[vꂽ��B
string escaper::insert(const string& i_str)
{
	m_id2str.push_back(i_str);
	//m_str2id[i_str] = m_id2str.size()-1;
	return string() + sm_escape_sjis_code + itos(m_id2str.size()-1) + " ";
}

// Ώە񒆂Ɋ܂܂�uGXP[vꂽ�쳂ɖ߂B
void escaper::unescape(string& io_str)
{
	const int	max = m_id2str.size();
	for (int i=0 ; i<max ; ++i)
		replace(io_str, string(sm_escape_sjis_code)+itos(i)+" ", m_id2str[i]);
}

// o�A
void escaper::clear()
{
	//m_str2id.clear();
	m_id2str.clear();
}


// �Aʂ�UlƂĉ��
bool Satori::evalcate_to_bool(const Condition& i_cond)
{
	string r;
	if ( !calculate(i_cond.c_str(), r) )
	{
		// vZs
		return false;
	}
	return  ( r!="0" && r!="O" );
}
