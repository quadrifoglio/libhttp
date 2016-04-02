#pragma once

#include <stdint.h>
#include <stdlib.h>

#define false 0
#define true 1

#define HTTP_MAX_LINE_LENGTH  8000
#define HTTP_MAX_REQUEST_SIZE 100000000 // 100MB

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

typedef enum {
	HTTP_ERR_NONE,
	HTTP_ERR_ALLOC,
	HTTP_ERR_SYNTAX,
	HTTP_ERR_TOO_LARGE
} http_error_t;

typedef struct {
	int major;
	int minor;
} http_version_t;

typedef struct {
	size_t count;
	char** parts;
} http_path_t;

typedef struct {
	size_t count;

	char** names;
	char** values;
} http_headers_t;

typedef struct {
	bool chunked;

	void* data;
	size_t len;
} http_body_t;

typedef struct {
	http_version_t version;

	char* method;
	char* uri;

	http_headers_t headers;
	http_body_t body;
} http_request_t;

typedef struct {
	int vmaj, vmin;

	int status;

	http_headers_t headers;
	http_body_t body;
} http_response_t;

typedef void (*http_request_cb)(http_request_t*, http_response_t*);
typedef void (*http_error_cb)(http_error_t, int sockfd);

const char*   http_error_str(http_error_t err);

http_path_t   http_path_parse(const char* path);
void          http_path_dispose(http_path_t* path);

http_error_t  http_header_parse(const char* line, char** name, char** value);
void          http_header_add(http_headers_t* hh, char* name, char* value);
char*         http_header_get(http_headers_t* h, const char* name);

http_error_t  http_request_parse(http_request_t* req, const char* line);
void          http_request_dispose(http_request_t* req);

void          http_response_init(http_response_t* res, int status);
http_error_t  http_response_format(http_response_t* res, char** buf);
void          http_response_dispose(http_response_t* res);

void          http_body_append(http_body_t* body, void* data, size_t len);

void          http_client_loop(int sockfd, http_request_cb, http_error_cb);

// UTILS

size_t        httpu_recv_until(int fd, void *buf, size_t len, const char *delims, size_t delimc);
ssize_t       httpu_strlen_delim(const char* src, size_t len, const char* delims, size_t delimc);
size_t        httpu_substr_delim(char** dst, const char* src, size_t len, const char* delims, size_t delimc);
const char*   httpu_status_str(int status);
