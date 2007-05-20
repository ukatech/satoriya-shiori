#include	"Task.h"

/*
	フローの概念：
		仕事（流れ）の単位
	フローの実装：
		タスクツリーと同等。

	フロー番号（ID)とルートタスクは１対１で対応する。

	FlowSystemはフローの切り替えを管理する。
	
	このクラスも、静的に持つよりポインタを持って new で確保するほうが
	タイミング管理上好ましいと思います。最後に持ってたtaskはデストラクタでdeleteするので。

2001/07/14 m
	削除（タイミング管理上の都合）、フロー番号定義を追加。

2001/07/14 a
	システムに唯一である理由が無いのでそのためのコードを削除。
	初期化・解放はコンストラクタ・デストラクタで行う形に。
	フロー番号に型名を持たせ、受け渡しはその型で行うように。
	コンストラクタの引数として、初回時のIDを指定するように。

2001/07/14,2 a
	FlowIDはクラス内という名前空間と重複するのでIDに変更。
	使い勝手が悪化したのでFlowSystem::Iによるアクセスを復帰。
	保持フロー解放タイミング管理用にrelease()を復帰。
	デストラクタは有名無実化。

2001/07/14,3 a
	コンストラクタ周りを変更。
	意味的な変化からreleaseをinitializeに名前変更。
	EMPTYTASKをデフォルト値に採用、無意味化したDUMMYを削除。
*/


class FlowSystem {
public:
	enum ID {
		EMPTYTASK,
		FLOWSELECT,
		TESTRENDER = 29,
	};

	// フロー切り替えを指示。次回run時に反映される。初回はrun前に必ず実行のこと。
	void	requestChange(ID iID);
	// 現在選択中のフローを実行する。
	// IDにより指示されているフローを持ってなければここで作成(new)、保持する。
	void	run();
	// フローを保持していれば解放し、初期状態に戻る。
	void	initialize();

private:
	Task*	mTask;	// 現在のフローに対応したルートタスク
	ID	mID;	// 現在のフローのID
	ID	mRequestedID;	// 実行が要求されているID

	void	remake();	// 現在のフローをdelete,RequestIDに応じた新しいフローをnew。

// 以下はこのクラスの存在をシステムで唯一とするためのコード
private:
	static FlowSystem	mInstance;
	FlowSystem();
public:
	static FlowSystem&	I;
	~FlowSystem();
};

