#include "request.h"
#include "server.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

static int send_404(int client_fd, HttpRequest *req) {
  HttpResponse resp = {0};
  char header[256];
  static char msg[] = "ERROR 404 NOT FOUND\n";

  if (make_http_response(&resp, header, sizeof(header), msg, sizeof(msg) - 1,
                         HTTP_STATUS_NOT_FOUND) < 0)
    return -1;

  printf("[LOG] Handler not found for path: %s\n", req->path);
  return (send_response(client_fd, &resp) < 0) ? -1 : 0;
}

const char *http_status_reason(HttpStatus code) {
  for (size_t i = 0;
       i < sizeof(HTTP_STATUS_TABLE) / sizeof(HTTP_STATUS_TABLE[0]); i++) {
    if (HTTP_STATUS_TABLE[i].code == code)
      return HTTP_STATUS_TABLE[i].reason;
  }
  return "UNKNOWN";
}

const char *http_method_string(HttpMethod method) {
  switch (method) {
  case HTTP_METHOD_GET:
    return "GET";
  case HTTP_METHOD_POST:
    return "POST";
  case HTTP_METHOD_DELETE:
    return "DELETE";
  case HTTP_METHOD_PUT:
    return "PUT";
  }
  return "UNKNOWN";
}

ssize_t send_all(int client_fd, const char *buf, size_t len) {
  const char *p = buf;
  size_t left = len;

  while (left > 0) {
    ssize_t n = send(client_fd, p, left, 0);
    if (n < 0)
      return -1;
    p += n;
    left -= (size_t)n;
  }
  return (ssize_t)len;
}

ssize_t send_response(int client_fd, HttpResponse *resp) {
  ssize_t header_sent_len = send_all(client_fd, resp->header, resp->header_len);
  ssize_t body_sent_len = send_all(client_fd, resp->body, resp->body_len);
  return header_sent_len + body_sent_len;
}

int make_http_response(HttpResponse *out, char *header, size_t header_cap,
                       char *body, size_t body_len, HttpStatus status) {
  if (!out || !header || !body)
    return -1;

  const char *reason = http_status_reason(status);

  int header_len = snprintf(header, header_cap,
                            "HTTP/1.1 %d %s\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: %zu\r\n"
                            "Connection: close\r\n"
                            "\r\n",
                            status, reason, body_len);
  if (header_len < 0 || (size_t)header_len >= header_cap)
    return -1;

  out->header = header;
  out->header_len = (size_t)header_len;
  out->body = body;
  out->body_len = body_len;
  return 0;
}

int parse_method_from_raw_buf(const char *buf, size_t len, HttpMethod *out) {
  if (!buf || !out)
    return -1;

  const char *sp = memchr(buf, ' ', len);
  if (!sp)
    return -1;

  size_t mlen = (size_t)(sp - buf);

  if (mlen == 3 && memcmp(buf, "GET", 3) == 0) {
    *out = HTTP_METHOD_GET;
    return 0;
  }
  if (mlen == 4 && memcmp(buf, "POST", 4) == 0) {
    *out = HTTP_METHOD_POST;
    return 0;
  }
  if (mlen == 3 && memcmp(buf, "PUT", 3) == 0) {
    *out = HTTP_METHOD_PUT;
    return 0;
  }
  if (mlen == 6 && memcmp(buf, "DELETE", 6) == 0) {
    *out = HTTP_METHOD_DELETE;
    return 0;
  }

  return -1;
}

int parse_path_from_raw_buf(const char *buf, size_t len, char **out,
                            size_t *out_len) {
  if (!buf || !out)
    return -1;

  const char *sp1 = memchr(buf, ' ', len);
  if (!sp1)
    return -1;

  size_t rem = len - (size_t)(sp1 - buf) - 1;
  if (rem == 0)
    return -1;

  const char *sp2 = memchr(sp1 + 1, ' ', rem);
  if (!sp2)
    return -1;

  size_t path_len = (size_t)(sp2 - (sp1 + 1));

  size_t cap = 256;
  while (cap < path_len + 1)
    cap *= 2;

  char *new_mem = realloc(*out, cap);
  if (!new_mem)
    return -1;

  *out = new_mem;
  memcpy(*out, sp1 + 1, path_len);
  (*out)[path_len] = '\0';

  if (out_len)
    *out_len = path_len;
  return 0;
}

int parse_header_from_raw_buf(const char *buf, size_t len, char **out,
                              size_t *out_len) {
  if (!buf || !out || !out_len)
    return -1;

  size_t header_len = 0;
  for (size_t i = 0; i + 3 < len; i++) {
    if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' &&
        buf[i + 3] == '\n') {
      header_len = i + 4;
      break;
    }
  }
  if (header_len == 0)
    return -1;

  size_t cap = 1024;
  while (cap < header_len)
    cap *= 2;

  char *new_mem = realloc(*out, cap);
  if (!new_mem)
    return -1;

  *out = new_mem;
  memcpy(*out, buf, header_len);
  *out_len = header_len;
  return 0;
}

int parse_body_from_raw_buf(const char *buf, size_t len, char **out,
                            size_t *out_len) {
  if (!buf || !out || !out_len)
    return -1;

  const char *start = NULL;
  for (size_t i = 0; i + 3 < len; i++) {
    if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' &&
        buf[i + 3] == '\n') {
      start = buf + i + 4;
      break;
    }
  }
  if (!start)
    return -1;

  if (start >= buf + len) {
    *out_len = 0;
    return 0;
  }

  size_t body_len = (size_t)((buf + len) - start);

  size_t cap = 1024;
  while (cap < body_len)
    cap *= 2;

  char *new_mem = realloc(*out, cap);
  if (!new_mem)
    return -1;

  *out = new_mem;
  memcpy(*out, start, body_len);
  *out_len = body_len;
  return 0;
}

int parse_http_request_from_raw_buf(const char *buf, size_t len,
                                    HttpRequest *out) {
  if (!buf || !out)
    return -1;

  HttpMethod method;
  if (parse_method_from_raw_buf(buf, len, &method) < 0)
    return -1;

  char *path = NULL;
  size_t path_len = 0;
  if (parse_path_from_raw_buf(buf, len, &path, &path_len) < 0) {
    free(path);
    return -1;
  }

  char *header = NULL;
  size_t header_len = 0;
  if (parse_header_from_raw_buf(buf, len, &header, &header_len) < 0) {
    free(path);
    free(header);
    return -1;
  }

  char *body = NULL;
  size_t body_len = 0;
  if (parse_body_from_raw_buf(buf, len, &body, &body_len) < 0) {
    free(path);
    free(header);
    free(body);
    return -1;
  }

  out->method = method;
  out->path = path;
  out->path_len = path_len;
  out->header = header;
  out->header_len = header_len;
  out->body = body;
  out->body_len = body_len;
  return 0;
}

static ssize_t find_header_end(const char *buf, size_t len) {
  for (size_t i = 0; i + 3 < len; i++) {
    if (buf[i] == '\r' && buf[i + 1] == '\n' && buf[i + 2] == '\r' &&
        buf[i + 3] == '\n') {
      return (ssize_t)(i + 4); // index just after "\r\n\r\n"
    }
  }
  return -1;
}

static ssize_t find_content_length(const char *hdr, size_t hdr_len) {
  const char *p = hdr;
  const char *end = hdr + hdr_len;

  while (p < end) {
    const char *line_end = memchr(p, '\n', (size_t)(end - p));
    if (!line_end)
      line_end = end;

    const char *colon = memchr(p, ':', (size_t)(line_end - p));
    if (colon) {
      size_t key_len = (size_t)(colon - p);
      if (key_len == strlen("Content-Length") &&
          strncasecmp(p, "Content-Length", key_len) == 0) {

        const char *val = colon + 1;
        while (val < line_end && (*val == ' ' || *val == '\t'))
          val++;

        char tmp[32];
        size_t vlen = (size_t)(line_end - val);
        if (vlen >= sizeof(tmp))
          vlen = sizeof(tmp) - 1;
        memcpy(tmp, val, vlen);
        tmp[vlen] = '\0';

        char *endptr = NULL;
        long long n = strtoll(tmp, &endptr, 10);
        if (endptr == tmp || n < 0)
          return -1;
        return (ssize_t)n;
      }
    }

    if (line_end == end)
      break;
    p = line_end + 1;
  }
  return 0; // no Content-Length -> treat as 0
}

ssize_t handle_client(int *client_fd, ServerConfig *config) {
  int fd = *client_fd;

  size_t buf_cap = 4096;
  size_t buf_len = 0;
  char *buf = malloc(buf_cap);
  if (!buf)
    return -1;

  HttpRequest req = {0};
  int parsed_ok = 0;

  // read until headers complete
  ssize_t header_end = -1;
  for (;;) {
    if (buf_len == buf_cap) {
      size_t new_cap = buf_cap * 2;
      char *new_buf = realloc(buf, new_cap);
      if (!new_buf)
        goto cleanup;
      buf = new_buf;
      buf_cap = new_cap;
    }

    ssize_t n = recv(fd, buf + buf_len, buf_cap - buf_len, 0);
    if (n < 0) {
      perror("recv");
      goto cleanup;
    }
    if (n == 0) {
      goto cleanup; // client closed
    }

    buf_len += (size_t)n;
    header_end = find_header_end(buf, buf_len);
    if (header_end >= 0)
      break;
  }

  // parse Content-Length
  ssize_t content_len = find_content_length(buf, (size_t)header_end);
  if (content_len < 0) {
    fprintf(stderr, "[ERR] invalid Content-Length\n");
    goto cleanup;
  }

  size_t body_need = (size_t)content_len;
  size_t body_have = buf_len - (size_t)header_end;

  // read until full body
  while (body_have < body_need) {
    if (buf_len == buf_cap) {
      size_t new_cap = buf_cap * 2;
      char *new_buf = realloc(buf, new_cap);
      if (!new_buf)
        goto cleanup;
      buf = new_buf;
      buf_cap = new_cap;
    }

    ssize_t n = recv(fd, buf + buf_len, buf_cap - buf_len, 0);
    if (n < 0) {
      perror("recv");
      goto cleanup;
    }
    if (n == 0) {
      fprintf(stderr, "[ERR] connection closed before full body\n");
      goto cleanup;
    }

    buf_len += (size_t)n;
    body_have += (size_t)n;
  }

  // parse full request
  if (parse_http_request_from_raw_buf(buf, buf_len, &req) < 0) {
    fprintf(stderr, "[ERR] parse_http_request_from_raw_buf failed\n");
    goto cleanup;
  }
  parsed_ok = 1;

  printf("[LOG] Received request:\n");

  printf("  path=%s len=%zu method=%s\n", req.path, req.path_len,
         http_method_string(req.method));

#ifdef DEBUG
  printf("=== HEADERS (%zu bytes) ===\n", req.header_len);
  fwrite(req.header, 1, req.header_len, stdout);
  printf("\n");

  printf("=== BODY (%zu bytes) ===\n", req.body_len);
  if (req.body_len > 0) {
    fwrite(req.body, 1, req.body_len, stdout);
    printf("\n");
  }
#endif /* ifdef DEBUG */

  int matched = 0;
  for (size_t i = 0; i < config->endpoints.len; i++) {
    if ((strcmp(config->endpoints.items[i].path, req.path) == 0) &&
        (config->endpoints.items[i].method == req.method)) {
      matched = 1;
      config->endpoints.items[i].handler(fd, &req,
                                         config->endpoints.items[i].ctx);
      break;
    }
  }

  if (!matched) {
    send_404(fd, &req);
  }

cleanup:
  if (parsed_ok) {
    free(req.body);
    free(req.header);
    free(req.path);
  }
  free(buf);
  return parsed_ok ? 0 : -1;
}
