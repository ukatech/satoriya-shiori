/*
	Test harness for the multi-instance exports of satori.dll
	(multi_loadu / multi_request / multi_unload / multi_Set_loghandler).

	Usage:
		multi_harness <path to satori.dll> <ghost A (ending with a backslash)> <ghost B> <ghost C>

	Loads A and B with multi_loadu and C with the legacy load, sends requests alternately, and checks that
	variables, random numbers, regex subroutine names and log callbacks are not mixed between instances,
	that the remaining instances keep working after one is unloaded, and how invalid ids are handled.
	Returns 0 when everything passes.
	The three ghosts use the same dictionary except for the year line; see "multi_harness" in README.md.
	satori_savedata.txt is written into each folder, so use disposable copies.

	Build (VC6):
		cl /nologo /O2 multi_harness.c

	This file is ASCII only.
*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef BOOL (__cdecl *LOADF)(HGLOBAL, long);
typedef HGLOBAL (__cdecl *REQF)(HGLOBAL, long *);
typedef BOOL (__cdecl *UNLOADF)(void);
typedef void (__cdecl *LOGF)(const wchar_t *, int, int);
typedef void (__cdecl *SETLOGF)(LOGF);
typedef long (__cdecl *MLOADF)(HGLOBAL, long);
typedef BOOL (__cdecl *MUNLOADF)(long);
typedef HGLOBAL (__cdecl *MREQF)(long, HGLOBAL, long *);
typedef void (__cdecl *MSETLOGF)(long, LOGF);

static LOADF p_load;
static REQF p_request;
static UNLOADF p_unload;
static SETLOGF p_set_loghandler;
static MLOADF p_multi_loadu;
static MLOADF p_multi_load;
static MUNLOADF p_multi_unload;
static MREQF p_multi_request;
static MSETLOGF p_multi_set_loghandler;

static int failures;

/* number of times each instance's log callback was called */
static long count_a, count_b, count_c;
static void __cdecl log_a(const wchar_t *s, int mode, int id) { (void)s; (void)mode; (void)id; count_a++; }
static void __cdecl log_b(const wchar_t *s, int mode, int id) { (void)s; (void)mode; (void)id; count_b++; }
static void __cdecl log_c(const wchar_t *s, int mode, int id) { (void)s; (void)mode; (void)id; count_c++; }

static void ok(int cond, const char *msg)
{
	printf("%s  %s\n", cond ? "OK" : "NG", msg);
	if (!cond) failures++;
}

static HGLOBAL to_global(const char *p, long len)
{
	HGLOBAL g = GlobalAlloc(GMEM_FIXED, len + 1);
	memcpy(g, p, len);
	((char *)g)[len] = '\0';
	return g;
}

/* id == 0 uses the legacy request, otherwise multi_request. Returns a malloc'ed copy of the response (NULL if none) */
static char *send_event(long id, const char *event_id, const char *ref0)
{
	char req[512];
	long len;
	HGLOBAL r;
	char *copy;

	sprintf(req,
		"GET SHIORI/3.0\r\nCharset: UTF-8\r\nSender: test\r\nSecurityLevel: local\r\nID: %s\r\nReference0: %s\r\n\r\n",
		event_id, ref0);
	len = (long)strlen(req);
	if (id == 0) {
		r = p_request(to_global(req, len), &len);
	}
	else {
		r = p_multi_request(id, to_global(req, len), &len);
	}
	if (r == NULL) return NULL;
	copy = (char *)malloc(len + 1);
	memcpy(copy, r, len);
	copy[len] = '\0';
	GlobalFree(r);
	return copy;
}

static int contains(const char *res, const char *expected)
{
	return res != NULL && strstr(res, expected) != NULL;
}

/* Extracts the Value header of a response (empty string if none). Static buffer: valid until the next call */
static const char *value_of(const char *res)
{
	static char buf[2048];
	const char *p;
	const char *e;
	buf[0] = '\0';
	if (res == NULL) return buf;
	p = strstr(res, "Value: ");
	if (p == NULL) return buf;
	p += 7;
	e = strstr(p, "\r\n");
	if (e == NULL) e = p + strlen(p);
	if (e - p > (int)sizeof(buf) - 1) e = p + sizeof(buf) - 1;
	memcpy(buf, p, e - p);
	buf[e - p] = '\0';
	return buf;
}

static void expect(long id, const char *event_id, const char *ref0, const char *expected, const char *name)
{
	char *res = send_event(id, event_id, ref0);
	char msg[256];
	sprintf(msg, "%s (expected \"%s\")", name, expected);
	ok(contains(res, expected), msg);
	if (!contains(res, expected)) {
		printf("    response: %s\n", res ? res : "(NULL)");
	}
	free(res);
}

/* utf8 != 0: multi_loadu (UTF-8 path), otherwise multi_load (ANSI path; the test folders must have ASCII-only paths) */
static long multi_load_dir(const char *dir, int utf8)
{
	long len = (long)strlen(dir);
	return (utf8 ? p_multi_loadu : p_multi_load)(to_global(dir, len), len);
}

int main(int argc, char **argv)
{
	HMODULE h;
	long a, b, a2;
	long n;
	long before_a, before_b, before_c;
	char *ra, *rb, *rc;
	char va[2048], vb[2048], vc[2048];

	if (argc != 5) {
		fprintf(stderr, "usage: multi_harness <dll> <ghostA\\> <ghostB\\> <ghostC\\>\n");
		return 1;
	}
	h = LoadLibraryA(argv[1]);
	if (!h) { fprintf(stderr, "LoadLibrary failed\n"); return 2; }
	p_load = (LOADF)GetProcAddress(h, "load");
	p_request = (REQF)GetProcAddress(h, "request");
	p_unload = (UNLOADF)GetProcAddress(h, "unload");
	p_set_loghandler = (SETLOGF)GetProcAddress(h, "Set_loghandler");
	p_multi_loadu = (MLOADF)GetProcAddress(h, "multi_loadu");
	p_multi_load = (MLOADF)GetProcAddress(h, "multi_load");
	p_multi_unload = (MUNLOADF)GetProcAddress(h, "multi_unload");
	p_multi_request = (MREQF)GetProcAddress(h, "multi_request");
	p_multi_set_loghandler = (MSETLOGF)GetProcAddress(h, "multi_Set_loghandler");
	if (!p_load || !p_request || !p_unload || !p_set_loghandler ||
		!p_multi_loadu || !p_multi_load || !p_multi_unload || !p_multi_request || !p_multi_set_loghandler) {
		fprintf(stderr, "export not found\n");
		return 3;
	}

	/* --- load --- */
	n = (long)strlen(argv[4]);
	ok(p_load(to_global(argv[4], n), n) != 0, "legacy load (ghost C)");
	a = multi_load_dir(argv[2], 1);
	b = multi_load_dir(argv[3], 1);
	printf("    ids: A=%ld B=%ld\n", a, b);
	ok(a > 0 && b > 0 && a != b, "multi_loadu returns distinct ids >= 1");

	/* log callbacks are per instance */
	p_multi_set_loghandler(a, log_a);
	p_multi_set_loghandler(b, log_b);
	p_set_loghandler(log_c);

	/* --- variables are not mixed --- */
	expect(a, "OnSet", "1", "set", "A: set v=1");
	expect(b, "OnSet", "2", "set", "B: set v=2");
	expect(0, "OnSet", "3", "set", "C(legacy): set v=3");
	expect(a, "OnGet", "-", "v is \xef\xbc\x91", "A: v is still 1");
	expect(b, "OnGet", "-", "v is \xef\xbc\x92", "B: v is still 2");
	expect(0, "OnGet", "-", "v is \xef\xbc\x93", "C(legacy): v is still 3");

	/* --- log callbacks are not mixed --- */
	before_a = count_a; before_b = count_b; before_c = count_c;
	free(send_event(b, "OnGet", "-"));
	ok(count_b > before_b, "log: request to B reaches B's handler");
	ok(count_a == before_a && count_c == before_c, "log: request to B does not reach A's / C's handler");
	before_a = count_a; before_b = count_b; before_c = count_c;
	free(send_event(0, "OnGet", "-"));
	ok(count_c > before_c, "log: request to C(legacy) reaches Set_loghandler's handler");
	ok(count_a == before_a && count_b == before_b, "log: request to C does not reach A's / B's handler");

	/* --- random numbers are not mixed (instances loaded at the same moment give different values) --- */
	ra = send_event(a, "OnRand", "-");
	rb = send_event(b, "OnRand", "-");
	rc = send_event(0, "OnRand", "-");
	strcpy(va, value_of(ra)); strcpy(vb, value_of(rb)); strcpy(vc, value_of(rc));
	printf("    OnRand: A=%s B=%s C=%s\n", va, vb, vc);
	ok(strcmp(va, vb) != 0 && strcmp(vb, vc) != 0 && strcmp(va, vc) != 0, "random: three instances give different values");
	free(ra); free(rb); free(rc);

	/* --- regex subroutine names are resolved by the dictionary of the instance that calls --- */
	expect(a, "OnRegexSub", "-", "abc#def", "A: (?&year) resolves to A's \\d{4}");
	expect(b, "OnRegexSub", "-", "abc2026def", "B: (?&year) resolves to B's [a-z]{4} (no match)");
	expect(0, "OnRegexSub", "-", "abc#def", "C(legacy): (?&year) resolves to C's \\d{4}");

	/* --- a dictionary reload (unload + load of the same instance, right after the response) touches only that instance --- */
	expect(b, "OnReload", "-", "reload", "B: reload the dictionary");
	expect(b, "OnGet", "-", "v is \xef\xbc\x92", "B keeps its variable through its own reload (saved and restored)");
	expect(a, "OnGet", "-", "v is \xef\xbc\x91", "A is not affected by B's reload");
	expect(0, "OnGet", "-", "v is \xef\xbc\x93", "C(legacy) is not affected by B's reload");

	/* --- the rest keep working after one is unloaded --- */
	ok(p_multi_unload(a) != 0, "multi_unload(A) succeeds");
	ok(send_event(a, "OnGet", "-") == NULL, "multi_request on unloaded A returns NULL");
	expect(b, "OnGet", "-", "v is \xef\xbc\x92", "B still answers after A was unloaded");
	expect(b, "OnRegexSub", "-", "abc2026def", "B's regex still resolves its own name after A was unloaded");
	expect(0, "OnRegexSub", "-", "abc#def", "C(legacy) regex still works after A was unloaded");
	expect(0, "OnGet", "-", "v is \xef\xbc\x93", "C(legacy) still answers");

	/* --- id reuse and restoring save data --- */
	a2 = multi_load_dir(argv[2], 0);	/* also covers multi_load (ANSI path) */
	ok(a2 > 0 && a2 != b, "multi_load after unload gives a valid id");
	expect(a2, "OnGet", "-", "v is \xef\xbc\x91", "reloaded A restores its saved v=1");

	/* --- invalid ids --- */
	ok(p_multi_unload(0) == 0, "multi_unload(0) fails");
	ok(p_multi_unload(9999) == 0, "multi_unload(unknown id) fails");
	ok(send_event(9999, "OnGet", "-") == NULL, "multi_request(unknown id) returns NULL");
	ok(send_event(-1, "OnGet", "-") == NULL, "multi_request(-1) returns NULL");
	{
		long id = multi_load_dir("", 1);
		ok(id > 0, "multi_loadu with an empty path still returns an id (load never fails)");
		if (id > 0) p_multi_unload(id);
	}

	/* --- cleanup --- */
	ok(p_multi_unload(b) != 0, "multi_unload(B) succeeds");
	ok(p_multi_unload(b) == 0, "multi_unload(B) twice fails the second time");
	ok(p_multi_unload(a2) != 0, "multi_unload(A reloaded) succeeds");
	p_unload();
	FreeLibrary(h);

	if (failures) {
		printf("%d check(s) failed\n", failures);
		return 1;
	}
	printf("multi test passed\n");
	return 0;
}
