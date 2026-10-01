# 里々 (Satori) 辞書ファイル検証レポート

## 1. 概要 (Executive Summary)

- **バリデータセルフテスト**: ✅ PASS
- **検証対象ファイル総数**: 29 ファイル
- **検出エラー (ERROR)**: 0 件
- **検出警告 (WARN)**: 0 件

## 2. バリデータ自身のエミュレーションテスト (Self-Test Status)

`tests/fixtures/` 内の各種構文パターン（UTF-8, CP932, BOM付き, エスケープ `φ`, 半角括弧, 空ファイル等）に対するセルフテスト結果：

- 結果: **PASS (すべての単体テスト合格)**

## 3. ファイル別静的検証結果 (Static Validation Details)

| ターゲット | ファイルパス | 文字コード | 判定 | ERROR | WARN | 詳細・行番号 |
| --- | --- | --- | --- | --- | --- | --- |
| `kampo-ghost` | `kampo-ghost/ghost/master/another/dic1.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic00_System.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic01_Base.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic02_Event.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic03_Menu.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic04_Change.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic05_Communicate.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic06_String.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic07_Time.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic08_RandomTalk.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic09_Test.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `kampo-ghost` | `kampo-ghost/ghost/master/dic_kampo_data.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/another/dic1.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic00_System.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic01_Base.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic02_Event.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic03_Menu.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic04_Change.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic05_Communicate.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic06_String.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic07_Time.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic08_RandomTalk.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `8-1` | `8-1/ghost/master/dic09_Test.txt` | UTF-8 | ✅ PASS | 0 | 0 | - |
| `satori-test` | `satoriya/satori/test/_dicUmemci.txt` | CP932 | ✅ PASS | 0 | 0 | - |
| `satori-test` | `satoriya/satori/test/a.txt` | CP932 | ✅ PASS | 0 | 0 | - |
| `satori-test` | `satoriya/satori/test/b.txt` | CP932 | ✅ PASS | 0 | 0 | - |
| `satori-test` | `satoriya/satori/test/dic0.txt` | CP932 | ✅ PASS | 0 | 0 | - |
| `satori-test` | `satoriya/satori/test/dic1.txt` | CP932 | ✅ PASS | 0 | 0 | - |
| `satori-test` | `satoriya/satori/test/satori_savedata.txt` | CP932 | ✅ PASS | 0 | 0 | - |

## 4. 実エンジン動作テスト (Real Engine Test Status)

### ステータス: **未検証 (Linux環境制約)**

#### 未検証の理由:
1. **C++ エンジンの Windows API 依存**:
   `satoriya/satori/` 内の C++ ソースコード (`satoriya/_/charset.cpp`) は Windows 固有の API (`<windows.h>`, `<mbctype.h>`, `_setmbcp`) に依存しています。
   Linux 用の `makefile.posix` では文字コード変換層 (`charset.cpp`) がビルドから除外されているため、`libsatori.so` の動的リンク時に `UTF8toSJIS` シンボル未定義エラーが発生します。
2. **Wine 非搭載**:
   CI環境に Wine が用意されていないため、`satori.dll` や Windows 用バイナリを直接実行することはできません。

#### 将来的な実エンジン検証案 (改善提案):
- **案A**: GitHub Actions ワークフローで `windows-latest` ランナーを使用し、Windows 上で `satori.dll` を動的ロードして動作テストを実施する。
- **案B**: `tests/` 配下に Linux/POSIX 向け `iconv` を用いた `UTF8toSJIS` / `SJIStoUTF8` 代替実装を用意し、C++ エンジンを Linux 上で完全ネイティブビルド可能にする。

## 5. 発見された問題点と修正提案 (Discovered Issues & Recommended Fixes)

検出された問題はありません。すべての辞書ファイルが静的検証を通過しました。
