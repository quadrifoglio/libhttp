#include "libhttp.h"

#include <stdio.h>
#include <string.h>

void onRequest(http_request_t* request, http_response_t* response) {
	printf("HTTP/%d.%d %s request to %s\n", request->vmaj, request->vmin, request->method, request->uri);

	for(size_t i = 0; i < request->headers.count; ++i) {
		printf("Header %s: %s\n", request->headers.names[i], request->headers.values[i]);
	}

	http_header_add(&response->headers, strdup("Content-Length"), strdup("13"));
	response->body = (u8*)"Hello world !";
	response->bodylen = 13;
}

int main(int argc, char** argv) {
	http_server_t server;
	server.onRequest = &onRequest;

	if(!http_listen(server, "127.0.0.1", 8000, 1)) {
		perror("Can not start HTTP server: ");
	}

	return 0;
}
