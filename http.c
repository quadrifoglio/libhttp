#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/*
 * Internal functions
 */
int read_until(int fd, char* buf, size_t bufsize, char delim) {
	int i = 0;

	while(i < bufsize) {
		if(read(fd, buf + i, 1) < 0) {
			return -1;
		}

		char c = buf[i];
		if(c == delim) {
			if(i + 1 < bufsize) {
				buf[i + 1] = 0;
			}

			return i + 1;
		}

		i++;
	}

	errno = ERANGE;
	return -1;
}

int parse_head_line(http_request_t* req, int sockfd, char* buf, size_t len) {
	int linelen = read_until(sockfd, buf, len, '\n');
	if(linelen < 0) {
		return -1;
	}

	char* e = strchr(buf, ' ');
	if(!e) {
		return -1;
	}

	int index = (int)(e - buf);

	char* method = malloc(index);
	memcpy(method, buf, index);
	req->method = method;

	buf = e + 3; // Advance buffer to just after the '/' character

	if(strlen(buf) != 10) { // Length should be 10, 'HTTP/1.1\r\n'
		puts("loul");
		return -1;
	}

	char* version = malloc(linelen - index);
	memcpy(version, buf, 10);
	req->version = version;

	return 0;
}

int parse_headers(http_request_t* req, int sockfd, char* buf, size_t len) {
	while(1) {
		int linelen = read_until(sockfd, buf, len, '\n');
		if(linelen < 0) {
			return -1;
		}

		if(linelen == 2) { // Empty line, end of headers, start of body
			break;
		}

		char* c = strchr(buf, ':');
		if(!c) {
			return -1;
		}

		int index = (int)(c - buf);
		char* name = calloc(1, index + 1);
		memcpy(name, buf, index);

		buf = c + 2; // Advance buf to juste after the ':'
		int size = strlen(buf);
		char* value = calloc(1, size);
		memcpy(value, buf, size - 1);

		// TODO: Store headers
	}

	return 0;
}

int parse_request(http_request_t* req, int sockfd, char* buf, size_t len) {
	if(parse_head_line(req, sockfd, buf, len) < 0) {
		return -1;
	}

	if(parse_headers(req, sockfd, buf, len) < 0) {
		return -1;
	}

	// TODO: Parse headers
	// TODO: Handle body

	return 0;
}

/*
 * API functions
 */
int http_handle(http_handler_t callback, int sockfd) {
	http_request_t request = {0};

	char buf[1024];
	memset(buf, 0, 1024);

	while(1) {
		if(parse_request(&request, sockfd, buf, 2048) < 0) {
			return -1;
		}

		// TODO: Use callback
		// TODO: Close TCP connection on demand

		// TEMP
		break;
	}

	return 0;
}

void http_request_dispose(http_request_t* request) {
	free(request->version);
	free(request->method);
	free(request);
}
