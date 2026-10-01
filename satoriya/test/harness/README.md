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
自動ウェイト（`\_w[n]`）は unicode 版で値が変わるのが正しいので、比較では除く。
