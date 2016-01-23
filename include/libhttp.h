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
	char* protocol;
	char* method;
	char* url;

	u8* body;
	size_t bodylen;
} http_request;

typedef struct {
	char* status;

	u8* body;
	size_t bodylen;
} http_response;

size_t http_request_parse(char* buf, size_t len, http_request* req);
size_t http_response_parse(char* buf, size_t len, http_request* req);
