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

### サブモジュール（正規表現エンジン DEELX）
`satoriya/deelx`（https://github.com/ponapalt/deelx 、ヘッダのみ）は git サブモジュールで、ssu の `regex_*` 関数が `../deelx/deelx.h` を使います。**ビルドの前に必ず最新にします**。

```
git submodule update --init --remote satoriya/deelx
```

- 参照先は常にリモートの最新に付け替える（`--remote`）。ポインタが変わったら `git status` に `modified: satoriya/deelx` が出るので、ビルドして問題がないことを確かめてから `satoriya/deelx を更新` のようなコミットで記録する（リリース前にも行う）。
- 新しく clone した直後は `git submodule update --init` が要る（`deelx` が空のままだとビルドが通らない）。
- deelx.h は ASCII・LF/CRLF どちらでもよい。POSIX の makefile は `utf8src/deelx/` にコピーして使う。
- DEELX は正規表現の書式エラーを報告しない（閉じていない括弧などはそのまま解釈される）。

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
4. `satoriya\make_satori.ps1` を実行する（どのフォルダからでもよい。7z が必要）。`satoriya\tmp\satori.zip` ができる。中身は satori.dll、satorite.exe、saori\ssu.dll。
   - 3つのファイルバージョンがそろっていない、`satori.cpp` の `gSatoriVersion` が `resource.rc` と違う、ビルドした satori.dll / satorite.exe に `phase McXYY-Z` が入っていない、のどれかならエラーで止まる（ビルドし忘れの検出）。
5. バージョン更新をコミットする。コミットメッセージは「変更の要約 McXYY-Z」（例：「2問題修正 Mc172-3」）。
6. X=2 は unicode ブランチ、X=1 は master でリリースする。push してから、同名のタグ `McXYY-Z` を打って push する。
7. `gh release create McXYY-Z satoriya/tmp/satori.zip --title McXYY-Z --target <ブランチ> --notes ...` で公開する。
   - ノートは日本語の箇条書き（`- ○○の問題修正`、`- ○○を追加`）。外部の PR によるものは末尾に ` by @ユーザー名` を付ける。
   - 過去の例は `gh release view Mc172-3` で見られる。
   - 公開前にノートの文面をユーザーに見せて確認する。

## ドキュメント

マニュアルは兄弟リポジトリ `../satori-docs`（https://github.com/ukatech/satori-docs 、公開先 https://ukatech.github.io/satori-docs/ ）にあります。書き方・ビルド・実機での確認手順はそちらの `CLAUDE.md` を参照してください。`main` に push すると GitHub Pages に自動で公開されます。

仕様変更・機能の新設・バグ修正をしたら、ソースのコミットと合わせて `satori-docs` も更新します（`satori-docs` 側で commit & push）。更新先は次のとおりです。

| 変更した場所 | 更新するページ（`satori-docs` 内） |
|--------------|------------------------------------|
| ssu の関数（`ssu.cpp`） | `ssu/<関数名>.md`、`ssu/index.md`、`INDEX.md` |
| （）内蔵関数（`satori_builtin.cpp` の `func_*`、表は `function_table`） | `functions/<関数名>.md`、`functions/index.md`、`INDEX.md`（ローカル専用かどうかも） |
| ＄システム変数（`satori_builtin.cpp` の `sysvar_*`、表は `system_variable_operation_real`） | `system/vars-*.md`（分類に合うページ）、`system/index.md` |
| （）の組み込み名（`satori_builtin.cpp` の `var_*`、表は `GetBuiltinValue`） | `system/names-*.md` |
| イベントの処理（`satori_EventOperation.cpp`、`satori_sentence.cpp` の `FindEventTalk`） | `shiori/events.md`、`shiori/default-behaviors.md`、`shiori/satori-events.md` |
| リクエスト・応答の処理（`satori_AnalyzeRequest.cpp`、`satori_CreateResponce.cpp`） | `shiori/protocol.md` |
| 辞書の書式・読み込み（`satori_load_dict.cpp`、`satori_load_unload.cpp`） | `grammar/01-dictionary-files.md`〜`grammar/04-script-lines.md` |
| 式（`calc.cpp`、`calc_float.cpp`） | `ssu/calc.md`、`ssu/calc_float.md`、`grammar/07-expressions.md` |
| 自動ウェイト・アンカー・改行（`satori_sentence.cpp`、`satoriTranslate.cpp`） | `grammar/11-auto-insert.md`、`grammar/13-sakura-script.md` |
| 重複回避（`Families.h`、`OverlapController.h`） | `grammar/12-overlap-avoidance.md` |
| 文字コード（`charset.cpp`） | `grammar/14-charset.md`、`other/unicode-changes.md` |
| セーブデータ | `grammar/15-save-data.md` |
| SAORI（`shiori_plugin.cpp`） | `other/saori.md` |
| ログに出すメッセージの追加・変更 | `other/error-messages.md` |

- 新しい関数・変数を作ったら、ページの新規作成に加えて、`INDEX.md` と各 `index.md` の表にも追加します。
- バグ修正で挙動が変わるときは、該当ページを直し、`other/unicode-changes.md` の「McXYY-Z での修正」に追記します（ページの本文には `Mc201-2以降` のようにバージョンを書きます）。
- リリースのバージョン記号（`McXYY-Z`）を上げるときは、`satori-docs` に書いたバージョンとずれていないか確認します。

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
  - SSTP：`satori_builtin.cpp` の `SendDirectSSTP`、`SSTPClient`、`dsstp`
  - FMO：`SakuraFMO`
  - ログ：`Sender`
- 日本語リテラルの長さを数値で書かない。`const_strlen(L"...")` や `strip_head_tail(str, L"頭", L"尻")` を使う（旧コードはSJISのバイト数を直書きしていた）。
- 1文字取得は `get_a_chr`、文字数は `count_chars`（サロゲートペアを1文字として扱う）。
- さくらスクリプトのコマンド名判定は ASCII に限る（`iswalpha` はかなや漢字でも真になる）。
- 内部特殊表現は私用領域の文字：`INTERNAL_MARK`(U+E0FF)、その種別 `INTERNAL_MARK_SCOPE`(U+E0FD) / `INTERNAL_MARK_SURFACE`(U+E0FC)、escaper(U+E09E)、Sender の flush 区切り(U+E0FE)。
  - 種別は master では `\x01` / `\x02` だったが、（バイト値、１）などの引数区切りと衝突して「引数の個数が正しくありません」になるので私用領域に移した。制御文字を内部表現に使わないこと。

### どこに何があるか
- 辞書の前処理（φエスケープ、＃コメント、カッコ内改行の連結、replace.txt 適用）：`satori_load_dict.cpp` の `pre_process`
- ＊／＠の単位への分割：`satori_load_dict.cpp` の `lines_to_units`
- （）の展開：`satori_tool.cpp` の `KakkoSection` / `UnKakko`
- （）の名前の解決：`Satori_Kakko.cpp` の `Call` / `CallReal`。SAORI・内蔵関数 → 単語群 → 文 → 変数 → 内蔵変数の順に探す。
- 内蔵関数・内蔵変数・＄システム変数：`satori_builtin.cpp`
  - 名前と処理の対応は表になっている（内蔵関数は `function_table`、内蔵変数は `GetBuiltinValue`、システム変数は `system_variable_operation_real`）。足すときは処理を書いて `satori.h` に宣言し、表に1行加える。
  - 完全一致の名前は map で引き、`文「…」の存在` のような前後で決まる名前は、表の上から順に調べる（順番に意味がある。汎用の `…の存在` は最後のほう）。
  - 内蔵関数の表の「localのみ」が真のものは、`SecurityLevel: local` 以外では実行されない。
- トーク本文からさくらスクリプトへの変換（自動ウェイト、スコープ切り替え）：`satori_sentence.cpp`
- 最終変換（自動アンカー、replace_after）：`satoriTranslate.cpp`
- ssu の実装：`ssu.cpp`
  - `regex_*`（`regex_match` / `regex_find` / `regex_findall` / `regex_count` / `regex_replace` / `regex_replace_first` / `regex_erase` / `regex_erase_first` / `regex_split` / `regex_escape`）は DEELX を使う。オプションは `i`（大小無視）`s`（`.`が改行にも一致）`m`（`^$`が行単位）`x`（拡張）の文字列で、`(?i)` 等のパターン内指定も可。`regex_match` / `regex_find` は Value0 に一致部分、Value1〜 に括弧のグループ。置換後は `$1` `$&` `${名前}` が使える。
  - satori.dll 内では SAORI を経由せず直接呼ばれる。
  - ssu.dll 単体ビルドは `SSU_SAORI_CALL_INTERFACE` を定義した場合。
- `SakuraScript.cpp` / `main.cpp` / `cn.cpp` / `TimeCommands.cpp` はどのプロジェクトにも入っていない（死んだコード）。
- POSIX 向けの分岐は `#ifdef POSIX`。
- ログ送信：`_/Sender.cpp`
  - 送信先は `logsend(hwnd)` エクスポート（`SakuraDLLHost.cpp`）で渡されたウィンドウ。無ければ最初の送信時に FindWindow で れしば → tama（`TamaWndClass`）の順に探す。
  - tama モードでは WM_COPYDATA の `dwData` がログコード（E_I=0 / E_E=2 / 文字コード通知 E_UTF8=17 など、YAYA と同じ値）。各行は遅延送信リストにログコードと一緒にためられ、`flush` で送られる。
  - errsender（ `GetSender().errsender() << ... << satori::endl` ）は E_E で送られ、`capability` で `response.errorlevel` 非対応と分かった後は MessageBox になる。

### 動作確認
- `satoriya/test/harness/harness.c` で satori.dll の `load` / `request` / `unload` を直接呼べる（SSP を起動せずにリクエスト単位で確認できる）。使い方は同じフォルダの README.md。
  - リクエストも応答も生のバイト列のまま扱う（文字コード変換なし）ので、master の DLL には SJIS、unicode 版には Charset ヘッダに合わせたバイト列を渡して公平に比較できる。tamac は unicode 版からの E_UTF8 通知で UTF-8 に切り替わるので、master との比較には向かない。
  - 里々の辞書の例（README・ドキュメント・テスト）は、トークを `：` から書き始めるのが一般的な書き方。最初の `：` の行が `\0`、次が `\1` で、以後交互になる。`：` なしで書き始めると、冒頭のデフォルトサーフェスの後の `\1` のまま話してしまう。
  - トーク名をそのまま ID に指定した場合、`On` で始まらない ID では冒頭のデフォルトサーフェス（`\0\s[0]\1\s[10]`）と末尾の `\e` が付かない。サーフェス関係を見るときは `On` で始まる名前のトークにする。
- 変更前の DLL は `git worktree` で master を別に展開してビルドすると比較しやすい。
  - 自動ウェイト（`\_w[n]`）は master と同じ値になる（Mc202-1 以降）。master は `chars_spoken` を SJIS のバイト数で数えていたので、unicode 版も `count_width`（半角換算。全角2・半角1）で数えて合わせている。
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

