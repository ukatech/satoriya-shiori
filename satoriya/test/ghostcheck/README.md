# ghostcheck

里々の辞書を `libsatori.so` で実際に動かして、壊れていないかを確かめる Linux 用の小さなチェッカーです。
AI に書かせた辞書を定期的に確かめる用途を想定しています（SSP は要りません）。

## 準備

```
git submodule update --init --depth 1 satoriya/uka_regexp
make -C satoriya/satori -f makefile.linux -j"$(nproc)"      # satoriya/satori/libsatori.so ができる
```

必要なもの: gcc / g++ / make / iconv / Python 3。辞書は UTF-8 で書きます（unicode ブランチ向け）。

## 使い方

```
python3 satoriya/test/ghostcheck/satori_check.py satoriya/satori/libsatori.so <辞書のフォルダ> [--runs 30] [--expect 期待ファイル] [--strict]
```

フォルダ内の `dic*.txt` から `＊イベント名` を全部拾い、1 イベントを `--runs` 回（既定 30 回）発火します。
里々は候補をランダムに選ぶので、複数回発火して候補を踏みます。次のとき失敗（終了コード 1）です。

- `ErrorLevel` / `ErrorDescription` が付く（辞書の構文エラー。**そのエラー以降の定義は読み込まれません**）
- `204 No Content`（辞書にあるイベントなのに応答が空。構文エラー以降の定義は多くがこうなります）
- `200 OK` 以外 / `request` が NULL

`Value` に全角の `（` `）` が残っているときは警告です。未定義の関数・変数はエラーにならず、括弧ごとそのまま出力されるためです。`--strict` で失敗扱いになります。

## 期待ファイル（複雑な辞書でゴールを書く）

```
# イベント名<TAB>応答に含まれるべき語（!で始めると含まれてはいけない語）
OnBoot	こんにちは。
OnBoot	!（
```

「含まれるべき語」は発火のうち 1 回でも出れば OK です。Value はさくらスクリプトのままなので、セリフの文面で書きます。

## 例

`examples/good`（通る）、`examples/bad`（構文エラーで失敗）、`examples/warn`（警告のみ）。

## 分かっていること / 分かっていないこと

- 構文エラーの `ErrorDescription` には行番号が出ません（括弧の不整合などの種別とファイル名だけ）。
- 辞書が読み込まれなかったかどうかは、エラーヘッダと 204 の両方で見ています。
- 条件付きイベント（`Reference` 依存など）や、SAORI を呼ぶ辞書はまだ見ていません。
