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

const wchar_t* gSatoriName = L"Satori";
const wchar_t* gSatoriNameW = L"里々";
const wchar_t* gSatoriCraftman = L"Yagi Kushigahama/The Maintenance Shop";
const wchar_t* gSatoriCraftmanW = L"櫛ヶ浜やぎ/整備班";
const wchar_t* gSatoriVersion = L"phase Mc201-4";
const wchar_t* gShioriVersion = L"3.0";
const wchar_t* gSaoriVersion = L"1.0";

const wchar_t* gSatoriLicense =
	L"Copyright (c) 2001-2005, Kusigahama Yagi.\n"
	L"Copyright (c) 2006-, SEIBIHAN.\n"
	L"All rights reserved.\n"
	L"\n"
	L"Redistribution and use in source and binary forms, with or without\n"
	L"modification, are permitted provided that the following conditions are met:\n"
	L"\n"
	L"1. Redistributions of source code must retain the above copyright notice, this\n"
	L"   list of conditions and the following disclaimer.\n"
	L"\n"
	L"2. Redistributions in binary form must reproduce the above copyright notice,\n"
	L"   this list of conditions and the following disclaimer in the documentation\n"
	L"   and/or other materials provided with the distribution.\n"
	L"\n"
	L"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\"\n"
	L"AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE\n"
	L"IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE\n"
	L"DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE\n"
	L"FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL\n"
	L"DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR\n"
	L"SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER\n"
	L"CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,\n"
	L"OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE\n"
	L"OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.";

//#ifdef SATORI_DLL
	// Satoriの唯一の実体
	Satori gSatori;
#ifdef POSIX
	int SakuraDLLHost::m_id = 0;
    std::vector<SakuraDLLHost *> SakuraDLLHost::m_dll(1, &gSatori);
#else
	SakuraDLLHost* SakuraDLLHost::m_dll = &gSatori;
#endif
//#else
//	SakuraDLLHost* SakuraDLLHost::m_dll = NULL;
//#endif // SATORI_DLL


// エスケープ文字列
const wchar_t escaper::sm_escape_code[2]={(wchar_t)0xE09E,0x00};

// 引数文字列を受け取り、メンバに格納し、「エスケープされた文字列」を返す。
wstring escaper::insert(const wstring& i_str)
{
	std::vector<wstring>::iterator it = std::find(m_id2str.begin(),m_id2str.end(),i_str);
	if ( it != m_id2str.end() ) {
		return wstring() + sm_escape_code + itos(std::distance(m_id2str.begin(),it)) + L" ";
	}
	else {
		m_id2str.push_back(i_str);
		return wstring() + sm_escape_code + itos(m_id2str.size()-1) + L" ";
	}
}

// 対象文字列中に含まれる「エスケープされた文字列」を元に戻す。
void escaper::unescape(wstring& io_str)
{
	const int	max = m_id2str.size();
	for (int i=0 ; i<max ; ++i)
		replace(io_str, wstring(sm_escape_code)+itos(i)+L" ", m_id2str[i]);
}

void escaper::unescape_for_dic(wstring& io_str)
{
	const int	max = m_id2str.size();
	for (int i=0 ; i<max ; ++i)
		replace(io_str, wstring(sm_escape_code)+itos(i)+L" ", L"φ"+m_id2str[i]);
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
	wstring r;
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
