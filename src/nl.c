#include <errno.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "nl.h"
#include "ip.h"
#include "try.h"

int nl_attr(struct nl_req* req, int type, const void* data, size_t alen) {
  int len = RTA_LENGTH(alen);
  if (NLMSG_ALIGN(req->n.nlmsg_len) + RTA_ALIGN(len) > req->len) {
    fprintf(stderr, "nl_attr: message exceeded bound of %lu\n", req->len);
    return -(errno = E2BIG);
  }

  struct rtattr* rta = NLMSG_TAIL(&req->n);
  rta->rta_len = len;
  rta->rta_type = type;
  if (alen) memcpy(RTA_DATA(rta), data, alen);
  req->n.nlmsg_len = NLMSG_ALIGN(req->n.nlmsg_len) + RTA_ALIGN(len);
  return 0;
}

int nl_send_simple(int sock, struct nl_req* req) {
  req->n.nlmsg_flags |= NLM_F_ACK;
  try(send(sock, &req->n, req->n.nlmsg_len, 0), "send failed: %s", strerror(errno));

  char buf[NLMSG_LENGTH(sizeof(struct nlmsgerr))];
  try(recv(sock, buf, sizeof(buf), 0), "recv failed: %s", strerror(errno));
  struct nlmsghdr* nlh = (typeof(nlh))buf;

  if (nlh->nlmsg_type == NLMSG_ERROR) {
    struct nlmsgerr* err = NLMSG_DATA(nlh);
    if (err->error != 0) {
      fprintf(stderr, "netlink error: %s\n", strerror(-err->error));
      return -(errno = -err->error);
    }
  }

  return 0;
}

int rtnl_create_veth_pair(int rtnl, const char* ifname, const char* peername) {
  struct nl_req* req = nl_req_link_alloca(256);
  req->n.nlmsg_type = RTM_NEWLINK;
  req->n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
  req->i.ifi_change = 0xffffffff;

  try(nl_str(req, IFLA_IFNAME, ifname));
  nl_nest(req, IFLA_LINKINFO, {
    try(nl_str(req, IFLA_INFO_KIND, "veth"));
    nl_nest(req, IFLA_INFO_DATA, {
      nl_nest(req, VETH_INFO_PEER, {
        struct ifinfomsg* peer_msg = RTA_DATA(it);
        req->n.nlmsg_len += sizeof(*peer_msg);
        peer_msg->ifi_family = AF_UNSPEC;
        peer_msg->ifi_change = 0xffffffff;
        try(nl_str(req, IFLA_IFNAME, peername));
      });
    });
  });

  return try(nl_send_simple(rtnl, req));
}

int rtnl_create_dummy(int rtnl, const char* ifname) {
  struct nl_req* req = nl_req_link_alloca(256);
  req->n.nlmsg_type = RTM_NEWLINK;
  req->n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
  req->i.ifi_change = 0xffffffff;
  try(nl_str(req, IFLA_IFNAME, ifname));
  nl_nest(req, IFLA_LINKINFO, try(nl_str(req, IFLA_INFO_KIND, "dummy")));
  return try(nl_send_simple(rtnl, req));
}

int rtnl_move_if_to_netns(int rtnl, const char* ifname, int netns) {
  struct nl_req* req = nl_req_link_alloca(128);
  req->n.nlmsg_type = RTM_SETLINK;
  req->n.nlmsg_flags = NLM_F_REQUEST;
  try(nl_str(req, IFLA_IFNAME, ifname));
  try(nl_int(req, IFLA_NET_NS_FD, netns));
  return try(nl_send_simple(rtnl, req));
}

int rtnl_link_set_up(int rtnl, const char* ifname) {
  struct nl_req* req = nl_req_link_alloca(128);
  req->n.nlmsg_type = RTM_SETLINK;
  req->n.nlmsg_flags = NLM_F_REQUEST;
  req->i.ifi_flags = req->i.ifi_change = IFF_UP;
  try(nl_str(req, IFLA_IFNAME, ifname));
  return try(nl_send_simple(rtnl, req));
}

int rtnl_link_add_addr(int rtnl, const char* ifname, ip_addr_t ip, uint8_t prefix) {
  struct nl_req* req = nl_req_addr_alloca(128);
  req->n.nlmsg_type = RTM_NEWADDR;
  req->n.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL;
  req->a.ifa_family = ip_proto(ip);
  req->a.ifa_prefixlen = prefix;

  // TODO

  return 0;
}
