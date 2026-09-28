#include "SakuraCS.h"

#pragma once
#ifndef SAKURA_DLL_HOST_INCLUDED
#define SAKURA_DLL_HOST_INCLUDED

// 「dllとして呼ばれる側」を作る際に使用するクラス。
// これを継承したクラスを定義し、m_dllに唯一の実体を代入する。
class SakuraDLLHost : public SakuraCS
{
#ifdef POSIX
private:
	static int m_id;
protected:
	static std::vector<SakuraDLLHost *> m_dll;
public:
    template<typename T>
    static int Create() {
        m_dll.emplace_back(new T());
        return m_dll.size() - 1;
    }
    static void Select(int id) {
        if (id <= 0 || id >= m_dll.size()) {
            return;
        }
        m_id = id;
    }
    static void Destroy(int id) {
        if (id <= 0 || id >= m_dll.size()) {
            return;
        }
        delete m_dll[id];
        m_dll.erase(m_dll.begin() + id);
    }
	static SakuraDLLHost* I() { return m_dll[m_id]; }
#else
protected:
	static SakuraDLLHost* m_dll;
public:
	static SakuraDLLHost* I() { return m_dll; }
#endif // POSIX

	SakuraDLLHost() : SakuraCS() {}
	virtual ~SakuraDLLHost() {}

	virtual bool	load(const wstring& i_base_folder) { return true; }
	virtual bool	unload() { return true; }

	virtual wstring  getversionlist(const wstring& i_base_folder) { return L""; }

	// 素のリクエストのバイト列を受け取り、素のレスポンスのバイト列を返す。
	// Charsetヘッダに従って文字コードを変換し、内部で↓を呼ぶ。
	std::string request_bytes(const std::string& i_request_bytes);

	// レスポンスの文字コード。既定ではリクエストと同じ。
	virtual CharactorSet response_charset(CharactorSet i_request_charset) { return i_request_charset; }

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

#endif //SAKURA_DLL_HOST_INCLUDED

