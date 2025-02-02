#include "satori.h"

//////////DEBUG/////////////////////////
#include "warning.h"
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
const char* gSatoriVersion = "phase Mc171-3";
const char* gShioriVersion = "3.0";
const char* gSaoriVersion = "1.0";

const char* gSatoriLicense =
	"Copyright (c) 2001-2005, Kusigahama Yagi.\n"
	"Copyright (c) 2006-, SEIBIHAN.\n"
	"All rights reserved.\n"
	"\n"
	"Redistribution and use in source and binary forms, with or without\n"
	"modification, are permitted provided that the following conditions are met:\n"
	"\n"
	"1. Redistributions of source code must retain the above copyright notice, this\n"
	"   list of conditions and the following disclaimer.\n"
	"\n"
	"2. Redistributions in binary form must reproduce the above copyright notice,\n"
	"   this list of conditions and the following disclaimer in the documentation\n"
	"   and/or other materials provided with the distribution.\n"
	"\n"
	"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
	"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
	"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
	"DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"
	"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
	"DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"
	"SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
	"CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
	"OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
	"OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";

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
	std::vector<string>::iterator it = std::find(m_id2str.begin(),m_id2str.end(),i_str);
	if ( it != m_id2str.end() ) {
		return string() + sm_escape_sjis_code + itos(std::distance(m_id2str.begin(),it)) + " ";
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
