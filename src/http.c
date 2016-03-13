#include "libhttp.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libmill.h>

bool http_get_line(tcpsock s, char** line) {
	char* buf = malloc(HTTP_MAX_LINE_LENGTH);
	size_t sz = tcprecvuntil(s, buf, HTTP_MAX_LINE_LENGTH, "\n", 1, now() + 1000);

	if(errno != 0) {
		return false;
	}

	*line = calloc(1, sz + 1);
	if(!(*line)) {
		errno = ENOMEM;
		return false;
	}

	memcpy(*line, buf, sz);
	free(buf);

	return true;
}

bool http_request_parse(http_request_t* req, char* line) {
	bool res = false;

	char* method = 0;
	size_t szm = httpu_substr_delim(&method, line, 10, " ", 1); // HTTP method must be less than 10 chars long
	if(szm == 0 || szm >= 10) {
		goto exit;
	}

	line += szm + 1;

	char* uri = 0;
	size_t szu = httpu_substr_delim(&uri, line, 2048, " ", 1); // HTTP URI must be less than 2048 chars long
	if(szu == 0 || szu >= 2048) {
		goto exit;
	}

	line += szu + 1;

	char* proto = 0;
	size_t szp = httpu_substr_delim(&proto, line, 8 + 1, "\r\n", 2);
	if(szp != 8) {
		goto exit;
	}

	res = true;
	req->vmaj = (*(line + 5)) - '0';
	req->vmin = (*(line + 7)) - '0';
	req->method = strdup(method);
	req->uri = strdup(uri);

exit:
	if(method) {
		free(method);
	}
	if(uri) {
		free(uri);
	}
	if(proto) {
		free(proto);
	}

	return res;
}

void http_request_dispose(http_request_t* req) {
	if(req->method) {
		free(req->method);
	}
	if(req->uri) {
		free(req->uri);
	}

	for(size_t i = 0; i < req->headers.count; ++i) {
		free(*(req->headers.names + i));
		free(*(req->headers.values + i));
	}

	if(req->headers.names) {
		free(req->headers.names);
	}
	if(req->headers.values) {
		free(req->headers.values);
	}
}

bool http_header_parse(char* line, char** name, char** value) {
	ssize_t n = httpu_strlen_delim(line, strlen(line), ":", 1);
	if(n <= 0) {
		return false;
	}

	*name = malloc(n); // Header name must be less than 256 chars long
	*value = 0;
	if(!(*name)) {
		errno = ENOMEM;
		return false;
	}

	size_t szn = httpu_substr_delim(name, line, n + 1, ":", 1);
	if(szn == 0 || szn >= (size_t)n + 1) {
		return false;
	}

	line += szn + 2;
	size_t len = strlen(line);

	if(len <= 0) {
		return false;
	}

	*value = calloc(1, len - 1);
	if(!(*value)) {
		errno = ENOMEM;
		return false;
	}

	memcpy(*value, line, len - 2);
	return true;
}

void http_header_add(http_headers_t* hh, char* name, char* value) {
	hh->count++;
	hh->names = realloc(hh->names, hh->count * sizeof(char*));
	hh->values = realloc(hh->values, hh->count * sizeof(char*));

	size_t i = hh->count - 1;
	*(hh->names + i) = name;
	*(hh->values + i) = value;
}

coroutine void client(http_server_t serv, tcpsock s) {
	while(true) {
		http_request_t request = {0};

		// Parsing request's first line
		char* line = 0;
		if(!http_get_line(s, &line) || !line) {
			// TODO: Report error
			break;
		}

		if(!http_request_parse(&request, line)) {
			// TODO: Report error
			break;
		}

		free(line);

		// Start parsing headers
		http_get_line(s, &line);
		while(line && strcmp(line, "\r\n")) {
			char* name;
			char* value;

			if(http_header_parse(line, &name, &value)) {
				http_header_add(&request.headers, name, value);

				free(line);
				http_get_line(s, &line);
			}
		}

		if(line) {
			free(line);
		}

		if(serv.onRequest) {
			serv.onRequest(&request);
		}

		http_request_dispose(&request);
		break;
	}
}

bool http_listen(http_server_t serv, char* addrs, int port, int backlog) {
	ipaddr addr = iplocal(addrs, port, 0);
	if(errno != 0) {
		return false;
	}

	tcpsock tcp = tcplisten(addr, backlog);
	if(errno != 0) {
		return false;
	}

	while(true) {
		tcpsock s = tcpaccept(tcp, -1);
		if(!s || errno != 0) {
			continue;
		}

		go(client(serv, s));
		break;
	}

	return true;
}

// UTILS


ssize_t httpu_strlen_delim(char* src, size_t len, char* delims, size_t delimc) {
	ssize_t i = 0;
	while(i < (ssize_t)len) {
		for(size_t j = 0; j < delimc; ++j) {
			if(*(src + i) == delims[j]) {
				goto endloop;
			}
		}

		++i;
	}

	if(i == (ssize_t)len) {
		return -1;
	}

endloop:
	return i;
}

size_t httpu_substr_delim(char** dst, char* src, size_t len, char* delims, size_t delimc) {
	// TODO: Don't allocate that much memory
	*dst = malloc(len);
	if(!(*dst)) {
		errno = ENOMEM;
		return 0;
	}

	char* c = src;

	size_t i = 0;
	while(i < len) {
		for(size_t j = 0; j < delimc; ++j) {
			if(*c == delims[j]) {
				goto endloop;
			}
		}

		*(*dst + i) = *c;
		++c;
		++i;
	}

endloop:
	if(i < len) {
		*(*dst + i) = 0;
	}

	return i;
}
