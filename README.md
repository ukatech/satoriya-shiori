# 里々（SATORI）

伺か（ukagaka）の SHIORI「里々」のソースコードです。日本語の文章に近い書き方で、ゴーストの辞書を書けます。

```
＊OnBoot
：こんにちは。
：やあ、今日もよろしく。
```

（最初の `：` の行は `\0`（本体側）、次の `：` の行は `\1`（相方側）が話し、以後交互に切り替わります）

## マニュアル

**https://ukatech.github.io/satori-docs/**

辞書の書き方、内蔵関数・システム変数、同梱 SAORI「ssu」の関数などをまとめています。マニュアルのソースは [ukatech/satori-docs](https://github.com/ukatech/satori-docs) です。

## ダウンロード

[Releases](https://github.com/ukatech/satoriya-shiori/releases) から `satori.zip` を入手してください。中身は次のとおりです。

| ファイル | 内容 |
|----------|------|
| `satori.dll` | SHIORI 本体 |
| `satorite.exe` | さとりて（里々の文をさくらスクリプトに変換して確認するツール） |
| `saori\ssu.dll` | 同梱 SAORI「ssu」（`calc` `split` `replace` などの関数） |

バージョンは `McXYY-Z` の形で、X が版の種類を表します。

| 版 | バージョン | ブランチ |
|----|------------|----------|
| ACP 版（Shift_JIS ベース） | `Mc1YY-Z` | `master` |
| Unicode 版 | `Mc2YY-Z` | `unicode` |

Unicode 版では、UTF-8 の辞書が使えるほか、サロゲートペアなどを 1 文字として扱えます。ACP 版との違いは [ACP 版との違い](https://ukatech.github.io/satori-docs/other/unicode-changes/) を参照してください。

## リポジトリの構成

| パス | 内容 |
|------|------|
| `satoriya/satori/` | 里々本体（satori.dll）、さとりて（satorite.exe）、ssu（ssu.dll） |
| `satoriya/_/` | 共通ライブラリ（文字列処理、文字コード変換、ログ送信など） |
| `satoriya/httpc/` | SAORI「httpc」（HTTP でデータを取得する） |
| `satoriya/obu/` | SAORI「おぶ」（ブラウザで開いているページの URL などを取得する） |
| `satoriya/sodate/` `satoriya/sodate_setup/` | 「sodate」（ネットワーク更新ファイルの作成とアップロードを行うツール） |
| `satoriya/test/` | 動作確認用のツール（DLL を直接呼ぶハーネスなど） |
| `satoriya/make_*.ps1` | 配布用 zip の作成スクリプト |

## ビルド

Visual C++ 6.0 でビルドします（C++98 の範囲で書かれています）。`satoriya/satori` で次を実行します。

```
msdev satori.dsw /MAKE "satori - Win32 Release" /REBUILD
msdev satori.dsw /MAKE "satorite - Win32 Release" /REBUILD
msdev satori.dsw /MAKE "ssu - Win32 Release" /REBUILD
```

出力先は satori が `Release\`、satorite が `Release_ST\`、ssu が `Release_SU\` です。ビルド後に `satoriya\make_satori.ps1` を実行すると、`satoriya\tmp\satori.zip` ができます（7-Zip が必要です）。

POSIX 環境向けには Linux 用の `makefile.linux`、macOS 用の `makefile.posix` などもありますが、主に保守しているのは Windows（VC6）版です。

ソースの文字コードは Shift_JIS、改行は CRLF です。編集するときはこれを保ってください。

## ライセンス

[BSD 2-Clause License](LICENSE) です。

Copyright (c) 2001-2005, Kusigahama Yagi.
Copyright (c) 2006-, SEIBIHAN.
