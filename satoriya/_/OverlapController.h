/*
	「指定された最大値の範囲で整数を選択する」ためのいろいろ。

  　選択方法はいろいろ。
  　ランダムとか重複回避とか順番とか。

	「重複回避状況」は保存/復帰が可能。
*/

#include	<iostream>	// シリアライズ用
#include	<cassert>

int	random(); // 32bitの適当な乱数値を返しといてください。
string	stringf(const char* iFormat, ...); // printf互換で文字列を生成し、string型で返す。

// 整数選択クラスのベースクラス。
class OverlapController {

private:
	int	mSize; // 最大数。0～この数-1が選択候補となる。
	int	mChoiceCount; // 選択回数
	
	//int	mTotalChoiceCount;	// 選択
	//int	mResetCount;

protected:
	// 「１つ選択」の実装。
	virtual int choice_impl()=0;
	// サイズ変更の実装。これ呼んでる間はsize()は元の値を返す。
	// 重複回避状況が保持されているかどうかを返す。
	virtual bool resize_impl(int) { return true; }
	// 重複回避状況クリアの実装。
	virtual void clear_impl() {}

	virtual string	serialize_impl() const =0;
	virtual void	unserialize_impl(string)=0;
	
public:
	OverlapController(int iSize) : mSize(iSize), mChoiceCount(0) { assert(iSize>0); }
	virtual ~OverlapController() { clear(); }

	// １つ選択して返す
	int choice() {
		++mChoiceCount;
		return	choice_impl();
	}

	// 重複回避状況のクリア
	void clear() {
		clear_impl();
		mChoiceCount = 0;
	}

	// 最大値の変更。
	bool resize(int iSize) {
		assert(iSize > 0);
		bool r = resize_impl(iSize);
		if (r)
			mChoiceCount = 0;
		mSize = iSize;
		return r;
	}

	// 最大値の取得
	int size() const { assert(mSize>0); return mSize; }
	// 最後に重複回避状況がリセットされてから、choiceが呼ばれた回数の取得
	int	choice_count() const { return mChoiceCount; }

	// 保存と復帰。
	// そのクラスのインスタンスがserializeしたものを、同一クラスのインスタンスがunserializeした際、
	// 重複回避状況が再現されればそれで良い。
	friend  ostream&	operator<<(ostream& out, const OverlapController& oc);
	friend  istream&	operator>>(istream& in, OverlapController& oc);
};
inline ostream&	operator<<(ostream& out, const OverlapController& oc) {
	out << "1 " << oc.mSize << " ";
	return out;
}
inline istream&	operator>>(istream& in, OverlapController& oc) {
	int	version;
	in >> version;
	in >> oc.mSize;
	return in;
}


// ランダムな選択
class OC_Random : public OverlapController {

	// １つ選択して返す
	virtual	int	choice_impl() {
		return	((unsigned)random()) % size();
	}

public:
	OC_Random(int iSize) : OverlapController(iSize) {}
	virtual ~OC_Random() {}
};

// 順番に選択
class OC_Sequential : public OverlapController {

	int	mIndex;

	// １つ選択して返す
	virtual int choice_impl() {
		if ( mIndex >= size() )
			mIndex = 0;
		return	mIndex++;
	}
	
	// 最大値の変更。
	virtual bool resize_impl(int iSize) {
		if ( iSize > mIndex )
			return	true;
		mIndex = 0;
		return	false;
	}

	// 重複回避状況のクリア
	virtual void clear_impl() {
		mIndex = 0;
	}
	
public:
	OC_Sequential(int iSize) : OverlapController(iSize), mIndex(0) {}
	virtual ~OC_Sequential() {}
};

// 逆順に選択
class OC_SequentialDesc : public OverlapController {

	int	mIndex;

	// １つ選択して返す
	virtual int choice_impl() {
		if ( --mIndex < 0 )
			mIndex = size()-1;
		return	mIndex;
	}
	
	// 最大値の変更。
	virtual bool resize_impl(int iSize) {
		// 検討の余地はあるが、現indexが不正になった場合だけクリアとする。増えた分は後から選ぶ。
		// 現index以上の場合は関係ない。
		if ( iSize > mIndex )
			return	true;
		mIndex = 0;
		return	false;
	}

	// 重複回避状況のクリア
	virtual void clear_impl() {
		mIndex = 0;
	}
	

public:
	OC_SequentialDesc(int iSize) : OverlapController(iSize), mIndex(0) {}
	virtual ~OC_SequentialDesc() {}
};

// 直前のものだけ重複回避
class OC_NonDual : public OverlapController {

	int	mLast;

	// １つ選択して返す
	virtual int choice_impl() {
		if ( size()<2 )
			return	0; // 常にゼロ

		int	theSelect;
		do {
			theSelect = ((unsigned)random()) % size();
		} while ( theSelect == mLast ); // 直前と同じでさえなければ構わない

		mLast = theSelect;
		return	theSelect;
	}

	// 最大値の変更。
	virtual bool resize_impl(int iSize) {
		if ( mLast < iSize ) // 無効化されてなければ
			return	true;	 // 状況に変化はない
		mLast = -1;
		return	false;
	}

	// 重複回避状況のクリア
	virtual void clear_impl() {
		mLast = -1;
	}

public:
	OC_NonDual(int iSize) : mLast(-1), OverlapController(iSize) {}
	virtual ~OC_NonDual() {}

};

// 完全に一巡するまで重複しない
#include	<deque>
class OC_NonOverlap : public OverlapController {

	std::deque<int>	mArray; // 選択済みを判定する配列
	int	mRestCount;				// また選択していない残り数

	// １つ選択して返す
	virtual int choice_impl() {
		if ( mArray.empty() )
			init(size());

		assert(mRestCount > 0);
		int select = ((unsigned)random()) % mRestCount; // 現在残ってる中のどれかを選ぶ。

		// ボールを敷居の手前と取り替える
		int	ball = mArray[select];
		mArray[select] = mArray[mRestCount-1];
		mArray[mRestCount-1] = ball;

		// 敷居をずらす（残数を減らす）。なくなったら全部未使用扱い。
		if ( --mRestCount==0 )
			mRestCount = size();

		return	ball;	// 選んだボールを返す
	}
	
	// 最大値の変更。
	virtual bool resize_impl(int iSize) {
		if ( iSize < size() ) { 
			// 小さくなったなら作り直し
			init(iSize);
			return	false;
		}
		else {
			// 大きくなったならそのサイズまでボールを追加。
			for (int i=size(); i<iSize ; ++i )
				mArray.push_front(i);
			mRestCount += iSize - size();
			return	true; // 重複回避状態は維持される
		}
	}
	
	// 重複回避状況のクリア
	virtual void clear_impl() {
		mRestCount = size();
	}
	
	// mArrayの初期化。必然的にmRestCountも初期化。
	void	init(int iSize) {

		// 順番はどうでもよくて、単に一意にボールを詰める
		mArray.resize(iSize);
		for (int i=0 ; i<iSize ; ++i) 
			mArray[i] = i;

		mRestCount = iSize;
	}
	
public:
	OC_NonOverlap(int iSize) : OverlapController(iSize) {}
	virtual ~OC_NonOverlap() {}
};



#include	<string>
using std::string;
#include	<vector>
using std::vector;


// これが無効なのはどういう時だ？
// サイズがゼロの時とかか。そーゆー時はChoiceしちゃいけないわけだ。
template<typename T>
class OverlapControllableVector : public vector<T> {
public:
	enum Mode { MIN_DUMMY=-1, RANDOM=0, SEQUENTIAL, SEQUENTIAL_DESC, NONDUAL, NONOVERLAP, MAX_DUMMY };

public:

	OverlapControllableVector() : mOC(NULL), mMode(MIN_DUMMY), vector<T>() {
	}
	
	~OverlapControllableVector() {
		if ( mOC!=NULL )
			delete mOC;
	}

	// １つ取得
	const T&		choice() { // OC使って要素を１つ選択
		assert( !empty() );	 // 空のときにはそもそも呼んではいけない

		if ( mOC==NULL )
			selectOC(RANDOM);

		if ( mOC->size() != size() )// OCとサイズが違うなら、
			mOC->resize(size());	// OC側をサイズ変更する。初期化されうる。

		const_iterator i=begin();
		advance(i, mOC->choice());
		return	*i;
	}

	// 重複回避設定を初期化
	void	clearOC() {
		if ( mOC!=NULL )
			mOC->clear();
	}
	
	// モード切り替え。重複回避設定は初期化される。
	void	selectOC(Mode iMode) {
		assert( iMode>MIN_DUMMY && iMode<MAX_DUMMY ); // モードは範囲内である必要がある
		assert( !empty() );
		if ( mMode == iMode )
			return; // 変化せず。

		if ( mOC != NULL )
			delete mOC; // 既存するなら消しておく

		mMode = iMode;
		switch ( mMode ) {
		case RANDOM: mOC = new OC_Random(size()); break;
		case SEQUENTIAL: mOC = new OC_Sequential(size()); break;
		case SEQUENTIAL_DESC: mOC = new OC_SequentialDesc(size()); break;
		case NONDUAL: mOC = new OC_NonDual(size()); break;
		case NONOVERLAP: mOC = new OC_NonOverlap(size()); break;
		default: assert(0); break;
		}
	}

	// OCの保存と復帰
	string	serializeOC() {
		//stringf("mode=%x,"
	}
	void	deserializeOC(const string&) {
		//sscanf
	}


private:
	OverlapController*	mOC;
	Mode	mMode;
};


/*
// モード切り替え
void	overlap_control::setMode(Mode iMode) {
	if ( iMode == mMode )
		return;
	mMode = iMode;
	if ( iMode == RANDOM )
		
}

string	overlap_control::serialize() {
	if ( mMode == NONOVERLAP ) {
		string	str = "2:";
		
	}
	else if ( mMode == SEQUENTIAL ) {
		char	buf[256];
		sprintf("1:%d:", mIndex);
	}
	else 
		return	"0:";
}
void	overlap_control::deserialize(const string&) {

}
*/
