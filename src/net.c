#include <stdint.h>
#include <unistd.h>

#include "args.h"
#include "ip.h"
#include "net.h"
#include "netns.h"
#include "nl.h"
#include "try.h"

int net_init(struct net_state* s, const struct run_args* args) {
  int result = 0;
  s->host_rtnl = s->rtnl = s->host_netns = s->netns = -1;
  s->host_veth = "veth53";
  s->veth = "eth0";
  s->dummy = "eth1";

  s->host_rtnl = try3(rtnl_socket());
  try3(netns_create(&s->host_netns, &s->netns));
  s->rtnl = try3(rtnl_socket());

  try3(rtnl_create_veth_pair(s->rtnl, s->host_veth, s->veth));
  try3(rtnl_move_if_to_netns(s->rtnl, s->host_veth, s->host_netns));
  try3(rtnl_create_dummy(s->rtnl, s->dummy));

  for (size_t i = 0; i < args->peer_count; i++) {
    ip_addr_t host_ip, ip;
    ip_prefix_select_two(args->peer[i], args->peer_len[i], &host_ip, &ip);
    try3(rtnl_link_add_addr(s->host_rtnl, s->host_veth, host_ip, args->peer_len[i]));
    try3(rtnl_link_add_addr(s->rtnl, s->veth, ip, args->peer_len[i]));
  }

  for (size_t i = 0; i < args->anycast_count; i++) {
    ip_addr_t ip = args->anycast[i];
    try3(rtnl_link_add_addr(s->rtnl, s->dummy, ip, ip_max_prefix(ip)));
  }

  try3(rtnl_link_set_up(s->host_rtnl, s->host_veth));
  try3(rtnl_link_set_up(s->rtnl, "lo"));
  try3(rtnl_link_set_up(s->rtnl, s->veth));
  try3(rtnl_link_set_up(s->rtnl, s->dummy));

  result = 0;
  goto success;
cleanup:;
  int tmp_errno = errno;
  net_destroy(s);
  errno = tmp_errno;
success:
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
