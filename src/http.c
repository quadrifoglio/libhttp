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

ssize_t httpu_strlen_delim(const char* src, size_t len, const char* delims, size_t delimc) {
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

size_t httpu_substr_delim(char** dst, const char* src, size_t len, const char* delims, size_t delimc) {
	// TODO: Don't allocate that much memory
	*dst = malloc(len);
	if(!(*dst)) {
		errno = ENOMEM;
		return 0;
	}

	size_t i = 0;
	while(i < len) {
		for(size_t j = 0; j < delimc; ++j) {
			if(*src == delims[j]) {
				goto endloop;
			}
		}

		*(*dst + i) = *src;
		++src;
		++i;
	}

endloop:
	if(i < len) {
		*(*dst + i) = 0;
	}

	return i;
}

const char* httpu_status_str(int status) {
	switch(status) {
		case 100:
			return "Continue";
		case 101:
			return "Switching Protocols";
		case 102:
			return "Processing";
		case 200:
			return "OK";
		case 201:
			return "Created";
		case 202:
			return "Accepted";
		case 203:
			return "Non-Authoritative Information";
		case 204:
			return "No Content";
		case 205:
			return "Reset Content";
		case 206:
			return "Partial Content";
		case 207:
			return "Multi-Status";
		case 210:
			return "Content Different";
		case 226:
			return "IM Used";
		case 300:
			return "Multiple Choices";
		case 301:
			return "Moved Permanently";
		case 302:
			return "Moved Temporarily";
		case 303:
			return "See Other";
		case 304:
			return "Not Modified";
		case 305:
			return "Use Proxy";
		case 306:
			return "Reserved";
		case 307:
			return "Temporary Redirect";
		case 308:
			return "Permanent Redirect";
		case 310:
			return "Too many Redirects";
		case 400:
			return "Bad Request";
		case 401:
			return "Unauthorized";
		case 402:
			return "Payment Required";
		case 403:
			return "Forbidden";
		case 404:
			return "Not Found";
		case 405:
			return "Method Not Allowed";
		case 406:
			return "Not Acceptable";
		case 407:
			return "Proxy Authentication Required";
		case 408:
			return "Request Time-out";
		case 409:
			return "Conflict";
		case 410:
			return "Gone";
		case 411:
			return "Length Required";
		case 412:
			return "Precondition Failed";
		case 413:
			return "Request Entity Too Large";
		case 414:
			return "Request-URI Too Long";
		case 415:
			return "Unsupported Media Type";
		case 416:
			return "Requested range unsatisfiable";
		case 417:
			return "Expectation failed";
		case 418:
			return "I’m a teapot";
		case 421:
			return "Bad mapping";
		case 422:
			return "Unprocessable entity";
		case 423:
			return "Locked";
		case 424:
			return "Method failure";
		case 425:
			return "Unordered Collection";
		case 426:
			return "Upgrade Required";
		case 428:
			return "Precondition Required";
		case 429:
			return "Too Many Requests";
		case 431:
			return "Request Header Fields Too Large";
		case 449:
			return "Retry With";
		case 450:
			return "Blocked by Windows Parental Controls";
		case 451:
			return "Unavailable For Legal Reasons";
		case 456:
			return "Unrecoverable Error";
		case 500:
			return "Internal Server Error";
		case 501:
			return "Not Implemented";
		case 502:
			return "Bad Gateway";
		case 503:
			return "Service Unavailable";
		case 504:
			return "Gateway Time-out";
		case 505:
			return "HTTP Version not supported";
		case 506:
			return "Variant also negociate";
		case 507:
			return "Insufficient storage";
		case 508:
			return "Loop detected";
		case 509:
			return "Bandwidth Limit Exceeded";
		case 510:
			return "Not extended";
		case 511:
			return "Network authentication required";
		case 520:
			return "Web server is returning an unknown error";
		default:
			return "";
	}
}
