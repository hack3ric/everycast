#ifndef EVERYCAST_NETLINK_H
#define EVERYCAST_NETLINK_H

#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "try.h"

#define NLMSG_TAIL(nmsg) ((struct rtattr*)(((void*)(nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

struct nl_req {
  size_t len;
  struct nlmsghdr n;
  union {
    struct ifinfomsg i;
    struct ifaddrmsg a;
  };
  // data follows (including part of union)
};

#define _nl_req_alloca(size, ilen)                                           \
  ({                                                                         \
    size_t total_size = (size) + sizeof(req->len) + sizeof(req->n) + (ilen); \
    struct nl_req* req = alloca(total_size);                                 \
    memset(req, 0, total_size);                                              \
    req->len = total_size - sizeof(req->len);                                \
    req->n.nlmsg_len = NLMSG_LENGTH(ilen);                                   \
    req;                                                                     \
  })
#define nl_req_link_alloca(size) _nl_req_alloca(size, sizeof(struct ifinfomsg))
#define nl_req_addr_alloca(size) _nl_req_alloca(size, sizeof(struct ifaddrmsg))

int nl_attr(struct nl_req* req, int type, const void* data, size_t alen);

static inline int nl_str(struct nl_req* req, int type, const char* data) {
  return nl_attr(req, type, data, strlen(data) + 1);
}

#define _gen_nl_attr_typed(_type)                                          \
  static inline int nl_##_type(struct nl_req* req, int type, _type data) { \
    return nl_attr(req, type, &data, sizeof(data));                        \
  }
_gen_nl_attr_typed(int);

static inline struct rtattr* nl_nest_start(struct nl_req* req, int type) {
  struct rtattr* nest = NLMSG_TAIL(&req->n);
  nl_attr(req, type, NULL, 0);
  return nest;
}

static inline void nl_nest_end(struct nl_req* req, struct rtattr* nest) {
  nest->rta_len = (void*)NLMSG_TAIL(&req->n) - (void*)nest;
}

#define nl_nest(req, type, block)                 \
  ({                                              \
    struct rtattr* it = nl_nest_start(req, type); \
    block;                                        \
    nl_nest_end(req, it);                         \
    it;                                           \
  })

int nl_send_simple(int sock, struct nl_req* req);

static inline int rtnl_socket() {
  return try(socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE), "failed to open rtnetlink socket: %s",
             strerror(errno));
}

int rtnl_create_veth_pair(int rtnl, const char* ifname, const char* peername);
int rtnl_create_dummy(int rtnl, const char* ifname);
int rtnl_move_if_to_netns(int rtnl, const char* ifname, int netns);
int rtnl_link_set_up(int rtnl, const char* ifname);

#endif  // EVERYCAST_NETLINK_H
