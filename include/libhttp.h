#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct {
	u8 protoMajor;
	u8 protoMinor;

	char* method;
	char* url;

	u8* body;
	size_t bodylen;
} http_request_t;

typedef struct {
	u8 protoMajor;
	u8 protoMinor;

	char* status;

	u8* body;
	size_t bodylen;
} http_response_t;

size_t http_request_parse(char* buf, size_t len, http_request_t* target);
void http_request_dispose(http_request_t* req);

size_t http_response_parse(char* buf, size_t len, http_response_t* target);
void http_response_dispose(http_response_t* req);
