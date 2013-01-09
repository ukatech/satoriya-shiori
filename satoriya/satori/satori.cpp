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
const char* gSatoriNameW = "里々";
const char* gSatoriCraftman = "Yagi Kushigahama/The Maintenance Shop";
const char* gSatoriCraftmanW = "櫛ヶ浜やぎ/整備班";
const char* gSatoriVersion = "phase Mc151-3";
const char* gShioriVersion = "3.0";
const char* gSaoriVersion = "1.0";



//#ifdef SATORI_DLL
	// Satoriの唯一の実体
	Satori gSatori;
	SakuraDLLHost* SakuraDLLHost::m_dll = &gSatori;
//#else
//	SakuraDLLHost* SakuraDLLHost::m_dll = NULL;
//#endif // SATORI_DLL


// エスケープ文字列
const char escaper::sm_escape_sjis_code[3]={(char)0x9e,(char)0xff,0x00};

// 引数文字列を受け取り、メンバに格納し、「エスケープされた文字列」を返す。
string escaper::insert(const string& i_str)
{
	vector<string>::iterator it = find(m_id2str.begin(),m_id2str.end(),i_str);
	if ( it != m_id2str.end() ) {
		return string() + sm_escape_sjis_code + itos(distance(m_id2str.begin(),it)) + " ";
	}
	else {
		m_id2str.push_back(i_str);
		return string() + sm_escape_sjis_code + itos(m_id2str.size()-1) + " ";
	}
}

// 対象文字列中に含まれる「エスケープされた文字列」を元に戻す。
void escaper::unescape(string& io_str)
{
	const int	max = m_id2str.size();
	for (int i=0 ; i<max ; ++i)
		replace(io_str, string(sm_escape_sjis_code)+itos(i)+" ", m_id2str[i]);
}

void escaper::unescape_for_dic(string& io_str)
{
	const int	max = m_id2str.size();
	for (int i=0 ; i<max ; ++i)
		replace(io_str, string(sm_escape_sjis_code)+itos(i)+" ", "φ"+m_id2str[i]);
}

// メンバをクリア
void escaper::clear()
{
	//m_str2id.clear();
	m_id2str.clear();
}


// 式を評価し、結果を真偽値として解釈する
bool Satori::evalcate_to_bool(const Condition& i_cond)
{
	string r;
	if ( !calculate(i_cond.c_str(), r) )
	{
		// 計算失敗
		return false;
	}
	return  ( zen2int(r) != 0 );
}

Satori::Satori()
{
	mShioriPlugins = new ShioriPlugins(this);
}

Satori::~Satori() {
	delete mShioriPlugins;
}
