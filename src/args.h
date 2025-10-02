#ifndef EVERYCAST_ARGS_H
#define EVERYCAST_ARGS_H

#include <stddef.h>

#include "ip.h"

struct args {
  enum { CMD_NULL, CMD_RUN } cmd;
  union {
    struct run_args {
      ip_addr_t anycasts[16];
      size_t anycasts_len;
#define PEERS_MAX_LEN 16
      ip_addr_t peers[PEERS_MAX_LEN];
      uint8_t peer_prefixes[PEERS_MAX_LEN];
      size_t peers_len;
    } run;
  };
};

int parse_args(struct args* args, int argc, char** argv);

#endif  // EVERYCAST_ARGS_H
