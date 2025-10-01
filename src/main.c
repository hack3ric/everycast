#include <stdio.h>
#include <stdlib.h>

#include "defs.h"
#include "net.h"
#include "try.h"

int main(int argc, char** argv) {
  UNUSED(argc);
  UNUSED(argv);

  struct net_state net_state;
  try(net_init(&net_state));

  printf("host netns = %d, netns = %d\n", net_state.host_netns, net_state.netns);
  printf("entering debug shell\n");
  system("/bin/bash");

  net_destroy(&net_state);

  return 0;
}
