#ifndef EVERYCAST_IP_H
#define EVERYCAST_IP_H

#include <linux/types.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>

typedef union ip_addr {
  struct in6_addr v6;
  struct {
    __be32 __pad[3];
    in_addr_t v4;
  };
  __be16 v6_addr16[8];
  __be32 v6_addr32[4];
  __be64 v6_addr64[2];
} ip_addr_t;

static inline int ip_proto(ip_addr_t ip) {
  if (ip.v6_addr32[0] == 0 && ip.v6_addr32[1] == 0 && ip.v6_addr32[2] == htonl(0xffff))
    return AF_INET;
  else
    return AF_INET6;
}

static inline int ip_max_prefix(ip_addr_t ip) {
  if (ip_proto(ip) == AF_INET)
    return 32;
  else
    return 128;
}

static inline void* ip_buf(ip_addr_t* ip) {
  if (ip_proto(*ip) == AF_INET)
    return &ip->v4;
  else
    return &ip->v6;
}

static inline ip_addr_t ip_canonicalize(ip_addr_t ip) {
  // Deprecated IPv4-mapped IPv6 range ::/96
  if (ip.v6_addr32[0] == 0 && ip.v6_addr32[1] == 0 && ip.v6_addr32[2] == 0 && ip.v6_addr32[3] != 0)
    ip.v6_addr16[5] = 0xffff;
  return ip;
}

int ip_parse(const char* ip_str, ip_addr_t* ip);
int ip_parse_with_prefix(char* str, ip_addr_t* ip, uint8_t* prefix);

#endif  // EVERYCAST_IP_H
