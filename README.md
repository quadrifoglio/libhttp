# libhttp

Basic HTTP parsing for C

Work in progress !

## Installation

Just copy include/libhttp.h and src/http.c into your project.

## Usage

```c
char* data = "GET / HTTP/1.1\r\n"; // Your HTTP request
http_request_t req = {0};

size_t n = http_request_parse(data, strlen(data), &req);
if(n != strlen(data)) {
	// Parsing error, bad request
}

printf("%s request on %s\n", req.method, req.url); // GET request on /
http_request_dispose(&req);

```
