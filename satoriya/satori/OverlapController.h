
#include <list>
#include <set>
#include "random.h"

// 各メソッドによる、「候補からの選択」クラス群
// 候補TはIDやポインタ等、ユニークかつ代入可能でなければならない。


// 無効値
#define INVALID_VALUE NULL
//#define INVALID_VALUE -1



// ----------------------------------------------------------------------
// 各メソッドの抽象基底クラス
template<typename T>
class OverlapController
{
public:
	OverlapController() {
		//
	}
	virtual ~OverlapController() {
		//
	}

	// 候補から一つを選択
	// 候補は一つ以上あることが保証されている。
	virtual T select(const std::list<T>&) =0;

	//選択可能なものを返す
	virtual void get_selectable(const std::list<T>&, std::list<T>&) = 0;

	//外部から選択したことにして重複回避を動かす
	virtual void apply_selected(const std::list<T>&, T){}

	// 全て使い切った状態かを返す。
	// 使い切ることができるやつだけ
	virtual bool is_used_all() { return false; }

	virtual int type(void) = 0;


	// イベント通知ハンドラ

	// 候補が追加された
	virtual void on_add(const std::list<T>& i_candidates, typename std::list<T>::const_iterator i_it) {}
	// 候補が消去されようとしている
	virtual void on_erase(const std::list<T>& i_candidates, typename std::list<T>::const_iterator i_it) {}
	// 重複回避状況を初期化
	virtual void on_clear() {}
};

// ----------------------------------------------------------------------
// 完全ランダム
template<typename T>
class OC_Random : public OverlapController<T>
{
public:
	//コントローラタイプ：あたらしいのを追加するときは必ず別IDにすること！typeidの遅い処理対策。
	virtual int type(void) {
		return 100;
	}

	// 候補から一つを選択
	virtual T select(const std::list<T>& i_candidates)
	{
		typename std::list<T>::const_iterator it = i_candidates.begin();
		std::advance( it, random(i_candidates.size()) );
		return *it;
	}

	//選択可能なものを返す
	virtual void get_selectable(const std::list<T>& i_candidates, std::list<T>& out_list)
	{
		for (std::list<T>::const_iterator it = i_candidates.begin(); it != i_candidates.end(); ++it)
		{
			out_list.push_back(*it);
		}
	}
};

// ----------------------------------------------------------------------
// 全て使い切るまで重複回避
template<typename T>
class OC_NonOverlap : public OverlapController<T>
{
	std::set<T> m_used;
	std::set<T> m_unused;
	T m_last;

public:
	OC_NonOverlap() : m_last(INVALID_VALUE) {}

	//コントローラタイプ：あたらしいのを追加するときは必ず別IDにすること！typeidの遅い処理対策。
	virtual int type(void) {
		return 200;
	}

	// 候補から一つを選択
	virtual T select(const std::list<T>&)
	{
		for ( ; ; ) {
			// 「未使用」が空っぽなら「使用済み」を全て「未使用」にする。
			if ( m_unused.empty() )
			{
				m_used.swap( m_unused );
			}

			// 「未使用」からランダムに一つを選び出す
			typename std::set<T>::iterator it = m_unused.begin();
			std::advance( it, random(m_unused.size()) );

			// 選んだ一つを「未使用」から「使用済み」に移す
			T t = *it;
			m_unused.erase(it);
			m_used.insert(t);

			if ( m_last != INVALID_VALUE && ((m_unused.size()+m_used.size()) >= 2) ) {
				if ( m_last == t ) {
					continue;
				}
			}
			m_last = t;
			return t;
		}
	}

	//選択可能なものを返す
	virtual void get_selectable(const std::list<T>&, std::list<T>& out_list)
	{
		for (std::set<T>::const_iterator it = m_unused.begin(); it != m_unused.end(); ++it)
		{
			out_list.push_back(*it);
		}
	}

	//外部から選択したことにして重複回避を動かす
	virtual void apply_selected(const std::list<T>&, T t)
	{
		{
			/*
			for (list<T>::const_iterator it = i_candidates.begin(); it != i_candidates.end(); ++it)
			{
				if (*it == t)
				{
					m_last = t;
					break;
				}
			}
			*/

			std::set<T>::iterator it = m_unused.find(t);
			if (it != m_unused.end())
			{
				m_unused.erase(t);
				m_used.insert(t);
			}
		}
	}

	// 候補を使い切った？
	virtual bool is_used_all()
	{
		return m_unused.empty() && !m_used.empty();
	}

	// 候補が追加された
	virtual void on_add(const std::list<T>&, typename std::list<T>::const_iterator i_it)
	{
		m_unused.insert(*i_it);
	}

	// 候補が消去されようとしている
	virtual void on_erase(const std::list<T>&, typename std::list<T>::const_iterator i_it)
	{
		// 現在どっちにあるかわからないので、両方に指示を出す
		m_unused.erase(*i_it);
		m_used.erase(*i_it);
		m_last = INVALID_VALUE;
	}
	
	// 重複回避状況を初期化
	virtual void on_clear() 
	{
		for ( typename std::set<T>::const_iterator it = m_used.begin() ; it != m_used.end() ; ++it )
		{
			m_unused.insert(*it);
		}
		m_used.clear();
		m_last = INVALID_VALUE;
	}
};

// ----------------------------------------------------------------------
// 直前との重複だけは回避
template<typename T>
class OC_NonDual : public OverlapController<T>
{
	T m_last;
public:
	OC_NonDual() : m_last(INVALID_VALUE) {}

	//コントローラタイプ：あたらしいのを追加するときは必ず別IDにすること！typeidの遅い処理対策。
	virtual int type(void) {
		return 300;
	}

	// 候補から一つを選択
	virtual T select(const std::list<T>& i_candidates)
	{
		// 一個しか無いなら回避のしようがない
		if ( i_candidates.size() == 1 )
		{
			return *(i_candidates.begin());
		}
		
		// ランダムに一つ選ぶ
		typename std::list<T>::const_iterator it = i_candidates.begin();
		std::advance( it, random(i_candidates.size()) );
		
		if ( m_last != INVALID_VALUE )
		{
			// 直前があれば、直前だけは避ける。
			if ( m_last == *it )
			{
				++it;
				if ( it == i_candidates.end() )
				{
					it = i_candidates.begin();
				}
			}
		}

		return (m_last = *it);
	}

	//選択可能なものを返す
	virtual void get_selectable(const std::list<T>& i_candidates, std::list<T>& out_list)
	{
		if (i_candidates.size() == 1)
		{
			out_list.push_back(*(i_candidates.begin()));
		}
		else
		{
			for (std::list<T>::const_iterator it = i_candidates.begin(); it != i_candidates.end(); ++it)
			{
				if (m_last != *it)
				{
					//直前だけ避ける
					out_list.push_back(*it);
				}
			}
		}
	}

	//外部から選択したことにして重複回避を動かす
	virtual void apply_selected(const std::list<T>& i_candidates, T t)
	{
		for (std::list<T>::const_iterator it = i_candidates.begin(); it != i_candidates.end(); ++it)
		{
			if (*it == t)
			{
				m_last = t;
				break;
			}
		}
	}

	// 候補が消去されようとしている
	virtual void on_erase(const std::list<T>& i_candidates, typename std::list<T>::const_iterator i_it)
	{
		if ( m_last == *i_it ) 
			m_last = INVALID_VALUE;
	}
	
	// 重複回避状況を初期化
	virtual void on_clear() 
	{
		m_last = INVALID_VALUE;
	}
};


// ----------------------------------------------------------------------
// 降順
template<typename T>
class OC_Sequential : public OverlapController<T>
{
	T m_last;
public:
	OC_Sequential() : m_last(INVALID_VALUE) {}

	//コントローラタイプ：あたらしいのを追加するときは必ず別IDにすること！typeidの遅い処理対策。
	virtual int type(void) {
		return 400;
	}

	// 候補から一つを選択
	virtual T select(const std::list<T>& i_candidates)
	{
		typename std::list<T>::const_iterator it = i_candidates.begin();

		if ( m_last != INVALID_VALUE )
		{
			while ( m_last != *it )
			{
				++it;
				if ( it == i_candidates.end() ) { break; }
			}
			// assert( m_last == *it );

			// 直前のものより１つだけ進める 直前が無効か最後までいったら最初から
			if ( it == i_candidates.end() )
			{
				it = i_candidates.begin();
			}
			else if ( ++it == i_candidates.end() )
			{
				it = i_candidates.begin();
			}
		}
		return (m_last = *it);
	}

	//選択可能なものを返す。
	virtual void get_selectable(const std::list<T>& i_candidates, std::list<T>& out_list)
	{
		typename std::list<T>::const_iterator it = i_candidates.begin();

		if (m_last != INVALID_VALUE)
		{
			while (m_last != *it)
			{
				++it;
				if (it == i_candidates.end()) { break; }
			}
			// assert( m_last == *it );

			// 直前のものより１つだけ進める 直前が無効か最後までいったら最初から
			if (it == i_candidates.end())
			{
				it = i_candidates.begin();
			}
			else if (++it == i_candidates.end())
			{
				it = i_candidates.begin();
			}
		}

		//ひとつだけになる
		out_list.push_back(*it);
	}

	//外部から選択したことにして重複回避を動かす
	virtual void apply_selected(const std::list<T>& i_candidates, T t)
	{
		for (std::list<T>::const_iterator it = i_candidates.begin(); it != i_candidates.end(); ++it)
		{
			if (*it == t)
			{
				m_last = t;
				break;
			}
		}
	}

	// 候補が消去されようとしている
	virtual void on_erase(const std::list<T>& i_candidates, typename std::list<T>::const_iterator& i_it)
	{
		if ( m_last == *i_it ) 
		{
			if ( i_it == i_candidates.begin() )
			{
				i_it = i_candidates.end();
			}
			m_last = *(--i_it);
		}
	}
	
	// 重複回避状況を初期化
	virtual void on_clear() 
	{
		m_last = INVALID_VALUE;
	}
};

// ----------------------------------------------------------------------
// 昇順
template<typename T>
class OC_SequentialDesc : public OverlapController<T>
{
	T m_last;
public:
	OC_SequentialDesc() : m_last(INVALID_VALUE) {}

	//コントローラタイプ：あたらしいのを追加するときは必ず別IDにすること！typeidの遅い処理対策。
	virtual int type(void) {
		return 500;
	}

	// 候補から一つを選択
	virtual T select(const std::list<T>& i_candidates)
	{
		typename std::list<T>::const_reverse_iterator it = i_candidates.rbegin();
		if ( m_last != INVALID_VALUE )
		{
			while ( m_last != *it )
			{
				++it;
				if ( it == i_candidates.rend() ) { break; }
			}
			//assert( m_last == *it );

			// 直前のものより１つだけ進める 直前が無効か最後までいったら最初から
			if ( it == i_candidates.rend() )
			{
				it = i_candidates.rbegin();
			}
			else if ( ++it == i_candidates.rend() )
			{
				it = i_candidates.rbegin();
			}
		}
		return (m_last = *it);
	}

	virtual void get_selectable(const std::list<T>& i_candidates, std::list<T>& out_list)
	{
		typename std::list<T>::const_reverse_iterator it = i_candidates.rbegin();
		if (m_last != INVALID_VALUE)
		{
			while (m_last != *it)
			{
				++it;
				if (it == i_candidates.rend()) { break; }
			}
			//assert( m_last == *it );

			// 直前のものより１つだけ進める 直前が無効か最後までいったら最初から
			if (it == i_candidates.rend())
			{
				it = i_candidates.rbegin();
			}
			else if (++it == i_candidates.rend())
			{
				it = i_candidates.rbegin();
			}
		}
		out_list.push_back(*it);
	}

	//外部から選択したことにして重複回避を動かす
	virtual void apply_selected(const std::list<T>& i_candidates, T t)
	{
		for (std::list<T>::const_iterator it = i_candidates.begin(); it != i_candidates.end(); ++it)
		{
			if (*it == t)
			{
				m_last = t;
				break;
			}
		}
	}

	// 候補が消去されようとしている
	virtual void on_erase(const std::list<T>& i_candidates, typename std::list<T>::const_iterator& i_it)
	{
		if ( m_last == *i_it ) 
		{
			++i_it;
			if ( i_it == i_candidates.end() )
			{
				i_it = i_candidates.begin();
			}
			m_last = *i_it;
		}
	}
	
	// 重複回避状況を初期化
	virtual void on_clear() 
	{
		m_last = INVALID_VALUE;
	}
};


#undef INVALID_VALUE

