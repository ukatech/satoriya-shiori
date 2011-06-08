#include "Family.h"
#include "random.h"


// 要素は単語またはトーク。

// 同じ名前を持つ要素の集合がFamily。

// Familiesは名前により特定されるFamilyの集合。
// ほぼ map< string, Family<T> > だがpublic継承はせず、インタフェースを限定する

template<typename T>
class Families
{
    typedef typename map< string, Family<T> >::iterator iterator;
    typedef typename map< string, Family<T> >::const_iterator const_iterator;
	
	set<string> m_clearOC_at_talk_end;
	map< string, Family<T> > m_elements;
	
public:
	//Families() { cout << "Families()" << endl; }
	//~Families() { cout << "~Families()" << endl; }
	
	// 要素の登録
	const T& add_element(const string& i_name, const T& i_t, const Condition& i_condition = Condition())
	{
		std::pair<iterator,bool> found = m_elements.insert(map< string, Family<T> >::value_type(i_name,Family<T>()));
		if ( found.second ) {
			found.first->second.set_namevec(i_name);
		}
		return found.first->second.add_element(i_t, i_condition);
	}
	
	// 過去互換の提供
	const map< string, Family<T> >& compatible() const
	{
		return m_elements;
	}
	
	// 名前からFamilyを取得
	Family<T>* get_family(string i_name)
	{
		iterator i = m_elements.find(i_name);
		return ( i == m_elements.end() ) ? NULL : &(i->second);
	}
	
	// 名前の存在を確認
	bool is_exist(const string& i_name) const
	{
		return m_elements.find(i_name) != m_elements.end();
	}
	
	// Tを１つ選択し、そのポインタを返す
	const T* select(const string& i_name, Evalcator& i_evalcator)
	{
		iterator it = m_elements.find(i_name);
		if ( it == m_elements.end() ) {
			return NULL;
		}
		return it->second.select(i_evalcator);
	}

	// Tをすべて選択する
	template <typename Candidates>
	void select_all(const string& i_name, Evalcator& i_evalcator,Candidates &candidates)
	{
		iterator it = m_elements.find(i_name);
		if ( it == m_elements.end() ) {
			return;
		}
		it->second.select_all(i_evalcator,candidates);
	}
	
	// 削除
	void erase(const string& i_name)
	{
		m_elements.erase(i_name);
		m_clearOC_at_talk_end.erase(i_name);
	}
	
	// トークの終了を通知。重複制御期間が「トーク中」であるFamilyの重複回避制御をクリアする
	void handle_talk_end()
	{
		for ( set<string>::iterator it = m_clearOC_at_talk_end.begin() ; it != m_clearOC_at_talk_end.end() ; ++it )
		{
			get_family(*it)->clear_OC();
		}
	}
	
	// family数
	int size_of_family() const
	{
		return m_elements.size();
	}
	
	// 全Familyの全要素数を計算
	int size_of_element() const
	{
		int r = 0;
		for ( const_iterator it = m_elements.begin() ; it != m_elements.end() ; ++it )
		{
			r += it->second.size_of_element();
		}
		return r;
	}
	
	// 全クリア
	void clear()
	{
		m_elements.clear();
		m_clearOC_at_talk_end.clear();
	}
	
	// 重複回避制御を選択する。引数はタイプ、期間
	void setOC(string i_name, string i_value)
	{
		iterator st, ed;
		if ( i_name == "＊" )
		{
			st = m_elements.begin();
			ed = m_elements.end();
		}
		else
		{
			st = m_elements.find(i_name);
			if ( st == m_elements.end() )
			{
				sender << "'" << i_name << "' は存在しません。" << endl;
				return;
			}
			++(ed = st);
		}
		
		strvec argv;
		const int n = split(i_value, "、,", argv);
		const string method = (n>=1) ? argv[0] : "無効";
		const string span = (n>=2) ? argv[1] : "起動中";
		
		for ( iterator it = st; it != ed ; ++it )
		{
			Family<T>& family = it->second;
			if ( family.empty() )
			{
				continue;
			}
			
			if (0)
				NULL;
			else if ( method=="直前" )
				family.set_OC(new OC_NonDual<const T*>);
			else if ( method=="降順" || method=="正順" )
				family.set_OC(new OC_Sequential<const T*>);
			else if ( method=="昇順" || method=="逆順" )
				family.set_OC(new OC_SequentialDesc<const T*>);
			else if ( method=="有効" || method=="完全" )
				family.set_OC(new OC_NonOverlap<const T*>);
			else if ( method=="無効" )
				family.set_OC(new OC_Random<const T*>);
			else
				sender << "重複回避制御の方法'" << method << "' は定義されていません。" << endl;
			
			if ( span == "トーク中" )
				m_clearOC_at_talk_end.insert(it->first);
			else if ( span == "起動中")
				m_clearOC_at_talk_end.erase(it->first);
			else
				sender << "重複回避の期間'" << method << "' は定義されていません。" << endl;
			
		}
	}
	
	const Talk* communicate_search(const string& iSentence, bool iAndMode) const
	{
		sender << "文名の検索を開始" << endl;
		sender << "　対象文字列: " << iSentence << endl;
		sender << "　全単語一致モード: " << (iAndMode?"true":"false") << endl;

		vector<const_iterator> elem_vector;

		std::string::size_type sentenceNamePos = find_hz(iSentence,"「");

		bool isComNameMode = sentenceNamePos != string::npos;
		if ( isComNameMode ) {
			sender << "　「発見、名前限定モードに移行" << endl;
			for ( const_iterator it = m_elements.begin() ; it != m_elements.end() ; ++it )
			{
				if ( it->second.is_comname() ) {
					string comName = it->second.get_comname();
					if ( comName.length() ) {
						if ( comName == "「" ) { //なんでも当たる記法
							elem_vector.push_back(it);
						}
						if ( iSentence.compare(0,comName.size(),comName) == 0 ) {
							elem_vector.push_back(it);
						}
					}
				}
			}
		}
		else {
			sender << "　「なし、通常コミュ探索モードに移行" << endl;
			for ( const_iterator it = m_elements.begin() ; it != m_elements.end() ; ++it )
			{
				if ( ! it->second.is_comname() ) {
					elem_vector.push_back(it);
				}
			}
			sentenceNamePos = 0;
		}

		if ( elem_vector.size() <= 0 ) {
			sender << "結果: 該当なし（そもそも候補なし）" << endl;
			return	NULL;
		}
		
		vector<const Talk*>	result;
		int	max_hit_point=0;
		for ( vector<const_iterator>::const_iterator it = elem_vector.begin() ; it != elem_vector.end() ; ++it )
		{
			// 語群を全角スペースで区切る
			const strvec &words = (**it).second.get_namevec();
			
			// いくつの単語がヒットしたか。単語１つで10てん、長さ１もじで1てん
			int	hit_point=0;
			strvec::const_iterator wds_it=words.begin();
			if ( isComNameMode ) { wds_it += 1; }; //ひとつめは名前（すでに抽出済）

			for ( ; wds_it!=words.end() ; ++wds_it )
			{
				if ( find_hz(iSentence,*wds_it,sentenceNamePos) != string::npos )
				{
					if ( (!isComNameMode) && compare_tail(*wds_it, "「") )	// 末尾が 「 であるものだけの場合はヒットと見なさないように。
						hit_point += 4;
					else
						hit_point += 10+(wds_it->size()/4);	// 一致した単語。
				}
				else
				{
					hit_point -= (iAndMode ? 999 : 1);	// 一致しなかった、見つからなかった単語
				}
			}
			if ( hit_point<=4 )
			{
				continue;	// いっこも一致しない場合
			}
			
			sender << "'" << (**it).first << "' : " << hit_point << "pt ,";
			
			if ( hit_point<max_hit_point) {
				sender << "却下" << endl;
				continue;
			} else if ( hit_point == max_hit_point ) {
				sender << "候補として追加" << endl;
			} else {
				max_hit_point = hit_point;
				sender << "単独で採用" << endl;
				result.clear();
			}
			
			
			(**it).second.get_elements_pointers(result);
		}
		
		sender << "結果: ";
		if ( result.size() <= 0 ) {
			sender << "該当なし（検索候補あり、単語検索失敗）" << endl;
			return	NULL;
		}
		
		return result[ random(result.size()) ];
	}
};



