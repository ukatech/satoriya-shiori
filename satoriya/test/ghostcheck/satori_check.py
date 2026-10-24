#!/usr/bin/env python3
"""里々の辞書を libsatori.so で実際に動かして確かめる、Linux 用の小さなチェッカー。

使い方:
    python3 satori_check.py <libsatori.so> <辞書のフォルダ> [--runs N] [--expect 期待ファイル]

やること:
  1. フォルダ内の dic*.txt から ＊イベント名 を全部拾う。
  2. 辞書を一時フォルダにコピーして load し、各イベントを N 回（既定 30 回）発火する。
     里々は同じ定義の候補をランダムに選ぶので、複数回発火して候補を踏む。
  3. 次のどれかに当たれば失敗として報告し、終了コード 1 で終わる。
       - ErrorLevel / ErrorDescription ヘッダが付く（辞書の構文エラー。以降の定義は読み込まれない）
       - 204 No Content が返る（辞書にあるイベントなのに応答が空）
       - SHIORI/3.0 200 OK 以外が返る / request が NULL を返す
  4. Value に全角の （ ） が残っていれば警告する（未定義の関数・変数は
     エラーにならず、括弧ごとそのまま出力されるため）。--strict で失敗扱いにする。

期待ファイル（任意。複雑な辞書でゴールを書いておく用）:
    イベント名<TAB>応答に含まれるべき文字列
    イベント名<TAB>!応答に含まれてはいけない文字列
  「含まれるべき」は N 回の発火のうちどれか 1 回に出れば OK（ランダム候補を考慮）。
  「含まれてはいけない」は 1 回でも出たら失敗。空行と # で始まる行は無視する。
  Value は SHIORI の応答そのまま（さくらスクリプト込み）なので、文字列は
  \\0 \\n などを含まない部分（セリフの文面）で書くのが楽。

注意:
  - 辞書は UTF-8 で書く（unicode ブランチ向け）。
  - 里々の dlopen 先は libsatori.so（makefile.linux の出力）。
"""
import argparse
import ctypes
import glob
import os
import re
import shutil
import sys
import tempfile

EVENT_RE = re.compile(r"^[＊*]\s*([^\s\t]+)")


def load_library(path):
    libc = ctypes.CDLL(None)
    libc.malloc.restype = ctypes.c_void_p
    libc.malloc.argtypes = [ctypes.c_size_t]
    libc.free.argtypes = [ctypes.c_void_p]

    lib = ctypes.CDLL(os.path.abspath(path))
    lib.load.argtypes = [ctypes.c_void_p, ctypes.c_long]
    lib.load.restype = ctypes.c_int
    lib.request.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_long)]
    lib.request.restype = ctypes.c_void_p
    lib.unload.restype = ctypes.c_int
    return lib, libc


class Shiori:
    """load / request / unload は渡したバッファを free するので、malloc したものを渡す。"""

    def __init__(self, lib_path, ghost_dir):
        self.lib, self.libc = load_library(lib_path)
        path = (ghost_dir.rstrip("/") + "/").encode()
        if not self.lib.load(self._heap(path), len(path)):
            raise RuntimeError("load に失敗しました: " + ghost_dir)

    def _heap(self, data):
        p = self.libc.malloc(len(data) + 1)
        ctypes.memmove(p, data + b"\0", len(data) + 1)
        return p

    def get(self, event_id):
        req = (
            "GET SHIORI/3.0\r\nCharset: UTF-8\r\nSender: SSP\r\n"
            "SecurityLevel: local\r\nID: %s\r\nReference0: master\r\n\r\n" % event_id
        ).encode("utf-8")
        n = ctypes.c_long(len(req))
        p = self.lib.request(self._heap(req), ctypes.byref(n))
        if not p:
            return None
        text = ctypes.string_at(p, n.value).decode("utf-8", "replace")
        self.libc.free(p)
        return text

    def close(self):
        self.lib.unload()


def parse_response(text):
    lines = text.replace("\r\n", "\n").split("\n")
    status = lines[0] if lines else ""
    headers = {}
    for line in lines[1:]:
        if ": " in line:
            k, v = line.split(": ", 1)
            headers[k] = v
    return status, headers


def find_events(dic_files):
    events = []
    for f in dic_files:
        with open(f, encoding="utf-8", errors="replace") as fh:
            for line in fh:
                m = EVENT_RE.match(line)
                if m and m.group(1) not in events:
                    events.append(m.group(1))
    return events


def read_expect(path):
    expect = {}
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            line = raw.rstrip("\r\n")
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            if "\t" not in line:
                sys.exit("期待ファイルの書式が違います（タブ区切り）: " + line)
            ev, want = line.split("\t", 1)
            expect.setdefault(ev, []).append(want)
    return expect


def main():
    ap = argparse.ArgumentParser(description="里々の辞書を実際に発火させて確かめる")
    ap.add_argument("library", help="libsatori.so のパス")
    ap.add_argument("dicdir", help="dic*.txt が入ったフォルダ")
    ap.add_argument("--runs", type=int, default=30, help="1イベントあたりの発火回数")
    ap.add_argument("--expect", help="期待ファイル")
    ap.add_argument("--strict", action="store_true", help="全角括弧の残りも失敗扱いにする")
    args = ap.parse_args()

    dic_files = sorted(glob.glob(os.path.join(args.dicdir, "dic*.txt")))
    if not dic_files:
        sys.exit("dic*.txt が見つかりません: " + args.dicdir)
    events = find_events(dic_files)
    expect = read_expect(args.expect) if args.expect else {}
    if not events and not expect:
        sys.exit("＊イベントが 1 つも見つかりません")
    for ev in expect:
        if ev not in events:
            events.append(ev)

    # セーブデータ等で元のフォルダを汚さないよう、一時フォルダにコピーして動かす
    work = tempfile.mkdtemp(prefix="satori_check_")
    failures, warnings = [], []
    try:
        for f in dic_files:
            shutil.copy(f, work)
        sh = Shiori(args.library, work)
        engine_errors = set()
        for ev in events:
            values, bad_status = [], None
            for _ in range(args.runs):
                text = sh.get(ev)
                if text is None:
                    bad_status = "request が NULL を返した"
                    break
                status, headers = parse_response(text)
                if "ErrorLevel" in headers or "ErrorDescription" in headers:
                    engine_errors.add(
                        "[%s] %s" % (headers.get("ErrorLevel", "?"), headers.get("ErrorDescription", "").strip())
                    )
                if "204" in status:
                    bad_status = "204 No Content（応答が空。辞書にあるのに発火しない）"
                    break
                if "200 OK" not in status:
                    bad_status = status
                    break
                values.append(headers.get("Value", ""))
            if bad_status:
                failures.append("%s: %s" % (ev, bad_status))
                continue
            distinct = sorted(set(values))
            print("OK  %-24s 異なる応答 %d 通り" % (ev, len(distinct)))
            for v in distinct:
                if "（" in v or "）" in v:
                    msg = "%s: 全角括弧が残っている（未定義の関数・変数の疑い）: %s" % (ev, v)
                    (failures if args.strict else warnings).append(msg)
                    break
            for want in expect.get(ev, []):
                if want.startswith("!"):
                    bad = want[1:]
                    if any(bad in v for v in values):
                        failures.append("%s: 含まれてはいけない文字列が出た: %s" % (ev, bad))
                elif not any(want in v for v in values):
                    failures.append("%s: %d 回発火しても期待する文字列が出なかった: %s" % (ev, args.runs, want))
        sh.close()
        for e in sorted(engine_errors):
            # エラー文に出る一時フォルダのパスを、元の辞書フォルダに戻して読みやすくする
            e = e.replace(work, args.dicdir.rstrip("/"))
            failures.append("里々のエラー: " + e)
    finally:
        shutil.rmtree(work, ignore_errors=True)

    for w in warnings:
        print("WARN " + w)
    for f in failures:
        print("NG   " + f)
    print("--- イベント %d 個 / 失敗 %d / 警告 %d" % (len(events), len(failures), len(warnings)))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
