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
出力先は satori が `Release\`、satorite が `Release_ST\`、ssu が `Release_SU\` です（`satori\Release\satorite.exe` などは古い残骸）。

## リリース

### バージョン記号 `McXYY-Z`
- X：1 = ACP 版（master）、2 = Unicode 版（unicode ブランチ）
- YY：機能追加で上げる（2桁）。上げたら Z は 1 に戻す（例：Mc171-4 → Mc172-1）。
- Z：バグフィックスで上げる。
- 機能追加かバグフィックスか判断が怪しいときは、どちらで上げるかユーザーに質問する。
- 接頭辞は常に `Mc`（`Tc` と書かれていても `Mc`）。

### 書き換える場所（2箇所）
- `satoriya/satori/resource.rc`（satori / satorite / ssu 共通、Shift_JIS）
  - `FILEVERSION X,XYY,Z,1` と `VALUE "FileVersion", "X, XYY, Z, 1\0"`
  - 例：Mc172-3 → `1,172,3,1`、Mc201-1 → `2,201,1,1`
  - `PRODUCTVERSION` は 1,0,0,1 のまま触らない。
- `satoriya/satori/satori.cpp` の `gSatoriVersion = L"phase McXYY-Z";`

### 手順
1. 前回のタグからの変更を `git log <前回タグ>..HEAD` で確認し、Y と Z のどちらを上げるか決める（怪しければ質問）。
2. 上の2箇所を書き換える。
3. msdev で satori / satorite / ssu を Release でリビルドし、エラー 0 を確認する（上の「ビルド」参照）。
4. `satoriya` で `make_satori.bat` を実行する（cmd 経由。7z が必要）。`satoriya\tmp\satori.zip` ができる。中身は satori.dll、satorite.exe、saori\ssu.dll、satori_license.txt。
5. バージョン更新をコミットする。コミットメッセージは「変更の要約 McXYY-Z」（例：「2問題修正 Mc172-3」）。
6. X=2 は unicode ブランチ、X=1 は master でリリースする。push してから、同名のタグ `McXYY-Z` を打って push する。
7. `gh release create McXYY-Z satoriya/tmp/satori.zip --title McXYY-Z --target <ブランチ> --notes ...` で公開する。
   - ノートは日本語の箇条書き（`- ○○の問題修正`、`- ○○を追加`）。外部の PR によるものは末尾に ` by @ユーザー名` を付ける。
   - 過去の例は `gh release view Mc172-3` で見られる。
   - 公開前にノートの文面をユーザーに見せて確認する。
   - Mc2XX（Unicode 版）は当面 `--prerelease` を付けてベータとして出す。検証が進んだらユーザーの指示で外す（`gh release edit McXYY-Z --prerelease=false`）。指示があるまでは外さない。

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
- ログ送信：`_/Sender.cpp`
  - 送信先は `logsend(hwnd)` エクスポート（`SakuraDLLHost.cpp`）で渡されたウィンドウ。無ければ最初の送信時に FindWindow で れしば → tama（`TamaWndClass`）の順に探す。
  - tama モードでは WM_COPYDATA の `dwData` がログコード（E_I=0 / E_E=2 / 文字コード通知 E_UTF8=17 など、YAYA と同じ値）。各行は遅延送信リストにログコードと一緒にためられ、`flush` で送られる。
  - errsender（ `GetSender().errsender() << ... << satori::endl` ）は E_E で送られ、`capability` で `response.errorlevel` 非対応と分かった後は MessageBox になる。

### 動作確認
- satori.dll を `LoadLibrary` して `loadu` / `request` / `unload` を直接呼ぶ小さなハーネスを VC6 で作ると、SSP を起動せずにリクエスト単位で確認できる。
- 変更前の DLL は `git worktree` で master を別に展開してビルドすると比較しやすい。
  - 自動ウェイト（`\_w[n]`）は master と値が違うのが正しい。master は `chars_spoken` を SJIS のバイト数で数えていたため全角1文字が2だったが、unicode 版は1文字=1（全角の自動ウェイトは master の半分になる）。これは仕様として受け入れ済みなので、比較では `\_w[...]` を除いて見る。
- SSP での確認：
  - 起動は `ssp.exe --option readonly --ghost <ゴーストのフォルダ名>` で行う（readonly ならユーザー環境を変更しない）。仕様は https://ssp.shillest.net/ukadoc/ssphelp/option.html
  - TCP 9801 に `NOTIFY SSTP/1.1` と `Event: ...` を送ると、応答の `Script:` にゴーストが返したスクリプトが入るので、画面を見なくても内容を確認できる。
  - 外部 SSTP から送った `\-` は無視される。終了させるには SSP のメインウィンドウに WM_CLOSE を送るか、手で終了する。
- テスト用のゴーストは既存のものをコピーして使い、元のゴーストは触らない。
- tama / tamac（`C:\D_DRIVE\MyDocuments\GitHub\tama`）でログを受けられる。
  - `tamac satori.dll` で読み込み時のログが stdout、エラー（errsender、E_E）が `[ERROR]` 付きで stderr に出る。エラーがあると終了コード 2。
  - `tamac satori.dll -r < request.txt` で標準入力の SHIORI リクエストを送り、応答を stdout に出す（ログは stderr）。
  - `--ci` の `::error` には file / line が入らない（里々のエラー文が `ファイル(行) :` 形式でないため）。
  - `tama/builds/Release/tamac.exe` は古いことがある（`-r` 非対応）。現行ソースは `/p:PlatformToolset=v145` と `/p:OutDir=<scratchpad>` を付けて MSBuild でビルドする。
  - tamac は load / unload を行うので、DLL のあるフォルダに `satori_savedata.txt` が書かれる。ゴーストのコピーで使う。
- さとりて相当の変換は `ShioriEcho`（`satori_AnalyzeRequest.cpp`）で tamac からできる。`＄デバッグ＝有効`（`satori_conf.txt` の ＊初期化 など）と `SecurityLevel: local` が必要。Reference0, 1, ... が1行ずつ里々の文として展開され、`Value` にさくらスクリプトが返る。

