
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
	OverlapController() {}
	virtual ~OverlapController() {}

	// 候補から一つを選択
	// 候補は一つ以上あることが保証されている。
	virtual T select(const list<T>&) =0;

	virtual int type(void) = 0;


	// イベント通知ハンドラ

	// 候補が追加された
	virtual void on_add(const list<T>& i_candidates, typename list<T>::const_iterator i_it) {}
	// 候補が消去されようとしている
	virtual void on_erase(const list<T>& i_candidates, typename list<T>::const_iterator i_it) {}
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
	virtual T select(const list<T>& i_candidates)
	{
		typename list<T>::const_iterator it = i_candidates.begin();
		advance( it, random(i_candidates.size()) );
		return *it;
	}
};

// ----------------------------------------------------------------------
// 全て使い切るまで重複回避
template<typename T>
class OC_NonOverlap : public OverlapController<T>
{
	set<T> m_used;
	set<T> m_unused;
	T m_last;

public:
	OC_NonOverlap() : m_last(INVALID_VALUE) {}

	//コントローラタイプ：あたらしいのを追加するときは必ず別IDにすること！typeidの遅い処理対策。
	virtual int type(void) {
		return 200;
	}

	// 候補から一つを選択
	virtual T select(const list<T>&)
	{
		for ( ; ; ) {
			// 「未使用」が空っぽなら「使用済み」を全て「未使用」にする。
			if ( m_unused.empty() )
			{
				m_used.swap( m_unused );
			}

			// 「未使用」からランダムに一つを選び出す
			typename set<T>::iterator it = m_unused.begin();
			advance( it, random(m_unused.size()) );

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

	// 候補が追加された
	virtual void on_add(const list<T>&, typename list<T>::const_iterator i_it)
	{
		m_unused.insert(*i_it);
	}

	// 候補が消去されようとしている
	virtual void on_erase(const list<T>&, typename list<T>::const_iterator i_it)
	{
		// 現在どっちにあるかわからないので、両方に指示を出す
		m_unused.erase(*i_it);
		m_used.erase(*i_it);
		m_last = INVALID_VALUE;
	}
	
	// 重複回避状況を初期化
	virtual void on_clear() 
	{
		for ( typename set<T>::const_iterator it = m_used.begin() ; it != m_used.end() ; ++it )
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
	virtual T select(const list<T>& i_candidates)
	{
		// 一個しか無いなら回避のしようがない
		if ( i_candidates.size() == 1 )
		{
			return *(i_candidates.begin());
		}
		
		// ランダムに一つ選ぶ
		typename list<T>::const_iterator it = i_candidates.begin();
		advance( it, random(i_candidates.size()) );
		
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

	// 候補が消去されようとしている
	virtual void on_erase(const list<T>& i_candidates, typename list<T>::const_iterator i_it)
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
	virtual T select(const list<T>& i_candidates)
	{
		typename list<T>::const_iterator it = i_candidates.begin();
		if ( m_last != INVALID_VALUE )
		{
			while ( m_last != *it )
			{
				++it;
			}
			assert( m_last == *it );

			// 直前のものより１つだけ進める
			if ( ++it == i_candidates.end() )
			{
				it = i_candidates.begin();
			}
		}
		return (m_last = *it);
	}

	// 候補が消去されようとしている
	virtual void on_erase(const list<T>& i_candidates, typename list<T>::const_iterator& i_it)
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
	virtual T select(const list<T>& i_candidates)
	{
		typename list<T>::const_reverse_iterator it = i_candidates.rbegin();
		if ( m_last != INVALID_VALUE )
		{
			while ( m_last != *it )
			{
				++it;
			}
			assert( m_last == *it );

			// 直前のものより１つだけ進める
			if ( ++it == i_candidates.rend() )
			{
				it = i_candidates.rbegin();
			}
		}
		return (m_last = *it);
	}

	// 候補が消去されようとしている
	virtual void on_erase(const list<T>& i_candidates, typename list<T>::const_iterator& i_it)
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

