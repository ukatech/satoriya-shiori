#include "Family.h"
#include "random.h"


// 要素は単語またはトーク。

// 同じ名前を持つ要素の集合がFamily。

// Familiesは名前により特定されるFamilyの集合。
// ほぼ map< string, Family<T> > だがpublic継承はせず、インタフェースを限定する

typedef enum {
	COMSEARCH_DEFAULT,
	COMSEARCH_LENGTH,
	COMSEARCH_TAG
} FamilyComSearchType;

template<typename T>
class Families
{
    typedef typename std::map< wstring, Family<T> >::iterator iterator;
    typedef typename std::map< wstring, Family<T> >::const_iterator const_iterator;
	
	std::set<wstring> m_clearOC_at_talk_end;
	std::map< wstring, Family<T> > m_elements;
	
public:
	//Families() { cout << "Families()" << endl; }
	//~Families() { cout << "~Families()" << endl; }
	
	// 要素の登録
	const T& add_element(const wstring& i_name, const T& i_t, const Condition& i_condition = Condition())
	{
		std::pair<iterator,bool> found = m_elements.insert(std::pair<wstring, Family<T> >(i_name,Family<T>()));
		//std::pair<iterator,bool> found = m_elements.insert(map< string, Family<T> >::value_type(i_name,Family<T>()));
		if ( found.second ) {
			found.first->second.set_namevec(i_name);
		}
		return found.first->second.add_element(i_t, i_condition);
	}
	
	// 過去互換の提供
	const std::map< wstring, Family<T> >& compatible() const
	{
		return m_elements;
	}
	
	// 名前からFamilyを取得
	Family<T>* get_family(wstring i_name)
	{
		iterator i = m_elements.find(i_name);
		return ( i == m_elements.end() ) ? NULL : &(i->second);
	}
	
	// 名前の存在を確認
	bool is_exist(const wstring& i_name) const
	{
		return m_elements.find(i_name) != m_elements.end();
	}
	
	// Tを１つ選択し、そのポインタを返す
	const T* select(const wstring& i_name, Evalcator& i_evalcator)
	{
		iterator it = m_elements.find(i_name);
		if ( it == m_elements.end() ) {
			return NULL;
		}
		return it->second.select(i_evalcator);
	}

	// Tをすべて選択する
	template <typename Candidates>
	void select_all(const wstring& i_name, Evalcator& i_evalcator,Candidates &candidates)
	{
		iterator it = m_elements.find(i_name);
		if ( it == m_elements.end() ) {
			return;
		}
		it->second.select_all(i_evalcator,candidates);
	}
	
	// 削除
	void erase(const wstring& i_name)
	{
		m_elements.erase(i_name);
		m_clearOC_at_talk_end.erase(i_name);
	}
	
	// トークの終了を通知。重複制御期間が「トーク中」であるFamilyの重複回避制御をクリアする
	void handle_talk_end()
	{
		for ( std::set<wstring>::iterator it = m_clearOC_at_talk_end.begin() ; it != m_clearOC_at_talk_end.end() ; ++it )
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
	void setOC(wstring i_name, wstring i_value)
	{
		iterator st, ed;
		if ( i_name == L"＊" )
		{
			st = m_elements.begin();
			ed = m_elements.end();
		}
		else
		{
			st = m_elements.find(i_name);
			if ( st == m_elements.end() )
			{
				GetSender().sender() << L"'" << i_name << L"' は存在しません。" << std::endl;
				return;
			}
			++(ed = st);
		}
		
		//無効、起動中　など複数指定の場合がある
		strvec argv;
		const int n = split(i_value, L"、,", argv);
		const wstring method = (argv.size()>=1) ? argv[0] : L"無効";
		const wstring span = (argv.size()>=2) ? argv[1] : L"起動中";
		
		for ( iterator it = st; it != ed ; ++it )
		{
			Family<T>& family = it->second;
			if ( family.empty() )
			{
				continue;
			}
			
			if ( method==L"直前" )
				family.set_OC(new OC_NonDual<const T*>);
			else if ( method==L"降順" || method==L"正順" )
				family.set_OC(new OC_Sequential<const T*>);
			else if ( method==L"昇順" || method==L"逆順" )
				family.set_OC(new OC_SequentialDesc<const T*>);
			else if ( method==L"有効" || method==L"完全" )
				family.set_OC(new OC_NonOverlap<const T*>);
			else if ( method==L"無効" )
				family.set_OC(new OC_Random<const T*>);
			else
				GetSender().sender() << L"重複回避制御の方法'" << method << L"' は定義されていません。" << std::endl;
			
			if ( span == L"トーク中" )
				m_clearOC_at_talk_end.insert(it->first);
			else if ( span == L"起動中")
				m_clearOC_at_talk_end.erase(it->first);
			else
				GetSender().sender() << L"重複回避の期間'" << span << L"' は定義されていません。" << std::endl;
			
		}
	}

	bool isOCUsedAll(const wstring& i_name)
	{
		iterator st, ed;
		if (i_name == L"＊")
		{
			//全対象には使えない
			return false;
		}
		else
		{
			st = m_elements.find(i_name);
			if (st == m_elements.end())
			{
				GetSender().sender() << L"'" << i_name << L"' は存在しません。" << std::endl;
				return false;
			}
			Family<T>& family = st->second;
			return family.is_OC_used_all();
		}
	}
	
	const Talk* communicate_search(const wstring& iSentence, bool iAndMode, FamilyComSearchType type, Evalcator& i_evalcator)
	{
		GetSender().sender() << L"文名の検索を開始" << std::endl;
		GetSender().sender() << L"　対象文字列: " << iSentence << std::endl;
		GetSender().sender() << L"　全単語一致モード: " << (iAndMode?L"true":L"false") << std::endl;

		std::vector<iterator> elem_vector;

		std::wstring::size_type sentenceNamePos = find_hz(iSentence,L"「");

		bool isComNameMode = sentenceNamePos != wstring::npos;
		if ( isComNameMode ) {
			GetSender().sender() << L"　「発見、名前限定モードに移行" << std::endl;
			for ( iterator it = m_elements.begin() ; it != m_elements.end() ; ++it )
			{
				if ( it->second.is_comname() ) {
					wstring comName = it->second.get_comname();
					if ( comName.length() ) {
						if ( comName == L"「" ) { //なんでも当たる記法
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
			GetSender().sender() << L"　「なし、通常コミュ探索モードに移行" << std::endl;
			for ( iterator it = m_elements.begin() ; it != m_elements.end() ; ++it )
			{
				if ( ! it->second.is_comname() ) {
					elem_vector.push_back(it);
				}
			}
			sentenceNamePos = 0;
		}

		if ( elem_vector.size() <= 0 ) {
			GetSender().sender() << L"結果: 該当なし（そもそも候補なし）" << std::endl;
			return	NULL;
		}
		
		std::vector<iterator> hit_vector;
		int	max_hit_point=0;
		for ( typename std::vector<iterator>::iterator it = elem_vector.begin() ; it != elem_vector.end() ; ++it )
		{
			// 語群を全角スペースで区切る
			const strvec &words = (**it).second.get_namevec();
			
			// いくつの単語がヒットしたか。単語１つで10点＋α、長さ１文字で1点。一致した部分文字列が長いほどボーナスあり。
			int	hit_point=0;
			strvec::const_iterator wds_it=words.begin();
			if ( isComNameMode ) { wds_it += 1; }; //ひとつめは名前（すでに抽出済）

			for ( ; wds_it!=words.end() ; ++wds_it )
			{
				bool test = false;
				if (type == COMSEARCH_TAG) {
					//+2は空白とカッコ分（全角2文字）。「が末尾にあるときは後ろが無いので空
					std::wstring s = (sentenceNamePos + 2 <= iSentence.size()) ? iSentence.substr(sentenceNamePos + 2) : std::wstring();
					test = s == *wds_it;
				}
				else {
					test = find_hz(iSentence, *wds_it, sentenceNamePos) != std::wstring::npos;
				}

				if ( test )
				{
					if ( (!isComNameMode) && compare_tail(*wds_it, L"「") ) { // 末尾が 「 であるものだけの場合はヒットと見なさないように。
						hit_point += 4;
					}
					else {
						//単語一致。点数計算。
						if ( type == COMSEARCH_LENGTH ) {
							hit_point += 10*count_chars(*wds_it);
						}
						else if (type == COMSEARCH_TAG){
							hit_point = 100;
						}
						else {
							hit_point += 10+(count_chars(*wds_it)/2);	// SJIS時代のバイト数/4相当
						}
					}
				}
				else
				{
					if (type != COMSEARCH_LENGTH && type != COMSEARCH_TAG)//タグ検索・合計文字数でコミュニケートヒットを算出する場合、減点を回避する
					{
						hit_point -= (iAndMode ? 999 : 1);	// 一致しなかった、見つからなかった単語
					}
				}
			}
			if ( hit_point<=4 )
			{
				continue;	// いっこも一致しない場合
			}
			
			GetSender().sender() << L"'" << (**it).first << L"' : " << hit_point << L"pt ,";
			
			if (type == COMSEARCH_TAG)
			{
				if (hit_point <= 0)
				{
					continue;
				}
			}
			else
			{
				if (hit_point < max_hit_point) {
					GetSender().sender() << L"却下" << std::endl;
					continue;
				}
				else if (hit_point == max_hit_point) {
					GetSender().sender() << L"候補として追加" << std::endl;
				}
				else {
					max_hit_point = hit_point;
					GetSender().sender() << L"単独で採用" << std::endl;
					hit_vector.clear();
				}
			}
			
			hit_vector.push_back(*it);
		}

		//重複回避のチェック
		std::vector<const Talk*>	result;
		bool is_remain_talk = false;
		for (typename std::vector<iterator>::iterator it = hit_vector.begin(); it != hit_vector.end(); ++it)
		{
			//選択候補が残っているか確認
			if (!(**it).second.is_OC_used_all(i_evalcator))
			{
				//のこっているものが１つでもあればOK
				is_remain_talk = true;
			}
		}

		//候補ランダムリストの作成
		for (typename std::vector<iterator>::iterator it = hit_vector.begin(); it != hit_vector.end(); ++it)
		{
			if (!is_remain_talk)
			{
				//選べるものがないのでリセット
				(**it).second.clear_OC();
			}
			(**it).second.get_elements_pointers_selectables(result, i_evalcator);
		}

		
		GetSender().sender() << L"結果: ";
		if ( result.size() <= 0 ) {
			GetSender().sender() << L"該当なし（検索候補あり、単語検索失敗）" << std::endl;
			return	NULL;
		}

		const Talk* res = result[ random(result.size()) ];

		//選択したものを重複回避に渡し直す
		//外部選択になっているので例外的…
		for (typename std::vector<iterator>::iterator it = elem_vector.begin(); it != elem_vector.end(); ++it)
		{
			(**it).second.applySelectedOC(i_evalcator, res);
		}
		
		return res;
	}
};



