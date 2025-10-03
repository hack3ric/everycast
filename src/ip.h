#ifndef EVERYCAST_IP_H
#define EVERYCAST_IP_H

#include <linux/types.h>
#include <linux/in.h>
#include <linux/in6.h>
#include <stdint.h>
#include <sys/socket.h>

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

typedef union ip_addr {
  struct in6_addr v6;
  struct {
    __be32 __pad1[2];
    __be16 __pad2;
    __be16 v4_marker;
    union {
      __be32 v4;
      __u8 v4_addr8[4];
    };
  };
  __u8 v6_addr8[16];
  __be16 v6_addr16[8];
  __be32 v6_addr32[4];
  __be64 v6_addr64[2];
} ip_addr_t;

int ip_proto(ip_addr_t ip);

static inline int ip_proto_max_prefix(int proto) {
  if (proto == AF_INET)
    return 32;
  else
    return 128;
}

static inline int ip_max_prefix(ip_addr_t ip) { return ip_proto_max_prefix(ip_proto(ip)); }

static inline void* ip_buf(ip_addr_t* ip) {
  if (ip_proto(*ip) == AF_INET)
    return &ip->v4;
  else
    return &ip->v6;
}

static inline int ip_buf_len(ip_addr_t ip) {
  if (ip_proto(ip) == AF_INET)
    return sizeof(ip.v4);
  else
    return sizeof(ip.v6);
}

ip_addr_t ip_canonicalize(ip_addr_t ip);

int ip_parse(const char* ip_str, ip_addr_t* ip);
int ip_parse_with_prefix(char* str, ip_addr_t* ip, uint8_t* prefix_len);
int ip_parse_prefix(char* str, ip_addr_t* prefix, uint8_t* len);
const char* ip_stringify(ip_addr_t ip, char* dst, size_t size);
void ip_prefix_select_two(ip_addr_t prefix, uint8_t len, ip_addr_t* ip1, ip_addr_t* ip2);

#endif  // EVERYCAST_IP_H
