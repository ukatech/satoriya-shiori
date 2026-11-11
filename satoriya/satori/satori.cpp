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
const wchar_t* gSatoriVersion = L"phase Mc206-5";
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
	long id;
	std::map<wstring,long>::const_iterator it = m_str2id.find(i_str);
	if ( it != m_str2id.end() ) {
		id = it->second;
	}
	else {
		id = (long)m_id2str.size();
		m_id2str.push_back(i_str);
		m_str2id.insert(std::make_pair(i_str, id));
	}
	return wstring() + sm_escape_code + itos(id) + L" ";
}

// 「印 + 番号 + 半角空白」を、記録した文字列に置き換える。1回の走査で済ませる。
// 印が無い（ほとんどの）文字列では、探すだけで戻る。
void escaper::unescape_impl(wstring& io_str, const wchar_t* i_prefix)
{
	if ( m_id2str.empty() ) {
		return;
	}
	const wchar_t	mark = sm_escape_code[0];
	wstring::size_type	pos = io_str.find(mark);
	if ( pos == wstring::npos ) {
		return;
	}

	const wstring::size_type	size = io_str.size();
	const wstring::size_type	count = m_id2str.size();
	const wstring::size_type	prefix_len = wcslen(i_prefix);
	wstring	result;
	result.reserve(size + 64);
	wstring::size_type	copied = 0;	// io_str のここまでを result に写した

	while ( pos != wstring::npos )
	{
		// 印の後ろの数字を読む。itos(i) と同じく、先頭の0は認めない。
		wstring::size_type	q = pos + 1;
		wstring::size_type	id = 0;
		while ( q < size && io_str[q] >= L'0' && io_str[q] <= L'9' && q - pos <= 9 ) {
			id = id * 10 + (io_str[q] - L'0');
			++q;
		}
		const wstring::size_type	digits = q - (pos + 1);
		const bool	ok = digits > 0
			&& !( digits > 1 && io_str[pos + 1] == L'0' )
			&& q < size && io_str[q] == L' '
			&& id < count;

		if ( ok ) {
			result.append(io_str, copied, pos - copied);
			if ( prefix_len > 0 ) {
				result.append(i_prefix, prefix_len);
			}
			result.append(m_id2str[id]);
			copied = q + 1;
			pos = io_str.find(mark, copied);
		}
		else {
			pos = io_str.find(mark, pos + 1);
		}
	}
	result.append(io_str, copied, wstring::npos);
	io_str.swap(result);
}

// 対象文字列中に含まれる「エスケープされた文字列」を元に戻す。
void escaper::unescape(wstring& io_str)
{
	unescape_impl(io_str, L"");
}

void escaper::unescape_for_dic(wstring& io_str)
{
	unescape_impl(io_str, L"φ");
}

// メンバをクリア
void escaper::clear()
{
	m_str2id.clear();
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

// リクエスト処理中に例外で中断されたとき、途中だった呼び出しの状態を初期化する。
// （通常は呼び出しが終わるたびに戻る値が、途中で抜けたせいで残ったままにならないように）
void Satori::on_request_exception()
{
	m_nest_count = 0;
	kakko_replace_history.clear();
	mCallStack.clear();
	mLoopCounters.clear();
	reset_nest_counters();
	GetSender().validate();	// ＄ログ無効などの一時的な状態は戻す
}
