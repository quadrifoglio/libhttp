# libhttp

Basic HTTP server library for the C programming language

Work in progress !

## Dependencies

This project relies on [libmill](https://github.com/sustrik/libmill) to provide concurrency functionality

## Installation

Just copy include/libhttp.h and src/http.c into your project.

## Usage

```c
void onRequest(http_request_t* request, http_response_t* response) {
	printf("HTTP/%d.%d %s request to %s\n", request->vmaj, request->vmin, request->method, request->uri);

	for(size_t i = 0; i < request->headers.count; ++i) {
		printf("Header %s: %s\n", request->headers.names[i], request->headers.values[i]);
	}
}

int main(int argc, char** argv) {
	http_server_t server;
	server.onRequest = &onRequest;

	if(!http_listen(server, "127.0.0.1", 8000, 1)) {
		perror("Can not start HTTP server: ");
	}

	return 0;
}
```
