#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Satori (里々) Dictionary Validator script.

This validator performs static syntax analysis and resource reference checks for
Ukagaka ghost dictionary files written for the Satori engine.

C++ Engine Source Code Reference & Rationale:
- Comment Delimiter: Full-width '＃' starts a comment line. (Ref: satoriya/satori/satori_load_dict.cpp, line 145, `pre_process`)
- Escape Character: Full-width 'φ' escapes the immediately following character. (Ref: satoriya/satori/satori_load_dict.cpp, line 140, `pre_process`)
- Bracket Nesting: Full-width '（' and '）' increment/decrement `kakko_nest_count`. (Ref: satoriya/satori/satori_load_dict.cpp, lines 157-160, `pre_process`)
  If `kakko_nest_count > 0` at EOF, `pre_process` returns false (syntax error).
- Half-width characters: Half-width parentheses '()', curly braces '{}', and quotes '""', "''" in talk body text
  are NOT treated as Satori syntax tags and do NOT affect bracket nesting.
- Entry Definitions: Lines starting with '＊', '＄', '＠', '％' define entry units (Ref: satoriya/satori/satori_load_dict.cpp, lines 38-55, `lines_to_units`).
  Duplicate entries (e.g. multiple '＊OnBoot') are supported by Satori (grouped into multiple units for random selection).
- Character Encodings: Satori C++ engine uses CUnicodeF (Ref: satoriya/_/charset.cpp, `getCharactorSet` / `CUnicodeF::utf8_to_sjis`).
  UTF-8 and CP932 (Shift_JIS) are supported.
"""

import os
import sys
import re
import argparse
from pathlib import Path
from typing import List, Dict, Tuple, Optional, Set

# Issue Severity Levels
LEVEL_ERROR = "ERROR"
LEVEL_WARN = "WARN"
LEVEL_INFO = "INFO"

class Issue:
    def __init__(self, level: str, line_num: int, message: str, code: str = ""):
        self.level = level
        self.line_num = line_num
        self.message = message
        self.code = code

    def __repr__(self):
        line_str = f"Line {self.line_num}: " if self.line_num > 0 else ""
        return f"[{self.level}] {line_str}{self.message}"

class FileValidationResult:
    def __init__(self, filepath: Path, encoding: str):
        self.filepath = filepath
        self.encoding = encoding
        self.issues: List[Issue] = []

    @property
    def error_count(self) -> int:
        return sum(1 for i in self.issues if i.level == LEVEL_ERROR)

    @property
    def warn_count(self) -> int:
        return sum(1 for i in self.issues if i.level == LEVEL_WARN)

    @property
    def status(self) -> str:
        if self.error_count > 0:
            return "FAIL"
        elif self.warn_count > 0:
            return "WARN"
        else:
            return "PASS"

def detect_encoding(filepath: Path) -> Tuple[Optional[str], Optional[str], Optional[str]]:
    """
    Attempts to read file content using UTF-8, UTF-8 with BOM, or CP932 (Shift_JIS).
    Returns (encoding_name, file_text, error_message).

    Ref: satoriya/_/charset.cpp lines 200-280 (getCharactorSet) & CUnicodeF::utf8_to_sjis
    """
    try:
        raw_bytes = filepath.read_bytes()
    except Exception as e:
        return None, None, f"Failed to read file: {e}"

    if not raw_bytes:
        return "Empty", "", None

    # Check UTF-8 with BOM
    if raw_bytes.startswith(b'\xef\xbb\xbf'):
        try:
            text = raw_bytes[3:].decode('utf-8')
            return "UTF-8 BOM", text, None
        except UnicodeDecodeError:
            pass

    # Try UTF-8 without BOM
    try:
        text = raw_bytes.decode('utf-8')
        return "UTF-8", text, None
    except UnicodeDecodeError:
        pass

    # Fallback to CP932 (Shift_JIS)
    try:
        text = raw_bytes.decode('cp932')
        return "CP932", text, None
    except UnicodeDecodeError:
        pass

    return None, None, "Unable to decode file as UTF-8 or CP932 (Shift_JIS)."

def validate_satori_file(filepath: Path, master_dir: Optional[Path] = None) -> FileValidationResult:
    """
    Validates a single Satori dictionary file.

    Implements parser logic mirroring Satori C++ `pre_process` function in `satoriya/satori/satori_load_dict.cpp`.
    """
    encoding, content, enc_err = detect_encoding(filepath)
    result = FileValidationResult(filepath, encoding or "Unknown")

    if enc_err:
        result.issues.append(Issue(LEVEL_ERROR, 0, enc_err, "E_ENCODING"))
        return result

    if not content or content.strip() == "":
        result.issues.append(Issue(LEVEL_WARN, 0, "File is empty", "W_EMPTY_FILE"))
        return result

    # Check for engine-unsupported encodings warning if applicable
    # Note: Satori C++ engine converts UTF-8 and CP932 to Shift_JIS internally.
    if encoding not in ("UTF-8", "UTF-8 BOM", "CP932", "Empty"):
        result.issues.append(Issue(LEVEL_WARN, 0, f"Unusual encoding detected: {encoding}", "W_ENCODING"))

    lines = content.splitlines()

    # C++ pre_process variables:
    # kakko_nest_count: tracks '（' nesting
    kakko_nest_count = 0
    kakko_start_lines: List[int] = []  # Stack of line numbers for unclosed '（'

    defined_entries: Set[str] = set()
    entry_header_pattern = re.compile(r'^[＊＄＠％]([^\n\r]+)')

    for idx, raw_line in enumerate(lines, start=1):
        line = raw_line.strip()

        # Check for entry header (e.g. ＊OnBoot, ＄Variable)
        header_match = entry_header_pattern.match(raw_line)
        if header_match:
            entry_name = header_match.group(1).split('：')[0].split('\t')[0].strip()
            if entry_name in defined_entries:
                # Satori supports duplicate entry headers (groups them into multiple units)
                result.issues.append(Issue(LEVEL_INFO, idx, f"Duplicate entry header definition: '＊{entry_name}'", "I_DUPLICATE_ENTRY"))
            else:
                defined_entries.add(entry_name)

        # Parse character by character mirroring `pre_process` in `satoriya/satori/satori_load_dict.cpp` lines 130-180
        p_idx = 0
        escape = False
        while p_idx < len(raw_line):
            ch = raw_line[p_idx]

            if escape:
                # Escape skips special handling for 'φ' or next char
                escape = False
                p_idx += 1
                continue

            if ch == 'φ':
                escape = True
                p_idx += 1
                continue

            if ch == '＃':
                # Full-width '＃' terminates line as comment
                break

            if ch == '（':
                kakko_nest_count += 1
                kakko_start_lines.append(idx)
            elif ch == '）':
                if kakko_nest_count > 0:
                    kakko_nest_count -= 1
                    if kakko_start_lines:
                        kakko_start_lines.pop()
                else:
                    # Unmatched closing bracket
                    result.issues.append(Issue(LEVEL_ERROR, idx, "Unmatched closing full-width bracket '）'", "E_UNMATCHED_CLOSING_BRACKET"))

            p_idx += 1

    # After checking all lines, verify `kakko_nest_count` == 0
    # Ref: `satoriya/satori/satori_load_dict.cpp` lines 183-186:
    # `else if ( line_number == in.size() ) { return false; }` when kakko_nest_count > 0
    if kakko_nest_count > 0:
        start_line = kakko_start_lines[-1] if kakko_start_lines else 0
        result.issues.append(Issue(
            LEVEL_ERROR,
            start_line,
            f"Unclosed full-width bracket '（' (nest depth: {kakko_nest_count})",
            "E_UNCLOSED_BRACKET"
        ))

    # Optional Resource / File Reference Checks
    if master_dir and master_dir.exists():
        # Scan for referenced image or sound files (e.g. Surface or file references in talk text)
        ref_matches = re.findall(r'([\w\-\./]+\.(?:png|pna|bmp|jpg|wav|mp3))', content, re.IGNORECASE)
        for ref in ref_matches:
            # Check if reference exists in master_dir or ghost dir
            ref_path = master_dir / ref
            alt_ref_path = master_dir.parent / ref
            if not ref_path.exists() and not alt_ref_path.exists():
                # Avoid duplicate warnings for same reference
                if not any(i.message.endswith(f"'{ref}'") for i in result.issues):
                    result.issues.append(Issue(LEVEL_WARN, 0, f"Referenced file not found in master directory: '{ref}'", "W_MISSING_REF"))

    return result

def is_dictionary_file(filepath: Path) -> bool:
    """
    Determines if a file is a Satori dictionary file.
    Excludes non-dictionary files like descript.txt, satori_conf.txt, replace.txt, satori_license.txt, etc.
    """
    if filepath.suffix.lower() not in ('.txt', '.dic', '.sat'):
        return False

    name_lower = filepath.name.lower()
    excluded_names = {
        'descript.txt', 'satori_conf.txt', 'satori_license.txt',
        'replace.txt', 'replace_after.txt', 'satorite.txt', 'れしば.txt',
        'developer_options.txt', 'install.txt', 'readme.txt', 'autoload.txt'
    }
    if name_lower in excluded_names:
        return False

    return True

def find_dictionary_files(target_dir: Path) -> List[Path]:
    """Recursively finds all dictionary files in target_dir."""
    dic_files = []
    if not target_dir.exists():
        return dic_files

    for path in sorted(target_dir.rglob('*')):
        if path.is_file() and is_dictionary_file(path):
            dic_files.append(path)
    return dic_files

def run_self_test() -> bool:
    """Executes validator unit tests using fixtures in `tests/fixtures/`."""
    fixtures_dir = Path(__file__).parent / 'fixtures'
    if not fixtures_dir.exists():
        print(f"[SELF-TEST] Fixtures directory not found: {fixtures_dir}")
        return False

    print("========================================")
    print(" Running Validator Self-Tests...")
    print("========================================")

    test_cases = [
        ("valid_utf8.txt", 0, "UTF-8"),
        ("valid_utf8_bom.txt", 0, "UTF-8 BOM"),
        ("valid_cp932.txt", 0, "CP932"),
        ("invalid_unclosed_kakko.txt", 1, None),
        ("valid_escaped_kakko.txt", 0, None),
        ("valid_halfwidth_brackets.txt", 0, None),
        ("empty_file.txt", 0, "Empty")
    ]

    all_passed = True
    for filename, expected_errors, expected_enc in test_cases:
        filepath = fixtures_dir / filename
        if not filepath.exists():
            print(f"  [FAIL] Missing fixture file: {filename}")
            all_passed = False
            continue

        res = validate_satori_file(filepath)
        enc_ok = (expected_enc is None) or (res.encoding == expected_enc)
        err_ok = (res.error_count == expected_errors)

        if enc_ok and err_ok:
            print(f"  [PASS] {filename} (Enc: {res.encoding}, Errors: {res.error_count})")
        else:
            print(f"  [FAIL] {filename}: expected Enc={expected_enc}, Errors={expected_errors}; got Enc={res.encoding}, Errors={res.error_count}")
            for issue in res.issues:
                print(f"         {issue}")
            all_passed = False

    print("========================================")
    print(f" Self-Test Result: {'PASS' if all_passed else 'FAIL'}")
    print("========================================\n")
    return all_passed

def generate_report(results_by_target: Dict[str, List[FileValidationResult]], report_path: Path, self_test_passed: bool):
    """Generates `test_report.md`."""
    total_files = 0
    total_errors = 0
    total_warns = 0

    for target, res_list in results_by_target.items():
        total_files += len(res_list)
        for r in res_list:
            total_errors += r.error_count
            total_warns += r.warn_count

    with open(report_path, 'w', encoding='utf-8') as f:
        f.write("# 里々 (Satori) 辞書ファイル検証レポート\n\n")
        f.write("## 1. 概要 (Executive Summary)\n\n")
        f.write(f"- **バリデータセルフテスト**: {'✅ PASS' if self_test_passed else '❌ FAIL'}\n")
        f.write(f"- **検証対象ファイル総数**: {total_files} ファイル\n")
        f.write(f"- **検出エラー (ERROR)**: {total_errors} 件\n")
        f.write(f"- **検出警告 (WARN)**: {total_warns} 件\n\n")

        f.write("## 2. バリデータ自身のエミュレーションテスト (Self-Test Status)\n\n")
        f.write("`tests/fixtures/` 内の各種構文パターン（UTF-8, CP932, BOM付き, エスケープ `φ`, 半角括弧, 空ファイル等）に対するセルフテスト結果：\n\n")
        f.write(f"- 結果: **{'PASS (すべての単体テスト合格)' if self_test_passed else 'FAIL (単体テスト失敗)'}**\n\n")

        f.write("## 3. ファイル別静的検証結果 (Static Validation Details)\n\n")
        f.write("| ターゲット | ファイルパス | 文字コード | 判定 | ERROR | WARN | 詳細・行番号 |\n")
        f.write("| --- | --- | --- | --- | --- | --- | --- |\n")

        for target_name, res_list in results_by_target.items():
            for r in res_list:
                rel_path = r.filepath.as_posix()
                status_icon = "✅ PASS" if r.status == "PASS" else ("⚠️ WARN" if r.status == "WARN" else "❌ FAIL")

                details_str = "<br>".join(
                    f"Line {i.line_num}: [{i.level}] {i.message}" for i in r.issues if i.level in (LEVEL_ERROR, LEVEL_WARN)
                )
                if not details_str:
                    details_str = "-"

                f.write(f"| `{target_name}` | `{rel_path}` | {r.encoding} | {status_icon} | {r.error_count} | {r.warn_count} | {details_str} |\n")

        f.write("\n## 4. 実エンジン動作テスト (Real Engine Test Status)\n\n")
        f.write("### ステータス: **未検証 (Linux環境制約)**\n\n")
        f.write("#### 未検証の理由:\n")
        f.write("1. **C++ エンジンの Windows API 依存**:\n")
        f.write("   `satoriya/satori/` 内の C++ ソースコード (`satoriya/_/charset.cpp`) は Windows 固有の API (`<windows.h>`, `<mbctype.h>`, `_setmbcp`) に依存しています。\n")
        f.write("   Linux 用の `makefile.posix` では文字コード変換層 (`charset.cpp`) がビルドから除外されているため、`libsatori.so` の動的リンク時に `UTF8toSJIS` シンボル未定義エラーが発生します。\n")
        f.write("2. **Wine 非搭載**:\n")
        f.write("   CI環境に Wine が用意されていないため、`satori.dll` や Windows 用バイナリを直接実行することはできません。\n\n")
        f.write("#### 将来的な実エンジン検証案 (改善提案):\n")
        f.write("- **案A**: GitHub Actions ワークフローで `windows-latest` ランナーを使用し、Windows 上で `satori.dll` を動的ロードして動作テストを実施する。\n")
        f.write("- **案B**: `tests/` 配下に Linux/POSIX 向け `iconv` を用いた `UTF8toSJIS` / `SJIStoUTF8` 代替実装を用意し、C++ エンジンを Linux 上で完全ネイティブビルド可能にする。\n\n")

        f.write("## 5. 発見された問題点と修正提案 (Discovered Issues & Recommended Fixes)\n\n")
        has_issues = False
        for target_name, res_list in results_by_target.items():
            for r in res_list:
                if r.error_count > 0 or r.warn_count > 0:
                    has_issues = True
                    f.write(f"### ファイル: `{r.filepath.as_posix()}`\n")
                    for issue in r.issues:
                        if issue.level in (LEVEL_ERROR, LEVEL_WARN):
                            f.write(f"- **[{issue.level}] Line {issue.line_num}**: {issue.message}\n")
                    f.write("\n")

        if not has_issues:
            f.write("検出された問題はありません。すべての辞書ファイルが静的検証を通過しました。\n")

    print(f"Report generated successfully: {report_path}")

def main():
    parser = argparse.ArgumentParser(description="Satori Dictionary Validator")
    parser.add_argument(
        '--target',
        default='kampo-ghost/ghost/master/',
        help="Target directory to validate ('kampo-ghost/ghost/master/', '8-1/ghost/master/', 'satoriya/satori/test/', 'all')"
    )
    parser.add_argument(
        '--report',
        default='test_report.md',
        help="Path for generated markdown report file"
    )
    parser.add_argument(
        '--self-test-only',
        action='store_true',
        help="Run self-tests only and exit"
    )

    args = parser.parse_args()

    # Step 1: Always run self-test first
    self_test_passed = run_self_test()

    if args.self_test_only:
        sys.exit(0 if self_test_passed else 1)

    if not self_test_passed:
        print("[ERROR] Validator self-test failed! Aborting validation.")
        sys.exit(1)

    # Step 2: Determine target directories
    target_dirs: Dict[str, Path] = {}
    if args.target == 'all':
        target_dirs['kampo-ghost'] = Path('kampo-ghost/ghost/master/')
        target_dirs['8-1'] = Path('8-1/ghost/master/')
        target_dirs['satori-test'] = Path('satoriya/satori/test/')
    else:
        target_path = Path(args.target)
        target_name = target_path.parts[0] if target_path.parts else "target"
        target_dirs[target_name] = target_path

    results_by_target: Dict[str, List[FileValidationResult]] = {}
    overall_errors = 0

    print("========================================")
    print(" Running Satori Dictionary Validation")
    print("========================================")

    for target_name, dir_path in target_dirs.items():
        print(f"\n[Target: {target_name}] ({dir_path})")
        dic_files = find_dictionary_files(dir_path)

        if not dic_files:
            print(f"  No dictionary files found in {dir_path}")
            results_by_target[target_name] = []
            continue

        res_list = []
        for filepath in dic_files:
            res = validate_satori_file(filepath, master_dir=dir_path)
            res_list.append(res)
            overall_errors += res.error_count

            status_str = f"[{res.status}]"
            print(f"  {status_str:6s} {filepath.as_posix()} (Enc: {res.encoding}, ERR: {res.error_count}, WARN: {res.warn_count})")
            for issue in res.issues:
                if issue.level in (LEVEL_ERROR, LEVEL_WARN):
                    print(f"         Line {issue.line_num}: [{issue.level}] {issue.message}")

        results_by_target[target_name] = res_list

    # Generate Markdown Report
    report_path = Path(args.report)
    generate_report(results_by_target, report_path, self_test_passed)

    print("\n========================================")
    print(f" Validation Complete. Overall Errors: {overall_errors}")
    print("========================================")

    # Exit with code 0 if self-test passed and overall dictionary errors is 0; otherwise 1.
    sys.exit(0 if (self_test_passed and overall_errors == 0) else 1)

if __name__ == '__main__':
    main()
