伺かのSHIORI "SATORI(里々)" のコードです。仕様は https://soliton.sub.jp/satori/ にあります。
C++98/旧いVC++（VC6）でコンパイルできるように作られており、文字コードは Shift_JIS なので、Sjis_ ではじまる編集・探索ツール群を使います。コード管理は git です。

## ビルド

### Windows（VC6）
`satoriya/satori` で、PowerShell から msdev を実行します。satori.dsw には satori / satorite / ssu の3プロジェクトがあります。

```
msdev satori.dsw /MAKE "satori - Win32 Release" /REBUILD /OUT "$env:TEMP\claude\satori_build\satori.log"
msdev satori.dsw /MAKE "satorite - Win32 Release" /REBUILD /OUT "$env:TEMP\claude\satori_build\satorite.log"
msdev satori.dsw /MAKE "ssu - Win32 Release" /REBUILD /OUT "$env:TEMP\claude\satori_build\ssu.log"
```

- ログは `Get-Content <log> -Encoding oem` で読む。最終行の「ｴﾗｰ n、警告 n」を見る。ssu は変更前から警告5件（STLヘッダ由来）が出る。
- 出力先は satori が `Release\`、satorite が `Release_ST\`、ssu が `Release_SU\`（`satori\Release\satorite.exe` などは古い残骸）。

### Windows（最新の Visual Studio、Win32 / x64）
`satori.sln`（vcxproj）で Win32 と x64 の両方を作れる。VS 2026（v145）なら次のようにする。vcxproj の `PlatformToolset` は v143 のままなので、手元のツールセットに合わせて `/p:PlatformToolset` で上書きする。

```
$msb = 'C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe'
& $msb satori.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v145
```

- x64 の出力先は `Release_x64\`、`Release_ST_x64\`、`Release_SU_x64\`（Win32 は VC6 と同じ `Release\` など）。Win32 の vcxproj ビルドは VC6 の成果物（`Release\` など）を上書きするので、リリース用は VC6 で作り直す。
- 64bit で通すために、`SendMessageTimeout` の結果は `SMTO_RESULT`（`_/Sender.h`。`GWLP_USERDATA` のある SDK では `DWORD_PTR`）、ウィンドウハンドルと整数の変換は `INT_PTR` を経由する。`DWORD_PTR` / `LONG_PTR` が無い古い SDK でも VC6 で通るよう、`#ifdef GWLP_USERDATA` で分けるか `INT_PTR`（VC6 付属の BASETSD.H にある）を使う。
- vcxproj のソース一覧は dsp とずれやすい（`dsstp.cpp` / `charset.cpp` の漏れ、死んだ `TimeCommands.cpp` の混入があった）。ソースを足したら dsp と vcxproj の両方に入れる。
- x64 の DLL は 32bit の harness では読めないので、動作確認はビルドまで。

### サブモジュール（uka_regexp）
`satoriya/uka_regexp`（https://github.com/ukatech/uka_regexp 、`uka_regexp.h` と `uka_regexp.cpp` の2ファイル）は ssu の `regex_*` が使う正規表現エンジンで、git サブモジュールです。伺か共用の PCRE2 寄りのエンジンで、Unlicense のためライセンス表記は要りません。**ビルドの前に必ず最新にします**。

```
git submodule update --init --remote satoriya/uka_regexp
```

- ポインタが変わると `git status` に `modified: satoriya/uka_regexp` が出る。ビルドして問題がないことを確かめてから `satoriya/uka_regexp を更新` のようなコミットで記録する（リリース前にも行う）。
- clone 直後は `git submodule update --init` が要る（空のままだとビルドが通らない）。
- 本体側では編集しない。直すときは `../uka_regexp` で直してから参照を更新する（ASCII のみなので Sjis_ 系ツールは不要）。
- ヘッダだけでなく `uka_regexp.cpp` も satori / satorite / ssu の3プロジェクトと makefile（`uka_regexp.o`）に入っている。
- 書式エラーは `CRegexpException`（`ERR_SYNTAX`）で報告される。バックトラックの上限超過は `ERR_STEP_LIMIT`（`call_ssu` で捕まえてエラー文にする）。

### Linux / macOS
- `satoriya/satori/makefile.linux`（Linux / BSD、`libsatori.so`）と `makefile.posix`（macOS、`libsatori.bundle`）。ソースを増やしたら両方と `makefile.emscripten` の一覧に足す。
- makefile は SJIS のソースを iconv で `utf8src/` に UTF-8 化してからコンパイルする。**ファイル名は git 上の大文字小文字どおりに書く**（Windows では気づかない）。
- `libssu` は makefile で作らない（`SSU_SAORI_CALL_INTERFACE` 付きの別ビルドが要る。satori 本体が ssu を内蔵している）。
- ローカルに Linux / macOS が無いので、手動実行の GitHub Actions（`.github/workflows/posix-build.yml`）で確かめる。push してから実行し、失敗したら直して繰り返す。

  ```
  gh workflow run posix-build.yml --ref unicode
  gh run watch <run-id> --exit-status
  gh run view <run-id> --log-failed
  ```
- スモークテスト（`satoriya/test/posix/posix_smoke.c`）は dlopen して load → request → unload まで通す。期待値を足すときは、先に Windows の satori.dll を harness で動かして確かめる。
- POSIX の `load` / `request` は渡したバッファを `free` するので、テストは `malloc` したものを渡す。

## リリース

### バージョン記号 `McXYY-Z`
- X：1 = ACP 版（master）、2 = Unicode 版（unicode ブランチ）
- YY：機能追加で上げる（2桁）。上げたら Z は 1 に戻す（例：Mc171-4 → Mc172-1）。
- Z：バグフィックスで上げる。
- 機能追加かバグフィックスか判断が怪しいときは、どちらで上げるかユーザーに質問する。
- 接頭辞は常に `Mc`（`Tc` と書かれていても `Mc`）。

### 書き換える場所
- `satoriya/satori/resource.rc`（Shift_JIS。satori / satorite / ssu 共通）：`FILEVERSION X,XYY,Z,1` と `VALUE "FileVersion", "X, XYY, Z, 1\0"`。例：Mc172-3 → `1,172,3,1`、Mc201-1 → `2,201,1,1`。`PRODUCTVERSION` は 1,0,0,1 のまま触らない。
- `satoriya/satori/satori.cpp` の `gSatoriVersion = L"phase McXYY-Z";`

### 手順
1. `git log <前回タグ>..HEAD` で変更を確認し、Y と Z のどちらを上げるか決める（怪しければ質問）。
2. 上の2箇所を書き換える。
3. satori / satorite / ssu を Release でリビルドし、エラー 0 を確認する。
4. `satoriya\make_satori.ps1` を実行する（7z が必要）。`satoriya\tmp\satori.zip`（satori.dll、satorite.exe、saori\ssu.dll）ができる。バージョンの不一致やビルド忘れはスクリプトがエラーで止める。
5. バージョン更新をコミットする。メッセージは「変更の要約 McXYY-Z」（例：「2問題修正 Mc172-3」）。
6. X=2 は unicode ブランチ、X=1 は master で push し、同名のタグ `McXYY-Z` を打って push する。
7. `gh release create McXYY-Z satoriya/tmp/satori.zip --title McXYY-Z --target <ブランチ> --notes ...` で公開する。
   - ノートは日本語の箇条書き（`- ○○の問題修正`、`- ○○を追加`）。外部の PR によるものは末尾に ` by @ユーザー名`。過去の例は `gh release view Mc172-3`。
   - 公開前にノートの文面をユーザーに見せて確認する。

## ドキュメント

マニュアルは兄弟リポジトリ `../satori-docs`（https://github.com/ukatech/satori-docs 、公開先 https://ukatech.github.io/satori-docs/ ）です。書き方・ビルド・実機での確認はそちらの `CLAUDE.md` を参照します。`main` に push すると GitHub Pages に自動で公開されます。

仕様変更・機能の新設・バグ修正をしたら、ソースのコミットと合わせて `satori-docs` も更新します（`satori-docs` 側で commit & push）。

| 変更した場所 | 更新するページ（`satori-docs` 内） |
|--------------|------------------------------------|
| ssu の関数（`ssu.cpp`） | `ssu/<関数名>.md`、`ssu/index.md`、`INDEX.md` |
| （）内蔵関数（`satori_builtin.cpp` の `func_*`） | `functions/<関数名>.md`、`functions/index.md`、`INDEX.md`（ローカル専用かどうかも） |
| ＄システム変数（`sysvar_*`） | `system/vars-*.md`（分類に合うページ）、`system/index.md` |
| （）の組み込み名（`var_*`） | `system/names-*.md` |
| イベントの処理（`satori_EventOperation.cpp`、`satori_sentence.cpp` の `FindEventTalk`） | `shiori/events.md`、`shiori/default-behaviors.md`、`shiori/satori-events.md` |
| リクエスト・応答（`satori_AnalyzeRequest.cpp`、`satori_CreateResponce.cpp`） | `shiori/protocol.md` |
| 辞書の書式・読み込み（`satori_load_dict.cpp`、`satori_load_unload.cpp`） | `grammar/01-dictionary-files.md`〜`grammar/04-script-lines.md` |
| 式（`calc.cpp`、`calc_float.cpp`） | `ssu/calc.md`、`ssu/calc_float.md`、`grammar/07-expressions.md` |
| 自動ウェイト・アンカー・改行（`satori_sentence.cpp`、`satoriTranslate.cpp`） | `grammar/11-auto-insert.md`、`grammar/13-sakura-script.md` |
| 重複回避（`Families.h`、`OverlapController.h`） | `grammar/12-overlap-avoidance.md` |
| 文字コード（`charset.cpp`） | `grammar/14-charset.md`、`other/unicode-changes.md` |
| セーブデータ | `grammar/15-save-data.md` |
| SAORI（`shiori_plugin.cpp`） | `other/saori.md` |
| ログに出すメッセージ | `other/error-messages.md` |

- 新しい関数・変数は、ページの新規作成に加えて `INDEX.md` と各 `index.md` の表にも足す。
- バグ修正で挙動が変わるときは、該当ページを直し、`other/unicode-changes.md` の「McXYY-Z での修正」に追記する（本文には `Mc201-2以降` のようにバージョンを書く）。
- リリースでバージョン記号を上げるときは、`satori-docs` に書いたバージョンとずれていないか確認する。

## 編集時の注意

- ソースは Shift_JIS + CRLF を維持する（VC6用）。Git Bash の `sed -i` は CRLF を LF に変えるので使わない。`Sjis_write` は LF で書き出すので、書いた後で CRLF に揃える。
- Python で cp932 として読むと、0x8160「〜」は U+FF5E、0x817C「−」は U+FF0D になる（VC6 の L"" リテラルも同じ）。UTF-8 辞書では U+301C / U+2212 が来ることがあるので、両方受け付ける。

### 文字列（unicode ブランチ以降）
- 内部の文字列はすべて `std::wstring`（`strvec` / `strmap` なども）。`std::string` は外部とのバイト列にだけ使い、変換は `_/charset.h` で行う。
- バイト列との境界は次の箇所だけ：SHIORI/SAORI の受け側（`SakuraDLLHost::request_bytes`、`load` / `loadu`）、SAORI の呼び出し側（`SakuraDLLClient`）、ファイル（`stltool` の `bytes_from_file` など）、SSTP（`SendDirectSSTP`、`SSTPClient`、`dsstp`）、FMO（`SakuraFMO`）、ログ（`Sender`）。
- 日本語リテラルの長さを数値で書かない（`const_strlen(L"...")` や `strip_head_tail` を使う）。
- 1文字取得は `next_a_chr`（`a_chr`。ヒープを使わない。VC6 の `basic_string` は1文字でもヒープを確保するので、1文字ずつ回すループで `wstring` を作らない）、文字数は `count_chars`（サロゲートペアを1文字として扱う）。
- さくらスクリプトのコマンド名判定は ASCII に限る（`iswalpha` はかなや漢字でも真になる）。
- 内部特殊表現は私用領域の文字（`INTERNAL_MARK` U+E0FF、種別 U+E0FD / U+E0FC、escaper U+E09E、Sender の flush 区切り U+E0FE）。master の `\x01` / `\x02` は、（バイト値、１）などの引数区切りと衝突して「引数の個数が正しくありません」になったので移した。制御文字を内部表現に使わない。

## コードの案内

- 辞書の前処理（φエスケープ、＃コメント、カッコ内改行の連結、replace.txt）：`satori_load_dict.cpp` の `pre_process`、＊／＠の単位への分割は `lines_to_units`
- （）の展開：`satori_tool.cpp` の `KakkoSection` / `UnKakko`
- （）の名前の解決：`satori_Kakko.cpp` の `Call` / `CallReal`。SAORI・内蔵関数 → 単語群 → 文 → 変数 → 内蔵変数の順に探す。
- トーク本文からさくらスクリプトへの変換（自動ウェイト、スコープ切り替え）：`satori_sentence.cpp`。最終変換（自動アンカー、replace_after）：`satoriTranslate.cpp`
- 内蔵関数・内蔵変数・＄システム変数：`satori_builtin.cpp`
  - 名前と処理の対応は表（`function_table` / `GetBuiltinValue` / `system_variable_operation_real`）。足すときは処理を書いて `satori.h` に宣言し、表に1行加える。
  - 完全一致の名前は map で引き、`文「…」の存在` のような前後で決まる名前は表の上から順に調べる（順番に意味がある。汎用の `…の存在` は最後のほう）。
  - `function_table` の「localのみ」が真のものは `SecurityLevel: local` 以外では実行されない。
- ssu：`ssu.cpp`。satori.dll 内では SAORI を経由せず直接呼ばれ、ssu.dll 単体ビルドは `SSU_SAORI_CALL_INTERFACE` を定義した場合。`regex_*` は uka_regexp（オプション `i` `s` `m` `x`、置換後は `$1` `$&` `${名前}`）。パターンに定義の無いサブルーチン名（`(?&名前)` `\g<名前>`）は、`set_ssu_name_resolver` で渡される関数（`shiori_plugin.cpp` の `resolve_ssu_regex_name`。`Satori::Call` で `（名前）` を展開する）が解決する。解決した個数だけ、エンジンが呼び出し専用のグループを末尾に足すので、`regex_push_groups` では数えない。
- `SakuraScript.cpp` / `main.cpp` / `cn.cpp` / `TimeCommands.cpp` はどのプロジェクトにも入っていない（死んだコード）。
- マルチインスタンス（`multi_loadu` / `multi_load` / `multi_unload` / `multi_request` / `multi_Set_loghandler`。YAYA・華和梨と同じ形）：入口は `SakuraDLLHost.cpp` の末尾（`SATORI_DLL` か `POSIX` のビルドだけ。ssu.dll には入れない）。実体は `SakuraDLLHost::m_instances`（id が添字、0 は空き）に入り、従来の `load` / `request` / `unload` の実体（`gSatori`、`m_dll`）とは別。
  - 実体を呼ぶ間は、必ず `SakuraDLLHostScope` を作る。`m_dll`（`I()`）・`GetSender()` の Sender・乱数（`mt19937_set_current`）を実体のものに差し替え、抜けると戻す。だから実体の中のコードは「自分が唯一の実体」として書ける。
  - 従来の入口（`load` / `loadu` / `unload` / `request` / `getversionlist` / `logsend` / `Set_loghandler`）も `SakuraDLLHostScope scope(SakuraDLLHost::Default())` を作ってから実体を使う。`I()` は呼び出し中の実体を返すだけなので、`multi_request` の途中（ログのコールバックなど）で従来の入口が再入されると、張らなければその実体に入ってしまう。`Default()` は最初のスコープに入る前の `m_dll`（既定の実体）を覚えている。
  - ログのインデント（`Sender::nest_object`）の深さは Sender のメンバ（`m_nest_count`）。実体をまたぐ入れ子（A → SAORI → B）でも混ざらない。
  - 実体ごとに持つ状態のうち、`Satori` のメンバでないのは Sender（`m_own_sender`）と乱数の状態（`m_own_mt`）だけ。残りは `Satori` のメンバ（再帰カウンタの `m_sentence_internal_nest_count` なども）。**新しい static を足すときは、定数表（遅延初期化）に限る。実体の状態はメンバに置く。**ある実体の呼び出しの途中で別の実体が呼ばれることがある（SAORI や SSTP を経由してベースウェアが同期で呼ぶ）ので、「1回のリクエストの中だけの値」でも static にしない。
  - ssu の正規表現のサブルーチン名の解決先（`set_ssu_name_resolver`）はプロセスで1つだが、`ShioriPlugins::request` が SAORI を呼ぶたびに呼んだ実体を設定し直すので、実体どうしで混ざらない。
  - VC6 は `Create<T>()` のようなメンバーテンプレートの明示的な型指定で内部エラーになる。`SakuraDLLHost::Add(new Satori())` の形にしてある。
  - `multi_load*` は失敗で `-1`、`multi_unload` は存在しない id で FALSE、`multi_request` は NULL。渡したメモリは失敗しても解放する。里々の `load` は常に成功する。
- ログ送信：`_/Sender.cpp`。送信先は `logsend(hwnd)` で渡されたウィンドウで、無ければ最初の送信時に れしば → tama（`TamaWndClass`）の順に FindWindow する。tama モードでは WM_COPYDATA の `dwData` がログコード（YAYA と同じ値）。errsender は E_E で送られ、`capability` で `response.errorlevel` 非対応と分かった後は MessageBox になる。

## 動作確認

- **multi_harness**：`satoriya/test/harness/multi_harness.c`（辞書と使い方は同じフォルダの README.md）。マルチインスタンスのエクスポートを、変数・乱数・正規表現のサブルーチン名・ログのコールバックが混ざらないか、unload 後も残りが動くか、無効な id まで含めて確かめる。POSIX は `test/posix/posix_smoke.c` が同じ趣旨の確認をする。
- **harness**：`satoriya/test/harness/harness.c`（使い方は同じフォルダの README.md）で、SSP を起動せずに satori.dll の `load` / `request` / `unload` を直接呼べる。リクエストも応答も生のバイト列（文字コード変換なし）なので、master の DLL には SJIS、unicode 版には Charset ヘッダに合わせたバイト列を渡して比較できる。
  - 辞書の例は、トークを `：` から書き始めるのが一般的。最初の `：` が `\0`、次が `\1` で、以後交互になる。`：` なしだと冒頭のデフォルトサーフェスの後の `\1` のまま話す。
  - トーク名をそのまま ID にすると、`On` で始まらない ID では冒頭のデフォルトサーフェス（`\0\s[0]\1\s[10]`）と末尾の `\e` が付かない。サーフェスを見るときは `On` で始まる名前にする。
  - master との比較は、master を `git worktree` で別に展開してビルドする。自動ウェイト（`\_w[n]`）は master と同じ値になる（Mc202-1 以降。master は SJIS のバイト数で数えていたので、unicode 版は `count_width` で半角換算して合わせている）。
- **SSP**：`ssp.exe --option readonly --ghost <ゴーストのフォルダ名>` で起動する（readonly ならユーザー環境を変更しない。https://ssp.shillest.net/ukadoc/ssphelp/option.html ）。TCP 9801 に `NOTIFY SSTP/1.1` と `Event: ...` を送ると、応答の `Script:` にスクリプトが入る。外部 SSTP の `\-` は無視されるので、終了は WM_CLOSE を送るか手で行う。
- **tama / tamac**（`C:\D_DRIVE\MyDocuments\GitHub\tama`）：`tamac satori.dll` で読み込み時のログ（エラーは stderr に `[ERROR]`、終了コード 2）、`tamac satori.dll -r < request.txt` で応答を stdout に出す。
  - `--ci` の `::error` には file / line が入らない（里々のエラー文が `ファイル(行) :` 形式でないため）。
  - `tama/builds/Release/tamac.exe` は古い（`-r` 非対応）ことがある。現行ソースは `/p:PlatformToolset=v145` と `/p:OutDir=<scratchpad>` を付けて MSBuild でビルドする。
  - さとりて相当の変換は `ShioriEcho`（`satori_AnalyzeRequest.cpp`）。`＄デバッグ＝有効`（`satori_conf.txt` の ＊初期化 など）と `SecurityLevel: local` が必要で、Reference0, 1, ... が1行ずつ里々の文として展開され、`Value` にさくらスクリプトが返る。
- harness も tamac も load / unload でゴーストのフォルダに `satori_savedata.txt` を書くので、テストには既存のゴーストのコピーを使い、元は触らない。
