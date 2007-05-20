
/*------------------------------------------------------------------
	BinaryTree Element

            ○parent
            ｜    
           ／＼
     this●    ○brother
         ｜    
        ／＼
      ○    ○
left-child    right-child


	'this' has pointer to children, pointer to parent, and template instance.

	親子の接続は、親側がLeftまたはRightに設定すること。
	子側からでは親のどちらに接続するか判定できないため。

	methods:

	BTE();
	BTE( const T& iT );
	~BTE();

	void	LeftChild( BTE* iLeftChild );	// 左子を接続
	void	RightChild( BTE* iRightChild );	// 右子を接続
	void	Independent();					// 親から独立

	BTE*	LeftChild();	// 左子取得
	BTE*	RightChild();	// 右子取得
	BTE*	Parent();		// 親取得
	T&	operator * ();		// 実体を取得

------------------------------------------------------------------*/

#include	<assert.h>

#ifdef	DBG
#undef	DBG
#endif	// DBG

#ifdef	_DEBUG
#define	DBG(v)	v
#else	// _DEBUG
#define	DBG(v)	((void)0)
#endif	// _DEBUG

template<class T>
class BTE {

private:
	BTE*	mLeftChild;
	BTE*	mRightChild;
	BTE*	mParent;
	T		mT;
	
	BTE( const BTE& );
	BTE& operator = ( const BTE& );

public:
	void	LeftChild( BTE* iLeftChild ) {
		assert(iLeftChild != this);
		DBG( CheckValid() );

		// 現在の左子を切り離す
		if ( mLeftChild != NULL ) {
			mLeftChild->mParent = NULL;
			mLeftChild = NULL;
		}
		
		// 新しい左子を接続する
		if ( iLeftChild != NULL ) {
			iLeftChild->mParent = this;
			mLeftChild = iLeftChild;
		}
	}
	void	RightChild( BTE* iRightChild ) {
		assert(iRightChild != this);
		DBG( CheckValid() );

		// 現在の右子を切り離す
		if ( mRightChild != NULL ) {
			mRightChild->mParent = NULL;
			mRightChild = NULL;
		}
		
		// 新しい右子を接続する
		if ( iRightChild != NULL ) {
			iRightChild->mParent = this;
			mRightChild = iRightChild;
		}
	}
	// 親から独立
	void	Independent() {
		DBG( CheckValid() );
		if ( mParent == NULL )
			NULL;
		else if ( mParent->LeftChild() == this )
			mParent->LeftChild(NULL);
		else if ( mParent->RightChild() == this )
			mParent->RightChild(NULL);
	}

	// メンバ取得
	BTE*	LeftChild() {
		DBG( CheckValid() );
		return mLeftChild; 
	}
	BTE*	RightChild() {
		DBG( CheckValid() );
		return mRightChild;
	}
	BTE*	Parent() {
		DBG( CheckValid() );
		return mParent;
	}
	T&	operator * () {
		DBG( CheckValid() );
		return mT;
	}
	T*	operator -> () {
		DBG( CheckValid() );
		return &mT;
	}
	T&	Real() {
		DBG( CheckValid() );
		return mT;
	}

	// デフォルトコンストラクタ
	BTE()
	 : mLeftChild(NULL)
	 , mRightChild(NULL)
	 , mParent(NULL)
	{
	}
	// T の値をコピーしてコンストラクト
	BTE( const T& iT )
	 : mLeftChild(NULL)
	 , mRightChild(NULL)
	 , mParent(NULL)
	 , mT(iT)
	{
	}
	// デストラクタ
	~BTE() {
		DBG( CheckValid() );
		LeftChild(NULL);
		RightChild(NULL);
		Independent();
	}

	// 孫子に渡って再帰処理
	template<class Function>
	void	Each( Function iFunction ) {
		if ( mLeftChild != NULL )
			mLeftChild->Each( iFunction );
		if ( mRightChild != NULL )
			mRightChild->Each( iFunction );
		iFunction( this );
	}

	// 正当性チェック。構築後に有効。
	void	CheckValid() {
		// 私は存在する。
		assert( this!=NULL );
		// 私に親がいるとき、私はその親の子である。
		assert( mParent==NULL || (mParent->mLeftChild==this || mParent->mRightChild==this ) );
		// 私に子がいれば、その親は私である。
		assert( mLeftChild==NULL || mLeftChild->mParent==this );
		assert( mRightChild==NULL || mRightChild->mParent==this );
	}
};


