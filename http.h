#pragma once

/*
 * HTTP Request data structure
 * Represents an HTTP request
 */
struct http_request {
	char* version; // Format: HTTP/<Major>.<Minor>
	char* method;
};

typedef struct http_request http_request_t;

/*
 * HTTP handler function
 * This type of function is meant to be called when we successfully parsed
 * an incoming HTTP request
 */
typedef void (*http_handler_t)(int, http_request_t*);

/*
 * http_handle takes a incoming TCP connection, parses the HTTP request
 * and then calls the specified handler function
 * It returns 0 in case of success, -1 otherwise
 */
int http_handle(http_handler_t callback, int sockfd);

/*
 * http_request_dispose frees the resources associated to a http_request_t
 * data structure
 */
void http_request_dispose(http_request_t* request);
