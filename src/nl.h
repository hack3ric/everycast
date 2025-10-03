#ifndef EVERYCAST_NETLINK_H
#define EVERYCAST_NETLINK_H

#include <errno.h>
#include <libmnl/libmnl.h>
#include <stddef.h>
#include <stdint.h>

#include "ip.h"
#include "try.h"

/* Generic netlink */

#define try_mnl(stmt)                                     \
  ({                                                      \
    bool ret = (stmt);                                    \
    if (!ret) {                                           \
      errno = E2BIG;                                      \
      _ret(-E2BIG, _0, "netlink message exceeds buffer"); \
    }                                                     \
    ret;                                                  \
  })

#define try_mnl_p(stmt)                                   \
  ({                                                      \
    void* ret = (stmt);                                   \
    if (!ret) {                                           \
      errno = E2BIG;                                      \
      _ret(-E2BIG, _0, "netlink message exceeds buffer"); \
    }                                                     \
    ret;                                                  \
  })

#define try_mnl_attr_put_check(...) try_mnl(mnl_attr_put_check(__VA_ARGS__))
#define try_mnl_attr_put_strz_check(...) try_mnl(mnl_attr_put_strz_check(__VA_ARGS__))
#define try_mnl_attr_put_u32_check(...) try_mnl(mnl_attr_put_u32_check(__VA_ARGS__))
#define try_mnl_attr_nest_start_check(...) try_mnl_p(mnl_attr_nest_start_check(__VA_ARGS__))

#define ENSURE_BUF(buf, buf_size)        \
  ({                                     \
    if (!buf) {                          \
      buf_size = MNL_SOCKET_BUFFER_SIZE; \
      buf = alloca(buf_size);            \
    }                                    \
  })

extern _Atomic uint32_t nl_seq_counter;

int nl_send(struct mnl_socket* sock, struct nlmsghdr* req, mnl_cb_t cb_data, void* data, char* buf,
            size_t buf_size);
int nl_send_batch(struct mnl_socket* sock, struct mnl_nlmsg_batch* batch, unsigned int seq,
                  mnl_cb_t cb_data, void* data, char* buf, size_t buf_size);

#define nl_send_ack(sock, req, buf, buf_size) nl_send(sock, req, NULL, NULL, buf, buf_size)
#define nl_send_batch_ack(sock, batch, seq, buf, buf_size) \
  nl_send_batch(sock, batch, seq, NULL, NULL, buf, buf_size)

/* rtnetlink */

int rtnl_create_veth_pair(struct mnl_socket* rtnl, const char* ifname, const char* peername);
int rtnl_create_dummy(struct mnl_socket* rtnl, const char* ifname);
int rtnl_move_if_to_netns(struct mnl_socket* rtnl, const char* ifname, int netns);
int rtnl_link_set_up(struct mnl_socket* rtnl, const char* ifname);
int rtnl_link_add_addr(struct mnl_socket* rtnl, uint32_t ifindex, ip_addr_t ip, uint8_t prefix);

#endif  // EVERYCAST_NETLINK_H
