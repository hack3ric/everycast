#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "ip.h"
#include "try.h"

int ip_parse(const char* str, ip_addr_t* ip) {
  if (inet_pton(AF_INET6, str, &ip->v6) != 1) {
    memset(ip, 0, sizeof(*ip));
    if (inet_pton(AF_INET, str, &ip->v4) != 1) {
      return -(errno = EINVAL);
    }
  }
  *ip = ip_canonicalize(*ip);
  return 0;
}

int ip_parse_with_prefix(char* str, ip_addr_t* ip, uint8_t* prefix) {
  char* prefix_str = strrchr(str, '/');
  if (!prefix_str) {
    fprintf(stderr, "no prefix length specified: %s\n", str);
    return -(errno = EINVAL);
  }
  *prefix_str = '\0';
  prefix_str++;

  try(ip_parse(str, ip));

  char* endptr;
  long prefix_ = strtol(prefix_str, &endptr, 10);
  if (prefix_ <= 0 || prefix_ > ip_max_prefix(*ip) || *endptr != '\0') {
    fprintf(stderr, "invalid prefix length: %s\n", prefix_str);
    return -(errno = EINVAL);
  }
  *prefix = prefix_;

  return 0;
}
