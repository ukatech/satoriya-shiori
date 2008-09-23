
#include <list>
#include <exception>
#include <stdexcept>
#include <typeinfo.h>
using namespace std;

#include "OverlapController.h"

// 選択係

//  選択方法をset_OC、候補となるTをupdateで設定する。
//  selectで設定されたT群から１つを選択して返す。

template<typename T>
class Selector
{
	list<T> m_candidates; // 選択の対象となる候補
	OverlapController<T>* m_OC; // 選択メソッド

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
	// i_candidatesは降順にソートされており、かつ、空であってはならない。
	void update_candidates(const list<T>& i_candidates)
	{
		if ( m_OC == NULL )
		{
			//cout << "Selector::update_candidates(), m_OC:" << m_OC << ", this:" << this << endl;
			m_OC = new OC_Random<T>;
			//cout << "                               m_OC:" << m_OC << ", this:" << this << endl;
		}

		#define NOW i_candidates
		#define OLD m_candidates

		typename list<T>::const_iterator now = NOW.begin();
		typename list<T>::iterator old = OLD.begin();

		while (true)
		{
			if ( now == NOW.end() )
			{
				//OLD.erase(old, OLD.end());
				while ( old != OLD.end() )
				{
					m_OC->on_erase(OLD, old);
					old = OLD.erase(old);
				}
				break;
			}
			if ( old == OLD.end() )
			{
				//OLD.insert(old, now, NOW.end());
				for (; now != NOW.end() ; ++now )
				{
					OLD.insert(old, *now);
					m_OC->on_add(OLD, now);
				}
				break;
			}
	
			if ( *now < *old )
			{
				OLD.insert(old, *now);
				m_OC->on_add(OLD, now);
				++now;
			}
			else if ( *now > *old )
			{
				m_OC->on_erase(OLD, old);
				old = OLD.erase(old);
				++old;
			}
			else
			{
				++now;
				++old;
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
		m_candidates.clear();
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
			throw runtime_error("select: candidates list is empty!");
		}
		
		return m_OC->select(m_candidates);
	}
};
