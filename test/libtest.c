#include "libhttp.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

void test_entire_request(void) {
	printf("Full valid request parsing... ");

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

	puts("OK !");
}

void test_steps_request(void) {
	printf("Step by step valid request parsing... ");

	static char* t1 = "GET /test HTTP/1.1\r\n";
	static char* t2 = "Connection: keep-alive\r\n";
	static char* t3 = "Accept: application/json\r\n\r\n";
	static char* t4 = "Cookie: mdr=lol";

	http_request_t req = {0};
	size_t n = http_request_parse_head(t1, &req);

	assert(n == strlen(t1));
	assert(req.protoMajor == 1);
	assert(req.protoMinor == 1);
	assert(req.method && strcmp(req.method, "GET") == 0);
	assert(req.url && strcmp(req.url, "/test") == 0);

	http_header_t h = {0};
	size_t nn = http_parse_header(t2, &h);

	assert(nn == strlen(t2));
	assert(strcmp(h.name, "Connection") == 0);
	assert(strcmp(h.value, "keep-alive") == 0);

	http_header_t hh = {0};
	size_t nnn = http_parse_header(t3, &hh);

	assert(nnn == strlen(t3));
	assert(strcmp(hh.name, "Accept") == 0);
	assert(strcmp(hh.value, "application/json") == 0);

	http_header_t hhh = {0};
	size_t nnnn = http_parse_header(t4, &hhh);

	assert(nnnn == strlen(t4));
	assert(strcmp(hhh.name, "Cookie") == 0);
	assert(strcmp(hhh.value, "mdr=lol") == 0);

	puts("OK !");
}

void test_entire_response(void) {
	printf("Full valid response parsing... ");

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
	assert(res.headers[0].name && strcmp(res.headers[0].name, "Connection") == 0);
	assert(res.headers[0].value && strcmp(res.headers[0].value, "keep-alive") == 0);
	assert(res.headers[1].name && strcmp(res.headers[1].name, "Content-Type") == 0);
	assert(res.headers[1].value && strcmp(res.headers[1].value, "text/plain") == 0);
	assert(res.headers[2].name && strcmp(res.headers[2].name, "Content-Length") == 0);
	assert(res.headers[2].value && strcmp(res.headers[2].value, "11") == 0);
	assert(res.body && strcmp((char*)res.body, "Hello world") == 0);

	http_response_dispose(&res);

	puts("OK !");
}

void test_steps_response(void) {
	printf("Step by step valid response parsing... ");

	static char* t1 = "HTTP/1.1 200 OK\r\n";
	static char* t2 = "Connection: keep-alive\r\n";
	static char* t3 = "Content-Type: application/json\r\n\r\n";
	static char* t4 = "Set-Cookie: mdr=tg";

	http_response_t res = {0};
	size_t n = http_response_parse_head(t1, &res);

	assert(n == strlen(t1));
	assert(res.protoMajor == 1);
	assert(res.protoMinor == 1);
	assert(res.status && strcmp(res.status, "200 OK") == 0);

	http_header_t h = {0};
	size_t nn = http_parse_header(t2, &h);

	assert(nn == strlen(t2));
	assert(h.name && strcmp(h.name, "Connection") == 0);
	assert(h.value && strcmp(h.value, "keep-alive") == 0);

	http_header_t hh = {0};
	size_t nnn = http_parse_header(t3, &hh);

	assert(nnn == strlen(t3));
	assert(hh.name && strcmp(hh.name, "Content-Type") == 0);
	assert(hh.value && strcmp(hh.value, "application/json") == 0);

	http_header_t hhh = {0};
	size_t nnnn = http_parse_header(t4, &hhh);

	assert(nnnn == strlen(t4));
	assert(hhh.name && strcmp(hhh.name, "Set-Cookie") == 0);
	assert(hhh.value && strcmp(hhh.value, "mdr=tg") == 0);

	puts("OK !");
}

int main(void) {
	test_entire_request();
	test_steps_request();

	test_entire_response();
	test_steps_response();

	return 0;
}
