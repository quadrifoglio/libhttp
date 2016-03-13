#include "libhttp.h"

#include <stdio.h>

void onRequest(http_request_t* request) {
	printf("HTTP/%d.%d %s requestuest to %s\n", request->vmaj, request->vmin, request->method, request->uri);

	for(size_t i = 0; i < request->headers.count; ++i) {
		printf("Header %s: %s\n", request->headers.names[i], request->headers.values[i]);
	}
}

int main(int argc, char** argv) {
	http_server_t server;
	server.onRequest = &onRequest;

	if(!http_bind(server, "127.0.0.1", 8000, 1)) {
		perror("Can not start HTTP server: ");
	}

	return 0;
}
