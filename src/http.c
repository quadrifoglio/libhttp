#include "libhttp.h"

#include <string.h>
#include <stdio.h>

size_t http_request_parse(char* buf, size_t len, http_request_t* target) {
	size_t n = 0;

	char* str = calloc(1, len + 1);
	memcpy(str, buf, len);

	char* tok = strtok(str, "\n");
	char* s = 0;
	if((s = strstr(tok, " ")) != 0) {
		target->method = calloc(1, (s - tok) + 1);
		memcpy(target->method, tok, s - tok);

		n += (s - tok) + 1;
		s++;
	}
	else {
		goto cleanup;
	}

	char* r = 0;
	if((r = strstr(s, " ")) != 0) {
		target->url = calloc(1, (r - s) + 1);
		memcpy(target->url, s, r - s);

		n += (r - s) + 2;
	}
	else {
		goto cleanup;
	}

	if((s = strstr(s, "HTTP/")) != 0 && strlen(s) == 8 + 1) {
		target->protoMajor = s[5] - '0';
		target->protoMinor = s[7] - '0';

		n += 9;
	}
	else {
		goto cleanup;
	}

	tok = strtok(0, "\n");
	if(!tok) { // No headers or body
		goto cleanup;
	}

	target->headers.base = 0;
	target->headers.count = 0;

	int hi = 0;
	while(tok) {
		if(*tok == '\r') { // Headers parsed, begining of body
			break;
		}

		char* s = 0;
		if((s = strstr(tok, ": ")) != 0) {
			char* hn = calloc(1, s - tok);
			memcpy(hn, tok, s - tok);

			char* hv = calloc(1, s - tok);
			memcpy(hv, s + 2, tok + (strlen(tok)-1) - (s + 2));

			target->headers.base = realloc(target->headers.base, (++target->headers.count) * 2 * sizeof(char*));
			target->headers.base[hi++] = hn;
			target->headers.base[hi++] = hv;
		}

		n += strlen(tok) + 1;
		tok = strtok(0, "\n");
	}

	target->bodylen = len - (n + 2);
	target->body = calloc(1, target->bodylen + 1);
	memcpy(target->body, buf + n + 2, target->bodylen);
	n += target->bodylen + 2;

cleanup:
	free(str);
	return n;
}

size_t http_response_parse(char* buf, size_t len, http_response_t* target) {
	return 0;
}

char* http_get_header_name(http_headers_t h, int index) {
	return h.base[2 * index];
}

char* http_get_header_value(http_headers_t h, int index) {
	return h.base[2 * index + 1];
}

void http_request_dispose(http_request_t* req) {
	if(req->method) {
		free(req->method);
	}
	if(req->url) {
		free(req->url);
	}
	if(req->headers.base) {
		for(int i = 0; i < (int)req->headers.count * 2; ++i) {
			free(req->headers.base[i]);
		}

		free(req->headers.base);
	}
	if(req->body) {
		free(req->body);
	}
}

void http_response_dispose(http_response_t* res) {
	if(res->status) {
		free(res->status);
	}
	if(res->body) {
		free(res->body);
	}
}
