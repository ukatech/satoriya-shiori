/*
	SHIORI DLL を直接呼ぶ動作確認用ハーネス

	使い方:
		harness <DLLのパス> <ゴーストのフォルダ(末尾に\)> <出力ファイル> <リクエストファイル1> [<リクエストファイル2> ...]

	load を1回呼び、各リクエストファイルの中身をそのままのバイト列で request に渡し、
	応答をそのままのバイト列で出力ファイルに書く（応答の間は "\n=====\n" で区切る）。最後に unload する。
	文字コードの変換は一切しないので、リクエストは Charset ヘッダに合わせたバイト列で用意すること。

	ビルド（VC6）:
		cl /nologo /O2 harness.c
*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef BOOL (__cdecl *LOADF)(HGLOBAL, long);
typedef HGLOBAL (__cdecl *REQF)(HGLOBAL, long *);
typedef BOOL (__cdecl *UNLOADF)(void);

static char *readfile(const char *fn, long *len)
{
	FILE *f = fopen(fn, "rb");
	char *buf;
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	*len = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = (char *)malloc(*len + 1);
	fread(buf, 1, *len, f);
	fclose(f);
	return buf;
}

int main(int argc, char **argv)
{
	HMODULE h;
	LOADF load;
	REQF request;
	UNLOADF unload;
	HGLOBAL g;
	long len;
	int i;
	FILE *out;

	if (argc < 5) {
		fprintf(stderr, "usage: harness <dll> <ghostdir\\> <out> <req1> [<req2> ...]\n");
		return 1;
	}
	h = LoadLibrary(argv[1]);
	if (!h) { fprintf(stderr, "LoadLibrary failed\n"); return 2; }
	load = (LOADF)GetProcAddress(h, "load");
	request = (REQF)GetProcAddress(h, "request");
	unload = (UNLOADF)GetProcAddress(h, "unload");
	if (!load || !request || !unload) { fprintf(stderr, "export not found\n"); return 3; }

	/* load に渡したメモリは DLL 側で解放される */
	len = (long)strlen(argv[2]);
	g = GlobalAlloc(GMEM_FIXED, len);
	memcpy(g, argv[2], len);
	load(g, len);

	out = fopen(argv[3], "wb");
	if (!out) { fprintf(stderr, "cannot open %s\n", argv[3]); return 4; }
	for (i = 4; i < argc; i++) {
		char *req = readfile(argv[i], &len);
		HGLOBAL r;
		if (!req) { fprintf(stderr, "cannot read %s\n", argv[i]); continue; }
		g = GlobalAlloc(GMEM_FIXED, len);
		memcpy(g, req, len);
		free(req);
		r = request(g, &len);
		if (r) {
			fwrite(r, 1, len, out);
			GlobalFree(r);
		}
		fwrite("\n=====\n", 1, 7, out);
	}
	fclose(out);
	unload();
	FreeLibrary(h);
	return 0;
}
