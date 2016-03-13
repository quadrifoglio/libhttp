#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <libmill.h>

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

typedef void(*http_request_cb)(http_request_t*);

typedef struct {
	http_request_cb onRequest;
} http_server_t;

bool http_request_parse(http_request_t* req, char* line);
void http_request_dispose(http_request_t* req);

bool http_header_parse(char* line, char** name, char** value);
void http_header_add(http_headers_t* hh, char* name, char* value);

bool http_listen(http_server_t, char* addr, int port, int backlog);

// UTILS

ssize_t httpu_strlen_delim(char* src, size_t len, char* delims, size_t delimc);
size_t httpu_substr_delim(char** dst, char* src, size_t len, char* delims, size_t delimc);
