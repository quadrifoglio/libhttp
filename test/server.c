#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <libmill.h>

#include "libhttp.h"

coroutine void client(tcpsock s) {
	char* data = malloc(1);
	size_t len = 1;
	size_t cur = 0;
	i64 dl = now() + 2000;

	do {
		u8 buf[256];
		size_t received = tcprecvuntil(s, buf, 256, "\r", 1, dl);
		if(received == 0) {
			break;
		}
		if(errno == ECONNRESET) {
			goto cleanup;
		}

		data = realloc(data, len + received);
		memcpy(data + cur, buf, received);

		cur += received;
		len += received;
		dl = now() + 1;
	} while(1);

	http_request_t req = {0};
	size_t np = http_request_parse(data, len, &req);
	if(np != len) {
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
			puts("Can't fork !");
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
