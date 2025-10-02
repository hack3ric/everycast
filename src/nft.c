#include <libmnl/libmnl.h>
#include <libnftnl/chain.h>
#include <libnftnl/expr.h>
#include <libnftnl/rule.h>
#include <libnftnl/table.h>
#include <linux/netlink.h>
#include <linux/socket.h>

int nft() {
  struct mnl_socket* sock = mnl_socket_open(NETLINK_NETFILTER);
}
