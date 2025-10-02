#ifndef EVERYCAST_NET_H
#define EVERYCAST_NET_H

#include "args.h"

struct net_state {
  int host_rtnl, rtnl;
  int host_netns, netns;
  const char *host_veth, *veth, *dummy;
};

int net_init(struct net_state* s, const struct run_args* args);
void net_destroy(struct net_state* s);

#endif  // EVERYCAST_NET_H
