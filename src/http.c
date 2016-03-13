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

void http_response_init(http_response_t* res, int status) {
	res->vmaj = 1;
	res->vmin = 1;
	res->status = status;

	res->headers.count = 0;
	res->headers.names = 0;
	res->headers.values = 0;

	res->body = 0;
	res->bodylen = 0;
}

void http_response_format(http_response_t* res, char** buf) {
	const char* line = "HTTP/%d.%d %d %s\r\n";
	const char* header = "%s: %s\r\n";

	char* str = 0;

	char* lineb = malloc(100);
	if(!lineb) {
		errno = ENOMEM;
		return;
	}

	sprintf(lineb, line, res->vmaj, res->vmin, res->status, httpu_status_str(res->status));

	str = calloc(1, strlen(lineb) + 2 + 1);
	if(!str) {
		errno = ENOMEM;
		return;
	}

	memcpy(str, lineb, strlen(lineb));
	free(lineb);

	for(size_t i = 0; i < res->headers.count; ++i) {
		char* n = *(res->headers.names + i);
		char* v = *(res->headers.values + i);

		size_t hl = strlen(n) + 2 + strlen(v) + 2;
		char* headerb = malloc(hl + 1);

		if(!headerb) {
			errno = ENOMEM;
			return;
		}

		sprintf(headerb, header, n, v);

		str = realloc(str, strlen(str) + strlen(headerb) + 1);
		if(!str) {
			errno = ENOMEM;
			return;
		}

		strcat(str, headerb);
		free(headerb);
	}

	char* ends = "\r\n";
	strcat(str, ends);

	if(res->body && res->bodylen > 0) {
		char* body = malloc(res->bodylen + 1);
		memcpy(body, res->body, res->bodylen);
		body[res->bodylen] = 0;

		str = realloc(str, strlen(str) + res->bodylen + 1);
		strcat(str, body);
		free(body);
	}

	*buf = str;
}

void http_response_dispose(http_response_t* res) {
	for(size_t i = 0; i < res->headers.count; ++i) {
		free(*(res->headers.names + i));
		free(*(res->headers.values + i));
	}

	if(res->headers.names) {
		free(res->headers.names);
	}
	if(res->headers.values) {
		free(res->headers.values);
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
		http_response_t response = {0};

		http_response_init(&response, 200);

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
			serv.onRequest(&request, &response);
		}

		char* resStr = 0;
		http_response_format(&response, &resStr);

		tcpsend(s, resStr, strlen(resStr), -1);
		tcpflush(s, -1);

		free(resStr);

		http_response_dispose(&response);
		http_request_dispose(&request);
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

char* httpu_status_str(int status) {
	char* str;

	switch(status) {
		case 100:
			str = "Continue";
			break;
		case 101:
			str = "Switching Protocols";
			break;
		case 102:
			str = "Processing";
			break;
		case 200:
			str = "OK";
			break;
		case 201:
			str = "Created";
			break;
		case 202:
			str = "Accepted";
			break;
		case 203:
			str = "Non-Authoritative Information";
			break;
		case 204:
			str = "No Content";
			break;
		case 205:
			str = "Reset Content";
			break;
		case 206:
			str = "Partial Content";
			break;
		case 207:
			str = "Multi-Status";
			break;
		case 210:
			str = "Content Different";
			break;
		case 226:
			str = "IM Used";
			break;
		case 300:
			str = "Multiple Choices";
			break;
		case 301:
			str = "Moved Permanently";
			break;
		case 302:
			str = "Moved Temporarily";
			break;
		case 303:
			str = "See Other";
			break;
		case 304:
			str = "Not Modified";
			break;
		case 305:
			str = "Use Proxy";
			break;
		case 306:
			str = "Reserved";
			break;
		case 307:
			str = "Temporary Redirect";
			break;
		case 308:
			str = "Permanent Redirect";
			break;
		case 310:
			str = "Too many Redirects";
			break;
		case 400:
			str = "Bad Request";
			break;
		case 401:
			str = "Unauthorized";
			break;
		case 402:
			str = "Payment Required";
			break;
		case 403:
			str = "Forbidden";
			break;
		case 404:
			str = "Not Found";
			break;
		case 405:
			str = "Method Not Allowed";
			break;
		case 406:
			str = "Not Acceptable";
			break;
		case 407:
			str = "Proxy Authentication Required";
			break;
		case 408:
			str = "Request Time-out";
			break;
		case 409:
			str = "Conflict";
			break;
		case 410:
			str = "Gone";
			break;
		case 411:
			str = "Length Required";
			break;
		case 412:
			str = "Precondition Failed";
			break;
		case 413:
			str = "Request Entity Too Large";
			break;
		case 414:
			str = "Request-URI Too Long";
			break;
		case 415:
			str = "Unsupported Media Type";
			break;
		case 416:
			str = "Requested range unsatisfiable";
			break;
		case 417:
			str = "Expectation failed";
			break;
		case 418:
			str = "I’m a teapot";
			break;
		case 421:
			str = "Bad mapping";
			break;
		case 422:
			str = "Unprocessable entity";
			break;
		case 423:
			str = "Locked";
			break;
		case 424:
			str = "Method failure";
			break;
		case 425:
			str = "Unordered Collection";
			break;
		case 426:
			str = "Upgrade Required";
			break;
		case 428:
			str = "Precondition Required";
			break;
		case 429:
			str = "Too Many Requests";
			break;
		case 431:
			str = "Request Header Fields Too Large";
			break;
		case 449:
			str = "Retry With";
			break;
		case 450:
			str = "Blocked by Windows Parental Controls";
			break;
		case 451:
			str = "Unavailable For Legal Reasons";
			break;
		case 456:
			str = "Unrecoverable Error";
			break;
		case 500:
			str = "Internal Server Error";
			break;
		case 501:
			str = "Not Implemented";
			break;
		case 502:
			str = "Bad Gateway";
			break;
		case 503:
			str = "Service Unavailable";
			break;
		case 504:
			str = "Gateway Time-out";
			break;
		case 505:
			str = "HTTP Version not supported";
			break;
		case 506:
			str = "Variant also negociate";
			break;
		case 507:
			str = "Insufficient storage";
			break;
		case 508:
			str = "Loop detected";
			break;
		case 509:
			str = "Bandwidth Limit Exceeded";
			break;
		case 510:
			str = "Not extended";
			break;
		case 511:
			str = "Network authentication required";
			break;
		case 520:
			str = "Web server is returning an unknown error";
			break;
		default:
			str = "";
			break;
	}

	return str;
}
