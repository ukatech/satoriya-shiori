# harness

SHIORI DLL（satori.dll）を `LoadLibrary` して `load` / `request` / `unload` を直接呼ぶ、動作確認用の小さなプログラム。
SSP を起動せずにリクエスト単位で応答を確認できる。

## ビルド（VC6）

```
cl /nologo /O2 harness.c
```

VC6 の `INCLUDE` / `LIB` が通っていない場合は `VC98\Include` / `VC98\Lib` を設定する。

## 使い方

```
harness <DLLのパス> <ゴーストのフォルダ(末尾に\)> <出力ファイル> <リクエストファイル1> [<リクエストファイル2> ...]
```

- `load` を1回呼び、リクエストファイルを順に `request` に渡して、最後に `unload` する。
- リクエストファイルの中身はそのままのバイト列で渡し、応答もそのままのバイト列で出力ファイルに書く。応答の間は `\n=====\n` で区切る。
- 文字コードの変換はしない。リクエストは `Charset` ヘッダに合わせたバイト列（CRLF 区切り、空行で終わる）で用意する。
- DLL のあるフォルダ（ゴーストのフォルダ）に `satori_savedata.txt` が書かれるので、ゴーストのコピーで使う。
- セーブデータの読み書きもまとめて確認したいときは、リクエストごとに harness を起動し直す（そのたびに load / unload される）。

リクエストの例：

```
GET SHIORI/3.0
Charset: UTF-8
Sender: SSP
SecurityLevel: local
ID: OnBoot
Reference0: master

```

## master との比較

master の DLL には SJIS のリクエスト、unicode 版には SJIS か UTF-8 のリクエストを渡し、応答を比べる。
自動ウェイト（`\_w[n]`）は半角換算（全角2・半角1）で数えるので、master と同じ値になる（Mc202-1 以降）。

## multi_harness

`multi_harness.c` は、マルチインスタンスのエクスポート（`multi_loadu` / `multi_request` / `multi_unload` / `multi_Set_loghandler`）を確かめる。
ゴースト A・B を `multi_loadu`、C を従来の `load` で読み込み、リクエストを交互に送って、変数・乱数・正規表現のサブルーチン名・ログのコールバックが実体どうしで混ざらないこと、片方を unload しても残りが動くこと、無効な id の扱いを調べる。全部通れば終了コード 0。

```
cl /nologo /O2 multi_harness.c user32.lib
multi_harness <satori.dllのパス> <ゴーストA\> <ゴーストB\> <ゴーストC\>
```

3つのゴーストのフォルダに、次の内容の `dic_multi.txt`（UTF-8）を置く。A と C は `＄year` の正規表現が `\d{4}`、B だけ `[a-z]{4}` にする（呼んだ実体の辞書で `(?&year)` が解決されることを見分けるため）。

```
＊OnSet
＄v＝（Reference0）
：set

＊OnGet
：v is （v）

＊OnRand
：（乱数0～99999999）

＊OnReload
＄辞書リロード＝実行
：reload

＊OnRegexSub
＄year	\d{4}
：（regex_replace、abc2026def、(?&year)、#）
```

各フォルダに `satori_savedata.txt` が書かれるので、使い捨てのコピーで使う。POSIX 版は `test/posix/posix_smoke.c` が同じ趣旨の確認（`multi_*`）をする。
