#include <stdint.h>
#include <unistd.h>

#include "ip.h"
#include "net.h"
#include "netns.h"
#include "nl.h"
#include "try.h"

int net_init(struct net_state* s) {
  int result = 0;
  s->rtnl = s->host_netns = s->netns = -1;
  s->host_veth_ifname = "veth53";
  s->veth_ifname = "eth0";
  s->dummy_ifname = "eth1";

  s->host_rtnl = try3(rtnl_socket());
  try3(netns_create(&s->host_netns, &s->netns));
  s->rtnl = try3(rtnl_socket());

  try3(rtnl_create_veth_pair(s->rtnl, s->host_veth_ifname, s->veth_ifname));
  try3(rtnl_move_if_to_netns(s->rtnl, s->host_veth_ifname, s->host_netns));
  try3(rtnl_create_dummy(s->rtnl, s->dummy_ifname));

  const ip_addr_t ip1 = {.v4_marker = 0xffff, .v4_addr8 = {172, 17, 53, 53}};
  const uint8_t prefix1 = 24;
  const ip_addr_t ip2 = {
    .v6_addr8 = {0xfc, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0x53, 0, 0, 0, 0, 0, 0, 0, 0x53}};
  const uint8_t prefix2 = 64;

  unsigned int ifindex = try3(netns_if_nametoindex(s->host_netns, s->host_veth_ifname));
  try3(rtnl_link_add_addr(s->host_rtnl, ifindex, ip1, prefix1));
  try3(rtnl_link_add_addr(s->host_rtnl, ifindex, ip2, prefix2));

  try3(rtnl_link_set_up(s->host_rtnl, s->host_veth_ifname));
  try3(rtnl_link_set_up(s->rtnl, "lo"));
  try3(rtnl_link_set_up(s->rtnl, s->veth_ifname));
  try3(rtnl_link_set_up(s->rtnl, s->dummy_ifname));

  return 0;
cleanup:;
  int tmp_errno = errno;
  net_destroy(s);
  errno = tmp_errno;
  return result;
}

void net_destroy(struct net_state* s) {
  if (s->host_rtnl) close(s->host_rtnl);
  if (s->rtnl) close(s->rtnl);
  if (s->host_netns) {
    netns_set(s->host_netns);
    close(s->host_netns);
  }
  if (s->netns) close(s->netns);
}
