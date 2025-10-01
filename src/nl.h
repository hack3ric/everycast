#ifndef EVERYCAST_NETLINK_H
#define EVERYCAST_NETLINK_H

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
  struct ifinfomsg i;
  char data[];
};

#define nl_req_alloca(size)                                 \
  ({                                                        \
    struct nl_req* req = alloca((size) + sizeof(req->len)); \
    memset(req, 0, (size) + sizeof(req->len));              \
    req->len = (size);                                      \
    req->n.nlmsg_len = NLMSG_LENGTH(sizeof(req->i));        \
    req;                                                    \
  })

int nl_attr(struct nl_req* req, int type, const void* data, size_t alen);

static inline int nl_str(struct nl_req* req, int type, const char* data) {
  return nl_attr(req, type, data, strlen(data) + 1);
}

static inline struct rtattr* nl_nest_start(struct nl_req* req, int type) {
  struct rtattr* nest = NLMSG_TAIL(&req->n);
  try2(nl_attr(req, type, NULL, 0));
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

int nl_send(int sock, struct nl_req* req);

#endif  // EVERYCAST_NETLINK_H
