#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sched.h>

#include "try.h"
#include "netns.h"

int create_netns(int* orig_fd, int* new_fd) {
  *orig_fd = try_open("/proc/self/ns/net", O_RDONLY);
  try(unshare(CLONE_NEWNET), "failed to unshare netns: %s", strerror(errno));
  *new_fd = try_open("/proc/self/ns/net", O_RDONLY);
  return 0;
}
