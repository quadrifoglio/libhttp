#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void fatal(const char* desc) {
	perror(desc);
	exit(1);
}

void handle(int sock, http_request_t* request) {
	http_request_dispose(request);
}

int main(int argc, char** argv) {
	int sock = socket(AF_INET, SOCK_STREAM, 6);
	if(sock < 0) {
		fatal("socket");
	}

	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
		fatal("setsockopt SO_REUSEADDR");
	}

	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(8000);

	if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		fatal("bind");
	}

	if(listen(sock, 5) < 0) {
		fatal("listen");
	}

	while(1) {
		struct sockaddr_in addr = {0};
		socklen_t len = sizeof(struct sockaddr_in);

		int client = accept(sock, (struct sockaddr*)&addr, &len);
		if(client < 0) {
			perror("accept");
			continue;
		}

		if(http_handle(handle, client) < 0) {
			puts("error while handling http request");
			continue;
		}
	}

	return 0;
}
