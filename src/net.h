#ifndef EVERYCAST_NET_H
#define EVERYCAST_NET_H

struct net_state {
  int host_rtnl, rtnl;
  int host_netns, netns;
  const char *host_veth_ifname, *veth_ifname, *dummy_ifname;
};

int net_init(struct net_state* s);
void net_destroy(struct net_state* s);

#endif  // EVERYCAST_NET_H
