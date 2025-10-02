#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "defs.h"
#include "ip.h"
#include "try.h"

static int _ip_parse(const char* str, ip_addr_t* ip, int* str_af) {
  if (str_af) *str_af = AF_INET6;
  if (inet_pton(AF_INET6, str, &ip->v6) != 1) {
    if (str_af) *str_af = AF_INET;
    memset(ip, 0, sizeof(*ip));
    if (inet_pton(AF_INET, str, &ip->v4) != 1) {
      return -(errno = EINVAL);
    }
  }
  *ip = ip_canonicalize(*ip);
  return 0;
}

int ip_parse(const char* str, ip_addr_t* ip) { return _ip_parse(str, ip, NULL); }

int ip_parse_with_prefix(char* str, ip_addr_t* ip, uint8_t* prefix_len) {
  char* prefix_str = strrchr(str, '/');
  if (!prefix_str) {
    fprintf(stderr, "no prefix length specified: %s\n", str);
    return -(errno = EINVAL);
  }
  *prefix_str = '\0';
  prefix_str++;

  int str_af;
  try(_ip_parse(str, ip, &str_af));

  char* endptr;
  long prefix_len_ = strtol(prefix_str, &endptr, 10);
  bool str_mapped_ipv4 = str_af == AF_INET6 && ip_proto(*ip) == AF_INET;

  if (prefix_len_ <= 0 || prefix_len_ > ip_proto_max_prefix(str_af) || *endptr != '\0' ||
      (str_mapped_ipv4 && prefix_len_ < 96)) {
    fprintf(stderr, "invalid prefix length: %s\n", prefix_str);
    return -(errno = EINVAL);
  }

  if (str_mapped_ipv4) prefix_len_ -= 96;
  *prefix_len = prefix_len_;

  return 0;
}

int ip_parse_prefix(char* str, ip_addr_t* prefix, uint8_t* len) {
  try(ip_parse_with_prefix(str, prefix, len));

  bool prefix_v4_valid = (ntohl(prefix->v4) & ((1ul << (32 - *len)) - 1)) == 0;
  bool prefix_v6_valid =
    (*len > 64 && prefix->v6_addr64[0] == 0 &&
     (ntohll(prefix->v6_addr64[1]) & ((1ul << (128 - 64 + *len)) - 1)) == 0) ||
    (*len <= 64 && (ntohll(prefix->v6_addr64[1]) & ((1ul << (128 - *len)) - 1)) == 0);
  bool prefix_valid = (ip_proto(*prefix) == AF_INET && prefix_v4_valid) ||
                      (ip_proto(*prefix) == AF_INET6 && prefix_v6_valid);

  if (!prefix_valid) {
    char buf[INET6_ADDRSTRLEN];
    const char* prefix_str = ip_stringify(*prefix, buf, sizeof(buf));
    fprintf(stderr, "non-zero outside prefix: %s/%u\n", prefix_str, *len);
    return -(errno = EINVAL);
  }

  return 0;
}

const char* ip_stringify(ip_addr_t ip, char* dst, size_t size) {
  return try2_p2(inet_ntop(ip_proto(ip), ip_buf(&ip), dst, size));
}
