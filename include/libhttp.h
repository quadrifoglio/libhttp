#pragma once

#include <stdint.h>
#include <stdlib.h>

#define false 0
#define true 1

#define HTTP_MAX_LINE_LENGTH 8000

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef _Bool bool;

// HTTP API

typedef struct {
	size_t count;

	char** names;
	char** values;
} http_headers_t;

typedef struct {
	int vmaj, vmin;

	char* method;
	char* uri;

	http_headers_t headers;
} http_request_t;

typedef struct {
	int vmaj, vmin;

	int status;
	http_headers_t headers;

	u8* body;
	size_t bodylen;
} http_response_t;

typedef void (*http_request_cb)(http_request_t*, http_response_t*);
typedef void (*http_error_cb)(http_request_t*, http_response_t*);

bool http_request_parse(http_request_t* req, const char* line);
void http_request_dispose(http_request_t* req);

void http_response_init(http_response_t* res, int status);
void http_response_format(http_response_t* res, char** buf);
void http_response_dispose(http_response_t* res);

bool http_header_parse(const char* line, char** name, char** value);
void http_header_add(http_headers_t* hh, char* name, char* value);

void http_client_loop(int sockfd, http_request_cb, http_error_cb);

// UTILS

size_t httpu_recv_until(int fd, void *buf, size_t len, const char *delims, size_t delimc);
ssize_t httpu_strlen_delim(const char* src, size_t len, const char* delims, size_t delimc);
size_t httpu_substr_delim(char** dst, const char* src, size_t len, const char* delims, size_t delimc);
const char* httpu_status_str(int status);
