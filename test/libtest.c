#include "libhttp.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

void test_request(void) {
	static char* data = "POST /test HTTP/1.1\r\n"
						"Connection: keep-alive\r\n"
						"Accept: text/plain\r\n"
						"Content-Length: 19\r\n\r\n"
						"key1=val1&key2=val2";

	http_request_t req = {0};
	size_t n = http_request_parse(data, strlen(data), &req);

	assert(n == strlen(data));
	assert(req.protoMajor == 1);
	assert(req.protoMinor == 1);
	assert(req.method && strcmp(req.method, "POST") == 0);
	assert(req.url && strcmp(req.url, "/test") == 0);
	assert(req.headers && req.headerCount == 3);
	assert(strcmp(req.headers[0].name, "Connection") == 0);
	assert(strcmp(req.headers[0].value, "keep-alive") == 0);
	assert(strcmp(req.headers[1].name, "Accept") == 0);
	assert(strcmp(req.headers[1].value, "text/plain") == 0);
	assert(strcmp(req.headers[2].name, "Content-Length") == 0);
	assert(strcmp(req.headers[2].value, "19") == 0);
	assert(req.body && strcmp((char*)req.body, "key1=val1&key2=val2") == 0);

	http_request_dispose(&req);
}

void test_response(void) {
	static char* data =	"HTTP/1.1 200 OK\r\n"
						"Connection: keep-alive\r\n"
						"Content-Type: text/plain\r\n"
						"Content-Length: 11\r\n\r\n"
						"Hello world";

	http_response_t res = {0};
	size_t n = http_response_parse(data, strlen(data), &res);

	assert(n == strlen(data));
	assert(res.protoMajor == 1);
	assert(res.protoMinor == 1);
	assert(res.status && strcmp(res.status, "200 OK") == 0);
	assert(res.headers && res.headerCount == 3);
	assert(strcmp(res.headers[0].name, "Connection") == 0);
	assert(strcmp(res.headers[0].value, "keep-alive") == 0);
	assert(strcmp(res.headers[1].name, "Content-Type") == 0);
	assert(strcmp(res.headers[1].value, "text/plain") == 0);
	assert(strcmp(res.headers[2].name, "Content-Length") == 0);
	assert(strcmp(res.headers[2].value, "11") == 0);
	assert(res.body && strcmp((char*)res.body, "Hello world") == 0);

	http_response_dispose(&res);
}

int main(void) {
	test_request();
	test_response();

	return 0;
}
