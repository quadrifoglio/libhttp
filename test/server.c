#include "libhttp.h"

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <libmill.h>

static volatile bool running = true;

void onRequest(http_request_t* request, http_response_t* response) {
	printf("HTTP/%d.%d %s request to %s\n", request->vmaj, request->vmin, request->method, request->uri);

	for(size_t i = 0; i < request->headers.count; ++i) {
		printf("%s: %s\n", request->headers.names[i], request->headers.values[i]);
	}

	// TODO: Remove the need to allocate header names and values
	http_header_add(&response->headers, strdup("Content-Length"), strdup("13"));
	response->body = (u8*)"Hello world !";
	response->bodylen = 13;
}

void onError(http_request_t* request, http_response_t* response) {
	perror("http");
}

coroutine void client(int csfd) {
	http_client(csfd, &onRequest, &onError);
	shutdown(csfd, SHUT_RDWR);
	close(csfd);
}

int main(int argc, char** argv) {
	int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(!sockfd) {
		perror("socket");
		return 1;
	}

	struct sockaddr_in sa = {0};
	sa.sin_family = AF_INET;
	sa.sin_port = htons(8000);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
		perror("bind");
		return 1;
	}

	if(listen(sockfd, 1) != 0) {
		perror("listen");
		return 1;
	}

	while(running) {
		int csfd = accept(sockfd, 0, 0);
		if(!csfd) {
			perror("accept");
			continue;
		}

		go(client(csfd));
	}

	shutdown(sockfd, 2);
	close(sockfd);

	return 0;
}
