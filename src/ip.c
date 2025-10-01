#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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

int ip_parse_with_prefix(char* str, ip_addr_t* ip, uint8_t* prefix) {
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
  long prefix_ = strtol(prefix_str, &endptr, 10);
  bool str_mapped_ipv4 = str_af == AF_INET6 && ip_proto(*ip) == AF_INET;

  if (prefix_ <= 0 || prefix_ > ip_proto_max_prefix(str_af) || *endptr != '\0' ||
      (str_mapped_ipv4 && prefix_ < 96)) {
    fprintf(stderr, "invalid prefix length: %s\n", prefix_str);
    return -(errno = EINVAL);
  }

  if (str_mapped_ipv4) prefix_ -= 96;
  *prefix = prefix_;

  return 0;
}
