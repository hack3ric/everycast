#include <stdio.h>
#include <stdlib.h>

#include "args.h"
#include "ip.h"
#include "net.h"
#include "try.h"

static int run(struct run_args* args) {
  for (size_t i = 0; i < args->anycast_count; i++) {
    char buf[INET6_ADDRSTRLEN];
    const char* ip_str = try_p2(ip_stringify(args->anycast[i], buf, sizeof(buf)));
    printf("anycast: %s\n", ip_str);
  }

  for (size_t i = 0; i < args->peer_count; i++) {
    char buf[INET6_ADDRSTRLEN];
    const char* ip_str = try_p2(ip_stringify(args->peer[i], buf, sizeof(buf)));
    printf("peer: %s/%u\n", ip_str, args->peer_len[i]);
  }

  struct net_state net_state = {};
  try(net_init(&net_state, args));

  printf("entering debug shell\n");
  system("/bin/bash");

  net_destroy(&net_state);
  return 0;
}

int main(int argc, char** argv) {
  struct args args = {};
  try(parse_args(&args, argc, argv));

  switch (args.cmd) {
    case CMD_RUN:
      return run(&args.run);
    default:
      break;
  }
  return 0;
}
