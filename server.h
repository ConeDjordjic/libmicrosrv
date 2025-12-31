#ifndef SERVER_H
#define SERVER_H

#include "request.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080

typedef int (*EndpointHandler)(int client_fd, HttpRequest *req, void *ctx);

typedef struct {
  HttpMethod method;
  char *path;
  size_t len;
  EndpointHandler handler;
  void *ctx;
} Endpoint;

typedef struct {
  Endpoint *items;
  size_t len;
  size_t cap;
} EndpointVec;

typedef struct ServerConfig {
  uint16_t port;
  EndpointVec endpoints;
} ServerConfig;

int serve(ServerConfig *config);

void endpoint_vec_init(EndpointVec *vec);

int endpoint_vec_free(EndpointVec *vec);

int endpoint_vec_push(EndpointVec *vec, Endpoint ep);

Endpoint new_endpoint(char *path, HttpMethod method, EndpointHandler handler,
                      void *ctx);

#endif // !SERVER_H
