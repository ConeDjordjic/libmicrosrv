#include "request.h"
#include "server.h"
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int handle_cone(int client_fd, HttpRequest *req) {
  HttpResponse resp = {0};
  char header[256];
  if ((make_http_response(&resp, header, sizeof(header), "Hello",
                          strlen("Hello"), HTTP_STATUS_OK) < 0))
    return -1;
  send_response(client_fd, &resp);
  return 0;
}

int main(int argc, char *argv[]) {

  ServerConfig config;
  config.port = 8080;

  Endpoint ep_cone = new_endpoint("/", HTTP_METHOD_POST, handle_cone);

  endpoint_vec_init(&config.endpoints);
  endpoint_vec_push(&config.endpoints, ep_cone);

  if (serve(&config) < 0) {
    perror("serve failed");
    endpoint_vec_free(&config.endpoints);
    return EXIT_FAILURE;
  }

  // UNREACHABLE
  return 0;
}
