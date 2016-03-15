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
		printf("%s: %s\n", request->headers.names[i], request->headers.values[i]);
	}
}

void onError(http_request_t* request, http_response_t* response) {
	// Not usable yet
}

int main(int argc, char** argv) {
	int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	// Initialization of the listening socket and stuff...

	int cfd = accept(sockfd, 0, 0);
	// Accepting the incomming connection...

	http_client_loop(cfd, &onRequest, &onError); // Process the HTTP requests from that TCP connection
	// The http_client function also sends the HTTP response (modifiable inside onRequest)

	shutdown(cfd);
	close(cfd);

	shutdown(sockfd);
	close(sockfd);

	return 0;
}
```

## Contributors

- [quadrifoglio](https://github.com/quadrifoglio)
- [motet-a](https://github.com/motet-a)
