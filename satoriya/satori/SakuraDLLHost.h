#include "SakuraCS.h"

#pragma once
#ifndef SAKURA_DLL_HOST_INCLUDED
#define SAKURA_DLL_HOST_INCLUDED

#include <vector>

class Sender;
struct mt19937_state;
class SakuraDLLHostScope;

// 「dllとして呼ばれる側」を作る際に使用するクラス。
// これを継承したクラスを定義し、m_dllに唯一の実体を代入する。
//
// 従来の load / request / unload は、m_dll の実体（DLLに1つだけの既定の実体）を扱う。
// multi_load* で作る実体は m_instances に入り、id（1以上）で区別する。
// どちらの実体も、呼び出しの間だけ SakuraDLLHostScope で m_dll と Sender と乱数の状態を差し替えて動かすので、
// 実体の中のコードは「自分が唯一の実体」として書いてよい。ロックはしないので、呼び出しは1つのスレッドから順に行うこと。
class SakuraDLLHost : public SakuraCS
{
protected:
	// 現在の実体。呼び出しの間だけ SakuraDLLHostScope が差し替える。定義は各DLL側に置く。
	static SakuraDLLHost* m_dll;

	// multi_load* で作った実体。id が添字（0は従来の実体のために空けてあり、使わない）。
	// 破棄しても vector からは取り除かず NULL にして、後の Register で再利用する（他の id がずれないように）。
	static std::vector<SakuraDLLHost *> m_instances;

	// マルチインスタンスの実体だけが持つ、実体専用のログ送信と乱数の状態。
	// 従来の実体は NULL で、DLLに1つだけの既定のものを使う。
	Sender*	m_own_sender;
	mt19937_state*	m_own_mt;

	friend class SakuraDLLHostScope;

public:
	static SakuraDLLHost* I() { return m_dll; }

	// new した実体（SakuraDLLHost の派生クラス）を表に登録し、実体専用の状態を作って、id（1以上）を返す。
	// 実体の所有権を受け取る。失敗したら実体を delete して例外を投げる。
	// （VC6 はメンバーテンプレートの明示的な型指定で内部エラーになるので、Create<T>() の形にしていない）
	static long Add(SakuraDLLHost* i_instance);
	// id の実体を返す。無効な id（0以下、範囲外、破棄済み）なら NULL。
	static SakuraDLLHost* Find(long i_id);
	// id の実体を unload して破棄する。無効な id なら false。
	// 破棄は unload が例外を投げても行う。
	static bool Destroy(long i_id);

	SakuraDLLHost();
	virtual ~SakuraDLLHost();

	virtual bool	load(const wstring& i_base_folder) { return true; }
	virtual bool	unload() { return true; }

	virtual wstring  getversionlist(const wstring& i_base_folder) { return L""; }

	// 素のリクエストのバイト列を受け取り、素のレスポンスのバイト列を返す。
	// Charsetヘッダに従って文字コードを変換し、内部で↓を呼ぶ。
	std::string request_bytes(const std::string& i_request_bytes);

	// レスポンスの文字コード。既定ではリクエストと同じ。
	virtual CharactorSet response_charset(CharactorSet i_request_charset) { return i_request_charset; }

	// request が例外で中断されたときに呼ばれる。途中だった処理の状態を戻すために使う。
	virtual void on_request_exception() {}

	// 変換済のリクエスト文字列を受け取り、レスポンス文字列を返す。
	// 内部で↓を呼ぶ。
	virtual wstring request(const wstring& i_request_string, CharactorSet i_request_charset);
	
	// リクエストを実行。
	// 継承してオーバーライドしてください。
	// 戻り値はリターンコード。取り得る値は200,204,400,500。
	virtual int	request(
		const wstring& i_protocol,
		const wstring& i_protocol_version,
		const wstring& i_command,
		const strpairvec& i_data,
		
		wstring& o_protocol,
		wstring& o_protocol_version,
		strpairvec& o_data)=0;
};

// ある実体を呼び出している間だけ、「現在の実体」を差し替える。スコープを抜けると元に戻す。
// 実体の load / request / unload / 破棄、実体専用のログ送信の設定は、必ずこれを作ってから行う。
class SakuraDLLHostScope
{
	SakuraDLLHost*	m_prev_host;
	Sender*	m_prev_sender;
	mt19937_state*	m_prev_mt;

	SakuraDLLHostScope(const SakuraDLLHostScope&);
	SakuraDLLHostScope& operator=(const SakuraDLLHostScope&);
public:
	explicit SakuraDLLHostScope(SakuraDLLHost* i_host);
	~SakuraDLLHostScope();
};

#endif //SAKURA_DLL_HOST_INCLUDED
