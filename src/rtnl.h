#ifndef EVERYCAST_RTNL_H
#define EVERYCAST_RTNL_H

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/socket.h>

#include "try.h"

static inline int rtnl_socket() {
  return try(socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE), "failed to open rtnetlink socket: %s",
             strerror(errno));
}

int rtnl_create_veth_pair(int rtnl, const char* ifname, const char* peername);

#endif  // EVERYCAST_RTNL_H
