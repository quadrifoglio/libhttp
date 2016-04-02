# libhttp

Basic HTTP library for the C programming language

Work in progress !

## Installation

Just copy include/libhttp.h and src/http.c into your project.

## Usage

```c
// Print something like "GET /test/test.html" to stdout
// And respond "Hello World !"
void request(http_request_t* request, http_response_t* response) {
	http_path_t path = http_path_parse(request->uri);

	for(size_t i = 0; i < path.count; ++i) {
		printf("/%s", path.parts[i]);
	}

	if(path.count == 0) {
		printf("/");
	}

	printf("\n");
	http_path_dispose(&path);

	http_body_append(&response->body, "Hello ", 6);
	http_body_append(&response->body, "World !", 7);
}

// Log libhttp's errors
void error(http_error_t err, int sockfd) {
	printf("HTTP error: %s\n", http_error_str(err));
}

int main(int argc, char** argv) {
	int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	// Initialization of the listening socket and stuff...

	int cfd = accept(sockfd, 0, 0);
	// Accepting the incomming connection...

	http_client_loop(cfd, request, error); // Process the HTTP requests from that TCP connection
	// The http_client_loop function sends the HTTP response (modifiable inside the 'request' callback)

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
