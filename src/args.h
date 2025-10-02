#ifndef EVERYCAST_ARGS_H
#define EVERYCAST_ARGS_H

#include <stddef.h>

#include "ip.h"

struct args {
  enum { CMD_NULL, CMD_RUN } cmd;
  union {
    struct run_args {
      ip_addr_t anycast[16];
      size_t anycast_count;
#define PEERS_MAX_LEN 16
      ip_addr_t peer[PEERS_MAX_LEN];
      uint8_t peer_len[PEERS_MAX_LEN];
      size_t peer_count;
    } run;
  };
};

int parse_args(struct args* args, int argc, char** argv);

#endif  // EVERYCAST_ARGS_H
