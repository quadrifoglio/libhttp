#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <libmill.h>

#include "libhttp.h"

coroutine void client(tcpsock s) {
	char buf[1024];
	size_t n = tcprecv(s, buf, sizeof(buf), now() + 1);
	if(n == 0 || n == 1024) { // Don't handle large requests
		goto cleanup;
	}
	if(errno == ECONNRESET) {
		goto cleanup;
	}

	http_request_t req = {0};
	size_t np = http_request_parse(buf, n, &req);
	if(np != n) {
		printf("Parsing failed\n");
		http_request_dispose(&req);
		goto cleanup;
	}

	printf("%s request on %s\n", req.method, req.url);

	for(int i = 0; i < (int)req.headerCount; ++i) {
		printf("%s: %s\n", req.headers[i].name, req.headers[i].value);
	}

	printf("\n\n");

	char* resp = "HTTP/1.1 200 OK\r\nContent-Length: 11\r\n\r\nHello world";
	tcpsend(s, resp, strlen(resp), -1);
	if(errno == ECONNRESET) {
		goto cleanup;
	}

	tcpflush(s, -1);
	if(errno == ECONNRESET) {
		goto cleanup;
	}

	http_request_dispose(&req);

cleanup:
	tcpclose(s);
}

int main(int argc, char** argv) {
	int port = 8000;
	int nproc = 1;

	if(argc > 1) {
		port = atoi(argv[1]);
	}
	if(argc > 2) {
		nproc = atoi(argv[2]);
	}

	ipaddr addr = iplocal(NULL, port, 0);
	tcpsock l = tcplisten(addr, 1);

	for (int i = 0; i < nproc - 1; ++i) {
		pid_t pid = mfork();

		if(pid < 0) {
			printf("Can't fork !");
			return 1;
		}

		if(pid > 0) {
			break;
		}
	}

	while(1) {
		tcpsock s = tcpaccept(l, -1);
		if(!s) {
			continue;
		}

		go(client(s));
	}

	return 0;
}
