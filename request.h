#ifndef REQUEST_H
#define REQUEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define BUFFER_SIZE 8192

typedef enum HttpMethod {
  HTTP_METHOD_GET = 0,
  HTTP_METHOD_POST,
  HTTP_METHOD_PUT,
  HTTP_METHOD_DELETE
} HttpMethod;

typedef struct HttpRequest {
  HttpMethod method;

  char *path;
  size_t path_len;

  char *header;
  size_t header_len;

  char *body;
  size_t body_len;
} HttpRequest;

typedef struct HttpResponse {
  const char *header;
  size_t header_len;

  const char *body;
  size_t body_len;
} HttpResponse;

typedef enum {
  HTTP_STATUS_OK = 200,
  HTTP_STATUS_BAD_REQUEST = 400,
  HTTP_STATUS_NOT_FOUND = 404,
  HTTP_STATUS_METHOD_NOT_ALLOWED = 405,
  HTTP_STATUS_INTERNAL_ERROR = 500,
} HttpStatus;

typedef struct {
  HttpStatus code;
  const char *reason;
} HttpStatusEntry;

static const HttpStatusEntry HTTP_STATUS_TABLE[] = {
    {HTTP_STATUS_OK, "OK"},
    {HTTP_STATUS_BAD_REQUEST, "BAD REQUEST"},
    {HTTP_STATUS_NOT_FOUND, "NOT FOUND"},
    {HTTP_STATUS_METHOD_NOT_ALLOWED, "METHOD NOT ALLOWED"},
    {HTTP_STATUS_INTERNAL_ERROR, "INTERNAL SERVER ERROR"},
};

const char *http_status_reason(HttpStatus code);

const char *http_method_string(HttpMethod method);

typedef struct ServerConfig ServerConfig; // forward declare

ssize_t send_all(int client_fd, const char *buf, size_t len);

ssize_t send_response(int client_fd, HttpResponse *resp);

int make_http_response(HttpResponse *out, char *header, size_t header_cap,
                       char *body, size_t body_len, HttpStatus status);

ssize_t handle_client(int *client_fd, ServerConfig *config);

int parse_method_from_raw_buf(const char *buf, size_t len, HttpMethod *out);

int parse_path_from_raw_buf(const char *buf, size_t len, char **out,
                            size_t *out_len);

int parse_header_from_raw_buf(const char *buf, size_t len, char **out,
                              size_t *out_len);

int parse_body_from_raw_buf(const char *buf, size_t len, char **out,
                            size_t *out_len);

int parse_http_request_from_raw_buf(const char *buf, size_t len,
                                    HttpRequest *out);

#endif // !REQUEST
