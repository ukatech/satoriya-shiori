
#include	"Selector.h"

#include <string>
#include <algorithm>
#include <map>
using namespace std;

typedef string Condition;

class Evalcator
{
public:
	virtual bool evalcate_to_bool(const Condition&) =0;

	virtual ~Evalcator(void) { }
};



// 同じ名前を持つ一群。
template<typename T>
class Family
{
	strvec NameVector;
	bool ComNameFound;

	// 条件式をキー、要素リストを値としたmap
	typedef vector<T> Elements;
	typedef map<Condition, Elements> CondsMap;
	CondsMap m_conds_map;

#if 0
	// mapだと条件式を含む場合に順番が死ぬわけで

	struct Condition
	{
		string cond_str;
		enum { NOT_EVAL, TRUE, FALSE } eval_result;
		int ref_coutner;
	};
	//みたいなのを別に持って

	class Conditions : set<Condition> {
		iterator ref(string name);
		void unref(iterator);
	};

	class Elements
	{
		T m_t;
		Conditions::iterator m_condition;
	public:
		Elements(Conditions i_conditions) {
			m_condition = i_conditions.ref();
		}
		~Elements() {
			m_condition.unref(m_condition)
		}

		bool eval(Evalcator)

	};
/*
	vector<Elements> m_elements;
	とかかなあ
	- - - - - - - - - - - - - - - - - - 
	Familyが「条件式」を持ってるのがそもそもの間違いだわね。
	条件式へのポインタだけもって、条件式自体は独立させた方がいい。
	「同一の条件式は一回だけ評価される」を前提とするならば。
	速度を考えないなら全要素個数分評価しなおすのが正しい気がするが……
	- - - - - - - - - - - - - - - - - - 
  　条件式を外部に持っていくべきか。
　　必要に応じて評価し、結果をキャッシュし、任意タイミングで評価をリセットする。
  　Evalcatorに保持させるとか。
*/

#endif



	// ユーザ定義の重複回避設定を適用するための「選択係」。
	// Word/Talkへのポインタを候補として保持する。
	Selector<const T*> m_selector;


public:
	Family() {
		//cout << "Family()" << endl;
		ComNameFound = false;
	}
	//~Family() { cout << "~Family()" << endl; }


	template<typename C>
	void get_elements_pointers(C& o_c) const
	{
		for ( typename CondsMap::const_iterator i = m_conds_map.begin() ; i != m_conds_map.end() ; ++i )
		{
			for ( typename Elements::const_iterator j = i->second.begin() ; j != i->second.end() ; ++j )
			{
				o_c.push_back(&(*j));
			}
		}
	}

	void set_namevec(const std::string &name)
	{
		split(name,"\t　",NameVector);
		
		for ( strvec::iterator wds_it=NameVector.begin() ; wds_it!=NameVector.end() ; ++wds_it ) {
			if ( compare_tail(*wds_it, "「") ) {
				if ( wds_it != NameVector.begin() ) {
					string comName = *wds_it;
					NameVector.erase(wds_it);
					NameVector.insert(NameVector.begin(),comName);
				}
				ComNameFound = true;
				break;
			}
		}
	}
	const strvec& get_namevec(void) const
	{
		return NameVector;
	}
	string get_comname(void) const
	{
		if ( ! ComNameFound || NameVector.empty() ) { return ""; }
		return NameVector[0];
	}
	bool is_comname(void) const
	{
		return ComNameFound && !NameVector.empty();
	}

	bool empty() const { return m_conds_map.empty(); }
	int size_of_element() const 
	{
		int r = 0;
		for ( typename CondsMap::const_iterator i = m_conds_map.begin() ; i != m_conds_map.end() ; ++i )
		{
			r += i->second.size();
		}
		return r;
	}
	bool is_exist_element(const T& i_t) const
	{
		for ( typename CondsMap::const_iterator i = m_conds_map.begin() ; i != m_conds_map.end() ; ++i )
			for ( typename Elements::const_iterator j = i->second.begin() ; j != i->second.end() ; ++j )
				if ( *j == i_t )
					return true;
		return false;
	}

	// 登録
	const T& add_element(const T& i_t, const Condition& i_condition = Condition())
	{
		Elements& lt = m_conds_map[i_condition];
		lt.push_back(i_t);
		return lt.back();
	}
	// 削除
	void delete_element(const T& i_t, const Condition& i_condition = Condition())
	{
		CondsMap::iterator it = m_conds_map.find(i_condition);
		if ( it == m_conds_map.end() ) { return; }
		Elements& lt = it->second;
		lt.erase(remove(lt.begin(),lt.end(),i_t),lt.end());
		if ( lt.empty() ) {
			m_conds_map.erase(it);
		}
	}
	
	// 重複回避を設定
	void set_OC(OverlapController<const T*>* i_oc)
	{
		m_selector.attach_OC(i_oc);
	}
	// 重複回避状況をクリア
	void clear_OC()
	{
		m_selector.clear_OC();
	}

	// 条件式を評価して候補をすべて選ぶ。
	// 引数は「評価者」と候補のvector。
	template <typename Candidates>
	void select_all(Evalcator& i_evalcator,Candidates &candidates)
	{
		assert(m_conds_map.size() > 0);
		if ( m_conds_map.size() == 1 && m_conds_map.begin()->first.empty() ) {
			// 無条件の群のみが存在する（条件式導入前と同じ状態）
			Elements& e = m_conds_map.begin()->second;
			assert(e.size() > 0);

			for ( typename vector<T>::const_iterator j = e.begin() ; j != e.end() ; ++j ) {
				candidates.push_back( &(*j) );
			}
		}
		else {
			//  候補を選択
			cout << "selecting" << endl;
			for ( typename CondsMap::const_iterator i = m_conds_map.begin() ; i != m_conds_map.end() ; ++i )
			{
				// 「無条件」であるか「条件式を評価した結果、0/０を返さなかったもの」を採用
				if ( i->first.empty() || i_evalcator.evalcate_to_bool(i->first) )
				{
					for ( typename vector<T>::const_iterator j = i->second.begin() ; j != i->second.end() ; ++j )
					{
						candidates.push_back( &(*j) );
						cout << "[" << *j << "]" << endl;
					}
				}
			}
		}
	}

	// 選んだ候補からさらにユーザ定義の重複回避設定を適用した選択を行う。
	// 引数は「評価者」。
	const T* select(Evalcator& i_evalcator)
	{
		list<const T*> candidates;
		select_all(i_evalcator,candidates);

		if ( candidates.empty() ) {
			return NULL;
		}
		else if ( candidates.size() == 1 ) {
			return *candidates.begin();
		}
		else {
			m_selector.update_candidates(candidates);
			return	m_selector.select();
		}
	}
};
