/*
 * POSIX smoke test for satori (libsatori.so / libsatori.bundle)
 *
 * Usage: posix_smoke <path to library> <ghost directory>
 *
 * dlopen() the built library and run SHIORI load / request / unload.
 * The ghost directory must hold the dictionary in test/posix/ghost
 * (dic_smoke.txt). Each request is a SHIORI/3.0 GET whose ID is an event
 * name in that dictionary, and the response Value must contain the
 * expected text.
 *
 * The library path must contain '/' (otherwise dlopen searches the library path).
 * This file is ASCII only.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int (*load_fn)(char *h, long len);
typedef int (*unload_fn)(void);
typedef char *(*request_fn)(char *h, long *len);

typedef long (*multi_load_fn)(char *h, long len);
typedef int (*multi_unload_fn)(long id);
typedef char *(*multi_request_fn)(long id, char *h, long *len);

static request_fn p_request;
static multi_request_fn p_multi_request;
static int failures;

/* Send a GET SHIORI/3.0 request for the event id and look for the expected text in the response. */
static void check(const char *id, const char *expected)
{
	char req[512];
	size_t n;
	char *h;
	long len;
	char *res;

	snprintf(req, sizeof(req),
		"GET SHIORI/3.0\r\nCharset: UTF-8\r\nSender: SSP\r\nSecurityLevel: local\r\nID: %s\r\nReference0: master\r\n\r\n", id);
	n = strlen(req);
	h = (char *)malloc(n + 1);
	if (h == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(2);
	}
	memcpy(h, req, n + 1);
	len = (long)n;

	/* request() takes ownership of the request buffer and frees it */
	res = p_request(h, &len);

	if (res == NULL) {
		printf("NG  %s: request returned NULL (expected \"%s\")\n", id, expected);
		failures++;
		return;
	}
	if (strstr(res, "SHIORI/3.0 200 OK") != NULL && strstr(res, expected) != NULL) {
		printf("OK  %s: \"%s\"\n", id, expected);
	}
	else {
		printf("NG  %s: expected \"%s\" in response:\n%.*s\n", id, expected, (int)len, res);
		failures++;
	}
	free(res);
}

/* Same as check(), but sends the request to the multi-instance entry (multi_request). expected == NULL means "must return NULL". */
static void check_multi(long mid, const char *id, const char *ref0, const char *expected)
{
	char req[512];
	size_t n;
	char *h;
	long len;
	char *res;

	snprintf(req, sizeof(req),
		"GET SHIORI/3.0\r\nCharset: UTF-8\r\nSender: SSP\r\nSecurityLevel: local\r\nID: %s\r\nReference0: %s\r\n\r\n", id, ref0);
	n = strlen(req);
	h = (char *)malloc(n + 1);
	if (h == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(2);
	}
	memcpy(h, req, n + 1);
	len = (long)n;

	/* multi_request() takes ownership of the request buffer and frees it, even for an unknown id */
	res = p_multi_request(mid, h, &len);

	if (expected == NULL) {
		if (res == NULL) {
			printf("OK  multi %ld %s: NULL\n", mid, id);
		}
		else {
			printf("NG  multi %ld %s: expected NULL\n", mid, id);
			failures++;
			free(res);
		}
		return;
	}
	if (res == NULL) {
		printf("NG  multi %ld %s: request returned NULL (expected \"%s\")\n", mid, id, expected);
		failures++;
		return;
	}
	if (strstr(res, "SHIORI/3.0 200 OK") != NULL && strstr(res, expected) != NULL) {
		printf("OK  multi %ld %s: \"%s\"\n", mid, id, expected);
	}
	else {
		printf("NG  multi %ld %s: expected \"%s\" in response:\n%.*s\n", mid, id, expected, (int)len, res);
		failures++;
	}
	free(res);
}

/* multi_loadu() takes ownership of the buffer and frees it */
static long multi_load_path(multi_load_fn fn, const char *dir)
{
	size_t n = strlen(dir);
	char *path = (char *)malloc(n + 2);
	if (path == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(2);
	}
	memcpy(path, dir, n);
	if (n == 0 || path[n - 1] != '/') {
		path[n++] = '/';
	}
	path[n] = '\0';
	return fn(path, (long)n);
}

int main(int argc, char **argv)
{
	void *lib;
	load_fn p_load;
	unload_fn p_unload;
	multi_load_fn p_multi_loadu;
	multi_unload_fn p_multi_unload;
	long ma, mb;
	size_t n;
	char *path;

	if (argc != 3) {
		fprintf(stderr, "usage: %s <library> <ghost directory>\n", argv[0]);
		return 2;
	}

	lib = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
	if (lib == NULL) {
		fprintf(stderr, "dlopen failed: %s\n", dlerror());
		return 1;
	}
	printf("OK  dlopen: %s\n", argv[1]);

	p_load = (load_fn)dlsym(lib, "load");
	p_unload = (unload_fn)dlsym(lib, "unload");
	p_request = (request_fn)dlsym(lib, "request");
	if (p_load == NULL || p_unload == NULL || p_request == NULL) {
		fprintf(stderr, "dlsym failed: %s\n", dlerror());
		return 1;
	}

	/* load() takes ownership of the buffer and frees it. The path ends with '/' */
	n = strlen(argv[2]);
	path = (char *)malloc(n + 2);
	if (path == NULL) {
		fprintf(stderr, "out of memory\n");
		return 2;
	}
	memcpy(path, argv[2], n);
	if (n == 0 || path[n - 1] != '/') {
		path[n++] = '/';
	}
	path[n] = '\0';
	if (!p_load(path, (long)n)) {
		fprintf(stderr, "load failed\n");
		return 1;
	}
	printf("OK  load: %s\n", argv[2]);

	check("OnHello", "hello world");
	check("OnVar", "x is \xef\xbc\x95");
	check("OnReplace", "heLLo");
	check("OnCalc", "\\07\\e");
	check("OnRegex", "abc-def");
	check("OnRegexSub", "abc#def");

	/* multi-instance entries: two more instances of the same ghost, next to the one loaded above.
	   Satori prints digits in full width: \xef\xbc\x91 is "1" and \xef\xbc\x92 is "2" */
	p_multi_loadu = (multi_load_fn)dlsym(lib, "multi_loadu");
	p_multi_unload = (multi_unload_fn)dlsym(lib, "multi_unload");
	p_multi_request = (multi_request_fn)dlsym(lib, "multi_request");
	if (p_multi_loadu == NULL || p_multi_unload == NULL || p_multi_request == NULL) {
		fprintf(stderr, "dlsym (multi_*) failed: %s\n", dlerror());
		return 1;
	}
	ma = multi_load_path(p_multi_loadu, argv[2]);
	mb = multi_load_path(p_multi_loadu, argv[2]);
	if (ma > 0 && mb > 0 && ma != mb) {
		printf("OK  multi_loadu: ids %ld %ld\n", ma, mb);
	}
	else {
		printf("NG  multi_loadu: ids %ld %ld\n", ma, mb);
		failures++;
	}
	if (ma > 0 && mb > 0) {
		check_multi(ma, "OnMultiSet", "1", "set");
		check_multi(mb, "OnMultiSet", "2", "set");
		check("OnMultiSet", "set");
		check_multi(ma, "OnMultiGet", "-", "mv is \xef\xbc\x91");
		check_multi(mb, "OnMultiGet", "-", "mv is \xef\xbc\x92");
		/* the instance loaded with load() got Reference0 "master" and is not touched by the others */
		check("OnMultiGet", "mv is master");
		check_multi(ma, "OnRegexSub", "-", "abc#def");

		if (p_multi_unload(ma)) {
			printf("OK  multi_unload\n");
		}
		else {
			printf("NG  multi_unload\n");
			failures++;
		}
		check_multi(ma, "OnMultiGet", "-", NULL);
		check_multi(mb, "OnMultiGet", "-", "mv is \xef\xbc\x92");
		check_multi(mb, "OnRegexSub", "-", "abc#def");
		check("OnHello", "hello world");
		if (!p_multi_unload(ma) && !p_multi_unload(0) && !p_multi_unload(9999)) {
			printf("OK  multi_unload: unknown ids fail\n");
		}
		else {
			printf("NG  multi_unload: unknown ids must fail\n");
			failures++;
		}
		check_multi(9999, "OnMultiGet", "-", NULL);
		p_multi_unload(mb);
	}

	p_unload();
	printf("OK  unload\n");

	if (dlclose(lib) != 0) {
		fprintf(stderr, "dlclose failed: %s\n", dlerror());
		return 1;
	}
	printf("OK  dlclose\n");

	if (failures != 0) {
		printf("%d check(s) failed\n", failures);
		return 1;
	}
	printf("smoke test passed\n");
	return 0;
}
