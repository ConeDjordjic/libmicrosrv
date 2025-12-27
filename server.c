#include "server.h"
#include "request.h"
#include <stdio.h>
#include <string.h>

static char *cstr_dup(const char *s) {
  size_t n = strlen(s);
  char *p = malloc(n + 1);
  if (!p)
    return NULL;
  memcpy(p, s, n + 1);
  return p;
}

Endpoint new_endpoint(char *path, HttpMethod method, EndpointHandler handler) {
  Endpoint endpoint = {
      .path = path, .len = strlen(path), .method = method, .handler = handler};

  return endpoint;
}

void endpoint_vec_init(EndpointVec *vec) {
  vec->items = NULL;
  vec->len = 0;
  vec->cap = 0;
}

int endpoint_vec_free(EndpointVec *vec) {
  if (!vec)
    return -1;

  for (size_t i = 0; i < vec->len; i++) {
    free(vec->items[i].path);
  }

  free(vec->items);
  *vec = (EndpointVec){0};
  return 0;
}

int endpoint_vec_push(EndpointVec *vec, Endpoint ep) {
  if (!vec)
    return -1;

  if (vec->len == vec->cap) {
    size_t new_cap = (vec->cap == 0) ? 8 : vec->cap * 2;
    Endpoint *new_items = realloc(vec->items, new_cap * sizeof(*new_items));
    if (!new_items)
      return -1;

    vec->items = new_items;
    vec->cap = new_cap;
  }

  char *copy = cstr_dup(ep.path);
  if (!copy)
    return -1;

  vec->items[vec->len].method = ep.method;
  vec->items[vec->len].path = copy;
  vec->items[vec->len].len = strlen(copy);
  vec->items[vec->len].handler = ep.handler;

  vec->len++;

  return 0;
}

int serve(ServerConfig *config) {
  struct sockaddr_in server_addr;
  int server_fd;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket failed");
    return EXIT_FAILURE;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(config->port);

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    perror("bind failed");
    return EXIT_FAILURE;
  }

  if (listen(server_fd, 1) < 0) {
    perror("listen failed");
    return EXIT_FAILURE;
  }

  printf("[LOG] Server listening on: %d\n", config->port);

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_fd;

    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr,
                            &client_addr_len)) < 0) {
      perror("accept failed");
      endpoint_vec_free(&config->endpoints);
      return EXIT_FAILURE;
    }

    if (handle_client(&client_fd, config) < 0) {
      perror("handle_client failed");
      endpoint_vec_free(&config->endpoints);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
