このディレクトリは伺かのSHIORI "SATORI(里々)" のコードです。
C++98/旧いVC++でコンパイルできるように製作されており、文字コードShift JISなので、Sjis_ではじまる編集・探索ツール群を使う必要があります。
コード管理ツールはgitです。
仕様は https://soliton.sub.jp/satori/ にあります。

## ビルド

VC6 の msdev を PowerShell から実行します（`satoriya/satori` で実行）。
satori.dsw には satori / satorite / ssu の3プロジェクトがあります。

```
msdev satori.dsw /MAKE "satori - Win32 Release" /REBUILD /OUT "$env:TEMP\claude\satori_build\satori.log"
msdev satori.dsw /MAKE "satorite - Win32 Release" /REBUILD /OUT "$env:TEMP\claude\satori_build\satorite.log"
msdev satori.dsw /MAKE "ssu - Win32 Release" /REBUILD /OUT "$env:TEMP\claude\satori_build\ssu.log"
```

ログは `Get-Content <log> -Encoding oem` で読めます。最終行に「ｴﾗｰ n、警告 n」が出ます。
ssu は変更前から警告5件（STLヘッダ由来）が出ます。

## ソースを読む・編集するときのヒント

### 編集時の注意
- ソースは Shift_JIS + CRLF を維持する（VC6用）。
- Git Bash の `sed -i` は CRLF を LF に変えてしまうので使わない。
- `Sjis_write` は LF で書き出すので、書いた後で CRLF に揃える。
- Python で cp932 として読むと、0x8160「〜」は U+FF5E、0x817C「−」は U+FF0D になる（VC6 の L"" リテラルも同じ）。UTF-8 辞書では U+301C / U+2212 が来ることがあるので、両方受け付ける必要がある。

### 文字列の扱い（unicode ブランチ以降）
- 内部の文字列はすべて `std::wstring`（`stltool.h` の `using std::wstring`、`strvec`/`strmap` なども wstring）。`std::string` は外部とのバイト列にだけ使う。
- 文字コード変換は `_/charset.h`（`MBtoW`/`WtoMB`/`UTF8toW`/`WtoSJIS`/`DetectCharset`/`CharsetFromName`）。
- バイト列との境界は次の箇所だけ：
  - SHIORI/SAORI の受け側：`SakuraDLLHost::request_bytes`、`load`/`loadu`
  - SAORI の呼び出し側：`SakuraDLLClient`
  - ファイル：`stltool` の `bytes_from_file` / `strvec_from_file` など
  - SSTP：`Satori_Kakko.cpp` の `SendDirectSSTP`、`SSTPClient`、`dsstp`
  - FMO：`SakuraFMO`
  - ログ：`Sender`
- 日本語リテラルの長さを数値で書かない。`const_strlen(L"...")` や `strip_head_tail(str, L"頭", L"尻")` を使う（旧コードはSJISのバイト数を直書きしていた）。
- 1文字取得は `get_a_chr`、文字数は `count_chars`（サロゲートペアを1文字として扱う）。
- さくらスクリプトのコマンド名判定は ASCII に限る（`iswalpha` はかなや漢字でも真になる）。
- 内部特殊表現は私用領域の文字：`INTERNAL_MARK`(U+E0FF)、escaper(U+E09E)、Sender の flush 区切り(U+E0FE)。

### どこに何があるか
- 辞書の前処理（φエスケープ、＃コメント、カッコ内改行の連結、replace.txt 適用）：`satori_load_dict.cpp` の `pre_process`
- ＊／＠の単位への分割：`satori_load_dict.cpp` の `lines_to_units`
- （）の展開：`satori_tool.cpp` の `KakkoSection` / `UnKakko`
- 組み込みの（）名前：`Satori_Kakko.cpp` の `inc_call` と `Call`
- ＄システム変数：`satori_tool.cpp` の `system_variable_operation_real`
- トーク本文からさくらスクリプトへの変換（自動ウェイト、スコープ切り替え）：`satori_sentence.cpp`
- 最終変換（自動アンカー、replace_after）：`satoriTranslate.cpp`
- ssu の実装：`ssu.cpp`
  - satori.dll 内では SAORI を経由せず直接呼ばれる。
  - ssu.dll 単体ビルドは `SSU_SAORI_CALL_INTERFACE` を定義した場合。
- `SakuraScript.cpp` / `main.cpp` / `cn.cpp` / `TimeCommands.cpp` はどのプロジェクトにも入っていない（死んだコード）。
- POSIX 向けの分岐は `#ifdef POSIX`。

### 動作確認
- satori.dll を `LoadLibrary` して `loadu` / `request` / `unload` を直接呼ぶ小さなハーネスを VC6 で作ると、SSP を起動せずにリクエスト単位で確認できる。
- 変更前の DLL は `git worktree` で master を別に展開してビルドすると比較しやすい。
- SSP での確認：
  - 起動は `ssp.exe --option readonly --ghost <ゴーストのフォルダ名>` で行う（readonly ならユーザー環境を変更しない）。仕様は https://ssp.shillest.net/ukadoc/ssphelp/option.html
  - TCP 9801 に `NOTIFY SSTP/1.1` と `Event: ...` を送ると、応答の `Script:` にゴーストが返したスクリプトが入るので、画面を見なくても内容を確認できる。
  - 外部 SSTP から送った `\-` は無視される。終了させるには SSP のメインウィンドウに WM_CLOSE を送るか、手で終了する。
- テスト用のゴーストは既存のものをコピーして使い、元のゴーストは触らない。
- tamac（YAYA 用ツール）は `logsend` エクスポートでログを受けるので、里々のログは受け取れない（`-r` での応答確認には使える）。

