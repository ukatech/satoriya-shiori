/* -*- c++ -*- */
#ifndef POSIX
#  include <windows.h>	// HMODULE,BOOL,HGLOBALとか
#endif

#include "SaoriClient.h"

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
		}
		~DllData() {
			if ( m_pSaoriClient ) {
				delete m_pSaoriClient;
			}
		}
		SaoriClient	*m_pSaoriClient;
		int	mRefCount;
	};
	std::map<wstring, CallData>	mCallData;	// 呼び出し名；呼び出し名ごとの情報
	std::map<wstring, DllData>	mDllData;	// DLLのフルパス；DLLごとの情報

	wstring	mBaseFolder;

	Satori *pSatori;

	ShioriPlugins(void) { }

public:
	ShioriPlugins(Satori *pSat) : pSatori(pSat) {
	}

	bool	load(const wstring& iBaseFolder);
	bool	load_a_plugin(const wstring& iPluginLine);
	void	load_default_entry(void);

	wstring	request(const wstring& iCallName, const strvec& iArguments, strvec& oResults, const wstring& iSecurityLevel);
	void	unload();

	bool	find(wstring iCallName) {
		return (mCallData.find(iCallName) != mCallData.end() );
	}
};

