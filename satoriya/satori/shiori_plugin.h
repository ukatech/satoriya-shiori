/* -*- c++ -*- */
#pragma once

#ifndef POSIX
#  include <windows.h>	// HMODULE,BOOL,HGLOBALとか
#endif

#include "SaoriClient.h"
#include <set>

class Satori;

// プラグインの総合管理
class ShioriPlugins {

	struct CallData {	// 呼び出し名ごとの情報
		wstring	mDllPath;
		strvec	mPreDefinedArguments;
		bool	mIsBasic;

		CallData()
		{
			mIsBasic = false;
		}
	};
	class DllData {	// DLLごとの情報
	public:
		DllData() {
			m_pSaoriClient = NULL;
			mIsInternal = false;
		}
		~DllData() {
			if ( m_pSaoriClient ) {
				delete m_pSaoriClient;
			}
		}
		SaoriClient	*m_pSaoriClient;
		int	mRefCount;
		bool	mIsInternal;	// 内蔵ssuを直接呼び出している（実体の ssu.dll が無い）
		wstring	mDirectModule;	// 本体の内蔵モジュールをDirect SSTP（SAORI over DirectSSTP）で呼んでいる（実体のDLLが無い）。その名前
	};
	std::map<wstring, CallData>	mCallData;	// 呼び出し名；呼び出し名ごとの情報
	std::map<wstring, DllData>	mDllData;	// DLLのフルパス；DLLごとの情報

	wstring	mBaseFolder;

	Satori *pSatori;

#ifndef POSIX
	// SAORI over DirectSSTP：本体の内蔵モジュール（internal_*）は、＠SAORIに書かなくても名前だけで呼べる。
	// 使えるモジュールは、最初に必要になったときに internal_system の ListModules で本体に問い合わせる。
	enum InternalModulesState {
		IMS_UNKNOWN,	// まだ問い合わせていない（本体の窓が分からなかったときもこのまま。次の呼び出しで再び試す）
		IMS_LISTED,	// 一覧を取得した（mInternalModules）
		IMS_ANY,	// 本体に internal_system が無い（一覧を取れない）。internal_ で始まる名前はすべて内蔵モジュールとみなす
		IMS_NONE	// 本体が SAORI over DirectSSTP に対応していない
	};
	InternalModulesState	mInternalModulesState;
	std::set<wstring>	mInternalModules;	// 内蔵モジュールの名前（小文字）

	void	fetch_internal_modules();
	bool	register_internal_module(const wstring& iCallName);
#endif

	ShioriPlugins(void) { }

public:
	ShioriPlugins(Satori *pSat) : pSatori(pSat) {
#ifndef POSIX
		mInternalModulesState = IMS_UNKNOWN;
#endif
	}

	bool	load(const wstring& iBaseFolder);
	bool	load_a_plugin(const wstring& iPluginLine);
	void	load_default_entry(void);

	wstring	request(const wstring& iCallName, const strvec& iArguments, strvec& oResults, const wstring& iSecurityLevel);
	void	unload();

	// 呼び出し名が定義されているか。本体の内蔵モジュールの名前なら、ここで登録する。
	bool	find(const wstring& iCallName) {
		if ( mCallData.find(iCallName) != mCallData.end() ) {
			return true;
		}
#ifndef POSIX
		return register_internal_module(iCallName);
#else
		return false;
#endif
	}
};

