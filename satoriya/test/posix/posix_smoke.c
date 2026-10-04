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

static request_fn p_request;
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

int main(int argc, char **argv)
{
	void *lib;
	load_fn p_load;
	unload_fn p_unload;
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
