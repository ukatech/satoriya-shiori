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

### サブモジュール（DEELX）
`satoriya/deelx`（https://github.com/ponapalt/deelx 、ヘッダのみ）は ssu の `regex_*` が使う正規表現エンジンで、git サブモジュールです。**ビルドの前に必ず最新にします**。

```
git submodule update --init --remote satoriya/deelx
```

- ポインタが変わると `git status` に `modified: satoriya/deelx` が出る。ビルドして問題がないことを確かめてから `satoriya/deelx を更新` のようなコミットで記録する（リリース前にも行う）。
- clone 直後は `git submodule update --init` が要る（空のままだとビルドが通らない）。
- DEELX は正規表現の書式エラーを報告しない（閉じていない括弧などはそのまま解釈される）。

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
- 1文字取得は `get_a_chr`、文字数は `count_chars`（サロゲートペアを1文字として扱う）。
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
- ssu：`ssu.cpp`。satori.dll 内では SAORI を経由せず直接呼ばれ、ssu.dll 単体ビルドは `SSU_SAORI_CALL_INTERFACE` を定義した場合。`regex_*` は DEELX（オプション `i` `s` `m` `x`、置換後は `$1` `$&` `${名前}`）。
- `SakuraScript.cpp` / `main.cpp` / `cn.cpp` / `TimeCommands.cpp` はどのプロジェクトにも入っていない（死んだコード）。
- ログ送信：`_/Sender.cpp`。送信先は `logsend(hwnd)` で渡されたウィンドウで、無ければ最初の送信時に れしば → tama（`TamaWndClass`）の順に FindWindow する。tama モードでは WM_COPYDATA の `dwData` がログコード（YAYA と同じ値）。errsender は E_E で送られ、`capability` で `response.errorlevel` 非対応と分かった後は MessageBox になる。

## 動作確認

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
