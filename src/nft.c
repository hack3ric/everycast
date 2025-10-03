#include <libmnl/libmnl.h>
#include <libnftnl/chain.h>
#include <libnftnl/common.h>
#include <libnftnl/expr.h>
#include <libnftnl/rule.h>
#include <libnftnl/table.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_tables.h>
#include <linux/netlink.h>
#include <linux/socket.h>
#include <stdio.h>
#include <stdlib.h>

#include "nl.h"
#include "try.h"

int nft() {
  int result = 0;
  char* buf = malloc(2 * MNL_SOCKET_BUFFER_SIZE);
  unsigned int seq = 0;
  struct nftnl_table* t = NULL;
  struct mnl_socket* sock = NULL;

  struct mnl_nlmsg_batch* batch = mnl_nlmsg_batch_start(buf, MNL_SOCKET_BUFFER_SIZE);

  nftnl_batch_begin(mnl_nlmsg_batch_current(batch), seq++);
  try_mnl(mnl_nlmsg_batch_next(batch));

  unsigned int table_seq = seq;
  struct nlmsghdr* nlh = nftnl_nlmsg_build_hdr(mnl_nlmsg_batch_current(batch), NFT_MSG_NEWTABLE,
                                               NFPROTO_INET, NLM_F_CREATE | NLM_F_ACK, seq++);

  t = try3_p(nftnl_table_alloc());
  try3(nftnl_table_set_str(t, NFTNL_TABLE_NAME, "everycast"));
  nftnl_table_set_u32(t, NFTNL_TABLE_FAMILY, NFPROTO_INET);
  nftnl_table_set_u32(t, NFTNL_TABLE_FLAGS, 0);
  nftnl_table_nlmsg_build_payload(nlh, t);
  try_mnl(mnl_nlmsg_batch_next(batch));  // TODO: try3

  nftnl_batch_end(mnl_nlmsg_batch_current(batch), seq++);
  try_mnl(mnl_nlmsg_batch_next(batch));  // TODO: try3

  sock = try3_p(mnl_socket_open(NETLINK_NETFILTER));
  try3(mnl_socket_bind(sock, 0, MNL_SOCKET_AUTOPID));
  unsigned int portid = mnl_socket_get_portid(sock);

  try3(mnl_socket_sendto(sock, mnl_nlmsg_batch_head(batch), mnl_nlmsg_batch_size(batch)));

  int len = try3(mnl_socket_recvfrom(sock, buf, 2 * MNL_SOCKET_BUFFER_SIZE));
  while (len > 0) {
    int cb_status =
      try3(mnl_cb_run(buf, len, table_seq, portid, NULL, NULL), "error: %s", strerror(errno));
    if (cb_status <= MNL_CB_STOP) break;
    len = try3(mnl_socket_recvfrom(sock, buf, sizeof(buf)), "oh no");
  }

  // struct nlmsghdr* nlh = nftnl_nlmsg_build_hdr(buf, NFT_MSG_NEWTABLE, NFPROTO_INET, 0, 1);
  // nftnl_table_nlmsg_build_payload(nlh, t);

  // try3(nl_send_simple(sock, nlh), "oh no");

cleanup:
  if (sock) mnl_socket_close(sock);
  free(buf);
  nftnl_table_free(t);
  return result;
}
