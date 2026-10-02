
#include <list>
#include <set>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <typeinfo>
using std::wstring;

#include "OverlapController.h"

// 選択係

//  選択方法をset_OC、候補となるTをupdateで設定する。
//  selectで設定されたT群から１つを選択して返す。

template<typename T>
class Selector
{
	std::list<T> m_candidates; // 選択の対象となる候補
	OverlapController<T>* m_OC; // 選択メソッド

	// m_OC を所有しているので代入は禁止（実装しない）。コピーは候補が空のときだけコピーコンストラクタで許す。
	Selector& operator=(const Selector&);

public:

	Selector() :
		m_candidates(),
		m_OC(NULL)
	{
		//cout << "Selector(), m_OC:" << m_OC << ", this:" << this << endl;
	}

	Selector(const Selector& other) :
		m_candidates(),
		m_OC(NULL) 
	{
		// 使ってるOverlapControllerはコピーできない。
		assert(other.m_candidates.empty());
		assert(other.m_OC == NULL);
		
		// まずい気もする。
		// しかし candidates とゆーシステムがそもそもコピーされることを考えてない。
		// だからこそ旧システムは数字で扱っていたわけだが。速度効率がなぁ。
	}


	~Selector()
	{
		if ( m_OC != NULL )
		{
			//cout << "~Selector(), m_OC:" << m_OC << ", this:" << this << endl;
			delete m_OC;
			m_OC = NULL;
			//cout << "             m_OC:" << m_OC << ", this:" << this << endl;
		}
	}

	// 選択対象候補を更新する。
	// i_candidatesは空であってはならない。
	// 候補の並びは選択方法（順次など）の意味を持つので、i_candidatesの並びのまま持つ。
	// 同じ候補かどうかは T の値（ポインタなど）の一致で判断する。大小比較には頼らない。
	void update_candidates(const std::list<T>& i_candidates)
	{
		if ( m_OC == NULL )
		{
			//cout << "Selector::update_candidates(), m_OC:" << m_OC << ", this:" << this << endl;
			m_OC = new OC_Random<T>;
			//cout << "                               m_OC:" << m_OC << ", this:" << this << endl;
		}

		// 前回と同じなら何もしない（条件付きの群でも、条件の結果が変わっていなければここで済む）
		if ( m_candidates.size() == i_candidates.size() &&
			 std::equal(i_candidates.begin(), i_candidates.end(), m_candidates.begin()) )
		{
			return;
		}

		// 1. 新しい候補に無いものを、今の並びのまま順に通知して消す。
		//    （OverlapController は隣の候補を見て、直前の選択を付け替えることがある）
		std::set<T> now_set;	// VC6 の set には範囲コンストラクタが無いので insert で作る
		for ( typename std::list<T>::const_iterator n = i_candidates.begin() ; n != i_candidates.end() ; ++n )
		{
			now_set.insert(*n);
		}
		for ( typename std::list<T>::iterator old = m_candidates.begin() ; old != m_candidates.end() ; )
		{
			if ( now_set.find(*old) == now_set.end() )
			{
				m_OC->on_erase(m_candidates, old);
				old = m_candidates.erase(old);
			}
			else
			{
				++old;
			}
		}

		// 2. 今の候補に無いものを、新しい並びにそろえてから通知して加える。
		std::set<T> old_set;
		for ( typename std::list<T>::const_iterator o = m_candidates.begin() ; o != m_candidates.end() ; ++o )
		{
			old_set.insert(*o);
		}
		m_candidates = i_candidates;
		for ( typename std::list<T>::const_iterator now = m_candidates.begin() ; now != m_candidates.end() ; ++now )
		{
			if ( old_set.find(*now) == old_set.end() )
			{
				m_OC->on_add(m_candidates, now);
			}
		}
	}
	
	// 選択状況をクリア
	void clear_OC()
	{
		//cout << "Selector::clear_OC(), m_OC:" << m_OC << ", this:" << this << endl;
		if ( m_OC != NULL )
		{
			m_OC->on_clear();
		}
	}
	
	// 選択方法を変更。
	// 引数には new で作ったものを渡すこと。解放はこのクラスで行う。
	void attach_OC(OverlapController<T>* i_OC)
	{
		//cout << "Selector::attach_OC(), m_OC:" << m_OC << ", this:" << this << endl;
		if ( m_OC == NULL || m_OC->type() != i_OC->type() ) {
			if ( m_OC != NULL ) {
				delete m_OC;
			}
			m_OC = i_OC;
			m_candidates.clear();
		}
		else {
			delete i_OC;
		}
		//m_candidates.clear();
		//cout << "                       m_OC:" << m_OC << ", this:" << this << endl;
	}
	
	// 選択を行う。ただし候補が空の時は実行してはいけない。
	T select()
	{
		if ( m_OC == NULL )
		{
			//cout << "Selector::select(), m_OC:" << m_OC << ", this:" << this << endl;
			m_OC = new OC_Random<T>;
			//cout << "                    m_OC:" << m_OC << ", this:" << this << endl;
		}

		if ( m_candidates.empty() )
		{
			throw std::runtime_error("select: candidates list is empty!");
		}
		
		return m_OC->select(m_candidates);
	}

	// 現在の選択対象候補
	const std::list<T>& candidates() const
	{
		return m_candidates;
	}

	bool isOCUsedAll()
	{
		return m_OC->is_used_all(m_candidates);
	}

	// 現在の選択メソッドの種類を返す
	int type()
	{
		if (m_OC == NULL)
		{
			m_OC = new OC_Random<T>;
		}

		return m_OC->type();
	}

	void getSelectables(std::list<T>& o_result)
	{
		if (m_OC == NULL)
		{
			m_OC = new OC_Random<T>;
		}
		if (m_candidates.empty())
		{
			throw std::runtime_error("select: candidates list is empty!");
		}

		m_OC->get_selectable(m_candidates, o_result);
	}

	void applySelected(const std::list<T>& i_candidates, T t)
	{
		if (m_OC != NULL)
		{
			m_OC->apply_selected(i_candidates, t);
		}
	}
};