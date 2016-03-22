#include "http.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

/**
 * Returns NULL on error and sets `errno`.
 */
static char* http_get_line(int sockfd) {
	char* line = calloc(HTTP_MAX_LINE_LENGTH + 1, 1);
	if(!line) {
		errno = ENOMEM;
		return NULL;
	}

	httpu_recv_until(sockfd, line, HTTP_MAX_LINE_LENGTH, "\n", 1);
	if(errno != 0) {
		free(line);
		return NULL;
	}

	return line;
}

bool http_request_parse(http_request_t* req, const char* line) {
	bool res = false;

	char* method = 0;
	char* uri = 0;
	char* proto = 0;

	size_t szm = httpu_substr_delim(&method, line, 10, " ", 1); // HTTP method must be less than 10 chars long
	if(szm == 0 || szm >= 10) {
		goto exit;
	}

	line += szm + 1;

	size_t szu = httpu_substr_delim(&uri, line, 2048, " ", 1); // HTTP URI must be less than 2048 chars long
	if(szu == 0 || szu >= 2048) {
		goto exit;
	}

	line += szu + 1;
	size_t szp = httpu_substr_delim(&proto, line, 8 + 1, "\r\n", 2);
	if(szp != 8) {
		goto exit;
	}

	res = true;
	req->vmaj = line[5] - '0';
	req->vmin = line[7] - '0';
	req->method = strdup(method);
	req->uri = strdup(uri);

exit:
	free(method);
	free(uri);
	free(proto);

	return res;
}

void http_request_dispose(http_request_t* req) {
	free(req->method);
	free(req->uri);

	for(size_t i = 0; i < req->headers.count; ++i) {
		free(req->headers.names[i]);
		free(req->headers.values[i]);
	}

	free(req->headers.names);
	free(req->headers.values);

	if(req->body.len > 0) {
		free(req->body.data);
	}
}

void http_response_init(http_response_t* res, int status) {
	res->vmaj = 1;
	res->vmin = 1;
	res->status = status;

	res->headers.count = 0;
	res->headers.names = 0;
	res->headers.values = 0;

	res->body.data = 0;
	res->body.len = 0;
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

	str = realloc(str, strlen(str) + 3);
	const char* ends = "\r\n";
	strcat(str, ends);

	if(res->body.data && res->body.len > 0) {
		char* body = malloc(res->body.len + 1);
		memcpy(body, res->body.data, res->body.len);
		body[res->body.len] = 0;

		str = realloc(str, strlen(str) + res->body.len + 1);
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

	free(res->headers.names);
	free(res->headers.values);

	if(res->body.len > 0) {
		free(res->body.data);
	}
}

bool http_header_parse(const char* line, char** name, char** value) {
	ssize_t n = httpu_strlen_delim(line, strlen(line), ":", 1);
	if(n <= 0) {
		return false;
	}

	// Header name must be less than 256 chars long
	size_t szn = httpu_substr_delim(name, line, n + 1, ":", 1);
	if(!(*name)) {
		errno = ENOMEM;
		return false;
	}

	if(szn == 0 || szn >= (size_t)n + 1) {
		free(*name);
		return false;
	}

	line += szn + 2;
	size_t len = strlen(line);

	if(len <= 0) {
		free(*name);
		return false;
	}

	*value = calloc(1, len - 1);
	if(!(*value)) {
		free(*name);
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

char* http_header_get(http_headers_t* h, const char* name) {
	for(size_t i = 0; i < h->count; ++i) {
		if(strcmp(h->names[i], name) == 0) {
			return h->values[i];
		}
	}

	return 0;
}

http_path_t http_path_parse(const char* path) {
	http_path_t p = {0};

	if(*path != '/') {
		return p;
	}

	char* u = strdup(path);
	++u;

	int i = 0;
	char* tok = strtok(u, "/");
	while(tok) {
		p.parts = realloc(p.parts, (++p.count) * sizeof(char**));
		p.parts[i] = strdup(tok);

		tok = strtok(0, "/");
		++i;
	}

	free(u - 1);
	return p;
}

void http_path_dispose(http_path_t* path) {
	for(size_t i = 0; i < path->count; ++i) {
		free(path->parts[i]);
	}

	free(path->parts);
}

void http_client_loop(int sockfd, http_request_cb onRequest, http_error_cb onError) {
	errno = 0;

	bool close = false;
	while(!close) {
		http_request_t request = {0};
		http_response_t response = {0};

		http_response_init(&response, 200);

		// Parsing request's first line
		char* line = http_get_line(sockfd);
		if(!line) {
			if(errno == ECONNRESET) {
				return;
			}
			else if(onError) {
				onError(&request, &response);
			}

			break;
		}

		if(!http_request_parse(&request, line)) {
			free(line);
			if(onError && errno != ECONNRESET) {
				onError(&request, &response);
			}

			break;
		}

		free(line);

		// Start parsing headers
		line = http_get_line(sockfd);
		while(line && strcmp(line, "\r\n") != 0) {
			char* name;
			char* value;

			if(http_header_parse(line, &name, &value)) {
				http_header_add(&request.headers, name, value);

				free(line);
				line = http_get_line(sockfd);
			}
		}
		free(line);

		if(errno == ECONNRESET) {
			http_request_dispose(&request);
			return;
		}

		char* hv = http_header_get(&request.headers, "Transfer-Encoding");
		if(hv && strcmp(hv, "chunked") == 0) {
			response.body.chunked = true;

			while(true) {
				char* line = http_get_line(sockfd);
				if(strcmp(line, "0\r\n") == 0) {
					puts("end");
					return;
				}

				size_t sz = httpu_strlen_delim(line, strlen(line), "\n", 1);
				if(sz <= 1) {
					// TODO: Handle error
					return;
				}

				if(line[sz] != '\n' || line[sz - 1] != '\r') {
					// TODO: Handle error
					puts("Invalid \\r\\n");
					return;
				}

				line[sz - 1] = 0;
				size_t s = strtol(line, NULL, 16);
				free(line);

				void* buf = malloc(s);
				recv(sockfd, buf, s, 0);
				free(buf);
			}
		}

		if(onRequest) {
			onRequest(&request, &response);
		}

		char* resStr = 0;
		http_response_format(&response, &resStr);

		send(sockfd, resStr, strlen(resStr), 0);
		free(resStr);

		hv = http_header_get(&request.headers, "Connection");
		if(hv && strcmp(hv, "keep-alive") == 0) {
			close = false;
		}
		else {
			close = true;
		}

		http_response_dispose(&response);
		http_request_dispose(&request);
	}
}

// UTILS

/*
 * Read bytes from the socket until either the buffer is full or one of
 * the specified delimiter characters is encountered
 */
size_t httpu_recv_until(int fd, void *buf, size_t len, const char *delims, size_t delimc) {
	char *pos = (char*)buf;

	for(size_t i = 0; i != len; ++i, ++pos) {
		ssize_t res = recv(fd, pos, 1, MSG_WAITALL);

		if(res == 1) {
			for(size_t j = 0; j != delimc; ++j) {
				if(*pos == delims[j]) {
					return i + 1;
				}
			}
		}
		else if(res == 0) {
			errno = ECONNRESET;
		}

		if(errno != 0) {
			return i + res;
		}
	}

	errno = ENOBUFS;
	return len;
}

/*
 * Calculate the specified string's size until one of the specified
 * delimiter characters is encountered
 */
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

/*
 * Get a substring from the specified string, stopping when one of the delimiter
 * characters is encountered
 */
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

/*
 * Get the HTTP response message based on a status code
 */
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
