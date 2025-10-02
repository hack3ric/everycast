#include <argp.h>
#include <asm-generic/errno-base.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "ip.h"
#include "try.h"

#include "args.h"

static const struct argp_option run_options[] = {
  {"anycast", 'a', "<IP>", 0, "Add anycast IP address", 0},
  {"peer", 'p', "<PREFIX>", 0, "Add peer prefix between host and anycast netns", 0},
  {},
};

static inline error_t run_args_parse_opt(int key, char* arg, struct argp_state* state) {
  struct run_args* args = (typeof(args))state->input;
  switch (key) {
    case 'a':
      if (args->anycasts_len >= ARRAY_LEN(args->anycasts)) {
        fprintf(stderr, "anycast IP count exceeds maximum of %lu\n", ARRAY_LEN(args->anycasts));
        return -(errno = E2BIG);
      }
      try(ip_parse(arg, &args->anycasts[args->anycasts_len]), "not a valid IP address: %s", arg);
      args->anycasts_len++;
      break;
    case 'p':
      if (args->peers_len >= PEERS_MAX_LEN) {
        fprintf(stderr, "anycast IP count exceeds maximum of %d\n", PEERS_MAX_LEN);
        return -(errno = E2BIG);
      }
      size_t len = args->peers_len;
      try(ip_parse_prefix(arg, &args->peers[len], &args->peer_prefixes[len]));
      args->peers_len++;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

const struct argp run_argp = {
  .options = run_options, .parser = run_args_parse_opt,
  // .args_doc = "<interface>",
  // .doc = "\vSee everycast(1) for detailed usage.",
};

static const struct argp_option options[] = {{}};
static const char doc[] =
  "\n"
  "Commands:\n"
  "\n"
  "      run                    Run an anycast instance\n"
  "\n"
  "Options:"
  "\v"
  /* "See everycast(1) for detailed usage." */;

static inline error_t argp_parse_cmd(struct argp_state* state, const char* cmdname,
                                     const struct argp* cmd_argp, void* args) {
  int argc = state->argc - state->next + 1;
  char** argv = &state->argv[state->next - 1];
  char* argv0 = argv[0];

  size_t len = strlen(state->name) + (1 + strlen(cmdname)) + 1;
  char new_argv0[len];
  new_argv0[0] = 0;
  strncat(new_argv0, state->name, len);
  strncat(new_argv0, " ", len);
  strncat(new_argv0, cmdname, len);
  argv[0] = new_argv0;
  int result = argp_parse(cmd_argp, argc, argv, ARGP_IN_ORDER, &argc, args);
  argv[0] = argv0;
  state->next += argc - 1;
  return result;
}

static inline error_t args_parse_opt(int key, char* arg, struct argp_state* state) {
  struct args* args = (typeof(args))state->input;
  if (args->cmd != CMD_NULL) return ARGP_ERR_UNKNOWN;

  switch (key) {
    case ARGP_KEY_ARG:
      if (strcmp(arg, "run") == 0) {
        args->cmd = CMD_RUN;
        return argp_parse_cmd(state, "run", &run_argp, &args->run);
      };
      fprintf(stderr, "unknown command %s\n", arg);
      exit(1);
      break;
    case ARGP_KEY_NO_ARGS:
      argp_usage(state);
      break;
    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static const struct argp argp = {
  .options = options,
  .parser = args_parse_opt,
  .args_doc = "COMMAND [OPTION...]",
  .doc = doc,
};

int parse_args(struct args* restrict args, int argc, char** restrict argv) {
  return try(argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, args), "error parsing arguments");
}
