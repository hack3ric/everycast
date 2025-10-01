#include <asm-generic/errno-base.h>
#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "nl.h"
#include "try.h"

int nl_attr(struct nl_req* req, int type, const void* data, size_t alen) {
  int len = RTA_LENGTH(alen);
  if (NLMSG_ALIGN(req->n.nlmsg_len) + RTA_ALIGN(len) > req->len) {
    fprintf(stderr, "addattr_l: message exceeded bound of %lu\n", req->len);
    return -(errno = E2BIG);
  }

  struct rtattr* rta = NLMSG_TAIL(&req->n);
  rta->rta_len = len;
  rta->rta_type = type;
  if (alen) memcpy(RTA_DATA(rta), data, alen);
  req->n.nlmsg_len = NLMSG_ALIGN(req->n.nlmsg_len) + RTA_ALIGN(len);
  return 0;
}

int nl_send(int sock, struct nl_req* req) {
  try(send(sock, &req->n, req->n.nlmsg_len, 0), "send failed: %s", strerror(errno));

  const int NL_RECV_BUF_SIZE = 1024;
  char buf[NL_RECV_BUF_SIZE];
  size_t len = try(recv(sock, buf, sizeof(buf), 0), "recv failed: %s", strerror(errno));
  struct nlmsghdr* nlh = (typeof(nlh))buf;
  if (!NLMSG_OK(nlh, len)) {
    fprintf(stderr, "received malformed netlink");
    return -(errno = EREMOTEIO);
  }

  if (nlh->nlmsg_type == NLMSG_ERROR) {
    struct nlmsgerr* err = NLMSG_DATA(nlh);
    if (err->error != 0) {
      fprintf(stderr, "netlink error: %s\n", strerror(-err->error));
      return -(errno = -err->error);
    }
  }

  return 0;
}
