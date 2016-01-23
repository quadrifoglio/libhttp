#include "libhttp.h"

#include <assert.h>
#include <string.h>

void test_request(void) {
	static char* data =	"GET /test HTTP/1.1\r\n"
						"Connection: keep-alive"
						"Accept: text/plain\r\n\r\n"
						"Hello world";

	http_request req;
	size_t n = http_request_parse(data, strlen(data), &req);

	assert(n == strlen(data));
}

void test_response(void) {
	static char* data =	"HTTP/1.1 200 OK\r\n"
						"Connection: keep-alive\r\n"
						"Content-Type: text/plain\r\n\r\n"
						"Hello world";

	http_response res;
	size_t n = http_response_parse(data, strlen(data), &res);

	assert(n == strlen(data));
}

int main(void) {
	test_request();

	return 0;
}
