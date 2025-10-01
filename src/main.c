#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "defs.h"
#include "netns.h"
#include "rtnl.h"
#include "try.h"

int main(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);

  int orig_netns, netns;
  try(create_netns(&orig_netns, &netns));
  printf("orig_netns = %d, netns = %d\n", orig_netns, netns);

  int rtnl = try(rtnl_socket());

  try(rtnl_create_veth_pair(rtnl, "veth53", "eth0"));

  printf("entering bash\n");
  system("/bin/bash");

  // NLMSG_TAIL();

  return 0;
}
