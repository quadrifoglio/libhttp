#include "libhttp.h"

#include <string.h>
#include <stdio.h>

size_t http_request_parse(char* buf, size_t len, http_request_t* target) {
	size_t n = 0;

	char* str = calloc(1, len + 1);
	memcpy(str, buf, len);

	char* tok = strtok(str, "\n");
	n = http_request_parse_head(tok, target);
	if(n != strlen(tok)) {
		goto cleanup;
	}

	n++; // New line character eaten by strtok

	tok = strtok(0, "\n");
	if(!tok) { // No headers or body
		goto cleanup;
	}

	target->headers = 0;
	target->headerCount = 0;

	while(tok) {
		if(*tok == '\r') { // Headers parsed, begining of body
			break;
		}

		http_header_t hh;
		size_t nn = http_parse_header(tok, &hh);
		if(nn != strlen(tok)) {
			goto cleanup;
		}

		target->headers = realloc(target->headers, (++target->headerCount) * sizeof(http_header_t));
		target->headers[target->headerCount - 1] = hh;

		n += nn + 1; // +1: \n eaten by strtok

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

size_t http_request_parse_head(char* line, http_request_t* target) {
	size_t n = 0;
	size_t l = strlen(line);

	char* s = 0;
	if((s = strstr(line, " ")) != 0) {
		target->method = calloc(1, (s - line) + 1);
		memcpy(target->method, line, s - line);

		n += (s - line) + 1;
		s++;
	}
	else {
		return n;
	}

	char* r = 0;
	if((r = strstr(s, " ")) != 0) {
		target->url = calloc(1, (r - s) + 1);
		memcpy(target->url, s, r - s);

		n += (r - s) + 1;
	}
	else {
		return n;
	}

	if((s = strstr(s, "HTTP/")) != 0 && strlen(s) >= 8) {
		target->protoMajor = s[5] - '0';
		target->protoMinor = s[7] - '0';

		n += 8;
	}
	else {
		return n;
	}

	while(n != l) {
		if(line[n] == '\r' || line[n] == '\n') {
			n++;
		}
		else {
			return n;
		}
	}

	return n;
}

size_t http_response_parse(char* buf, size_t len, http_response_t* target) {
	size_t n = 0;

	char* str = calloc(1, len + 1);
	memcpy(str, buf, len);

	char* tok = strtok(str, "\n");
	n = http_response_parse_head(tok, target);
	if(n != strlen(tok)) {
		goto cleanup;
	}

	n++; // New line eaten by strtok

	tok = strtok(0, "\n");
	if(!tok) { // No headers or body
		goto cleanup;
	}

	while(tok) {
		if(*tok == '\r') { // Headers parsed, begining of body
			break;
		}

		http_header_t hh;
		size_t nn = http_parse_header(tok, &hh);
		if(nn != strlen(tok)) {
			goto cleanup;
		}

		target->headers = realloc(target->headers, (++target->headerCount) * sizeof(http_header_t));
		target->headers[target->headerCount - 1] = hh;

		n += nn + 1; // +1: \n eaten by strtok

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

size_t http_response_parse_head(char* line, http_response_t* target) {
	size_t n = 0;
	size_t l = strlen(line);

	char* s = 0;
	if(l > 8 && (s = strstr(line, "HTTP/")) != 0 && s == line) {
		target->protoMajor = line[5] - '0';
		target->protoMinor = line[7] - '0';

		n += 8;
	}
	else {
		return n;
	}

	if((s = strstr(s + 8, " ")) != 0) {
		char* end;
		if((end = strstr(s, "\r")) == 0) {
			end = line + l;
			n++;
		}

		target->status = calloc(1, end - (s + 1));
		memcpy(target->status, s + 1, end - (s + 1));

		n += strlen(s);
	}
	else {
		return n;
	}

	while(n != l) {
		if(line[n] == '\r' || line[n] == '\n') {
			n++;
		}
		else {
			return n;
		}
	}

	return n;
}

size_t http_parse_header(char* line, http_header_t* hh) {
	size_t n = 0;
	size_t l = strlen(line);

	char* s = 0;
	if((s = strstr(line, ": ")) != 0) {
		hh->name = calloc(1, s - line);
		memcpy(hh->name, line, s - line);

		char* end;
		if((end = strstr(s, "\r")) == 0) {
			end = line + l;
		}
		else {
			n++;
		}

		hh->value = calloc(1, end - (s + 2));
		memcpy(hh->value, s + 2, end - (s + 2));

		n += strlen(hh->name) + strlen(hh->value) + 2;
	}
	else {
		return n;
	}

	while(n != l) {
		if(line[n] == '\r' || line[n] == '\n') {
			n++;
		}
		else {
			return n;
		}
	}

	return n;
}

void http_request_dispose(http_request_t* req) {
	if(req->method) {
		free(req->method);
	}
	if(req->url) {
		free(req->url);
	}
	if(req->headers) {
		for(int i = 0; i < (int)req->headerCount; ++i) {
			free(req->headers[i].name);
			free(req->headers[i].value);
		}

		free(req->headers);
	}
	if(req->body) {
		free(req->body);
	}
}

void http_response_dispose(http_response_t* res) {
	if(res->status) {
		free(res->status);
	}
	if(res->headers) {
		for(int i = 0; i < (int)res->headerCount; ++i) {
			free(res->headers[i].name);
			free(res->headers[i].value);
		}

		free(res->headers);
	}
	if(res->body) {
		free(res->body);
	}
}
