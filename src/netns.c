#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "netns.h"
#include "try.h"

int netns_create(int* orig_fd, int* new_fd) {
  *orig_fd = try_open("/proc/self/ns/net", O_RDONLY);
  try(unshare(CLONE_NEWNET), "failed to unshare netns: %s", strerror(errno));
  *new_fd = try_open("/proc/self/ns/net", O_RDONLY);
  return 0;
}

inline int netns_set(int netns) { return setns(netns, CLONE_NEWNET); }

static int _netns_if_nametoindex(int netns, const char* ifname, unsigned int* index, int* errno_) {
  if (netns_set(netns)) {
    fprintf(stderr, "netns_set(%d) failed: %s\n", netns, strerror(errno));
    goto error;
  }
  *index = if_nametoindex(ifname);
  if (*index <= 0) {
    fprintf(stderr, "if_nametoindex(\"%s\") failed: %s\n", ifname, strerror(errno));
    goto error;
  }
  return 0;
error:
  *errno_ = errno;
  return -1;
}

long netns_if_nametoindex(int netns, const char* ifname) {
  if (try(netns_is_current(netns))) {
    unsigned int index = if_nametoindex(ifname);
    if (index <= 0) {
      fprintf(stderr, "if_nametoindex(\"%s\") failed: %s\n", ifname, strerror(errno));
      return -errno;
    }
    return index;
  }

  int result = 0;

  struct {
    unsigned int index;
    int errno_;
  }* shm =
    try_p(mmap(NULL, sizeof(*shm), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0),
          "mmap failed: %s", strerror(errno));
  memset(shm, 0, sizeof(*shm));

  pid_t pid = fork();
  if (pid == 0) {
    exit(_netns_if_nametoindex(netns, ifname, &shm->index, &shm->errno_));
  } else {
    int wstatus;
    try3(waitpid(pid, &wstatus, 0));
    if (WIFEXITED(wstatus)) {
      int code = WEXITSTATUS(wstatus);
      if (code == 0) {
        result = shm->index;
        goto cleanup;
      }
      fprintf(stderr, "subprocess failed with errno %d: %s\n", code, strerror(shm->errno_));
      result = -(errno = shm->errno_);
    } else {
      fprintf(stderr, "subprocess failed with signal %d\n", WTERMSIG(wstatus));
      result = -(errno = EPIPE);
    }
  }

cleanup:;
  int tmp_errno = errno;
  munmap(shm, sizeof(*shm));
  errno = tmp_errno;
  return result;
}

int netns_is_current(int netns) {
  struct stat current_ns_stat, fd_ns_stat;

  try(stat("/proc/self/ns/net", &current_ns_stat), "stat failed: %s", strerror(errno));
  try(fstat(netns, &fd_ns_stat), "fstat failed: %s", strerror(errno));

  return (current_ns_stat.st_ino == fd_ns_stat.st_ino);
}
