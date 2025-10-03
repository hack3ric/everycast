#include <alloca.h>
#include <libmnl/libmnl.h>
#include <linux/if_addr.h>
#include <linux/if_link.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/veth.h>
#include <net/if.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "ip.h"
#include "nl.h"
#include "try.h"

/* Generic netlink */

_Atomic uint32_t nl_seq_counter = 1;

struct mnl_socket* nl_open_simple(int bus) {
  struct mnl_socket* sock =
    try2_p(mnl_socket_open(bus), "failed to open netlink socket: %s", strerror(errno));
  try4(mnl_socket_bind(sock, 0, MNL_SOCKET_AUTOPID), "failed to bind netlink socket: %s",
       strerror(errno));
  return sock;
cleanup:
  if (sock) mnl_socket_close(sock);
  return NULL;
}

static int _nl_send(struct mnl_socket* sock, const void* req, size_t size) {
  return try(mnl_socket_sendto(sock, req, size), "netlink sendto failed: %s", strerror(errno));
}

static int _nl_recv(struct mnl_socket* sock, unsigned int seq, char* buf, char buf_size,
                    mnl_cb_t cb_data, void* data) {
  ENSURE_BUF(buf, buf_size);
  int len;
  unsigned int portid = mnl_socket_get_portid(sock);
  do {
    len = try(mnl_socket_recvfrom(sock, buf, sizeof(buf)), "oh no");
    int cb_status = try(mnl_cb_run(buf, len, seq, portid, cb_data, data),
                        "netlink request failed: %s", strerror(errno));
    if (cb_status <= MNL_CB_STOP) break;
  } while (len > 0);
  return 0;
}

int nl_send(struct mnl_socket* sock, struct nlmsghdr* req, mnl_cb_t cb_data, void* data, char* buf,
            size_t buf_size) {
  ENSURE_BUF(buf, buf_size);
  try(_nl_send(sock, req, req->nlmsg_len));
  try(_nl_recv(sock, req->nlmsg_seq, buf, buf_size, cb_data, data));
  return 0;
}

// Assuming all batch members that returns some value are using the same seq num
int nl_send_batch(struct mnl_socket* sock, struct mnl_nlmsg_batch* batch, unsigned int seq,
                  mnl_cb_t cb_data, void* data, char* buf, size_t buf_size) {
  ENSURE_BUF(buf, buf_size);
  try(_nl_send(sock, mnl_nlmsg_batch_head(batch), mnl_nlmsg_batch_size(batch)));
  try(_nl_recv(sock, seq, buf, buf_size, cb_data, data));
  return 0;
}

/* rtnetlink */

int rtnl_create_veth_pair(struct mnl_socket* rtnl, const char* ifname, const char* peername) {
  char buf[MNL_SOCKET_BUFFER_SIZE];

  struct nlmsghdr* nlh = mnl_nlmsg_put_header(buf);
  nlh->nlmsg_type = RTM_NEWLINK;
  nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
  nlh->nlmsg_seq = atomic_fetch_add(&nl_seq_counter, 1);

  struct ifinfomsg* ifi = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifi));
  ifi->ifi_family = AF_UNSPEC;
  ifi->ifi_change = 0xffffffff;

  struct nlattr* linkinfo = try_mnl_attr_nest_start_check(nlh, sizeof(buf), IFLA_LINKINFO);
  struct nlattr* data = try_mnl_attr_nest_start_check(nlh, sizeof(buf), IFLA_INFO_DATA);
  struct nlattr* peer_linkinfo = try_mnl_attr_nest_start_check(nlh, sizeof(buf), VETH_INFO_PEER);

  struct ifinfomsg* peer_ifi = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifi));
  peer_ifi->ifi_family = AF_UNSPEC;
  peer_ifi->ifi_change = 0xffffffff;
  try_mnl_attr_put_strz_check(nlh, sizeof(buf), IFLA_IFNAME, peername);

  mnl_attr_nest_end(nlh, peer_linkinfo);
  mnl_attr_nest_end(nlh, data);
  try_mnl_attr_put_strz_check(nlh, sizeof(buf), IFLA_INFO_KIND, "veth");
  mnl_attr_nest_end(nlh, linkinfo);
  try_mnl_attr_put_strz_check(nlh, sizeof(buf), IFLA_IFNAME, ifname);

  return try(nl_send_ack(rtnl, nlh, buf, sizeof(buf)));
}

int rtnl_create_dummy(struct mnl_socket* rtnl, const char* ifname) {
  char buf[MNL_SOCKET_BUFFER_SIZE];

  struct nlmsghdr* nlh = mnl_nlmsg_put_header(buf);
  nlh->nlmsg_type = RTM_NEWLINK;
  nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
  nlh->nlmsg_seq = atomic_fetch_add(&nl_seq_counter, 1);

  struct ifinfomsg* ifi = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifi));
  ifi->ifi_family = AF_UNSPEC;
  ifi->ifi_change = 0xffffffff;

  try_mnl_attr_put_strz_check(nlh, sizeof(buf), IFLA_IFNAME, ifname);
  struct nlattr* linkinfo = try_mnl_attr_nest_start_check(nlh, sizeof(buf), IFLA_LINKINFO);
  try_mnl_attr_put_strz_check(nlh, sizeof(buf), IFLA_INFO_KIND, "dummy");
  mnl_attr_nest_end(nlh, linkinfo);

  return try(nl_send_ack(rtnl, nlh, buf, sizeof(buf)));
}

int rtnl_move_if_to_netns(struct mnl_socket* rtnl, const char* ifname, int netns) {
  char buf[MNL_SOCKET_BUFFER_SIZE];

  struct nlmsghdr* nlh = mnl_nlmsg_put_header(buf);
  nlh->nlmsg_type = RTM_SETLINK;
  nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
  nlh->nlmsg_seq = atomic_fetch_add(&nl_seq_counter, 1);

  struct ifinfomsg* ifi = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifi));

  try_mnl_attr_put_strz_check(nlh, sizeof(buf), IFLA_IFNAME, ifname);
  try_mnl_attr_put_u32_check(nlh, sizeof(buf), IFLA_NET_NS_FD, netns);
  return try(nl_send_ack(rtnl, nlh, buf, sizeof(buf)));
}

int rtnl_link_set_up(struct mnl_socket* rtnl, const char* ifname) {
  char buf[128];

  struct nlmsghdr* nlh = mnl_nlmsg_put_header(buf);
  nlh->nlmsg_type = RTM_SETLINK;
  nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
  nlh->nlmsg_seq = atomic_fetch_add(&nl_seq_counter, 1);

  struct ifinfomsg* ifi = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifi));
  ifi->ifi_flags = ifi->ifi_change = IFF_UP;

  try_mnl_attr_put_strz_check(nlh, sizeof(buf), IFLA_IFNAME, ifname);
  return try(nl_send_ack(rtnl, nlh, buf, sizeof(buf)));
}

int rtnl_link_add_addr(struct mnl_socket* rtnl, uint32_t ifindex, ip_addr_t ip, uint8_t prefix) {
  char buf[MNL_SOCKET_BUFFER_SIZE];

  struct nlmsghdr* nlh = mnl_nlmsg_put_header(buf);
  nlh->nlmsg_type = RTM_NEWADDR;
  nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL | NLM_F_ACK;
  nlh->nlmsg_seq = atomic_fetch_add(&nl_seq_counter, 1);

  struct ifaddrmsg* ifa = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifa));
  ifa->ifa_family = ip_proto(ip);
  ifa->ifa_prefixlen = prefix;
  ifa->ifa_index = ifindex;
  ifa->ifa_flags = IFA_F_PERMANENT;

  try_mnl_attr_put_check(nlh, sizeof(buf), IFA_LOCAL, ip_buf_len(ip), ip_buf(&ip));
  try_mnl_attr_put_check(nlh, sizeof(buf), IFA_ADDRESS, ip_buf_len(ip), ip_buf(&ip));

  return try(nl_send_ack(rtnl, nlh, buf, sizeof(buf)));
}
